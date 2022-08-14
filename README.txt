Names:
Daniel Maksimov - Madan64
Inbar Shavit - SHINBA82

Submission Date: 14\8\2022

Introdution
===========

The purpose of the project is to process an assembly (.as) files for an imaginary processor.
Given an assembly file the program will generate the following files (Machine code):
- Object file (.ob)
- Entries file (.ent)
- Externals file (.ext) 


Workflow
========
The workflow consists of 3 stages: 
I.   Preprocessor - macro handling.
II.  Assembler pass 1 - encodes data and handles symbol definitions.
III. Assembler pass 2 - encodes instructions, operands and generates output files.

Stage I: Preprocessor
---------------------


The assembly .as file goes through `process_file()` which calls
the Preprocessor(preprocessor.*) for macro handling. The result is an .am file.

    -> parse_file example.am
    -> assembler
    Returns example.ob.

    If previous step is successful:
    -> parse_file example.am
    -> second pass (secondary_assembler_parse)
    Creates output files.


Data Types
==========
We implemented a vector (dynamic array) and hash_table data structures.
- `struct vector`.
- `struct hash_table`.
- `struct pair`.

assembler.h
- `enum ARE`: bits 0-1. e.g. ABSOLUTE.
- `enum addressing_mode`: bits 2-3, 4-5. e.g. REGISTER.
- `enum datatype` (directive types). e.g. STRING.
- `struct command_field` (pg. 17) bitfield for a 10-bit instruction.
- `struct additional_word_field` (pg. 19): `ARE`+`data` a 10-bit additional word.
- `operand_encoder` a pointer to a function that takes a token and returns a `command_field`.
- `union command` `command_field` or `int`, used to represent command as `int` in instruction array.
- `enum symbol_type` (DATA\CODE).
- `struct symbol`: line-address + symbol_type.

Files:
- hash_table.* 
- vector.*
- parser.* Parses and validates input files.
- preprocessor.*:
    preprocessor_parse(): Macro handling
- assembler.*: 
    assembler_parse(): First pass
    secondary_assembler_parse(): Second pass
- main.c: Iterates over files from argv and assembles them.

Errors
======
All errors are printed to stderr.

parser.c: 
    parse_file():
        - "Can't open file"
          Failed opening input file skiping to next file.

First pass:
If an error is encountered in the first pass it will be reported and the second pass will be canceled.

Possible errors
---------------
assembler.c: 
    parse_command():
        - "Too many args"
        - "Not enough args"

    encode():
        - "Invalid type definition"

    encode_data():
        - "No support for floating-point numbers"

    assembler_parse():
        - "Symbol name too long"
        - "Symbol already defined"
        - "Entry already defined"
        - "Extern already defined"
        - "Command Doesn't exist"

    end_first_pass():
        - "Memory limit exceeded"

Second pass:
If an error is encountered in the second pass it will be reported and no files will be generated.
    parse_operand():
        - "Invalid immediate"
        - "Invalid index"
        - "Index out of range"

    generate_ext/ent/ob():
        - "Can't open .ext/ent/ob file"


Makefile
========
Helpful flags to supress redundant warnings.
- Wno-format: printing size_t using %zu.
- Wno-incompatible-pointer-types: Need to cast some pointers. 
- Wno-unused-variable: variables defined in .h file and used in .c files.
- Wno-declaration-after-statement: define variable in declaration(int i = 0).