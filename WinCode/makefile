CC=cl
CFLAGS=/nologo /W3 /EHsc /DUNICODE /D_UNICODE
LDFLAGS=/link user32.lib gdi32.lib

all: SimpleWin.exe

SimpleWin.exe: SimpleWin.c libz80/z80.c
    $(CC) $(CFLAGS) SimpleWin.c libz80/z80.c /FeSimpleWin.exe $(LDFLAGS)

clean:
    del SimpleWin.exe
