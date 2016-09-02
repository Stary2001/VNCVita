TARGET = VNCVita
TITLE_ID = STAR00002
OBJS   = main.o vnc/vnc.o vnc/encodings.o

LIBS = -lvita2d -lfreetype -lpng -lz -ldebugnet -lSceTouch_stub -lSceDisplay_stub -lSceGxm_stub -lSceCtrl_stub -lSceNet_stub -lSceNetCtl_stub -lSceSysmodule_stub -lSceAppUtil_stub -lSceCommonDialog_stub -lm

PREFIX = $(VITASDK)/bin/arm-vita-eabi
DB = /home/stary2001/vita-headers/db.json

CC      = $(PREFIX)-gcc
CXX	= $(PREFIX)-g++
LD	= $(PREFIX)-ld

DEBUGGER_IP ?= $(shell ip addr list `ip route | grep default | grep -oP 'dev \K[a-z0-9]* '` | grep -oP 'inet \K[0-9\.]*')
DEBUGGER_PORT = 18194
DEFS = -DDEBUGGER_IP=\"$(DEBUGGER_IP)\" -DDEBUGGER_PORT=$(DEBUGGER_PORT)

CFLAGS = -Wl,-q -Wall -O3 -I$(VITASDK)/include -L$(VITASDK)/lib $(DEFS)

all: $(TARGET).vpk

$(TARGET).vpk: eboot.bin
	mkdir -p vpktmp/sce_sys || true
	mkdir vpktmp/sce_sys/livearea/contents -p || true
	vita-mksfoex -s TITLE_ID=$(TITLE_ID) "$(TARGET)" vpktmp/sce_sys/param.sfo
	cp UbuntuMono-R.ttf vpktmp/font.ttf
	cp eboot.bin vpktmp/eboot.bin
	cp meta/icon0.png vpktmp/sce_sys/icon0.png
	cp meta/template.xml vpktmp/sce_sys/livearea/contents/template.xml
	cp meta/bg.png vpktmp/sce_sys/livearea/contents/bg.png
	cp meta/startup.png vpktmp/sce_sys/livearea/contents/startup.png
	cd vpktmp && zip -r --symlinks ../$@ *

eboot.bin: $(TARGET).velf
	vita-make-fself $< $@

%.velf: %.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@ >/dev/null || true

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET)_fixup.elf $(TARGET).elf $(OBJS)

ENCODINGS = $(wildcard vnc/encodings/*.c)

vnc/encodings.o : $(ENCODINGS)

vpksend: $(TARGET).vpk
	curl -T $(TARGET).vpk ftp://$(PSVITAIP):1337/ux0:/
	@echo "Sent."

send: eboot.bin
	curl -T eboot.bin ftp://$(PSVITAIP):1337/ux0:/app/$(TITLE_ID)/
	@echo "Sent."
