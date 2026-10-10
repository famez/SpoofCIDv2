#include <project.h>

// ===========================================================================
// SpoofCIDv2 — inyector de CID/CSD vía UDB (SD_CID_RESPONDER)
//
// La lógica de tiempo real (detección de comando + serialización de la
// respuesta R2 sincronizada a HOST_CLK) vive en el UDB. El firmware sólo:
//   1) construye la trama (contenido + CRC7),
//   2) al detectar CMD2/CMD9 arma la transmisión y ceba el primer byte,
//   3) alimenta los bytes restantes conforme el UDB los pide (BYTE_REQ).
// El UDB gestiona SW_IN (toma del bus) por hardware durante la TX.
// ===========================================================================

// --- Registros del UDB (nombres generados por PSoC Creator tras Build) ---
#define SD_STATUS        CY_GET_REG8(SD_CID_RESPONDER_1_StatusReg__STATUS_REG)
#define SD_CTRL_WRITE(v) CY_SET_REG8(SD_CID_RESPONDER_1_CtrlReg__CONTROL_REG, (v))
// Byte a transmitir: registro D0 de la datapath del UDB (el UDB lo copia a A0
// en cada punto de carga y lo desplaza).
#define SD_DATA_WRITE(v) CY_SET_REG8(SD_CID_RESPONDER_1_dp_u0__D0_REG, (v))

// StatusReg
#define ST_ACTIVE    0x01u   // CMD2/CMD9 detectado
#define ST_IS9       0x02u   // 0=CMD2 (CID), 1=CMD9 (CSD)
#define ST_STATE_MSK 0x0Cu
#define ST_STATE_RSP 0x08u   // state==10 (RESPONSE)
#define ST_TX_RUN    0x10u

// CtrlReg
#define CT_TX_ARM    0x01u
#define CT_DET_CLR   0x02u

// Trama R2: 136 bits = 17 bytes. La tarjeta conduce el byte 0 (0x3F, header
// idéntico); el PSoC conduce los bytes 1..16 (bits 8..135), 16 bytes.
#define R2_BYTES     17u
#define R2_LAST      16u   // índice del último byte que transmite el PSoC

// ---------------------------------------------------------------------------
// CID y CSD a inyectar (16 bytes, formato Linux /sys/bus/mmc — MSB primero).
//
// Estructura CID (128 bits = 16 bytes):
//   Byte  0    : MID  — Manufacturer ID
//   Bytes 1-2  : OID  — OEM/Application ID (ASCII)
//   Bytes 3-7  : PNM  — Product Name (ASCII)
//   Byte  8    : PRV  — Product Revision (BCD mayor.menor)
//   Bytes 9-12 : PSN  — Serial Number (32 bits)
//   Bytes 13-14: MDT  — Manufacturing Date (año/mes)
//   Byte  15   : CRC7 + end bit (se recalcula siempre por el firmware)
// ---------------------------------------------------------------------------
#define SPOOF_CID { \
    0x1Bu, 0x53u, 0x4Du, 0x53u, 0x44u, 0x33u, 0x32u, 0x47u, \
    0x80u, 0xDEu, 0xADu, 0xBEu, 0xEFu, 0x01u, 0x23u, 0x00u  \
}
// MID=0x1B (Samsung), OID="SM", PNM="SD32G", PRV=8.0, PSN=0xDEADBEEF, MDT=Jan 2023

// CID real de la tarjeta de pruebas (leído con el monitor), por si hace falta:
//    0xADu, 0x4Cu, 0x53u, 0x4Du, 0x53u, 0x4Cu, 0x30u, 0x20u,
//    0x10u, 0x41u, 0x02u, 0x3Au, 0xEEu, 0x01u, 0x99u, 0x00u
// MID=0xAD, OID="LS", PNM="MSL0 ", PRV=1.0, PSN=0x41023AEE, MDT=Sep 2025

// CSD tipo 1 — ajustar según capacidad/tarjeta objetivo. El byte 15 (CRC) se recalcula.
// SPOOF_CSD_ENABLE=0: en CMD9 no se toma el bus y pasa el CSD real (PRUEBA).
#define SPOOF_CSD_ENABLE  0u
#define SPOOF_CSD { \
    0x40u, 0x0Eu, 0x00u, 0x32u, 0x5Bu, 0x59u, 0x00u, 0x00u, \
    0x1Du, 0x40u, 0x00u, 0x00u, 0x00u, 0x00u, 0x81u, 0x00u  \
}

static const uint8 spoof_cid[16] = SPOOF_CID;
static const uint8 spoof_csd[16] = SPOOF_CSD;

