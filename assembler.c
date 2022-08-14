#include "parser.h"
#include "vector.h"
#include "assembler.h"
#include "hash_table.h"

#define COMMANDS 16
#define REGISTERS 8
#define DIRECTIVES 5

#define BASE 32
#define MAX_SIZE 2

#define MEMORY_START 100
#define MAX_SYMBOL_LENGTH MAX_LINE_LENGTH

#define IS_SYMBOL(token, length) ':' == token[length - 1]

char *convert_to_base32(unsigned int num)
{
    char arr[BASE] = "!@#$%^&*<>abcdefghijklmnopqrstuv";
    char *ret = malloc(MAX_SIZE + 1); /* extra room for '\0'. */

    ret[MAX_SIZE] = 0;
    ret[0] = arr[(num & 31 << 5) / BASE]; /* 5 last bits. */
    ret[1] = arr[num & 31];               /* 5 first bits. */

    return ret;
}

/* Global state: */
static char *in_file;

static hash_table symbol_table;
static hash_table command_table;
static hash_table register_table;
static hash_table directive_table;
static hash_table externs;
static hash_table entries;

static vector data;
static vector instructions;

static char *outfile;

static size_t L;  /* current word counter. */
static size_t DC; /* data counter. */
static size_t LC; /* line counter for error tracking. */
static size_t IC; /* instruction counter for second pass. */
static bool HAS_ERRORS;

void raise(char *msg) /* Set error flag and output to stderr. */
{
    HAS_ERRORS = true;
    fprintf(stderr, "In line %zu ERROR: %s! Check .am file.\n", LC, msg);
}

char *generate_ext(char *file_name)
{
    if (1 > externs.size)
        return NULL;

    /* Replace file extension. */
    char *ext_file_name = replace_file_extension(file_name, "ext");

    /* Creating the .ext file. */
    FILE *ext_file;
    if (!(ext_file = fopen(ext_file_name, "w")))
    {
        raise("Can't open .ext file.\n");
        return NULL;
    }
    int i = 0;
    pair *p;
    for (; i < TABLE_SIZE; i++)
    {
        p = externs.table[i];
        while (p)
        {
            int addr_dec = *(int *)p->value;

            if (addr_dec == 0xdeadbeef) /* Hexadecimal place holder. */
            {
                p = p->next;
                continue;
            }
            char *addr_b32 = convert_to_base32(addr_dec + MEMORY_START);
            char *line = malloc(strlen(p->key) + 4); /* Allocate line for address, and token. */

            /* Construct line. */
            memcpy(line, p->key, strlen(p->key));
            line[strlen(p->key)] = '\t';
            memcpy(line + strlen(p->key) + 1, addr_b32, 3);
            line[strlen(p->key) + 3] = '\0';

            fputs(line, ext_file);
            fputc('\n', ext_file);
            free(line);
            free(addr_b32);
            p = p->next;
        }
    }
    fclose(ext_file);
    return ext_file_name;
}

char *generate_ent(char *file_name)
{
    if (1 > entries.size)
        return NULL;

    /* Creating the .ent file. */
    char *ent_file_name = replace_file_extension(file_name, "ent");

    FILE *ent_file;

    if (!(ent_file = fopen(ent_file_name, "w")))
    {
        raise("Can't open .ent file\n");
        return NULL;
    }
    int i = 0;
    for (; i < TABLE_SIZE; i++)
    {
        pair *p = entries.table[i];
        while (p)
        {
            int addr_dec = *(int *)p->value;
            char *addr_b32 = convert_to_base32(addr_dec);
            char *line = (char *)malloc(strlen(p->key) + 4);

            memcpy(line, p->key, strlen(p->key));
            line[strlen(p->key)] = ' ';
            memcpy(line + strlen(p->key) + 1, addr_b32, 3);
            line[strlen(p->key) + 3] = '\0';
            fputs(line, ent_file);
            fputc('\n', ent_file);

            free(line);
            free(addr_b32);
            p = p->next;
        }
    }
    fclose(ent_file);
    return ent_file_name;
}

