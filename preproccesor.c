#include "preproccesor.h"
#include "parser.h"

void begin_preprocessor(const char *file_name, char *outfile)
{
    macro_table = create_table();

    fp = fopen(outfile, "w");

    return outfile;
}

void preprocessor_parse(const char *line, char *line_copy, char *token)
{
    static char *macro_name = NULL;
    static char *macro_body = NULL;
    static bool in_macro = false;

    if (in_macro)
    {
        if (token && 0 == strcmp(token, "endmacro"))
        {
            in_macro = false;
            table_insert(&macro_table, macro_name, macro_body, strlen(macro_body) + 1);

            free(macro_body);
            free(macro_name);
            macro_body = NULL;
            macro_name = NULL;
            return;
        }

        int len = strlen(macro_body);
        int line_len = strlen(line);
        int new_len = len + line_len + 1;
        macro_body = realloc(macro_body, new_len);
        strcat(macro_body + len, line);
        macro_body[new_len - 1] = '\0';
        return;
    }
    if (token)
    {
        pair *p = table_get(&macro_table, token);
        if (p)
        {
            fwrite(p->value, 1, strlen(p->value), fp);
            return;
        }
        if (0 == strcmp(token, "macro"))
        {
            in_macro = true;

            token = strtok(NULL, delim);
            /* TODO: if (!token) return; */

            macro_name = malloc(strlen(token) + 1);
            strcpy(macro_name, token);

            macro_body = malloc(1);
            macro_body[0] = '\0';
            return;
        }
    }
    fwrite(line, sizeof(char), strlen(line), fp);
}

void end_preprocessor()
{
    fclose(fp);
    free_table(&macro_table);
}