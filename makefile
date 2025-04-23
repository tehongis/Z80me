
CC = gcc
CFLAGS = -Wall -Ilibz80
OBJ = main.o libz80/z80.o
TARGET = Z80me

run	: $(TARGET)
	./$(TARGET)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

libz80/z80.o: libz80/z80.c libz80/z80.h
	$(CC) $(CFLAGS) -c libz80/z80.c -o libz80/z80.o

CPMjump.bin	:	CPM/CPMjump.z80
		z80asm -o CPMjump.bin CPM/CPMjump.z80

CPM22.bin	:	CPM/CPM22.Z80
		z80asm -o CPM22.bin CPM/CPM22.Z80

clean:
	rm -f *.o libz80/*.o $(TARGET)