void generate_ob(char *outfile)
{
    FILE *ob_file;
    ob_file = fopen(outfile, "w");
    if (!ob_file)
    {
        raise("Can't open .ob file");
        return;
    }

    int i = 0;
    char *p;
    char *address;

    /* Write instructions to output file. */
    for (; i < instructions.size; i++)
    {
        p = convert_to_base32(instructions.data[i]);
        address = convert_to_base32(i + MEMORY_START);

        fputs(address, ob_file);
        fputc('\t', ob_file);
        fputs(p, ob_file);
        fputc('\n', ob_file);

        free(p);
        free(address);
    }

    /* Write data to output file. */
    for (i = 0; i < data.size; i++)
    {
        p = convert_to_base32(data.data[i]);
        address = convert_to_base32(i + MEMORY_START + instructions.size);

        fputs(address, ob_file);
        fputc('\t', ob_file);
        fputs(p, ob_file);
        fputc('\n', ob_file);

        free(p);
        free(address);
    }

    fclose(ob_file);
}

bool assembler_get_errors()
{
    return HAS_ERRORS;
}

addressing_mode get_addressing_mode(const char *token)
{
    pair *p;
    char *dot = strchr(token, '.');

    if ('#' == token[0])
        return IMMEDIATE;
    if ((p = table_get(&register_table, token)))
        return REGISTER;
    if (dot && strlen(token) > dot - token + 1)
        return INDEXED;
    return DIRECT;
}

unsigned int get_additional_wordc(addressing_mode mode)
{
    unsigned int additional_wordc[] = {1, 1, 2, 1};

    /*
    Additional word counts:
    IMMEDIATE - 1,
    DIRECT - 1,
    INDEXED - 2,
    REGISTER - 1.
    */

    return additional_wordc[mode];
}

void check_too_many_args()
{
    char* rest = strtok(NULL, delim);
    if (rest && rest[0] != ';')
        raise("Too many args");
}

command_field nullary(char *token)
{
    if (token)
        raise("Too many args");

    command res;
    res.command_decimal = 0; /* Initialize command to empty. */

    return res.command_binary;
}

command_field unary(char *token) /* Destination operand only. */
{
    if(!token)
        raise("Not enough args");

    command_field res;
    res.destination_operand = get_addressing_mode(token);

    L = get_additional_wordc(res.destination_operand);

    return res;
}

command_field binary(char *token) /* Both operands. */
{
    command_field res;

    char *dest = strtok(NULL, delim);
    if (!token || !dest)
    {
        raise("Not enough args");
        return res;
    }

    res.destination_operand = get_addressing_mode(dest);
    res.source_operand = get_addressing_mode(token);

    L = (get_additional_wordc(res.destination_operand) + get_additional_wordc(res.source_operand)) -
            (res.destination_operand == res.source_operand && res.destination_operand == REGISTER);

    return res;
}

void encode_string(char *token)
{
    int i = 0;
    token = strtok(NULL, "");
    bool inString = false;
    int parenthesis = 0; /* Checks if string is valid. */

    for (; i < strlen(token); i++) /* Iterate over string. */
    {
        if (token[i] == '"') /* String between "". */
        {
            if (inString)
            {
                parenthesis++;
                break;
            }

            inString = true; /* Begin string. */
            parenthesis++;
            continue;
        }
        if (inString)
        {
            vector_push_back(&data, token[i]); /* Push character to data. */
            ++DC;
        }
    }

    if (parenthesis!=2)
        raise("Invalid string defenition");

    vector_push_back(&data, 0); /* Null terminated string. */
    ++DC;
}

/* ASCII to integer wrapper: */
int atoi_wrapper(char* str, int* res)
{
    int val = atoi(str);
    if (!val && *str != '0') /* Invalid integer */
    {
        *res = 1;
        return val;
    }
    if (atof(str) != val) /* Float error */
    {
        *res = 2;
        return val;
    }

    *res = 0; /* Valid */
    return val;
}

void encode_data(char *token)
{
    int res;
    int val = atoi_wrapper(token, &res);
    if (res == 1)
        raise("Invalid data(int) definition");
    else if (res == 2)
        raise("No support for floating-point numbers");
    vector_push_back(&data, val); /* Push parsed token(integer) to data. */
    ++DC;
}

void encode(datatype type, char *token)
{
    switch (type)
    {
    case INT:
        /* List of integers. */
        while ((token = strtok(NULL, delim)))
            encode_data(token);
        break;

    case STRING:
        encode_string(token);
        break;

    case STRUCT:
        /* data, string. */
        token = strtok(NULL, delim);
        encode_data(token);
        encode_string(token);
        break;
    }
}

operand_encoder get_command_parser(unsigned int command)
{
    if (command <= 4) /* mov-lea. */
        return binary;
    if (command <= 13) /* not-jsr. */
        return unary;
    return nullary; /* rts, hlt. */
}

command_field parse_command(unsigned int opcode, char *token)
{
    token = strtok(NULL, delim);
    command_field res = get_command_parser(opcode)(token);

    check_too_many_args();

    res.opcode = opcode;
    res.ARE = ABSOLUTE; /* ABSOLUTE for commands. */

    return res;
}

