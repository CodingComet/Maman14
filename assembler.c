#include "parser.h"
#include "assembler.h"
#include "hash_table.h"
#include "vector.h"

#define COMMANDS 16
#define REGISTERS 8
#define DIRECTIVES 5

#define MAX_SYMBOL_LENGTH 30

#define BASE 32
#define MAX_SIZE 2

#define IS_SYMBOL(token, length) ':' == token[length - 1]

char *convert_to_base32(unsigned int num)
{
    char arr[BASE] = "!@#$%^&*<>abcdefghijklmnopqrstuv";
    char *ret = malloc(MAX_SIZE + 1); /* extra room for '\0' */

    ret[MAX_SIZE] = 0;
    ret[0] = arr[(num & 0b1111100000) / BASE]; /* 5 last bits */
    ret[1] = arr[num & 0b11111];               /* 5 first bits */

    return ret;
}

char *generate_ext(char *file_name, hash_table *external_symbol_table)
{

    if (1 > external_symbol_table->size)
        return NULL;
    /* creating the .ext file */
    char *ext_file_name = malloc(strlen(file_name) + 2);
    memcpy(ext_file_name, file_name, strlen(file_name) - 3);
    memcpy(ext_file_name + strlen(file_name) - 3, ".ext", 4);
    ext_file_name[strlen(file_name) + 1] = '\0';
    FILE *ext_file;
    ext_file = fopen(ext_file_name, "w");
    int i = 0;
    pair *p;
    for (; i < TABLE_SIZE; i++)
    {
        p = external_symbol_table->table[i];
        while (p)
        {
            int addr_dec = *(int *)p->value;

            if (addr_dec == 0xdeadbeef)
            {
                p = p->next;
                continue;
            }
            char *addr_b32 = convert_to_base32(addr_dec + 100);
            char *line = malloc(strlen(p->key) + 4);
            memcpy(line, p->key, strlen(p->key));
            line[strlen(p->key)] = ' ';
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
    free(ext_file_name);
    return ext_file_name;
}

char *generate_ent(char *file_name, hash_table *entries_symbol_table)
{

    if (1 > entries_symbol_table->size)
        return NULL;
    /* creating the .ent file */

    char *ent_file_name = malloc(strlen(file_name) + 2);
    memcpy(ent_file_name, file_name, strlen(file_name) - 3);
    memcpy(ent_file_name + strlen(file_name) - 3, ".ent", 4);
    ent_file_name[strlen(file_name) + 1] = '\0';

    FILE *ent_file;

    if (!(ent_file = fopen(ent_file_name, "w")))
    {
        printf("Error opening file %s\n", file_name);
    }
    int i = 0;
    for (; i < TABLE_SIZE; i++)
    {
        pair *p = entries_symbol_table->table[i];
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
    free(ent_file_name);
    return ent_file_name;
}

void generate_ob(char *file_name, vector *data, vector *instructions)
{
    FILE *ob_file;
    ob_file = fopen(file_name, "w");

    int i = 0;
    char *p;
    char *address;

    for (; i < instructions->size; i++)
    {
        p = convert_to_base32(instructions->data[i]);
        address = convert_to_base32(i + 100);

        fputs(address, ob_file);
        fputc('\t', ob_file);
        fputs(p, ob_file);
        fputc('\n', ob_file);

        free(p);
        free(address);
    }

    for (i = 0; i < data->size; i++)
    {
        p = convert_to_base32(data->data[i]);
        address = convert_to_base32(i + 100 + instructions->size);

        fputs(address, ob_file);
        fputc('\t', ob_file);
        fputs(p, ob_file);
        fputc('\n', ob_file);

        free(p);
        free(address);
    }

    fclose(ob_file);

    return file_name;
}

static char *in_file;

static hash_table symbol_table;
static hash_table command_table;
static hash_table register_table;
static hash_table directive_table;
static hash_table external_symbol_table;
static hash_table entries;

static vector data;
static vector instructions;

static char *outfile;

static unsigned int DC; /*data counter*/
static unsigned int L;  /*current word counter*/

bool assembler_get_errors()
{
    return false;
}

addressing_mode get_addressing_mode(const char *token)
{
    /* TODO: see table in page 32 */

    pair *p;
    char *dot = strchr(token, '.');

    if ('#' == token[0])
        return IMMEDIATE;
    if (p = table_get(&register_table, token))
        return REGISTER;
    if (dot && strlen(token) > dot - token + 1)
        return INDEXED;
    return DIRECT;
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
        .source_operand = get_addressing_mode(token)}; /* token deallocate */

    L = get_additional_wordc(res.destination_operand) + get_additional_wordc(res.source_operand) -
        (res.destination_operand == res.source_operand && res.destination_operand == REGISTER); /* 2 registers in 1 word */

    return res;
}

void encode_string(char *token)
{
    int i = 0;
    token = strtok(NULL, "");
    bool inString = false;

    for (; i < strlen(token); i++)
    {
        if (token[i] == '"')
        {
            if (inString)
                break;

            inString = true;
            continue;
        }
        if (inString)
        {
            vector_push_back(&data, token[i]);
            ++DC;
        }
    }
    vector_push_back(&data, 0);
    ++DC;
}

void encode_data(char *token)
{
    vector_push_back(&data, atoi(token));
    ++DC;
}

void encode(datatype type, char *token)
{
    switch (type)
    {
    case INT:
        while (token = strtok(NULL, delim))
        {
            encode_data(token);
        }

        break;

    case STRING:
        encode_string(token);
        break;

    case STRUCT:
        token = strtok(NULL, delim);
        encode_data(token);
        encode_string(token);
        break;
    }
}

operand_encoder get_command_parser(unsigned int command)
{
    if (command <= 4)
        return binary;
    if (command <= 13)
        return unary;
    return nullary;
}

command_field parse_command(unsigned int opcode, char *token)
{
    token = strtok(NULL, delim);
    command_field res = get_command_parser(opcode)(token);

    res.opcode = opcode;
    res.ARE = 0;

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
        "r7"};

    command_table = table_from_array(commands, COMMANDS);
    directive_table = table_from_array(directives, DIRECTIVES);
    register_table = table_from_array(registers, REGISTERS);
}

char *begin_assembler(const char *file_name)
{
    in_file = file_name;

    outfile = malloc(strlen(file_name) + 2);
    outfile[strlen(file_name) + 1] = '\0';

    strcpy(outfile, file_name);

    strcpy(outfile + strlen(file_name) - 3, ".ob");
    outfile[strlen(file_name)] = '\0';

    /*1.*/ DC = 0;
    L = 0;

    data = create_vector(2);
    instructions = create_vector(2);

    entries = create_table();
    symbol_table = create_table();
    external_symbol_table = create_table();

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
    symbol s;
    command c;
    unsigned int i = 0xdeadbeef;
    bool symbol_flag = false;
    char symbol_name[MAX_SYMBOL_LENGTH];
    size_t length = strlen(token);

    if (IS_SYMBOL(token, length)) /*3. Check for symbol */
    {
        if (length - 1 > MAX_SYMBOL_LENGTH)
        {
            printf("Symbol name too long!\n");
            /* ERROR */
        }

        symbol_flag = true;

        /* Copy data into symbol name */
        symbol_name[length - 1] = '\0';
        memcpy(symbol_name, token, length - 1);
    }

    if (symbol_flag) /* Next token */
        token = strtok(NULL, delim);
    if (p = table_get(&directive_table, token)) /* 5. - 9. */
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
        case 3:
            token = strtok(NULL, delim);
            table_insert(&entries, token, &i, sizeof(int));
            break; /* .entry */
        case 4:
            token = strtok(NULL, delim);
            table_insert(&external_symbol_table, token, &i, sizeof(int));
            break; /* .extern */
        default:
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

            if (!table_get(&symbol_table, symbol_name))
            {
                s.ptr = instructions.size;
                s.type = CODE;
                table_insert(&symbol_table, symbol_name, &s, sizeof(s));
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
        printf("Command doesn't exist: %s\n", token);
        /*ERROR*/
        return;
    }

    c.command_binary = parse_command(*(int *)p->value, token);

    vector_push_back(&instructions, c.command_decimal);
    for (i = 0; i < L; i++) /* Insert additional words as placeholders */
        vector_push_back(&instructions, 0xC0DE);

    if (symbol_name)
        free(symbol_name);
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
            s->ptr += instructions.size;
        s->ptr += 100;
    }
}

