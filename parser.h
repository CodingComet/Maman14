#pragma once
#include "hash_table.h"
#define MAX_LINE_LENGTH 80

typedef enum
{
    PREPROCESSOR,
    ASSEMBLER
} ASSEMBLER_STATES;

static const char *delim = " \t\n";

int process_file(const char *file_name);
void parse_line(void (*parse_callback)(const char *line, char *line_copy, char *token), const char *line);