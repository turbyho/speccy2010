CD     = cd
CAT    = cat
RM     = rm -f

MAKE   = make
CC     = gcc
CFLAGS = -Os -s

SJASMPLUS = sjasmplus --nologo

BIN2H   = _utils/bin2h
FNTEX   = _utils/fntex
SCRPACK = _utils/scrpack

BOOTLOGO_SRC = bootlogo/speccy2010.logo.scr
BOOTLOGO_DST = ../arm/shell/logo.h

FNT_CLASSIC_SRC = fonts/classic.fnt
FNT_CLASSIC_DST = ../arm/shell/font0.h

#------------------------------------------------------------------------------
default: all

all: bootlogo fonts snaploader bsdos mb02


mb02: bsdos
	$(BIN2H) bsdos308s/bootstrap.bin "epromBootstrap" "SPEC_MB02_BOOTSTAP_H_INCLUDED" > bsdos308s/bootstrap.h

bsdos: bsdos308s/bsdos308s.a80
	$(CD) bsdos308s; $(SJASMPLUS) --lst=bsdos308s.lst bsdos308s.a80; \
	$(CAT) Xch140x1.bin bsdos308s.bin > bsdos308s.rom

snaploader: snaploader/loader.a80
	$(CD) snaploader; $(SJASMPLUS) loader.a80
	$(BIN2H) snaploader/loader.bin "snapshotBootstrap" "SPEC_SNAPSHOT_BOOTSTRAP_H_INCLUDED" > snaploader/loader.h

fonts: bin2h fntex
	$(FNTEX) $(FNT_CLASSIC_SRC)
	$(BIN2H) "$(FNT_CLASSIC_SRC).bin" "shellFont0" "SHELL_FONT0_H_INCLUDED" > $(FNT_CLASSIC_DST)

bootlogo: bin2h scrpack
	$(SCRPACK) $(BOOTLOGO_SRC)
	$(BIN2H) "$(BOOTLOGO_SRC).bin" "logoPacked" "SHELL_LOGO_H_INCLUDED" > $(BOOTLOGO_DST)

scrpack: $(SCRPACK).c
	$(CC) $(CFLAGS) -o $(SCRPACK) $<

fntex: $(FNTEX).c
	$(CC) $(CFLAGS) -o $(FNTEX) $<

bin2h: $(BIN2H).c
	$(CC) $(CFLAGS) -o $(BIN2H) $<

clean:
	@$(RM) \
		$(BOOTLOGO_SRC).bin \
		$(FNT_CLASSIC_SRC).bin \
		snaploader/loader.bin \
		snaploader/loader.h \
		bsdos308s/bsdos308s.lst \
		bsdos308s/bsdos308s.bin \
		bsdos308s/bsdos308s.rom \
		bsdos308s/bootstrap.bin \
		bsdos308s/bootstrap.h

distclean: clean
	$(RM) $(BIN2H) $(FNTEX) $(SCRPACK)

.PHONY: bin2h fntex scrpack snaploader bsdos clean distclean
.SUFFIXES: .c .a80
