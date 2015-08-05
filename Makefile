TARGET = VNCVita
OBJS   = main.o gui/font_data.o gui/font.o gui/label.o gui/pane.o gui/gui.o util/list.o vnc/vnc.o

LIBS = -lvita2d -lSceKernel_stub -lSceDisplay_stub -lSceGxm_stub	\
	-lSceCtrl_stub -lSceNet_stub -lpng -lz

PREFIX  = $(DEVKITARM)/bin/arm-none-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wall -specs=psp2.specs -O3
ASFLAGS = $(CFLAGS)

all: $(TARGET)_fixup.elf

%_fixup.elf: %.elf
	psp2-fixup -q -S $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET)_fixup.elf $(TARGET).elf $(OBJS)

copy: $(TARGET)_fixup.elf
	@cp $(TARGET)_fixup.elf /mnt/rejuvenate-0.3-test4/
	@echo "Copied!"
