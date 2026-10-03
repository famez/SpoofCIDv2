#include <project.h>

#define SD_STATUS_REG   CY_GET_REG8(SD_CID_RESPONDER_1_StatusReg__STATUS_REG)
#define SD_CAPTURE_REG  CY_GET_REG8(SD_CID_RESPONDER_1_CaptureReg__STATUS_REG)

#define R2_BYTES  17u   // R2 = 136 bits = 17 bytes

static volatile uint8 g_buf[R2_BYTES];
static volatile uint8 g_idx;
static volatile uint8 g_cmd_type;   // 2 = CID, 9 = CSD
static volatile uint8 g_cmd_ready;  // nuevo comando detectado
static volatile uint8 g_data_ready; // 17 bytes capturados

static void uart_hex8(uint8 v)
{
    const char hex[] = "0123456789ABCDEF";
    UART_1_UartPutChar((uint32)hex[v >> 4u]);
    UART_1_UartPutChar((uint32)hex[v & 0x0Fu]);
}

// ISR: DBG0 — CMD2/CMD9 detectado (fase COMMAND, host→tarjeta)
// Los ISR deben ser mínimos: solo flags y LEDs, NUNCA UART
CY_ISR(SD_CMD_ISR_Handler)
{
    uint8 sta = SD_STATUS_REG;
    g_cmd_type  = (sta & 0x01u) ? 2u : 9u;
    g_idx       = 0u;
    g_data_ready = 0u;
    g_cmd_ready  = 1u;
    LED_REQ_Write(1u);   // host transmite
    LED_RSP_Write(0u);
}

// ISR: DBG1 — byte capturado listo (fase RESPONSE, tarjeta→host)
CY_ISR(SD_BYTE_ISR_Handler)
{
    if (g_idx < R2_BYTES)
        g_buf[g_idx++] = SD_CAPTURE_REG;

    if (g_idx == 1u) {
        LED_REQ_Write(0u);   // fin fase COMMAND
        LED_RSP_Write(1u);   // inicio fase RESPONSE
    }

    if (g_idx >= R2_BYTES)
        g_data_ready = 1u;
}

int main(void)
{
    CyGlobalIntEnable;

    UART_1_Start();
    UART_1_UartPutString("SpoofCIDv2 monitor ready\r\n");

    SD_CMD_ISR_StartEx(SD_CMD_ISR_Handler);
    SD_BYTE_ISR_StartEx(SD_BYTE_ISR_Handler);

    for (;;) {
        __asm("wfi");

        if (g_cmd_ready) {
            g_cmd_ready = 0u;
            UART_1_UartPutString(g_cmd_type == 2u
                ? "CMD2 ALL_SEND_CID\r\n"
                : "CMD9 SEND_CSD\r\n");
        }

        if (g_data_ready) {
            g_data_ready = 0u;

            // byte 0 = header 0x3F (start+dir+cmd_index), bytes 1-16 = CID/CSD
            UART_1_UartPutString(g_cmd_type == 2u ? "CID: " : "CSD: ");
            for (uint8 i = 1u; i < R2_BYTES; i++) {
                uart_hex8(g_buf[i]);
                UART_1_UartPutChar(' ');
            }
            UART_1_UartPutString("\r\n");

            CyDelay(200u);
            LED_RSP_Write(0u);
        }
    }
}
