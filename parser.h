#pragma once
#include "hash_table.h"
#include "assembler.h"
#include "preprocessor.h"

#define MAX_LINE_LENGTH 81

static const char *delim = " \t\n,";

typedef void (*parse_line_cb)(const char *line, char *line_copy, char *token);

void parse_line(parse_line_cb parse_callback, const char *line);
char *parse_file(const char *file_name, char *(*begin_callback)(const char *f), void (*end_callback)(), parse_line_cb parse_callback);
int process_file(const char *file_name);

char *replace_file_extension(const char* file_name, const char* extension);
