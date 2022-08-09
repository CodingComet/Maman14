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
        process_file(argv[i]);
    terminate_assembler();
    fflush(stdout);

    return EXIT_SUCCESS;
}
