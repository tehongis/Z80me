
CC = gcc
CFLAGS = -Wall
LDFLAGS = -L. -lz80 -lSDL2
OBJ = main.o
TARGET = Z80me

run	: $(TARGET)
	./$(TARGET)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f *.o $(TARGET)
