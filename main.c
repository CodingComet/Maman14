#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

int main(int argc, char *argv[])
{
    int i;
    char log_row_separator[] = "===";

    if (argc < 2)
    {
        printf("Usage: %s <.as file> [more .as files]\n", argv[0]);
        return EXIT_SUCCESS;
    }

    init_assembler();
    for (i = 1; i < argc; i++)
    {
        printf("%s\n", log_row_separator);
        printf("Processing file %d/%d `%s`...\n", i, argc - 1, argv[i]);
        if (!process_file(argv[i]))
            continue;

        printf("Failed to assemble file `%s`, See more in stderr.\n", argv[i]);
    }
    printf("%s\n", log_row_separator);
    terminate_assembler();
    fflush(stdout);

    return EXIT_SUCCESS;
}