void secondary_assembler_parse(const char *line, char *line_copy, char *token)
{
    if (token == NULL) /* empty */
        return;
    if (token[0] == ';')
        return;

    static size_t IC = 0;

    int i = 0;
    command c;
    pair *p1, *p2;
    additional_word w = {.word_decimal = 0};
    operand_encoder oe;
    int length = strlen(token);

    if (IS_SYMBOL(token, length))
        token = strtok(NULL, delim);

    if (p1 = table_get(&directive_table, token)) /* 5. - 9. */
    {
        if (*(int *)p1->value != 3)
            return;
        token = strtok(NULL, delim);
        p1 = table_get(&entries, token);
        p2 = table_get(&symbol_table, token);

        free(p1->value);
        p1->value = malloc(sizeof(int));
        memcpy(p1->value, p2->value, sizeof(int));
        return;
    }

    if (!(p1 = table_get(&command_table, token)))
    {
        printf("Command doesn't exist: %s\n", token);
        /*ERROR*/
        return;
    }
    token = strtok(NULL, delim);

    c.command_decimal = instructions.data[IC++];
    oe = get_command_parser(c.command_binary.opcode);
    if (oe != nullary)
    {
        if (oe == binary)
        {
            switch (c.command_binary.source_operand)
            {
            case IMMEDIATE:
                w.word_binary.ARE = ABSOLUTE;
                w.word_binary.data = atoi(token + 1);
                instructions.data[IC++] = w.word_decimal;
                break;
            case DIRECT:
                if (p1 = table_get(&symbol_table, token))
                {
                    w.word_binary.ARE = RELOCATABLE;
                    w.word_binary.data = (*(symbol *)p1->value).ptr;
                }
                else
                {
                    if (p1 = table_get(&external_symbol_table, token)) /* Check if symbol is external and register usage*/
                        table_insert(&external_symbol_table, token, &IC, sizeof(size_t));
                    w.word_binary.ARE = EXTERNAL;
                    w.word_binary.data = 0;
                }

                instructions.data[IC++] = w.word_decimal;
                break;
            case INDEXED:
            {
                char *dot = strchr(token, '.');

                /* Generate symbol from token with indexed access */
                char *symbol_name = malloc(dot - token) + 1;
                symbol_name[dot - token] = 0;
                memcpy(symbol_name, token, dot - token);

                if (p1 = table_get(&symbol_table, symbol_name))
                {
                    w.word_binary.ARE = RELOCATABLE;
                    w.word_binary.data = (*(symbol *)p1->value).ptr;
                }
                else
                {
                    if (p1 = table_get(&external_symbol_table, symbol_name)) /* Check if symbol is external and register usage*/
                        table_insert(&external_symbol_table, symbol_name, &IC, sizeof(size_t));
                    w.word_binary.ARE = EXTERNAL;
                    w.word_binary.data = 0;
                }
                free(symbol_name);

                instructions.data[IC++] = w.word_decimal;
                w.word_binary.ARE = ABSOLUTE;
                w.word_binary.data = atoi(dot + 1);
                instructions.data[IC++] = w.word_decimal;

                break;
            }

            case REGISTER:
                w.word_binary.ARE = ABSOLUTE;
                w.word_binary.data = atoi(token + 1) << 4;
                instructions.data[IC++] = w.word_decimal;
                break;

            default:
                break;
            }
            token = strtok(NULL, delim);
        }

        w.word_decimal = 0;

        switch (c.command_binary.destination_operand)
        {
        case IMMEDIATE:
            w.word_binary.ARE = ABSOLUTE;
            w.word_binary.data = atoi(token + 1);
            instructions.data[IC++] = w.word_decimal;
            break;
        case DIRECT:
            if (p1 = table_get(&symbol_table, token))
            {
                w.word_binary.ARE = RELOCATABLE;
                w.word_binary.data = (*(symbol *)p1->value).ptr;
            }
            else
            {
                if (p1 = table_get(&external_symbol_table, token)) /* Check if symbol is external and register usage*/
                    table_insert(&external_symbol_table, token, &IC, sizeof(size_t));
                w.word_binary.ARE = EXTERNAL;
                w.word_binary.data = 0;
            }

            instructions.data[IC++] = w.word_decimal;
            break;
        case INDEXED:
        {
            char *dot = strchr(token, '.');

            /* Generate symbol from token with indexed access */
            char *symbol_name = malloc(dot - token) + 1;
            symbol_name[dot - token] = 0;
            memcpy(symbol_name, token, dot - token);

            if (p1 = table_get(&symbol_table, symbol_name))
            {
                w.word_binary.ARE = RELOCATABLE;
                w.word_binary.data = (*(symbol *)p1->value).ptr;
            }
            else
            {
                if (p1 = table_get(&external_symbol_table, symbol_name)) /* Check if symbol is external and register usage*/
                    table_insert(&external_symbol_table, symbol_name, &IC, sizeof(size_t));
                w.word_binary.ARE = EXTERNAL;
                w.word_binary.data = 0;
            }
            free(symbol_name);

            IC++;
            w.word_binary.ARE = ABSOLUTE;
            w.word_binary.data = atoi(dot + 1);
            instructions.data[IC++] = w.word_decimal;

            break;
        }
        case REGISTER:

            w.word_binary.ARE = ABSOLUTE;
            if (c.command_binary.destination_operand == c.command_binary.source_operand && c.command_binary.destination_operand == REGISTER)
            {
                w.word_decimal = instructions.data[IC - 1];
                w.word_binary.data += atoi(token + 1);
                instructions.data[IC - 1] = w.word_decimal;
            }
            else
            {
                w.word_binary.data = atoi(token + 1);
                instructions.data[IC++] = w.word_decimal;
            }
            break;
        default:
            break;
        }
    }
}

void end_assembler()
{
    symbol *s;
    pair *p1, *p2;
    unsigned i = 0;

    /*
    for (i = 0; i < instructions.size; i++)
    {
        char *b32 = convert_to_base32(instructions.data[i]);
        printf("Code: %s\n", b32);
        fflush(stdout);
        free(b32);
    }


    for (i = 0; i < data.size; i++)
    {
        char *b32 = convert_to_base32(data.data[i]);
        printf("Code: %s\n", b32);
        fflush(stdout);
        free(b32);
    }
    */

    free(generate_ent(in_file, &entries));
    free(generate_ext(in_file, &external_symbol_table));
    if (!assembler_get_errors())
        generate_ob(outfile, &data, &instructions);

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
    free_vector(&data);
    free_vector(&instructions);

    free_table(&entries);
    free_table(&symbol_table);
    free_table(&external_symbol_table);
}

void terminate_assembler()
{
    free_table(&directive_table);
    free_table(&command_table);
    free_table(&register_table);
}