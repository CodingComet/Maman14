#include "parser.h"

void parse_line(parse_line_cb parse_callback, const char *line)
{
    static char line_copy[MAX_LINE_LENGTH];

    strcpy(line_copy, line);
    char *token = strtok(line_copy, delim);

    parse_callback(line, line_copy, token);
}

char *parse_file(const char *file_name, char *(*begin_callback)(const char *f), void (*end_callback)(), parse_line_cb parse_callback)
{
    FILE *fp_in;
    fp_in = fopen(file_name, "r");

    if (!fp_in)
    {
        printf("Error opening file %s\n", file_name);
        return NULL;
    }

    char line[MAX_LINE_LENGTH+1];
    char *res = begin_callback(file_name);
    while (fgets(line, MAX_LINE_LENGTH, fp_in))
        parse_line(parse_callback, line);
    end_callback();
    fclose(fp_in);
    return res;
}

int process_file(const char *file_name)
{
    char *preprocessor_res, *assembler_res;

    if (!(preprocessor_res = parse_file(file_name, begin_preprocessor, end_preprocessor, preprocessor_parse)))
        return EXIT_FAILURE;
    if (!(assembler_res = parse_file(preprocessor_res, begin_assembler, end_first_pass, assembler_parse)))
        return EXIT_FAILURE;

    if (!assembler_get_errors())
        parse_file(preprocessor_res, begin_second_pass, end_assembler, secondary_assembler_parse);
    else
        end_assembler();

    free(assembler_res);
    free(preprocessor_res);

    return assembler_get_errors();
}

char *replace_file_extension(const char *file_name, const char *extension)
{
    char *dot = strrchr(file_name, '.');
    int length = dot - file_name;
    char *res = malloc(length + 1 + strlen(extension) + 1);
    res[length + 1 + strlen(extension)] = 0;
    memcpy(res, file_name, length);
    strcpy(res + length + 1, extension);
    res[length] = '.';

    return res;
}
