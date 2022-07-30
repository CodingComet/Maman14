#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hash_table.h"

typedef struct
{
    unsigned int ARE : 2;
    unsigned int destination_operand : 2;
    unsigned int source_operand : 2;
    unsigned int opcode : 4;
} command_field;

typedef union
{
    command_field command_binary;
    int command_decimal;
} command;

typedef enum
{
    DATA,
    CODE
} symbol_type;

typedef struct
{
    unsigned int ptr;
    symbol_type type;
} symbol;

void init_assembler();
char *begin_assembler(const char *file_name);
void assembler_parse(const char *line, char *line_copy, char *token);
void end_assembler();
void terminate_assembler();
