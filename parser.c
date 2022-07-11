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
        return EXIT_FAILURE;
    }

    char line[MAX_LINE_LENGTH];
    char *res = begin_callback(file_name); /*TODO: assert? res*/
    while (fgets(line, MAX_LINE_LENGTH, fp_in) != NULL)
        parse_line(parse_callback, line);
    fclose(fp_in);
    end_callback();
    return res;
}

int process_file(const char *file_name)
{
    char *preprocessor_res, *assembler_res;
    preprocessor_res = parse_file(file_name, begin_preprocessor, end_preprocessor, preprocessor_parse);
    assembler_res = parse_file(preprocessor_res, begin_assembler, end_assembler, assembler_parse);

    free(assembler_res);
    free(preprocessor_res);

    return EXIT_SUCCESS;
}
