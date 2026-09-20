# led_test — diagnóstico hardware PSoC 4

**DIRECTORIO CONGELADO — no modificar.**

Firmware mínimo para verificar que la PCB y el programador funcionan
correctamente antes de cualquier desarrollo. Si este test falla, el
problema es de hardware o de flasheo, no de software.

## Qué hace

Ciclo continuo de LEDs en P4.2 (LED_REQ) y P4.3 (LED_RSP):

```
apagado → LED_REQ → ambos → LED_RSP → apagado → ...
```

Cada fase dura ~1 segundo. No usa UART, UDB ni interrupciones.

## Requisitos

- `arm-none-eabi-gcc`
- `openocd` con soporte CMSIS-DAP
- Programador: Raspberry Pi Pico con firmware DebugProbe

## Compilar y flashear

```bash
make        # genera firmware.hex
make flash  # flashea vía OpenOCD (CMSIS-DAP)
```

Comando OpenOCD directo:

```bash
openocd -f interface/cmsis-dap.cfg -f target/psoc4.cfg \
        -c "program firmware.hex verify reset exit"
```

## Conexiones programador

| Pico (DebugProbe) | PCB SpoofCIDv2 |
|-------------------|----------------|
| GP2 (SWCLK)       | SWCLK          |
| GP3 (SWDIO)       | SWDIO          |
| GND               | GND            |