// Estado compartido con los ISR
static volatile uint8 g_ctrl;             // sombra del CtrlReg (write-only)
static uint8          g_frame_cid[R2_BYTES]; // tramas R2 precalculadas al arrancar
static uint8          g_frame_csd[R2_BYTES]; // (start/dir + datos + CRC)
static const uint8   *g_frame;            // trama en curso (apunta a una de las dos)
static volatile uint8 g_tx_idx;           // índice del próximo byte a entregar
static volatile uint8 g_tx_done;          // 1 cuando termina la inyección
static volatile uint8 g_is_cid;           // 1=se inyectó CID (CMD2), 0=CSD (CMD9)
static volatile uint8 g_missed;          // 1=CMD_DET llegó tarde, no se inyectó
static volatile uint8 g_skipped;         // 1=CMD9 con spoof de CSD desactivado

static void ctrl_commit(void)
{
    SD_CTRL_WRITE(g_ctrl);
}

// ---------------------------------------------------------------------------
// CRC7 para CID/CSD — G(x) = x^7 + x^3 + 1. Entrada: 15 bytes de contenido.
// ---------------------------------------------------------------------------
static uint8 crc7_compute(const uint8 *data, uint8 len)
{
    uint8 crc = 0u;
    for (uint8 i = 0u; i < len; i++) {
        uint8 b = data[i];
        for (uint8 j = 0u; j < 8u; j++) {
            // crc guarda los 7 bits en [6:0]: tras desplazar, su MSB queda en
            // bit 7 y se compara con el bit de datos entrante.
            crc = (uint8)(crc << 1u);
            if (((b ^ crc) & 0x80u) != 0u) crc ^= 0x09u;
            b <<= 1u;
        }
    }
    return crc & 0x7Fu;
}

// ---------------------------------------------------------------------------
// build_frame — construye la trama R2 de 17 bytes (MSB primero):
//   byte 0     = 0x3F  (start=0, dir=0, reserved=111111)
//   bytes 1-15 = contenido CID/CSD[127:8]
//   byte 16    = {CRC7[6:0], end_bit=1}
// ---------------------------------------------------------------------------
static void build_frame(uint8 *frame, const uint8 *data)
{
    frame[0] = 0x3Fu;
    for (uint8 i = 0u; i < 15u; i++) frame[1u + i] = data[i];
    frame[16] = (uint8)((crc7_compute(data, 15u) << 1u) | 1u);
}

// ---------------------------------------------------------------------------
// CMD_DET_ISR — CMD2/CMD9 detectado por el UDB.
// Elige la trama (precalculada), ceba el primer byte de contenido (frame[1]) y
// arma el spoof. El UDB tomará el bus (SW_IN) por hardware en el bit 8 de la
// respuesta real de la tarjeta, alineándose a su Ncr.
//
// Plazo: armar antes de la primera carga (ctr==5 de la respuesta), que llega
// >=48 ciclos de HOST_CLK después de CMD_DET (~120 us a 400 kHz). Por eso la
// trama NO se construye aquí (el CRC7 tarda ~150 us): sólo se elige el puntero.
// ---------------------------------------------------------------------------
CY_ISR(CMD_DET_ISR_Handler)
{
    uint8 st = SD_STATUS;

    CMD_DET_ISR_Disable();

    g_is_cid = (st & ST_IS9) ? 0u : 1u;

    // Spoof de CSD desactivado: no armar; el UDB deja pasar la R2 real y
    // limpia active al terminarla. El main loop rehabilita CMD_DET.
    if ((g_is_cid == 0u) && (SPOOF_CSD_ENABLE == 0u)) {
        g_skipped = 1u;
        g_tx_done = 1u;
        return;
    }

    // Si la respuesta ya empezó, armar ahora haría que el UDB arrancase en una
    // carga posterior (bits desalineados). Dejar pasar la respuesta real.
    if ((st & ST_STATE_MSK) == ST_STATE_RSP) {
        g_missed  = 1u;
        g_tx_done = 1u;
        return;
    }

    g_frame = g_is_cid ? g_frame_cid : g_frame_csd;
    SD_DATA_WRITE(g_frame[1]);      // ceba D0; frame[0]=0x3F lo conduce la tarjeta
    g_tx_idx = 2u;                  // siguiente byte a entregar

    g_ctrl |= CT_TX_ARM;            // arma: el UDB arranca en el bit 8 real
    ctrl_commit();

    LED_REQ_Write(1u);
    LED_RSP_Write(0u);
}

// ---------------------------------------------------------------------------
// BYTE_REQ_ISR — el UDB cargó un byte y pide el siguiente (frame[2..16]).
// Plazo: 8 ciclos de HOST_CLK (20 us a 400 kHz) hasta la siguiente carga. El
// ISR compilado son ~20 instrucciones + entrada/salida: ~3-4 us a 24 MHz.
// ---------------------------------------------------------------------------
CY_ISR(BYTE_REQ_ISR_Handler)
{
    if (g_tx_idx <= R2_LAST) {
        SD_DATA_WRITE(g_frame[g_tx_idx]);   // refresca D0 para la próxima carga
        g_tx_idx++;
    } else {
        // Ya entregados los 16 bytes (1..16). El UDB suelta el bus solo al
        // final del bit 135; desarmar evita que arranque en otra R2.
        g_ctrl &= (uint8)~CT_TX_ARM;        // desarmar (no corta el byte en curso)
        ctrl_commit();
        BYTE_REQ_ISR_Disable();
        g_tx_done = 1u;
    }
}

