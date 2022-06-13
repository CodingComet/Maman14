#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hash_table.h"

static FILE *fp = NULL;
static hash_table macro_table;

void begin_preprocessor(const char *file_name, char* outfile);
void preprocessor_parse(const char *line, char *line_copy, char *token);
void end_preprocessor();
