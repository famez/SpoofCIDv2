#include <project.h>

#define SD_STATUS_REG   CY_GET_REG8(SD_CID_RESPONDER_1_StatusReg__STATUS_REG)
#define SD_CAPTURE_REG  CY_GET_REG8(SD_CID_RESPONDER_1_CaptureReg__STATUS_REG)

// R2 = 136 bits; FSM consume 2 bits (start+dir) → 134 bits contados → 16 bytes completos
#define R2_BYTES  16u

static volatile uint8 g_cid_buf[R2_BYTES];
static volatile uint8 g_csd_buf[R2_BYTES];
static volatile uint8 g_cid_ready;
static volatile uint8 g_csd_ready;
static volatile uint8 g_cid_cmd;   // señal: CMD2 detectado, aún sin datos
static volatile uint8 g_csd_cmd;   // señal: CMD9 detectado, aún sin datos

static volatile uint8 g_capturing; // 2 = capturando CID, 9 = capturando CSD
static volatile uint8 g_idx;

static void uart_hex8(uint8 v)
{
    const char hex[] = "0123456789ABCDEF";
    UART_1_UartPutChar((uint32)hex[v >> 4u]);
    UART_1_UartPutChar((uint32)hex[v & 0x0Fu]);
}

static void uart_buf(const volatile uint8 *buf, uint8 n)
{
    for (uint8 i = 0u; i < n; i++) {
        uart_hex8(buf[i]);
        UART_1_UartPutChar(' ');
    }
    UART_1_UartPutString("\r\n");
}

// ISR: DBG0 — CMD2/CMD9 detectado (fase COMMAND)
CY_ISR(SD_CMD_ISR_Handler)
{
    SD_CMD_ISR_Disable();
    uint8 sta = SD_STATUS_REG;
    if (sta & 0x01u) {          // cmd2 = 1
        g_capturing = 2u;
        g_cid_ready  = 0u;
        g_idx        = 0u;
        g_cid_cmd    = 1u;
    } else {                    // cmd9 = 1
        g_capturing = 9u;
        g_csd_ready  = 0u;
        g_idx        = 0u;
        g_csd_cmd    = 1u;
    }
    LED_REQ_Write(1u);
    LED_RSP_Write(0u);
}

// ISR: DBG1 — byte listo (fase RESPONSE, 1 ciclo después de byte_ready)
CY_ISR(SD_BYTE_ISR_Handler)
{
    if (g_idx < R2_BYTES) {
        uint8 b = SD_CAPTURE_REG;
        if (g_capturing == 2u)
            g_cid_buf[g_idx] = b;
        else
            g_csd_buf[g_idx] = b;
        g_idx++;
    }

    if (g_idx == 1u) {
        LED_REQ_Write(0u);
        LED_RSP_Write(1u);
    }

    if (g_idx >= R2_BYTES) {
        if (g_capturing == 2u)
            g_cid_ready = 1u;
        else
            g_csd_ready = 1u;
        SD_CMD_ISR_Enable();
    }
}

int main(void)
{
    CyGlobalIntEnable;
    UART_1_Start();
    UART_1_UartPutString("SpoofCIDv2 monitor ready\r\n");
    SD_CMD_ISR_StartEx(SD_CMD_ISR_Handler);
    SD_BYTE_ISR_StartEx(SD_BYTE_ISR_Handler);

    for (;;) {
        if (g_cid_ready) {
            g_cid_ready = 0u;
            g_cid_cmd   = 0u;
            UART_1_UartPutString("CMD2 ALL_SEND_CID\r\nCID: ");
            uart_buf(g_cid_buf, R2_BYTES);
            LED_RSP_Write(0u);
        }
        if (g_csd_ready) {
            g_csd_ready = 0u;
            g_csd_cmd   = 0u;
            UART_1_UartPutString("CMD9 SEND_CSD\r\nCSD: ");
            uart_buf(g_csd_buf, R2_BYTES);
            LED_RSP_Write(0u);
        }
    }
}
