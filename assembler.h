#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum
{
    ABSOLUTE,
    EXTERNAL,
    RELOCATABLE
} ARE;

typedef enum
{
    IMMEDIATE,
    DIRECT,
    INDEXED,
    REGISTER
} addressing_mode;

typedef enum
{
    INT,
    STRING,
    STRUCT
} datatype;

typedef struct
{
    int ARE : 2;
    int destination_operand : 2;
    int source_operand : 2;
    unsigned opcode : 4;
} command_field;

typedef struct
{
    int ARE : 2;
    int data : 8;
} additional_word_field;

typedef union
{
    additional_word_field word_binary;
    int word_decimal;
} additional_word;

typedef command_field (*operand_encoder)(char *);

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
    int ptr;
    symbol_type type;
} symbol;

void init_assembler();
char *begin_assembler(const char *file_name);
void assembler_parse(const char *line, char *line_copy, char *token);
void secondary_assembler_parse(const char *line, char *line_copy, char *token);
void end_first_pass();
void end_assembler();
void terminate_assembler();
bool assembler_get_errors();
