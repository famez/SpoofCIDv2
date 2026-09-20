#include <stdint.h>

#define PRT4_DR (*(volatile uint32_t *)0x40040400u)
#define PRT4_PC (*(volatile uint32_t *)0x40040408u)

#define LED_REQ (1u << 2)   /* P4.2 */
#define LED_RSP (1u << 3)   /* P4.3 */

static void delay(volatile uint32_t n) { while (n--) {} }

int main(void)
{
    PRT4_PC = (6u << 6) | (6u << 9);

    for (;;) {
        PRT4_DR &= ~(LED_REQ | LED_RSP);   /* nada */
        delay(4000000u);
        PRT4_DR |= LED_REQ;                 /* primero */
        delay(4000000u);
        PRT4_DR |= LED_RSP;                 /* primero y segundo */
        delay(4000000u);
        PRT4_DR &= ~LED_REQ;               /* segundo */
        delay(4000000u);
    }
}
