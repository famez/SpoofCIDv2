#include <project.h>

#define PRT4_DR  (*(volatile uint32 *)0x40040400u)
#define LED_REQ  (1u << 2u)
#define LED_RSP  (1u << 3u)

#define SD_STATUS_ADDR   SD_CID_RESPONDER_1_StatusReg__STATUS_REG   /* ST_00 = 0x400F0060 */
#define SD_CTR_ADDR      SD_CID_RESPONDER_1_fifo_cnt_rd__STATUS_REG /* ST_03 = 0x400F0063 */
#define SD_STATUS_REG    CY_GET_REG8(SD_STATUS_ADDR)

static void uart_hex8(uint8 v)
{
    const char hex[] = "0123456789ABCDEF";
    UART_1_UartPutChar((uint32)hex[v >> 4u]);
    UART_1_UartPutChar((uint32)hex[v & 0x0Fu]);
}

int main(void)
{
    CyGlobalIntEnable;

    UART_1_Start();
    UART_1_UartPutString("SpoofCIDv2 monitor ready\r\n");

    UART_1_UartPutString("STATUS_ADDR=0x");
    uart_hex8((uint8)(SD_STATUS_ADDR >> 24u));
    uart_hex8((uint8)(SD_STATUS_ADDR >> 16u));
    uart_hex8((uint8)(SD_STATUS_ADDR >>  8u));
    uart_hex8((uint8)(SD_STATUS_ADDR));
    UART_1_UartPutString("\r\n");

    for (;;) {
        uint8 sta = SD_STATUS_REG;

        if (sta & 0x01u) {
            UART_1_UartPutString("CMD2 ALL_SEND_CID\r\n");
            PRT4_DR |= LED_REQ;
            while (SD_STATUS_REG & 0x01u) {}
            PRT4_DR &= ~LED_REQ;
        }

        if (sta & 0x02u) {
            UART_1_UartPutString("CMD9 SEND_CSD\r\n");
            PRT4_DR |= LED_RSP;
            while (SD_STATUS_REG & 0x02u) {}
            PRT4_DR &= ~LED_RSP;
        }
    }
}
