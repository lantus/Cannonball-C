# Project: Cannonball
# Compiler: m68k-Amiga-OS3
# Compiler Type: MingW 3
# Makefile created by wxDev-C++ 6.10.2 on 07/07/17 09:41

CPP       = m68k-amigaos-g++.exe
CC        = m68k-amigaos-gcc.exe
WINDRES   = windres.exe
OBJ       = obj/audio.o obj/input.o obj/rendersw.o obj/timer.o obj/hwroad.o obj/hwsprites.o obj/hwtiles.o obj/segapcm.o obj/ym2151.o obj/config.o obj/menu.o obj/ttrial.o obj/osound.o obj/osoundint.o obj/oanimseq.o obj/oattractai.o obj/obonus.o obj/ocrash.o obj/oentry.o obj/oferrari.o obj/ohiscore.o obj/ohud.o obj/oinitengine.o obj/oinputs.o obj/olevelobjs.o obj/ologo.o obj/omap.o obj/omusic.o obj/ooutputs.o obj/opalette.o obj/oroad.o obj/osmoke.o obj/osprite.o obj/osprites.o obj/ostats.o obj/otiles.o obj/otraffic.o obj/outils.o obj/outrun.o obj/asyncserial.o obj/interface.o obj/main.o obj/romloader.o obj/roms.o obj/trackloader.o obj/utils.o obj/video.o obj/xmlutils.o obj/crc.o obj/sxmlc.o obj/sxmlsearch.o obj/midimusic.o obj/amiga_timer.o obj/amiga_video.o
LINKOBJ   = obj/audio.o obj/input.o obj/rendersw.o obj/timer.o obj/hwroad.o obj/hwsprites.o obj/hwtiles.o obj/segapcm.o obj/ym2151.o obj/config.o obj/menu.o obj/ttrial.o obj/osound.o obj/osoundint.o obj/oanimseq.o obj/oattractai.o obj/obonus.o obj/ocrash.o obj/oentry.o obj/oferrari.o obj/ohiscore.o obj/ohud.o obj/oinitengine.o obj/oinputs.o obj/olevelobjs.o obj/ologo.o obj/omap.o obj/omusic.o obj/ooutputs.o obj/opalette.o obj/oroad.o obj/osmoke.o obj/osprite.o obj/osprites.o obj/ostats.o obj/otiles.o obj/otraffic.o obj/outils.o obj/outrun.o obj/asyncserial.o obj/interface.o obj/main.o obj/romloader.o obj/roms.o obj/trackloader.o obj/utils.o obj/video.o obj/xmlutils.o obj/crc.o obj/sxmlc.o obj/sxmlsearch.o obj/midimusic.o obj/amiga_timer.o obj/amiga_video.o
LIBS      = -L"C:/Development/AmiDevCpp/usr/local/amiga/m68k-amigaos/lib" -L"C:/Development/AmiDevCpp/usr/local/amiga/m68k-amigaos/lib/libb/libnix" -s -noixemul -noixemul src/main/amiga/PTPlay30B.o  -s 
INCS      = -I"C:/Development/AmiDevCpp/usr/local/amiga/m68k-amigaos/sys-include" -I"C:/Development/AmiDevCpp/usr/local/amiga/m68k-amigaos/sys-include/SDL" -I"src/main" -I"src/main/SDL" -I"src/main/Amiga"
CXXINCS   = -I"C:/Development/AmiDevCpp/usr/local/amiga/m68k-amigaos/sys-include" -I"C:/Development/AmiDevCpp/usr/local/amiga/m68k-amigaos/sys-include/SDL" -I"src/main" -I"src/main/SDL" -I"src/main/Amiga"
RCINCS    = --include-dir "C:/DEVELO~1/AMIDEV~1/include"
BIN       = release/Cannonball-C.exe
DEFINES   =  -D_AMIGA_
CXXFLAGS  = $(CXXINCS) $(DEFINES)    -s -noixemul -fexpensive-optimizations -O3
CFLAGS    = $(INCS) $(DEFINES) -m68020-60 -msoft-float -fomit-frame-pointer   -s -noixemul -fexpensive-optimizations -O3
GPROF     = gprof.exe
RM        = rm -f
LINK      = m68k-amigaos-g++.exe

.PHONY: all all-before all-after clean clean-custom
all: all-before $(BIN) all-after

clean: clean-custom
	$(RM) $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(LINK) $(LINKOBJ) -o "release\Cannonball-C.exe" $(LIBS)

obj/audio.o: $(GLOBALDEPS) src/main/sdl/audio.c
	$(CC) -c src/main/sdl/audio.c -o obj/audio.o $(CFLAGS)

