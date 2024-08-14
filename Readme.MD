# .mdy Programming Language Documentation

## Table of Contents

1. [Introduction](#introduction)
2. [Language Features](#language-features)
    - [Basic Syntax](#basic-syntax)
    - [Data Types](#data-types)
    - [Variables](#variables)
    - [Arithmetic Operations](#arithmetic-operations)
    - [Comparison Operations](#comparison-operations)
    - [Logical Operations](#logical-operations)
    - [Bitwise Operations](#bitwise-operations)
    - [Control Structures](#control-structures)
    - [Functions](#functions)
    - [File I/O](#file-io)
    - [Input and Output](#input-and-output)
    - [Importing Files](#importing-files)
3. [Example Programs](#example-programs)
4. [Internal Structure](#internal-structure)
    - [Tokenizer](#tokenizer)
    - [Parser](#parser)
    - [Interpreter](#interpreter)
5. [Extending the Language](#extending-the-language)

## Introduction

This document provides an overview of the .Uma programming language, detailing its features, syntax, and usage. The language is designed to support a variety of data types, operations, control structures, and I/O functionalities.

## Language Features

### Basic Syntax

- **Statements**: Each statement must end with a semicolon `;`.
- **Comments**: Single-line comments start with `//`.

### Data Types

- **Numeric Types**: `int`, `float`
- **String**: `str`

- **Collection Types**: `list`, `dict`

### Variables

- **Declaration and Assignment**:
    ```python
     x = 10;
     y = 5.5;
     name = "John";
    ```

- **Compound Assignment**:
    ```python
    x += 5;
    y -= 2.2;
    ```

### Arithmetic Operations

- **Operators**: `+`, `-`, `*`, `/`, `%`, `**`, `//`
    ```python
     sum = 5 + 3;
     power = 2 ** 3;
     quotient = 10 // 3;
    ```

### Comparison Operations

- **Operators**: `==`, `!=`, `>`, `<`, `>=`, `<=`
    ```python
    isEqual = (x == y);
    isGreater = (x > y);
    ```

### Logical Operations

- **Operators**: `&&` (and), `||` (or), `!` (not)
    ```python
    result = (x > 5 && y < 10);
    notFlag = !flag;
    ```

### Bitwise Operations

- **Operators**: `&`, `|`, `^`, `~`, `<<`, `>>`
    ```python
    result = x & y;
    shifted = x << 2;
    ```

### Control Structures

- **If-Else**:
    ```python
    if (x > 0) {
        print("Positive");
    } else {
        print("Non-positive");
    }
    ```

- **While Loop**:
    ```python
    while (x > 0) {
        print(x);
        x -= 1;
    }
    ```

- **For Loop**:
    ```c
    for (int i = 0; i < 10; i += 1) {
        print(i);
    }
    ```

### Functions

- **Definition**:
    ```python
    sum=(int a, int b) {
        return a + b;
    }
    ```

- **Call**:
    ```python
    result = sum(5, 3);
    ```

### File I/O

- **Open and Close**:
    ```python
    file = open("test.txt", "w");
    close(file);
    ```

- **Read and Write**:
    ```python
    write(file, "Hello, world!");
    str content = read(file);
    ```

### Input and Output

- **Print**:
    ```c
    print("Hello, world!");
    ```

- **Input**:
    ```c
    str name = scan("Enter your name: ");
    ```



### Importing Files

- **Import**:
    ```c
    import('another_file.txt');
    ```

## Example Programs

### Hello World

```python
print("Hello, world!");
```

### Sum of Two Numbers

```python
a = 5;
b = 10;
sum = a + b;
print("Sum: " + sum);
```

### File I/O Example

```python
file = open("test.txt", "w");
write(file, "Hello, world!");
close(file);

file = open("test.txt", "r");
content = read(file);
print(content);
close(file);
```

## Internal Structure

### Tokenizer

- **Purpose**: Converts source code into a stream of tokens.
- **Functions**:
    - `tokenize(const char* input)`: Tokenizes the input string.

### Parser

- **Purpose**: Parses the stream of tokens into an Abstract Syntax Tree (AST).
- **Functions**:
    - `ASTNode* parse()`: Parses the token stream into an AST.
    - `ASTNode* parse_statement()`: Parses individual statements.

### Interpreter

- **Purpose**: Evaluates the AST.
- **Functions**:
    - `Literal* evaluate_ast(ASTNode* root)`: Evaluates the AST.
    - `Literal evaluate_import(ASTNode* node)`: Handles import operations.
    - `Literal* evaluate_input(ASTNode* node)`: Handles input operations.
    - `Literal evaluate_scan(ASTNode* node)`: Handles scan operations.

## Extending the Language

To extend the language, follow these steps:

1. **Add New Token Types**: Update the tokenizer to recognize new keywords and symbols.
2. **Update the Parser**: Extend the parser to handle new syntax and generate corresponding AST nodes.
3. **Implement Evaluation Logic**: Update the interpreter to evaluate the new AST nodes.

### Example: Adding a New Keyword

1. **Tokenizer**:
    ```c
    if (strcmp(identifier, "new_keyword") == 0) {
        return TOKEN_NEW_KEYWORD;
    }
    ```

2. **Parser**:
    ```c
    ASTNode* parse_new_keyword() {
        consume(NEW_KEYWORD);
        // parsing logic...
    }

    ASTNode* parse_statement() {
        if (current_token.type == NEW_KEYWORD) {
            return parse_new_keyword();
        }
        // other cases...
    }
    ```

3. **Interpreter**:
    ```c
    Literal evaluate_new_keyword(ASTNode* node) {
        // evaluation logic...
    }

    Literal* evaluate_ast(ASTNode* root) {
        switch (root->type) {
            case AST_NEW_KEYWORD:
                evaluate_new_keyword(root);
                return NULL;
            // other cases...
        }
    }
    ```

This documentation provides an overview of the .Uma programming language and serves as a guide for using and extending it.