void init_assembler()
{
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
        "r7"};

    /* Storing keywords for parsing */
    command_table = table_from_array(commands, COMMANDS);
    directive_table = table_from_array(directives, DIRECTIVES);
    register_table = table_from_array(registers, REGISTERS);
}

char *begin_assembler(char *file_name)
{
    LC = 0;

    in_file = file_name;

    /* Store(and return) output file name. */
    outfile = replace_file_extension(file_name, "ob");

    /* 1. */ DC = 0;
    HAS_ERRORS = false;

    /* Initialize assembler data collectors. */
    data = create_vector(2);
    instructions = create_vector(2);

    entries = create_table();
    symbol_table = create_table();
    externs = create_table();

    return outfile;
}

void assembler_parse(const char *line, char *line_copy, char *token)
{
    LC++;

    if (token == NULL) /* Empty. */
        return;
    if (token[0] == ';')
        return;

    L = 0;
    pair *p;
    symbol s;
    command c;
    unsigned int i = 0xdeadbeef; /* Place holder for addresses. */
    bool symbol_flag = false;
    char symbol_name[MAX_SYMBOL_LENGTH];
    size_t length = strlen(token);

    if (IS_SYMBOL(token, length)) /* 3. */
    {
        if (length - 1 > MAX_SYMBOL_LENGTH)
            raise("Symbol name too long");

        symbol_flag = true;

        /* Copy data into symbol name. */
        symbol_name[length - 1] = '\0';
        memcpy(symbol_name, token, length - 1);
    
        token = strtok(NULL, delim);
        if (table_get(&symbol_table, symbol_name))
            raise("Symbol already defined");
    }
    if ((p = table_get(&directive_table, token))) /* 5. - 9. */
    {
        switch (*(int *)p->value)
        {
        case 3:
            token = strtok(NULL, delim);
            if (table_get(&entries, token))
                raise("Entry already defined");
            table_insert(&entries, token, &i, sizeof(int));
            break; /* .entry */
        case 4:
            token = strtok(NULL, delim);
            if (table_get(&externs, token))
                raise("Extern already defined");
            table_insert(&externs, token, &i, sizeof(int));
            break; /* .extern */
        default:   /* .data, .string, .struct */
            s.ptr = DC;
            s.type = DATA;
            table_insert(&symbol_table, symbol_name, &s, sizeof(s));
            encode(*(datatype *)p->value, token);
            break;
        }

        return;
    }
    else /* 11. */
    {
        if (symbol_flag)
        {
            s.ptr = instructions.size;
            s.type = CODE;
            table_insert(&symbol_table, symbol_name, &s, sizeof(s));
        }
    }

    if (!(p = table_get(&command_table, token)))
    {
        raise("Command doesn't exist");
        return;
    }
    
    c.command_binary = parse_command(*(int *)p->value, token);
    
    vector_push_back(&instructions, c.command_decimal);
    for (i = 0; i < L; i++)                      /* Insert additional words as placeholders. */
        vector_push_back(&instructions, 0xC0DE); /* 0xC0DE is hexadecimal placeholder for operands.*/
}

void end_first_pass()
{
    pair *p;
    int i = 0;
    symbol *s;

    for (; i < TABLE_SIZE; i++)
    {
        if (!(p = symbol_table.table[i]))
            continue;
        s = p->value;

        if (s->type == DATA)
            s->ptr += instructions.size; /* 17. */
        s->ptr += MEMORY_START;          /* Start memory at 100. */
    }

    if(instructions.size + data.size > 155)
        raise("Memory limit exceeded");

    IC = 0;
    LC = 0;
}