obj/input.o: $(GLOBALDEPS) src/main/sdl/input.c
	$(CC) -c src/main/sdl/input.c -o obj/input.o $(CFLAGS)

obj/rendersw.o: $(GLOBALDEPS) src/main/sdl/rendersw.c src/main/sdl/rendersw.h src/main/stdint.h src/main/globals.h src/main/stdint.h src/main/setup.h
	$(CC) -c src/main/sdl/rendersw.c -o obj/rendersw.o $(CFLAGS)

obj/timer.o: $(GLOBALDEPS) src/main/sdl/timer.c
	$(CC) -c src/main/sdl/timer.c -o obj/timer.o $(CFLAGS)

obj/hwroad.o: $(GLOBALDEPS) src/main/hwvideo/hwroad.c
	$(CC) -c src/main/hwvideo/hwroad.c -o obj/hwroad.o $(CFLAGS)

obj/hwsprites.o: $(GLOBALDEPS) src/main/hwvideo/hwsprites.c
	$(CC) -c src/main/hwvideo/hwsprites.c -o obj/hwsprites.o $(CFLAGS)

obj/hwtiles.o: $(GLOBALDEPS) src/main/hwvideo/hwtiles.c
	$(CC) -c src/main/hwvideo/hwtiles.c -o obj/hwtiles.o $(CFLAGS)

obj/segapcm.o: $(GLOBALDEPS) src/main/hwaudio/segapcm.c
	$(CC) -c src/main/hwaudio/segapcm.c -o obj/segapcm.o $(CFLAGS)

obj/ym2151.o: $(GLOBALDEPS) src/main/hwaudio/ym2151.c
	$(CC) -c src/main/hwaudio/ym2151.c -o obj/ym2151.o $(CFLAGS)

obj/config.o: $(GLOBALDEPS) src/main/frontend/config.c src/main/frontend/config.h
	$(CC) -c src/main/frontend/config.c -o obj/config.o $(CFLAGS)

obj/menu.o: $(GLOBALDEPS) src/main/frontend/menu.c src/main/frontend/menu.h
	$(CC) -c src/main/frontend/menu.c -o obj/menu.o $(CFLAGS)

obj/ttrial.o: $(GLOBALDEPS) src/main/frontend/ttrial.c
	$(CC) -c src/main/frontend/ttrial.c -o obj/ttrial.o $(CFLAGS)

obj/osound.o: $(GLOBALDEPS) src/main/engine/audio/osound.c
	$(CC) -c src/main/engine/audio/osound.c -o obj/osound.o $(CFLAGS)

obj/osoundint.o: $(GLOBALDEPS) src/main/engine/audio/osoundint.c
	$(CC) -c src/main/engine/audio/osoundint.c -o obj/osoundint.o $(CFLAGS)

obj/oanimseq.o: $(GLOBALDEPS) src/main/engine/oanimseq.c
	$(CC) -c src/main/engine/oanimseq.c -o obj/oanimseq.o $(CFLAGS)

obj/oattractai.o: $(GLOBALDEPS) src/main/engine/oattractai.c
	$(CC) -c src/main/engine/oattractai.c -o obj/oattractai.o $(CFLAGS)

obj/obonus.o: $(GLOBALDEPS) src/main/engine/obonus.c
	$(CC) -c src/main/engine/obonus.c -o obj/obonus.o $(CFLAGS)

obj/ocrash.o: $(GLOBALDEPS) src/main/engine/ocrash.c
	$(CC) -c src/main/engine/ocrash.c -o obj/ocrash.o $(CFLAGS)

obj/oentry.o: $(GLOBALDEPS) src/main/engine/oentry.c src/main/engine/oentry.h
	$(CC) -c src/main/engine/oentry.c -o obj/oentry.o $(CFLAGS)

obj/oferrari.o: $(GLOBALDEPS) src/main/engine/oferrari.c
	$(CC) -c src/main/engine/oferrari.c -o obj/oferrari.o $(CFLAGS)

obj/ohiscore.o: $(GLOBALDEPS) src/main/engine/ohiscore.c
	$(CC) -c src/main/engine/ohiscore.c -o obj/ohiscore.o $(CFLAGS)

obj/ohud.o: $(GLOBALDEPS) src/main/engine/ohud.c
	$(CC) -c src/main/engine/ohud.c -o obj/ohud.o $(CFLAGS)

obj/oinitengine.o: $(GLOBALDEPS) src/main/engine/oinitengine.c
	$(CC) -c src/main/engine/oinitengine.c -o obj/oinitengine.o $(CFLAGS)

