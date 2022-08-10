#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

int main(int argc, char *argv[])
{
    int i;

    if (argc < 2)
    {
        printf("Usage: %s <.as file> [more .as files]\n", argv[0]);
        return EXIT_SUCCESS;
    }

    init_assembler();
    for (i = 1; i < argc; i++)
    {
        printf("=====================================\n");
        printf("Processing file %s, %d\\%d...\n", argv[i], i, argc - 1);
        if (!process_file(argv[i]))
            continue;

        printf("Failed to assemble file %s, See more in stderr.\n", argv[i]);
    }
    printf("=====================================\n");
    terminate_assembler();
    fflush(stdout);

    return EXIT_SUCCESS;
}
