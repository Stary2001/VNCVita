TARGET = VNCVita
OBJS   = main.o vnc/vnc.o vnc/encodings.o

LIBS = -lvita2d -ldebugnet -lSceTouch_stub -lSceDisplay_stub -lSceGxm_stub -lSceCtrl_stub -lSceNet_stub -lSceNetCtl_stub -lm

PREFIX = $(VITASDK)/bin/arm-vita-eabi
DB = /home/stary2001/vita-headers/db.json

CC      = $(PREFIX)-gcc
CXX	= $(PREFIX)-g++
LD	= $(PREFIX)-ld

CFLAGS  = -Wl,-q -Wall -O3 -I$(VITASDK)/include -L$(VITASDK)/lib

all: $(TARGET).velf

%.velf: %.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@ $(DB) >/dev/null

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET)_fixup.elf $(TARGET).elf $(OBJS)

ENCODINGS = $(wildcard vnc/encodings/*.c)

vnc/encodings.o : $(ENCODINGS)