obj/oinputs.o: $(GLOBALDEPS) src/main/engine/oinputs.c
	$(CC) -c src/main/engine/oinputs.c -o obj/oinputs.o $(CFLAGS)

obj/olevelobjs.o: $(GLOBALDEPS) src/main/engine/olevelobjs.c src/main/trackloader.h src/main/globals.h src/main/stdint.h
	$(CC) -c src/main/engine/olevelobjs.c -o obj/olevelobjs.o $(CFLAGS)

obj/ologo.o: $(GLOBALDEPS) src/main/engine/ologo.c
	$(CC) -c src/main/engine/ologo.c -o obj/ologo.o $(CFLAGS)

obj/omap.o: $(GLOBALDEPS) src/main/engine/omap.c
	$(CC) -c src/main/engine/omap.c -o obj/omap.o $(CFLAGS)

obj/omusic.o: $(GLOBALDEPS) src/main/engine/omusic.c
	$(CC) -c src/main/engine/omusic.c -o obj/omusic.o $(CFLAGS)

obj/ooutputs.o: $(GLOBALDEPS) src/main/engine/ooutputs.c
	$(CC) -c src/main/engine/ooutputs.c -o obj/ooutputs.o $(CFLAGS)

obj/opalette.o: $(GLOBALDEPS) src/main/engine/opalette.c src/main/trackloader.h src/main/globals.h src/main/stdint.h
	$(CC) -c src/main/engine/opalette.c -o obj/opalette.o $(CFLAGS)

obj/oroad.o: $(GLOBALDEPS) src/main/engine/oroad.c
	$(CC) -c src/main/engine/oroad.c -o obj/oroad.o $(CFLAGS)

obj/osmoke.o: $(GLOBALDEPS) src/main/engine/osmoke.c
	$(CC) -c src/main/engine/osmoke.c -o obj/osmoke.o $(CFLAGS)

obj/osprite.o: $(GLOBALDEPS) src/main/engine/osprite.c
	$(CC) -c src/main/engine/osprite.c -o obj/osprite.o $(CFLAGS)

obj/osprites.o: $(GLOBALDEPS) src/main/engine/osprites.c src/main/trackloader.h src/main/globals.h src/main/stdint.h
	$(CC) -c src/main/engine/osprites.c -o obj/osprites.o $(CFLAGS)

obj/ostats.o: $(GLOBALDEPS) src/main/engine/ostats.c
	$(CC) -c src/main/engine/ostats.c -o obj/ostats.o $(CFLAGS)

obj/otiles.o: $(GLOBALDEPS) src/main/engine/otiles.c src/main/trackloader.h src/main/globals.h src/main/stdint.h
	$(CC) -c src/main/engine/otiles.c -o obj/otiles.o $(CFLAGS)

obj/otraffic.o: $(GLOBALDEPS) src/main/engine/otraffic.c
	$(CC) -c src/main/engine/otraffic.c -o obj/otraffic.o $(CFLAGS)

obj/outils.o: $(GLOBALDEPS) src/main/engine/outils.c
	$(CC) -c src/main/engine/outils.c -o obj/outils.o $(CFLAGS)

obj/outrun.o: $(GLOBALDEPS) src/main/engine/outrun.c
	$(CC) -c src/main/engine/outrun.c -o obj/outrun.o $(CFLAGS)

obj/asyncserial.o: $(GLOBALDEPS) src/main/cannonboard/asyncserial.c
	$(CC) -c src/main/cannonboard/asyncserial.c -o obj/asyncserial.o $(CFLAGS)

obj/interface.o: $(GLOBALDEPS) src/main/cannonboard/interface.c src/main/cannonboard/interface.h
	$(CC) -c src/main/cannonboard/interface.c -o obj/interface.o $(CFLAGS)

obj/main.o: $(GLOBALDEPS) src/main/main.c src/main/sdl/timer.h src/main/sdl/input.h src/main/Video.h src/main/stdint.h src/main/globals.h src/main/stdint.h src/main/roms.h src/main/romloader.h src/main/frontend/config.h src/main/hwvideo/hwtiles.h src/main/hwvideo/hwsprites.h src/main/hwvideo/hwroad.h src/main/romloader.h src/main/trackloader.h src/main/globals.h src/main/stdint.h src/main/main.h src/main/globals.h src/main/sdl/audio.h src/main/setup.h src/main/frontend/config.h src/main/frontend/menu.h src/main/cannonboard/interface.h src/main/engine/oinputs.h src/main/engine/outrun.h src/main/engine/oaddresses.h src/main/engine/osprites.h src/main/engine/oentry.h src/main/engine/osprite.h src/main/engine/outrun.h src/main/engine/oroad.h src/main/engine/oinitengine.h src/main/engine/outrun.h src/main/engine/audio/OSoundInt.h src/main/engine/ooutputs.h src/main/engine/omusic.h src/main/engine/outrun.h
	$(CC) -c src/main/main.c -o obj/main.o $(CFLAGS)

