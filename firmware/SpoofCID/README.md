# SpoofCID — firmware bare-metal PSoC 4

Firmware de monitorización del bus SD para SpoofCIDv2 (CY8C4245AXI-483).

## Estado actual

**Fase 1 — monitor (solo lectura).** El UDB detecta CMD2/CMD9 en hardware
leyendo HOST_CLK (P2.2) y SD_CMD (P2.3). La CPU reporta los comandos
detectados por UART. No escribe nada en el bus SD.

## Arquitectura

```
SD host ──CLK──► P2.2 ──► UDB fabric (SD_CID_RESPONDER.v)
         ──CMD──► P2.3 ──►      │
                                │ StatusReg (0x400f0062)
                                ▼
                              CPU ──► UART (P4.1, 115200 baud)
```

El bitstream UDB se carga desde arrays de bytes en `udb_init()`.
Extraído de PSoC Creator 4.4 con HOST_CLK=P2[2], SD_CMD=P2[3].

La CPU usa IRQ 7 (udb_interrupt) o polling según `USE_UDB_IRQ` en `main.c`.

## Salida UART esperada

```
SpoofCIDv2 monitor ready
CMD2 (ALL_SEND_CID)
CMD9 (SEND_CSD)
```

## Requisitos

- `arm-none-eabi-gcc`
- `openocd` con soporte CMSIS-DAP

## Compilar y flashear

```bash
make        # genera firmware.hex
make flash  # flashea vía OpenOCD
```

## Pines

| Pin  | Función          | Dirección |
|------|------------------|-----------|
| P2.2 | HOST_CLK (SD)    | Entrada   |
| P2.3 | SD_CMD           | Entrada   |
| P4.0 | UART RX (SCB0)   | Entrada   |
| P4.1 | UART TX (SCB0)   | Salida    |
| P4.2 | LED_REQ          | Salida    |
| P4.3 | LED_RSP          | Salida    |

## Archivos

- `main.c` — firmware principal
- `startup.s` — tabla de vectores + Reset_Handler
- `link.ld` — linker script para PSoC 4 (32 KB flash, 4 KB RAM)
- `SD_CID_RESPONDER.v` — fuente Verilog del componente UDB (referencia)
- `Makefile` — build + flash

## Depuración UDB pendiente

El UDB puede no detectar comandos si:
1. Los blobs de bitstream no corresponden exactamente a los pines físicos
2. El número de IRQ del UDB en este chip no es el 7
3. Los registros de estado tienen direcciones distintas a las esperadas

Ver issues en el repositorio para el plan de depuración.
