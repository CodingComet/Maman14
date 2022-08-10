#include "parser.h"
#include "preproccesor.h"

static FILE *fp = NULL;
static hash_table macro_table;

static char *outfile;

char *begin_preprocessor(const char *file_name)
{
    outfile = replace_file_extension(file_name, "am");
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
            /* Insert macro body into marco table. */
            in_macro = false;
            table_insert(&macro_table, macro_name, macro_body, strlen(macro_body) + 1);

            free(macro_body);
            free(macro_name);
            macro_body = NULL;
            macro_name = NULL;
            return;
        }

        /* Insert line into macro. */
        int len = strlen(macro_body);
        int line_len = strlen(line);
        int new_len = len + line_len + 1;
        macro_body = realloc(macro_body, new_len); /* Realloc macro body. */
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
        if (0 == strcmp(token, "macro")) /* Start macro */
        {
            in_macro = true;

            token = strtok(NULL, delim);

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
    printf("Generated file %s.\n", outfile);
    fclose(fp);
    free_table(&macro_table);
}