obj/romloader.o: $(GLOBALDEPS) src/main/romloader.c src/main/stdint.h src/main/romloader.h src/main/thirdparty/crc/crc.h
	$(CC) -c src/main/romloader.c -o obj/romloader.o $(CFLAGS)

obj/roms.o: $(GLOBALDEPS) src/main/roms.c src/main/stdint.h src/main/roms.h src/main/romloader.h
	$(CC) -c src/main/roms.c -o obj/roms.o $(CFLAGS)

obj/trackloader.o: $(GLOBALDEPS) src/main/trackloader.c src/main/trackloader.h src/main/globals.h src/main/stdint.h src/main/roms.h src/main/romloader.h src/main/engine/outrun.h src/main/engine/oaddresses.h src/main/engine/osprites.h src/main/engine/oentry.h src/main/engine/osprite.h src/main/engine/outrun.h src/main/engine/oroad.h src/main/engine/oinitengine.h src/main/engine/outrun.h src/main/engine/audio/OSoundInt.h src/main/engine/oaddresses.h
	$(CC) -c src/main/trackloader.c -o obj/trackloader.o $(CFLAGS)

obj/utils.o: $(GLOBALDEPS) src/main/utils.c src/main/utils.h src/main/stdint.h src/main/setup.h src/main/engine/outrun.h src/main/engine/oaddresses.h src/main/engine/osprites.h src/main/engine/oentry.h src/main/engine/osprite.h src/main/engine/outrun.h src/main/engine/oroad.h src/main/engine/oinitengine.h src/main/engine/outrun.h src/main/engine/audio/OSoundInt.h
	$(CC) -c src/main/utils.c -o obj/utils.o $(CFLAGS)

obj/video.o: $(GLOBALDEPS) src/main/video.c src/main/Video.h src/main/stdint.h src/main/globals.h src/main/stdint.h src/main/roms.h src/main/romloader.h src/main/frontend/config.h src/main/hwvideo/hwtiles.h src/main/hwvideo/hwsprites.h src/main/hwvideo/hwroad.h src/main/setup.h src/main/globals.h src/main/sdl/rendergl.h src/main/stdint.h src/main/sdl/rendersw.h src/main/stdint.h
	$(CC) -c src/main/video.c -o obj/video.o $(CFLAGS)

obj/xmlutils.o: $(GLOBALDEPS) src/main/xmlutils.c src/main/xmlutils.h src/main/stdint.h src/main/thirdparty/sxmlc/sxmlc.h src/main/thirdparty/sxmlc/sxmlsearch.h src/main/thirdparty/sxmlc/sxmlc.h src/main/utils.h src/main/stdint.h
	$(CC) -c src/main/xmlutils.c -o obj/xmlutils.o $(CFLAGS)

obj/crc.o: $(GLOBALDEPS) src/main/thirdparty/crc/crc.c src/main/thirdparty/crc/crc.h
	$(CC) -c src/main/thirdparty/crc/crc.c -o obj/crc.o $(CFLAGS)

obj/sxmlc.o: $(GLOBALDEPS) src/main/thirdparty/sxmlc/sxmlc.c src/main/thirdparty/sxmlc/sxmlc.h
	$(CC) -c src/main/thirdparty/sxmlc/sxmlc.c -o obj/sxmlc.o $(CFLAGS)

obj/sxmlsearch.o: $(GLOBALDEPS) src/main/thirdparty/sxmlc/sxmlsearch.c src/main/thirdparty/sxmlc/sxmlc.h src/main/thirdparty/sxmlc/sxmlsearch.h src/main/thirdparty/sxmlc/sxmlc.h
	$(CC) -c src/main/thirdparty/sxmlc/sxmlsearch.c -o obj/sxmlsearch.o $(CFLAGS)

obj/midimusic.o: $(GLOBALDEPS) src/main/amiga/midimusic.c
	$(CC) -c src/main/amiga/midimusic.c -o obj/midimusic.o $(CFLAGS)

obj/amiga_timer.o: $(GLOBALDEPS) src/main/amiga/amiga_timer.c
	$(CC) -c src/main/amiga/amiga_timer.c -o obj/amiga_timer.o $(CFLAGS)

obj/amiga_video.o: $(GLOBALDEPS) src/main/amiga/amiga_video.c src/main/amiga/video.h
	$(CC) -c src/main/amiga/amiga_video.c -o obj/amiga_video.o $(CFLAGS)
