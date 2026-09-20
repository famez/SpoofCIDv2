CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
FLAGS   = -mcpu=cortex-m0 -mthumb -O1 -nostartfiles -nostdlib -T link.ld

all: firmware.hex

firmware.elf: startup.s main.c
	$(CC) $(FLAGS) -o $@ $^

firmware.hex: firmware.elf
	$(OBJCOPY) -O ihex $< $@

flash: firmware.hex
	openocd -f interface/cmsis-dap.cfg -f target/psoc4.cfg \
	        -c "program firmware.hex verify reset exit"

clean:
	rm -f *.elf *.hex
