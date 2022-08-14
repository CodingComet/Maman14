
# Compiler:
CC = gcc
CFLAGS = -Wall -pedantic -ansi -Wno-unused-variable -Wno-format -Wno-incompatible-pointer-types -Wno-declaration-after-statement


TARGET = assembler

ifeq ($(OS), Windows_NT)
RM = del # Delete file in windows
else
RM = rm # Delete file in unix
endif

$(TARGET): main.c
	$(CC) -o $(TARGET).o $(CFLAGS) hash_table.c vector.c preprocessor.c assembler.c parser.c main.c

clean:
	$(RM) $(TARGET).o