// ---------------------------------------------------------------------------
// emergency_clear — sólo si el UDB no terminó solo (p. ej. el host dejó de dar
// reloj a mitad de trama). det_clr actúa en un flanco de HOST_CLK: mantenerlo
// hasta ver IDLE y active=0 (o timeout si no hay reloj en absoluto).
// ---------------------------------------------------------------------------
static void emergency_clear(void)
{
    uint8 guard = 0u;

    g_ctrl = CT_DET_CLR;                // también desarma
    ctrl_commit();
    while (((SD_STATUS & (ST_STATE_MSK | ST_ACTIVE)) != 0u) && (guard < 50u)) {
        CyDelayUs(1u);
        guard++;
    }
    g_ctrl = 0u;
    ctrl_commit();
}

int main(void)
{
    // Arranque mínimo: el host empieza a inicializar la tarjeta en cuanto hay
    // alimentación, así que se arma la detección lo antes posible y sin
    // imprimir nada. No hace falta inicializar a mano:
    //   - globales g_*: ya valen 0 (.bss la pone a cero el arranque de C)
    //   - CtrlReg: arranca a 0 (cy_init_value); D0 de la datapath a 0xFF
    //   - ~EN del switch: P1[6] sin pin en TopDesign, el pull-down R9 lo
    //     mantiene a GND → switch siempre habilitado.

    // Tramas precalculadas (~150 us): el CMD_DET_ISR no tiene tiempo de
    // calcular el CRC7. CMD2 llega como pronto tras el bucle de ACMD41 del
    // host (decenas de ms), así que hay margen de sobra.
    build_frame(g_frame_cid, spoof_cid);
    build_frame(g_frame_csd, spoof_csd);
    g_frame = g_frame_cid;

    CMD_DET_ISR_StartEx(CMD_DET_ISR_Handler);
    BYTE_REQ_ISR_StartEx(BYTE_REQ_ISR_Handler);
    CyGlobalIntEnable;

    // UART sólo para los mensajes de diagnóstico tras cada inyección.
    UART_1_Start();

    for (;;) {
        if (g_tx_done != 0u) {
            uint16 guard = 0u;
            uint8  missed = g_missed;
            uint8  skipped = g_skipped;

            // El UDB termina solo: al final de la R2 (bit 135) suelta el bus
            // (tx_run=0), vuelve a IDLE y limpia active. Esperar a verlo antes
            // de rehabilitar CMD_DET (que es por nivel de active). También vale
            // para el caso "tarde": la R2 real acaba igual.
            // Timeout (136 bits a 100 kHz = 1.36 ms) por si el host dejara de
            // relojear: entonces reset de emergencia con det_clr.
            while (((SD_STATUS & (ST_ACTIVE | ST_TX_RUN)) != 0u) && (guard < 3000u)) {
                CyDelayUs(1u);
                guard++;
            }
            if (guard >= 3000u) {
                emergency_clear();
            }

            // Rearmar la detección ANTES de imprimir: UartPutString puede
            // bloquear ~2 ms y el siguiente CMD2/CMD9 llegaría con el ISR off.
            g_tx_idx  = 0u;
            g_missed  = 0u;
            g_skipped = 0u;
            g_tx_done = 0u;
            BYTE_REQ_ISR_Enable();
            CMD_DET_ISR_Enable();

            LED_REQ_Write(0u);
            if (skipped != 0u) {
                UART_1_UartPutString("CMD9: CSD real (spoof desactivado)\r\n");
            } else if (missed != 0u) {
                UART_1_UartPutString(g_is_cid ? "CMD2: CMD_DET tarde, NO inyectado\r\n"
                                              : "CMD9: CMD_DET tarde, NO inyectado\r\n");
            } else {
                LED_RSP_Write(1u);
                UART_1_UartPutString(g_is_cid ? "CMD2: inyectado SPOOF_CID\r\n"
                                              : "CMD9: inyectado SPOOF_CSD\r\n");
                LED_RSP_Write(0u);
            }
        } else if (((g_ctrl & CT_TX_ARM) != 0u) && ((SD_STATUS & ST_ACTIVE) == 0u)) {
            // Armado pero el UDB ya no tiene detección: el host mandó otro
            // comando sin que llegara la respuesta a CMD2/CMD9 (el UDB limpia
            // active al empezar un comando nuevo). Abortar y volver a escuchar.
            g_ctrl &= (uint8)~CT_TX_ARM;
            ctrl_commit();
            g_tx_idx = 0u;
            CMD_DET_ISR_Enable();

            LED_REQ_Write(0u);
            UART_1_UartPutString(g_is_cid ? "CMD2: sin respuesta, abortado\r\n"
                                          : "CMD9: sin respuesta, abortado\r\n");
        }
    }
}