void parse_operand(char *token, int mode, int IC)
{
    int val, res;
    pair *p;
    additional_word w;
    w.word_decimal = 0;

    switch (mode)
    {
    case -1: /* Case for storing two registers in single word. */
        w.word_decimal = instructions.data[IC - 1];
        w.word_binary.data += atoi(token + 1);
        instructions.data[IC - 1] = w.word_decimal;
        L = 0;
        break;

    case IMMEDIATE:
        w.word_binary.ARE = ABSOLUTE;
        val = atoi_wrapper(token+1, &res);
        if (res)
            raise("Invalid immediate");
        w.word_binary.data = val; /* Get number after #. */
        instructions.data[IC] = w.word_decimal;
        L = 1;

        break;

    case DIRECT:
        if ((p = table_get(&symbol_table, token))) /* Check if token is a defined symbol. */
        {
            w.word_binary.ARE = RELOCATABLE;
            w.word_binary.data = (*(symbol *)p->value).ptr;
        }
        else
        {
            if ((p = table_get(&externs, token)))                     /* Check if symbol is external. */
                table_insert(&externs, token, &IC, sizeof(size_t)); /* Save symbol usage. */
            w.word_binary.ARE = EXTERNAL;
            w.word_binary.data = 0;
        }

        instructions.data[IC] = w.word_decimal;
        L = 1;

        break;

    case INDEXED:
    {
        char *dot = strchr(token, '.');

        /* Generate symbol from token with indexed access. */
        char symbol_name[MAX_SYMBOL_LENGTH];
        symbol_name[dot - token] = 0;
        memcpy(symbol_name, token, dot - token);

        if ((p = table_get(&symbol_table, symbol_name))) /* Check if token is a defined symbol. */
        {
            w.word_binary.ARE = RELOCATABLE;
            w.word_binary.data = (*(symbol *)p->value).ptr;
        }
        else
        {
            if ((p = table_get(&externs, symbol_name))) /* Check if symbol is external and save usage. */
                table_insert(&externs, symbol_name, &IC, sizeof(size_t));
            w.word_binary.ARE = EXTERNAL;
            w.word_binary.data = 0;
        }


        instructions.data[IC] = w.word_decimal;

        /* Index */
        w.word_binary.ARE = ABSOLUTE;
        val = atoi_wrapper(dot + 1, &res);
        if (res)
            raise("Invalid index");
        else if (val < 1 || val > 2)
            raise("Index out of range");
        w.word_binary.data = val; /* Get number after the dot(S1.1) -> 1. */
        instructions.data[IC + 1] = w.word_decimal;
        L = 2;

        break;
    }

    case REGISTER:
        w.word_binary.ARE = ABSOLUTE;
        w.word_binary.data = atoi(token + 1) << 4; /* Set second half of data to register. */
        instructions.data[IC] = w.word_decimal;
        L = 1;

        break;

    default:
        L = 0;
        break;
    }
}

void begin_second_pass()
{
    HAS_ERRORS = 0;
}

/* Pass no. 2 */
void secondary_assembler_parse(const char *line, char *line_copy, char *token)
{
    if (token == NULL) /* empty */
        return;
    if (token[0] == ';')
        return;

    L = 0;
    LC++;

    int i = 0;
    command c;
    pair *p1, *p2;
    operand_encoder oe;
    int length = strlen(token);

    if (IS_SYMBOL(token, length)) /* If defining a symbol ignore and get next token. */
        token = strtok(NULL, delim);

    if ((p1 = table_get(&directive_table, token))) /* 4. */
    {
        if (*(int *)p1->value != 3)
            return;
        /* 5. */
        token = strtok(NULL, delim);
        p1 = table_get(&entries, token);
        p2 = table_get(&symbol_table, token);

        free(p1->value);
        p1->value = malloc(sizeof(int));
        memcpy(p1->value, p2->value, sizeof(int));
        return;
    }

    p1 = table_get(&command_table, token);

    /* First operand */
    token = strtok(NULL, delim);

    c.command_decimal = instructions.data[IC++];
    oe = get_command_parser(c.command_binary.opcode);
    if (oe == nullary)
        return;

    if (oe == binary)
    {
        parse_operand(token, c.command_binary.source_operand, IC);
        token = strtok(NULL, delim);
    }

    IC += L; /* Increment IC by additional line count (0 in case of unary). */
    if (c.command_binary.source_operand == REGISTER && c.command_binary.destination_operand == REGISTER)
        parse_operand(token, -1, IC); /* for -1 see parse_operand(). */
    else
        parse_operand(token, c.command_binary.destination_operand, IC);
    IC += L; /* Increment IC by additional line count of destination operand. */
}

void end_assembler()
{
    if (!assembler_get_errors())
    {
        char *ent_file = generate_ent(in_file);
        char *ext_file = generate_ext(in_file);

        generate_ob(outfile);
        printf("Generated file %s.\n", outfile);

        if (ent_file)
            printf("Generated file %s.\n", ent_file);
        if (ext_file)
            printf("Generated file %s.\n", ext_file);

        free(ent_file);
        free(ext_file);
    }

    free_vector(&data);
    free_vector(&instructions);

    free_table(&entries);
    free_table(&symbol_table);
    free_table(&externs);
}

void terminate_assembler()
{
    free_table(&directive_table);
    free_table(&command_table);
    free_table(&register_table);
}
