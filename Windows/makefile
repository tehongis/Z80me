CC=cl
CFLAGS=/nologo /W3 /EHsc /DUNICODE /D_UNICODE
LDFLAGS=/link user32.lib gdi32.lib

all: Z80me.exe

Z80me.exe: main.c ../libz80/z80.c
    $(CC) $(CFLAGS) main.c ../libz80/z80.c /FeZ80me.exe $(LDFLAGS)

clean:
    del Z80me.exe main.obj z80.obj
