#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hash_table.h"

char *begin_preprocessor(const char *file_name);
void preprocessor_parse(const char *line, char *line_copy, char *token);
void end_preprocessor();