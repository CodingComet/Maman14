#include "parser.h"
#include "preproccesor.h"

void parse_line(void (*parse_callback)(const char *line, char *line_copy, char *token), const char *line)
{
    static char line_copy[MAX_LINE_LENGTH];

    strcpy(line_copy, line);
    char *token = strtok(line_copy, delim);

    parse_callback(line, line_copy, token);
}

int process_file(const char *file_name)
{
    FILE *fp_in;
    fp_in = fopen(file_name, "r");

    if (!fp_in)
    {
        printf("Error opening file %s\n", file_name);
        return EXIT_FAILURE;
    }

    char line[MAX_LINE_LENGTH];
    char *outfile = malloc(strlen(file_name) + 1);

    strcpy(outfile, file_name);
    strcpy(outfile + strlen(file_name) - 3, ".am");
    outfile[strlen(file_name)] = '\0';

    begin_preprocessor(file_name, outfile);
    while (fgets(line, MAX_LINE_LENGTH, fp_in) != NULL)
        parse_line(preprocessor_parse, line);
    end_preprocessor();

    fclose(fp_in);

    return EXIT_SUCCESS;
}
