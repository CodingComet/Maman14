#include "parser.h"
#include "assembler.h"
#include "hash_table.h"
#include "vector.h"

#define COMMANDS 16
#define REGISTERS 9
#define DIRECTIVES 5

#define MAX_SYMBOL_LENGTH 30

static FILE *fp_ob;
static FILE *fp_ent;
static FILE *fp_ext;

static hash_table symbol_table;
static hash_table command_table;
static hash_table register_table;
static hash_table directive_table;
static hash_table external_symbol_table;

static vector data;
static vector commands;

static unsigned int IC; /*data counter;*/
static unsigned int DC; /*instruction counter*/
static unsigned int L;  /*current word counter*/

typedef enum
{
    IMMEDIATE,
    DIRECT,
    REGISTER,
    INDEXED
} addressing_mode;

typedef command_field (*operand_encoder)(char *);

#define BASE 32
#define MAX_SIZE 2 

char * convert_to_base32(unsigned int num) {
    char arr[BASE] = "!@#$%^&*<>abcdefghijklmnopqrstuv";
    char * ret = malloc(MAX_SIZE + 1); /* extra room for '\0' */

    ret[BASE] = '\0';
    ret[0] = arr[(num & 0b1111100000) / BASE]; /* 5 last bits */
    ret[1] = arr[num & 0b11111]; /* 5 first bits */

    return ret;
}

unsigned int get_addressing_mode(const char *token)
{
    /* TODO: see table in page 32 */
    /* TODO: check if PSW is valid register for regular usage */

    pair *p;
    char *dot;

    if ('#' == token[0])
        return 0;
    if (p = table_get(&register_table, token))
        return 3;
    if ((dot = strpbrk(token, ".")) && strlen(token) > dot - token + 1)
        return 2;
    return 1;
}

unsigned int get_additional_wordc(addressing_mode mode)
{
    unsigned int additional_wordc[] = {
        1, 1, 2, 1};

    return additional_wordc[mode];
}

command_field nullary(char *token)
{
    command_field res = {
        .ARE = 0,
        .source_operand = 0,
        .destination_operand = 0};
    L = 0;

    return res;
}

command_field unary(char *token)
{
    command_field res = {
        .destination_operand = get_addressing_mode(token)};

    L = get_additional_wordc(res.destination_operand);

    return res;
}

command_field binary(char *token)
{
    command_field res = {
        .destination_operand = get_addressing_mode(strtok(NULL, delim)),
        .source_operand = get_addressing_mode(token)};

    L = get_additional_wordc(res.destination_operand) + get_additional_wordc(res.source_operand) -
        (res.destination_operand == res.source_operand && res.destination_operand == 2); /* 2 registers in 1 word */

    return res;
}

operand_encoder get_command_parser(unsigned int command)
{
    if (command < 4)
        return binary;
    if (command < 14)
        return unary;
    return nullary;
}

command_field parse_command(unsigned int opcode, char *token)
{
    token = strtok(NULL, delim);
    command_field res = get_command_parser(opcode)(token);
    res.opcode = opcode;

    /*TODO: ERROR CHECK*/

    /*A is not dependent on memory*/
    /*R is dependent on local symbol*/
    /*E is dependent on external symbol*/

    return res;
}

void init_assembler()
{
    int i;
    char *commands[COMMANDS] = {
        "mov",
        "cmp",
        "add",
        "sub",
        "not",
        "clr",
        "lea",
        "inc",
        "dec",
        "jmp",
        "bne",
        "get",
        "prn",
        "jsr",
        "rts",
        "hlt"};

    char *directives[DIRECTIVES] = {
        ".data",
        ".string",
        ".struct",
        ".entry",
        ".extern"};

    char *registers[REGISTERS] = {
        "r0",
        "r1",
        "r2",
        "r3",
        "r4",
        "r5",
        "r6",
        "r7",
        "PSW"
        /* TODO: "check PSW" */
    };

    symbol_table = create_table();
    command_table = table_from_array(commands, COMMANDS);
    directive_table = table_from_array(directives, DIRECTIVES);
    register_table = table_from_array(registers, REGISTERS);
    external_symbol_table = create_table();

    /*1.*/ DC = 0;
    /*2.*/ IC = 0;
    L = 0;
}

char *begin_assembler(const char *file_name)
{
    static bool is_initialized = false;
    if (!is_initialized)
    {
        init_assembler();
        is_initialized = true;
    }

    char *outfile = malloc(strlen(file_name) + 2);
    outfile[strlen(file_name) + 1] = '\0';

    strcpy(outfile, file_name);

    strcpy(outfile + strlen(file_name) - 3, ".ob");
    outfile[strlen(file_name)] = '\0';
    fp_ob = fopen(outfile, "w");

    return outfile;
}

void assembler_parse(const char *line, char *line_copy, char *token)
{
    if (token == NULL) /* empty */
        return;
    if (token[0] == ';')
        return;

    L = 0;
    pair *p;
    command c;
    symbol symbol;
    bool symbol_flag = false;
    char symbol_name[MAX_SYMBOL_LENGTH];
    size_t length = strlen(token);

    if (':' == token[length - 1]) /*3.*/
    {
        if (length - 1 > MAX_SYMBOL_LENGTH)
        {
            printf("Symbol name too long!");
            /* ERROR */
        }

        symbol_flag = true;
        symbol_name[length - 1] = '\0';
        memcpy(symbol_name, token, length - 1);
    }

    if (symbol_flag)
        token = strtok(NULL, delim);
    if (p = table_get(&directive_table, token))
    {
        /*
        ".data",
        ".string",
        ".struct",
        ".entry",
        ".extern"
        */
        switch (*(int *)p->value)
        {
        case 0:
            break; /* .data */
        case 1:
            break; /* .string */
        case 2:
            break; /* .struct */
        case 3:
            break; /* .entry */
        case 4:
            break; /* .extern */
        }
        return;
    }
    else /* 11. */
    {
        if (symbol_flag)
        {
            if (!table_get(&symbol_table, symbol_name))
            {
                symbol.ptr = IC;
                symbol.type = CODE;
                table_insert(&symbol_table, symbol_name, &symbol, sizeof(symbol));
            }
            else
            {
                printf("Symbol already defined: %s\n", line);
                /*ERROR*/
            }
        }
    }

    if (!(p = table_get(&command_table, token)))
    {
        printf("Command doesn't exist: %s\n");
        /*ERROR*/
        return;
    }

    c.command_binary = parse_command(*(int *)p->value, token);

    IC += L;

    if (symbol_name)
        free(symbol_name);
}

void end_assembler()
{
    fclose(fp_ob);
    /* TODO: check if used */
#if 0
    fclose(fp_ent);
    fclose(fp_ext);
#endif

#if 1 /* TODO: if there are errrors */
    return;
#else
    /* step 17, 18 */
#endif
}

void terminate_assembler()
{
    free_table(&directive_table);
    free_table(&symbol_table);
    free_table(&command_table);
    free_table(&external_symbol_table);
}