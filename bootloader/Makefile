#---------------------------------------------------------------------------
#
# Program name
#
PROGRAM    = optiboot

#---------------------------------------------------------------------------
#
# Toolchain and tool locations
#
TOOLROOT = ../../toolchain/install/bin
GCCROOT = $(TOOLROOT)/bin/

CC         = $(TOOLROOT)/avr-gcc
OBJCOPY    = $(TOOLROOT)/avr-objcopy
OBJDUMP    = $(TOOLROOT)/avr-objdump
SIZE       = $(TOOLROOT)/avr-size

OBJ        = $(PROGRAM).o
OPTIMIZE = -Os -fno-inline-small-functions -fno-split-wide-types -mrelax

DEFS = -DBAUD_RATE=57600 -DLED=B5 -DLED_START_FLASHES=3
LIBS =

#---------------------------------------------------------------------------
#
# Target
#
all: atmega328

atmega328: TARGET = atmega328
atmega328: MCU_TARGET = atmega328p
atmega328: AVR_FREQ = 8000000L
atmega328: CFLAGS += -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) -DF_CPU=$(AVR_FREQ) $(DEFS)
atmega328: LDSECTIONS  = -Wl,--section-start=.text=0x7e00 -Wl,--section-start=.version=0x7ffe
atmega328: LDFLAGS = $(LDSECTIONS) -Wl,--relax -nostartfiles -nostdlib
atmega328: $(PROGRAM)_atmega328.hex
atmega328: $(PROGRAM)_atmega328.lst

#---------------------------------------------------------------------------
#
# Generic build instructions
#
%.elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)
	$(SIZE) $@

clean:
	rm -rf *.o *.elf *.lst *.map *.sym *.lss *.eep *.srec *.bin *.hex *.tmp.sh

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -j .version --set-section-flags .version=alloc,load -O ihex $< $@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -j .version --set-section-flags .version=alloc,load -O srec $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -j .version --set-section-flags .version=alloc,load -O binary $< $@

.PHONY: clean all
