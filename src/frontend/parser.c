#include "parser.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== Parser State ========== */

static Token* current(Parser* p) { return &p->tokens[p->pos]; }
static Token* peek_tok(Parser* p) {
    if (p->pos + 1 < p->count) return &p->tokens[p->pos + 1];
    return &p->tokens[p->count - 1]; /* EOF */
}
static int at_end(Parser* p) { return current(p)->type == TOK_EOF; }
static int check(Parser* p, TokenType t) { return current(p)->type == t; }
static int current_line(Parser* p) { return current(p)->line; }

static Token* advance(Parser* p) {
    Token* t = current(p);
    if (!at_end(p)) p->pos++;
    return t;
}

static int match(Parser* p, TokenType t) {
    if (check(p, t)) { advance(p); return 1; }
    return 0;
}

static Token* expect(Parser* p, TokenType t) {
    if (check(p, t)) return advance(p);
    fprintf(stderr, "Error at line %d: Expected '%s', got '%s'\n",
            current_line(p), token_type_name(t),
            current(p)->value ? current(p)->value : token_type_name(current(p)->type));
    exit(EXIT_FAILURE);
    return NULL;
}

/* ========== Node Constructors ========== */

static ASTNode* node_alloc(NodeType type, int line) {
    ASTNode* n = (ASTNode*)calloc(1, sizeof(ASTNode));
    n->type = type;
    n->line = line;
    return n;
}

static ASTNode* node_int(int line, long long val) {
    ASTNode* n = node_alloc(NODE_INT_LIT, line);
    n->data.int_val = val;
    return n;
}

static ASTNode* node_float(int line, double val) {
    ASTNode* n = node_alloc(NODE_FLOAT_LIT, line);
    n->data.float_val = val;
    return n;
}

static ASTNode* node_string(int line, const char* val) {
    ASTNode* n = node_alloc(NODE_STRING_LIT, line);
    n->data.string_val = melody_strdup(val);
    return n;
}

static ASTNode* node_bool(int line, int val) {
    ASTNode* n = node_alloc(NODE_BOOL_LIT, line);
    n->data.bool_val = val;
    return n;
}

static ASTNode* node_null(int line) {
    return node_alloc(NODE_NULL_LIT, line);
}

static ASTNode* node_identifier(int line, const char* name) {
    ASTNode* n = node_alloc(NODE_IDENTIFIER, line);
    n->data.identifier = melody_strdup(name);
    return n;
}

static ASTNode* node_binary(int line, TokenType op, ASTNode* left, ASTNode* right) {
    ASTNode* n = node_alloc(NODE_BINARY_OP, line);
    n->data.binary.op = op;
    n->data.binary.left = left;
    n->data.binary.right = right;
    return n;
}

static ASTNode* node_unary(int line, TokenType op, ASTNode* operand) {
    ASTNode* n = node_alloc(NODE_UNARY_OP, line);
    n->data.unary.op = op;
    n->data.unary.operand = operand;
    return n;
}

static ASTNode* node_assign(int line, ASTNode* target, ASTNode* value) {
    ASTNode* n = node_alloc(NODE_ASSIGN, line);
    n->data.assign.target = target;
    n->data.assign.value = value;
    return n;
}

static ASTNode* node_compound_assign(int line, TokenType op, ASTNode* target, ASTNode* value) {
    ASTNode* n = node_alloc(NODE_COMPOUND_ASSIGN, line);
    n->data.compound.op = op;
    n->data.compound.target = target;
    n->data.compound.value = value;
    return n;
}

static ASTNode* node_list_lit(int line, ASTNode** items, int count) {
    ASTNode* n = node_alloc(NODE_LIST_LIT, line);
    n->data.list.items = items;
    n->data.list.count = count;
    return n;
}

static ASTNode* node_dict_lit(int line, ASTNode** keys, ASTNode** values, int count) {
    ASTNode* n = node_alloc(NODE_DICT_LIT, line);
    n->data.dict.keys = keys;
    n->data.dict.values = values;
    n->data.dict.count = count;
    return n;
}

static ASTNode* node_index(int line, ASTNode* object, ASTNode* index) {
    ASTNode* n = node_alloc(NODE_INDEX, line);
    n->data.index.object = object;
    n->data.index.index = index;
    return n;
}

static ASTNode* node_index_assign(int line, ASTNode* object, ASTNode* idx, ASTNode* value) {
    ASTNode* n = node_alloc(NODE_INDEX_ASSIGN, line);
    n->data.index_assign.object = object;
    n->data.index_assign.idx = idx;
    n->data.index_assign.value = value;
    return n;
}

static ASTNode* node_func_def(int line, const char* name, char** params, int pc, ASTNode* body) {
    ASTNode* n = node_alloc(NODE_FUNC_DEF, line);
    n->data.func_def.name = melody_strdup(name);
    n->data.func_def.params = params;
    n->data.func_def.param_count = pc;
    n->data.func_def.body = body;
    return n;
}

static ASTNode* node_func_call(int line, ASTNode* callee, ASTNode** args, int argc) {
    ASTNode* n = node_alloc(NODE_FUNC_CALL, line);
    n->data.call.callee = callee;
    n->data.call.args = args;
    n->data.call.arg_count = argc;
    return n;
}

static ASTNode* node_method_call(int line, ASTNode* object, const char* method,
                                 ASTNode** args, int argc) {
    ASTNode* n = node_alloc(NODE_METHOD_CALL, line);
    n->data.method.object = object;
    n->data.method.method = melody_strdup(method);
    n->data.method.args = args;
    n->data.method.arg_count = argc;
    return n;
}

static ASTNode* node_return(int line, ASTNode* val) {
    ASTNode* n = node_alloc(NODE_RETURN, line);
    n->data.return_val = val;
    return n;
}

static ASTNode* node_if(int line, ASTNode* cond, ASTNode* then_b, ASTNode* else_b) {
    ASTNode* n = node_alloc(NODE_IF, line);
    n->data.if_stmt.condition = cond;
    n->data.if_stmt.then_block = then_b;
    n->data.if_stmt.else_block = else_b;
    return n;
}

static ASTNode* node_while(int line, ASTNode* cond, ASTNode* body) {
    ASTNode* n = node_alloc(NODE_WHILE, line);
    n->data.while_loop.condition = cond;
    n->data.while_loop.body = body;
    return n;
}

static ASTNode* node_for(int line, ASTNode* init, ASTNode* cond, ASTNode* incr, ASTNode* body) {
    ASTNode* n = node_alloc(NODE_FOR, line);
    n->data.for_loop.init = init;
    n->data.for_loop.condition = cond;
    n->data.for_loop.increment = incr;
    n->data.for_loop.body = body;
    return n;
}

static ASTNode* node_for_in(int line, const char* var, ASTNode* iter, ASTNode* body) {
    ASTNode* n = node_alloc(NODE_FOR_IN, line);
    n->data.for_in.var_name = melody_strdup(var);
    n->data.for_in.iterable = iter;
    n->data.for_in.body = body;
    return n;
}

static ASTNode* node_block(int line, ASTNode** stmts, int count) {
    ASTNode* n = node_alloc(NODE_BLOCK, line);
    n->data.block.stmts = stmts;
    n->data.block.count = count;
    return n;
}

static ASTNode* node_import(int line, ASTNode* path) {
    ASTNode* n = node_alloc(NODE_IMPORT, line);
    n->data.import_path = path;
    return n;
}

static ASTNode* node_del(int line, ASTNode* target) {
    ASTNode* n = node_alloc(NODE_DEL, line);
    n->data.del_target = target;
    return n;
}

static ASTNode* node_try_catch(int line, ASTNode* try_b, const char* var, ASTNode* catch_b) {
    ASTNode* n = node_alloc(NODE_TRY_CATCH, line);
    n->data.try_catch.try_block = try_b;
    n->data.try_catch.catch_var = melody_strdup(var);
    n->data.try_catch.catch_block = catch_b;
    return n;
}

static ASTNode* node_throw(int line, ASTNode* val) {
    ASTNode* n = node_alloc(NODE_THROW, line);
    n->data.throw_val = val;
    return n;
}

static ASTNode* node_class_def(int line, const char* name, const char* superclass,
                               char** method_names, ASTNode** method_bodies, int method_count) {
    ASTNode* n = node_alloc(NODE_CLASS_DEF, line);
    n->data.class_def.name = melody_strdup(name);
    n->data.class_def.superclass = superclass ? melody_strdup(superclass) : NULL;
    n->data.class_def.method_names = method_names;
    n->data.class_def.method_bodies = method_bodies;
    n->data.class_def.method_count = method_count;
    return n;
}

/* ========== Forward Declarations ========== */

static ASTNode* parse_expression(Parser* p);
static ASTNode* parse_assignment(Parser* p);
static ASTNode* parse_statement(Parser* p);
static ASTNode* parse_block(Parser* p);
static int is_func_def(Parser* p);
static ASTNode* parse_func_body(Parser* p, const char* name);

/* ========== Precedence Chain (lowest to highest) ========== */

/* 15. Primary: literals, identifiers, (expr), [list], {dict} */
static ASTNode* parse_primary(Parser* p) {
    int line = current_line(p);

    if (check(p, TOK_INT)) {
        Token* t = advance(p);
        return node_int(line, strtoll(t->value, NULL, 10));
    }
    if (check(p, TOK_FLOAT)) {
        Token* t = advance(p);
        return node_float(line, strtod(t->value, NULL));
    }
    if (check(p, TOK_STRING)) {
        Token* t = advance(p);
        return node_string(line, t->value);
    }
    if (check(p, TOK_TRUE))  { advance(p); return node_bool(line, 1); }
    if (check(p, TOK_FALSE)) { advance(p); return node_bool(line, 0); }
    if (check(p, TOK_NULL))  { advance(p); return node_null(line); }

    /* import("...") as expression for module assignment: math = import("math") */
    if (check(p, TOK_IMPORT)) {
        advance(p);
        expect(p, TOK_LPAREN);
        ASTNode* path = parse_expression(p);
        expect(p, TOK_RPAREN);
        return node_import(line, path);
    }

    if (check(p, TOK_IDENTIFIER)) {
        Token* t = advance(p);
        return node_identifier(line, t->value);
    }

    /* 'super' resolves to an identifier for super.method() calls */
    if (check(p, TOK_SUPER)) {
        advance(p);
        return node_identifier(line, "super");
    }

    /* Anonymous function: (params) { body } */
    if (check(p, TOK_LPAREN) && is_func_def(p)) {
        return parse_func_body(p, "<lambda>");
    }

    /* Parenthesized expression */
    if (check(p, TOK_LPAREN)) {
        advance(p);
        ASTNode* expr = parse_expression(p);
        expect(p, TOK_RPAREN);
        return expr;
    }

    /* List literal: [items] */
    if (check(p, TOK_LBRACKET)) {
        advance(p);
        ASTNode** items = NULL;
        int count = 0, cap = 0;
        if (!check(p, TOK_RBRACKET)) {
            do {
                if (count >= cap) { cap = cap ? cap * 2 : 8; items = (ASTNode**)realloc(items, sizeof(ASTNode*) * cap); }
                items[count++] = parse_expression(p);
            } while (match(p, TOK_COMMA));
        }
        expect(p, TOK_RBRACKET);
        return node_list_lit(line, items, count);
    }

    /* Dict literal: {key: value, ...} */
    if (check(p, TOK_LBRACE)) {
        advance(p);
        ASTNode** keys = NULL;
        ASTNode** values = NULL;
        int count = 0, cap = 0;
        if (!check(p, TOK_RBRACE)) {
            do {
                if (count >= cap) { cap = cap ? cap * 2 : 8; keys = (ASTNode**)realloc(keys, sizeof(ASTNode*) * cap); values = (ASTNode**)realloc(values, sizeof(ASTNode*) * cap); }
                keys[count] = parse_expression(p);
                expect(p, TOK_COLON);
                values[count] = parse_expression(p);
                count++;
            } while (match(p, TOK_COMMA));
        }
        expect(p, TOK_RBRACE);
        return node_dict_lit(line, keys, values, count);
    }

    fprintf(stderr, "Error at line %d: Unexpected token '%s'\n",
            line, current(p)->value ? current(p)->value : token_type_name(current(p)->type));
    exit(EXIT_FAILURE);
    return NULL;
}

/* 14. Postfix: function calls, indexing, method calls */
static ASTNode* parse_postfix(Parser* p) {
    ASTNode* expr = parse_primary(p);

    while (1) {
        int line = current_line(p);

        /* Function call: expr(args) */
        if (check(p, TOK_LPAREN)) {
            advance(p);
            ASTNode** args = NULL;
            int argc = 0, cap = 0;
            if (!check(p, TOK_RPAREN)) {
                do {
                    if (argc >= cap) { cap = cap ? cap * 2 : 8; args = (ASTNode**)realloc(args, sizeof(ASTNode*) * cap); }
                    args[argc++] = parse_expression(p);
                } while (match(p, TOK_COMMA));
            }
            expect(p, TOK_RPAREN);
            expr = node_func_call(line, expr, args, argc);
            continue;
        }

        /* Indexing: expr[index] */
        if (check(p, TOK_LBRACKET)) {
            advance(p);
            ASTNode* idx = parse_expression(p);
            expect(p, TOK_RBRACKET);
            expr = node_index(line, expr, idx);
            continue;
        }

        /* Method call: expr.method(args) */
        if (check(p, TOK_DOT)) {
            advance(p);
            Token* name = expect(p, TOK_IDENTIFIER);
            if (check(p, TOK_LPAREN)) {
                advance(p);
                ASTNode** args = NULL;
                int argc = 0, cap = 0;
                if (!check(p, TOK_RPAREN)) {
                    do {
                        if (argc >= cap) { cap = cap ? cap * 2 : 8; args = (ASTNode**)realloc(args, sizeof(ASTNode*) * cap); }
                        args[argc++] = parse_expression(p);
                    } while (match(p, TOK_COMMA));
                }
                expect(p, TOK_RPAREN);
                expr = node_method_call(line, expr, name->value, args, argc);
            } else {
                /* Property access - treat as dict index with string key */
                expr = node_index(line, expr, node_string(line, name->value));
            }
            continue;
        }

        break;
    }

    return expr;
}

/* 13. Unary: -, !, ~, + */
static ASTNode* parse_unary(Parser* p) {
    int line = current_line(p);
    if (check(p, TOK_MINUS) || check(p, TOK_BANG) || check(p, TOK_TILDE) || check(p, TOK_PLUS)) {
        Token* t = advance(p);
        return node_unary(line, t->type, parse_unary(p));
    }
    return parse_postfix(p);
}

/* 12. Exponentiation: ** (right-associative) */
static ASTNode* parse_exponent(Parser* p) {
    ASTNode* left = parse_unary(p);
    if (check(p, TOK_POWER)) {
        int line = current_line(p);
        advance(p);
        ASTNode* right = parse_exponent(p); /* right-associative */
        return node_binary(line, TOK_POWER, left, right);
    }
    return left;
}

/* 11. Multiplication: *, /, %, // */
static ASTNode* parse_multiplication(Parser* p) {
    ASTNode* left = parse_exponent(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH) ||
           check(p, TOK_PERCENT) || check(p, TOK_FLOOR_DIV)) {
        int line = current_line(p);
        Token* t = advance(p);
        left = node_binary(line, t->type, left, parse_exponent(p));
    }
    return left;
}

/* 10. Addition: +, - */
static ASTNode* parse_addition(Parser* p) {
    ASTNode* left = parse_multiplication(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        int line = current_line(p);
        Token* t = advance(p);
        left = node_binary(line, t->type, left, parse_multiplication(p));
    }
    return left;
}

/* 9. Shift: <<, >> */
static ASTNode* parse_shift(Parser* p) {
    ASTNode* left = parse_addition(p);
    while (check(p, TOK_SHL) || check(p, TOK_SHR)) {
        int line = current_line(p);
        Token* t = advance(p);
        left = node_binary(line, t->type, left, parse_addition(p));
    }
    return left;
}

/* 8. Comparison: <, >, <=, >= */
static ASTNode* parse_comparison(Parser* p) {
    ASTNode* left = parse_shift(p);
    while (check(p, TOK_LT) || check(p, TOK_GT) ||
           check(p, TOK_LTE) || check(p, TOK_GTE)) {
        int line = current_line(p);
        Token* t = advance(p);
        left = node_binary(line, t->type, left, parse_shift(p));
    }
    return left;
}

/* 7. Equality: ==, != */
static ASTNode* parse_equality(Parser* p) {
    ASTNode* left = parse_comparison(p);
    while (check(p, TOK_EQ) || check(p, TOK_NEQ)) {
        int line = current_line(p);
        Token* t = advance(p);
        left = node_binary(line, t->type, left, parse_comparison(p));
    }
    return left;
}

/* 6. Bitwise AND: & */
static ASTNode* parse_bit_and(Parser* p) {
    ASTNode* left = parse_equality(p);
    while (check(p, TOK_AMPERSAND)) {
        int line = current_line(p);
        advance(p);
        left = node_binary(line, TOK_AMPERSAND, left, parse_equality(p));
    }
    return left;
}

/* 5. Bitwise XOR: ^ */
static ASTNode* parse_bit_xor(Parser* p) {
    ASTNode* left = parse_bit_and(p);
    while (check(p, TOK_CARET)) {
        int line = current_line(p);
        advance(p);
        left = node_binary(line, TOK_CARET, left, parse_bit_and(p));
    }
    return left;
}

/* 4. Bitwise OR: | */
static ASTNode* parse_bit_or(Parser* p) {
    ASTNode* left = parse_bit_xor(p);
    while (check(p, TOK_PIPE)) {
        int line = current_line(p);
        advance(p);
        left = node_binary(line, TOK_PIPE, left, parse_bit_xor(p));
    }
    return left;
}

/* 3. Logical AND: && */
static ASTNode* parse_logical_and(Parser* p) {
    ASTNode* left = parse_bit_or(p);
    while (check(p, TOK_DOUBLE_AMP)) {
        int line = current_line(p);
        advance(p);
        left = node_binary(line, TOK_DOUBLE_AMP, left, parse_bit_or(p));
    }
    return left;
}

/* 2. Logical OR: || */
static ASTNode* parse_logical_or(Parser* p) {
    ASTNode* left = parse_logical_and(p);
    while (check(p, TOK_DOUBLE_PIPE)) {
        int line = current_line(p);
        advance(p);
        left = node_binary(line, TOK_DOUBLE_PIPE, left, parse_logical_and(p));
    }
    return left;
}

/*
 * Lookahead: is the current (params) { body } a function definition?
 * Returns 1 if (identifiers/commas only) followed by '{'.
 */
static int is_func_def(Parser* p) {
    int save = p->pos;
    int depth = 0;
    int valid = 1;

    if (p->tokens[p->pos].type != TOK_LPAREN) { p->pos = save; return 0; }
    p->pos++; depth = 1;

    while (depth > 0 && p->pos < p->count) {
        TokenType t = p->tokens[p->pos].type;
        if (t == TOK_LPAREN) depth++;
        else if (t == TOK_RPAREN) { depth--; if (depth == 0) break; }
        else if (t != TOK_IDENTIFIER && t != TOK_COMMA) valid = 0;
        p->pos++;
    }

    if (depth != 0) { p->pos = save; return 0; }
    p->pos++; /* skip closing ')' */
    int result = valid && p->pos < p->count && p->tokens[p->pos].type == TOK_LBRACE;
    p->pos = save;
    return result;
}

/* Parse function parameters and body: (a, b) { ... } */
static ASTNode* parse_func_body(Parser* p, const char* name) {
    int line = current_line(p);
    expect(p, TOK_LPAREN);

    char** params = NULL;
    int pc = 0, pcap = 0;
    if (!check(p, TOK_RPAREN)) {
        do {
            Token* t = expect(p, TOK_IDENTIFIER);
            if (pc >= pcap) { pcap = pcap ? pcap * 2 : 8; params = (char**)realloc(params, sizeof(char*) * pcap); }
            params[pc++] = melody_strdup(t->value);
        } while (match(p, TOK_COMMA));
    }
    expect(p, TOK_RPAREN);

    ASTNode* body = parse_block(p);
    return node_func_def(line, name, params, pc, body);
}

/* 1. Assignment: =, +=, -=, *=, /=, %= or function definition */
static ASTNode* parse_assignment(Parser* p) {
    ASTNode* left = parse_logical_or(p);
    int line = current_line(p);

    /* Simple assignment: x = expr */
    if (check(p, TOK_ASSIGN)) {
        /* Function definition: name = (params) { body } */
        if (left->type == NODE_IDENTIFIER && peek_tok(p)->type == TOK_LPAREN) {
            /* Save position and check if this is a func def */
            int save = p->pos;
            advance(p); /* consume '=' */
            if (is_func_def(p)) {
                return parse_func_body(p, left->data.identifier);
            }
            /* Not a func def, restore and parse as normal assignment */
            p->pos = save;
        }

        /* Index assignment: a[i] = expr */
        if (left->type == NODE_INDEX) {
            advance(p); /* consume '=' */
            ASTNode* val = parse_expression(p);
            return node_index_assign(line, left->data.index.object,
                                     left->data.index.index, val);
        }

        if (left->type == NODE_IDENTIFIER) {
            advance(p);
            ASTNode* val = parse_expression(p);
            return node_assign(line, left, val);
        }

        fprintf(stderr, "Error at line %d: Invalid assignment target\n", line);
        exit(EXIT_FAILURE);
    }

    /* Compound assignment: +=, -=, etc. */
    if (check(p, TOK_PLUS_EQ) || check(p, TOK_MINUS_EQ) ||
        check(p, TOK_STAR_EQ) || check(p, TOK_SLASH_EQ) ||
        check(p, TOK_PERCENT_EQ)) {
        Token* t = advance(p);
        ASTNode* val = parse_expression(p);
        return node_compound_assign(line, t->type, left, val);
    }

    return left;
}

/* Expression: starts the precedence chain */
static ASTNode* parse_expression(Parser* p) {
    return parse_assignment(p);
}

/* ========== Statements ========== */

/* Block: { statement* } */
static ASTNode* parse_block(Parser* p) {
    int line = current_line(p);
    expect(p, TOK_LBRACE);

    ASTNode** stmts = NULL;
    int count = 0, cap = 0;
    while (!check(p, TOK_RBRACE) && !at_end(p)) {
        if (count >= cap) { cap = cap ? cap * 2 : 16; stmts = (ASTNode**)realloc(stmts, sizeof(ASTNode*) * cap); }
        stmts[count++] = parse_statement(p);
    }
    expect(p, TOK_RBRACE);

    return node_block(line, stmts, count);
}

/* If/elif/else chain */
static ASTNode* parse_else_chain(Parser* p) {
    if (check(p, TOK_ELSE)) {
        advance(p);
        if (check(p, TOK_IF)) {
            /* else if */
            int line = current_line(p);
            advance(p); /* consume 'if' */
            expect(p, TOK_LPAREN);
            ASTNode* cond = parse_expression(p);
            expect(p, TOK_RPAREN);
            ASTNode* then_b = parse_block(p);
            ASTNode* else_b = parse_else_chain(p);
            return node_if(line, cond, then_b, else_b);
        }
        return parse_block(p);
    }
    if (check(p, TOK_ELIF)) {
        int line = current_line(p);
        advance(p);
        expect(p, TOK_LPAREN);
        ASTNode* cond = parse_expression(p);
        expect(p, TOK_RPAREN);
        ASTNode* then_b = parse_block(p);
        ASTNode* else_b = parse_else_chain(p);
        return node_if(line, cond, then_b, else_b);
    }
    return NULL;
}

static ASTNode* parse_if_stmt(Parser* p) {
    int line = current_line(p);
    advance(p); /* consume 'if' */
    expect(p, TOK_LPAREN);
    ASTNode* cond = parse_expression(p);
    expect(p, TOK_RPAREN);
    ASTNode* then_b = parse_block(p);
    ASTNode* else_b = parse_else_chain(p);
    return node_if(line, cond, then_b, else_b);
}

static ASTNode* parse_while_stmt(Parser* p) {
    int line = current_line(p);
    advance(p); /* consume 'while' */
    expect(p, TOK_LPAREN);
    ASTNode* cond = parse_expression(p);
    expect(p, TOK_RPAREN);
    ASTNode* body = parse_block(p);
    return node_while(line, cond, body);
}

static ASTNode* parse_for_stmt(Parser* p) {
    int line = current_line(p);
    advance(p); /* consume 'for' */
    expect(p, TOK_LPAREN);

    /* Check for for-in: for (x in expr) */
    if (check(p, TOK_IDENTIFIER) && peek_tok(p)->type == TOK_IN) {
        char* var = melody_strdup(advance(p)->value);
        advance(p); /* consume 'in' */
        ASTNode* iter = parse_expression(p);
        expect(p, TOK_RPAREN);
        ASTNode* body = parse_block(p);
        return node_for_in(line, var, iter, body);
    }

    /* C-style for: for (init; cond; incr) */
    ASTNode* init = parse_expression(p);
    expect(p, TOK_SEMICOLON);
    ASTNode* cond = parse_expression(p);
    expect(p, TOK_SEMICOLON);
    ASTNode* incr = parse_expression(p);
    expect(p, TOK_RPAREN);
    ASTNode* body = parse_block(p);
    return node_for(line, init, cond, incr, body);
}

static ASTNode* parse_fn_stmt(Parser* p) {
    advance(p); /* consume 'fn' */
    Token* name = expect(p, TOK_IDENTIFIER);
    return parse_func_body(p, name->value);
}

static ASTNode* parse_return_stmt(Parser* p) {
    int line = current_line(p);
    advance(p); /* consume 'return' */
    ASTNode* val = NULL;
    if (!check(p, TOK_SEMICOLON) && !check(p, TOK_RBRACE)) {
        val = parse_expression(p);
    }
    match(p, TOK_SEMICOLON);
    return node_return(line, val ? val : node_null(line));
}

static ASTNode* parse_import_stmt(Parser* p) {
    int line = current_line(p);
    advance(p); /* consume 'import' */
    expect(p, TOK_LPAREN);
    ASTNode* path = parse_expression(p);
    expect(p, TOK_RPAREN);
    match(p, TOK_SEMICOLON);
    return node_import(line, path);
}

static ASTNode* parse_del_stmt(Parser* p) {
    int line = current_line(p);
    advance(p); /* consume 'del' */
    ASTNode* target = parse_expression(p);
    match(p, TOK_SEMICOLON);
    return node_del(line, target);
}

static ASTNode* parse_try_stmt(Parser* p) {
    int line = current_line(p);
    advance(p); /* consume 'try' */
    ASTNode* try_b = parse_block(p);
    expect(p, TOK_CATCH);
    expect(p, TOK_LPAREN);
    Token* var = expect(p, TOK_IDENTIFIER);
    expect(p, TOK_RPAREN);
    ASTNode* catch_b = parse_block(p);
    return node_try_catch(line, try_b, var->value, catch_b);
}

static ASTNode* parse_throw_stmt(Parser* p) {
    int line = current_line(p);
    advance(p); /* consume 'throw' */
    ASTNode* val = parse_expression(p);
    match(p, TOK_SEMICOLON);
    return node_throw(line, val);
}

static ASTNode* parse_class_def(Parser* p) {
    int line = current_line(p);
    advance(p); /* consume 'class' */
    Token* name = expect(p, TOK_IDENTIFIER);

    char* superclass = NULL;
    if (match(p, TOK_EXTENDS)) {
        Token* sc = expect(p, TOK_IDENTIFIER);
        superclass = melody_strdup(sc->value);
    }

    expect(p, TOK_LBRACE);

    char** method_names = NULL;
    ASTNode** method_bodies = NULL;
    int count = 0, cap = 0;

    while (!check(p, TOK_RBRACE) && !at_end(p)) {
        expect(p, TOK_FN);
        Token* mname = expect(p, TOK_IDENTIFIER);
        ASTNode* body = parse_func_body(p, mname->value);

        if (count >= cap) {
            cap = cap ? cap * 2 : 8;
            method_names = (char**)realloc(method_names, sizeof(char*) * cap);
            method_bodies = (ASTNode**)realloc(method_bodies, sizeof(ASTNode*) * cap);
        }
        method_names[count] = melody_strdup(mname->value);
        method_bodies[count] = body;
        count++;
    }

    expect(p, TOK_RBRACE);

    return node_class_def(line, name->value, superclass, method_names, method_bodies, count);
}

/* Statement dispatcher */
static ASTNode* parse_statement(Parser* p) {
    /* Block statements (no trailing semicolon needed) */
    if (check(p, TOK_IF))       return parse_if_stmt(p);
    if (check(p, TOK_WHILE))    return parse_while_stmt(p);
    if (check(p, TOK_FOR))      return parse_for_stmt(p);
    if (check(p, TOK_FN))       return parse_fn_stmt(p);
    if (check(p, TOK_CLASS))    return parse_class_def(p);
    if (check(p, TOK_TRY))     return parse_try_stmt(p);

    /* Keyword statements */
    if (check(p, TOK_RETURN))   return parse_return_stmt(p);
    if (check(p, TOK_IMPORT))   return parse_import_stmt(p);
    if (check(p, TOK_DEL))      return parse_del_stmt(p);
    if (check(p, TOK_THROW))   return parse_throw_stmt(p);

    if (check(p, TOK_BREAK)) {
        int line = current_line(p);
        advance(p); match(p, TOK_SEMICOLON);
        return node_alloc(NODE_BREAK, line);
    }
    if (check(p, TOK_CONTINUE)) {
        int line = current_line(p);
        advance(p); match(p, TOK_SEMICOLON);
        return node_alloc(NODE_CONTINUE, line);
    }

    /* Expression statement */
    ASTNode* expr = parse_expression(p);

    /* Function defs don't require semicolons */
    if (expr->type == NODE_FUNC_DEF) {
        match(p, TOK_SEMICOLON);
    } else {
        match(p, TOK_SEMICOLON);
    }

    return expr;
}

/* ========== Public API ========== */

ASTNode* parse(Token* tokens, int token_count) {
    Parser p;
    p.tokens = tokens;
    p.count = token_count;
    p.pos = 0;
    p.had_error = 0;

    int line = 1;
    ASTNode** stmts = NULL;
    int count = 0, cap = 0;

    while (!at_end(&p)) {
        if (count >= cap) { cap = cap ? cap * 2 : 32; stmts = (ASTNode**)realloc(stmts, sizeof(ASTNode*) * cap); }
        stmts[count++] = parse_statement(&p);
    }

    ASTNode* prog = node_alloc(NODE_PROGRAM, line);
    prog->data.block.stmts = stmts;
    prog->data.block.count = count;
    return prog;
}

void ast_free(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case NODE_STRING_LIT: free(node->data.string_val); break;
        case NODE_IDENTIFIER: free(node->data.identifier); break;
        case NODE_BINARY_OP:
            ast_free(node->data.binary.left);
            ast_free(node->data.binary.right);
            break;
        case NODE_UNARY_OP:
            ast_free(node->data.unary.operand);
            break;
        case NODE_ASSIGN:
            ast_free(node->data.assign.target);
            ast_free(node->data.assign.value);
            break;
        case NODE_COMPOUND_ASSIGN:
            ast_free(node->data.compound.target);
            ast_free(node->data.compound.value);
            break;
        case NODE_LIST_LIT:
            for (int i = 0; i < node->data.list.count; i++)
                ast_free(node->data.list.items[i]);
            free(node->data.list.items);
            break;
        case NODE_DICT_LIT:
            for (int i = 0; i < node->data.dict.count; i++) {
                ast_free(node->data.dict.keys[i]);
                ast_free(node->data.dict.values[i]);
            }
            free(node->data.dict.keys);
            free(node->data.dict.values);
            break;
        case NODE_INDEX:
            ast_free(node->data.index.object);
            ast_free(node->data.index.index);
            break;
        case NODE_INDEX_ASSIGN:
            ast_free(node->data.index_assign.object);
            ast_free(node->data.index_assign.idx);
            ast_free(node->data.index_assign.value);
            break;
        case NODE_FUNC_DEF:
            free(node->data.func_def.name);
            for (int i = 0; i < node->data.func_def.param_count; i++)
                free(node->data.func_def.params[i]);
            free(node->data.func_def.params);
            ast_free(node->data.func_def.body);
            break;
        case NODE_FUNC_CALL:
            ast_free(node->data.call.callee);
            for (int i = 0; i < node->data.call.arg_count; i++)
                ast_free(node->data.call.args[i]);
            free(node->data.call.args);
            break;
        case NODE_METHOD_CALL:
            ast_free(node->data.method.object);
            free(node->data.method.method);
            for (int i = 0; i < node->data.method.arg_count; i++)
                ast_free(node->data.method.args[i]);
            free(node->data.method.args);
            break;
        case NODE_RETURN:
            ast_free(node->data.return_val);
            break;
        case NODE_IF:
            ast_free(node->data.if_stmt.condition);
            ast_free(node->data.if_stmt.then_block);
            ast_free(node->data.if_stmt.else_block);
            break;
        case NODE_WHILE:
            ast_free(node->data.while_loop.condition);
            ast_free(node->data.while_loop.body);
            break;
        case NODE_FOR:
            ast_free(node->data.for_loop.init);
            ast_free(node->data.for_loop.condition);
            ast_free(node->data.for_loop.increment);
            ast_free(node->data.for_loop.body);
            break;
        case NODE_FOR_IN:
            free(node->data.for_in.var_name);
            ast_free(node->data.for_in.iterable);
            ast_free(node->data.for_in.body);
            break;
        case NODE_BLOCK:
        case NODE_PROGRAM:
            for (int i = 0; i < node->data.block.count; i++)
                ast_free(node->data.block.stmts[i]);
            free(node->data.block.stmts);
            break;
        case NODE_IMPORT:
            ast_free(node->data.import_path);
            break;
        case NODE_DEL:
            ast_free(node->data.del_target);
            break;
        case NODE_TRY_CATCH:
            ast_free(node->data.try_catch.try_block);
            free(node->data.try_catch.catch_var);
            ast_free(node->data.try_catch.catch_block);
            break;
        case NODE_THROW:
            ast_free(node->data.throw_val);
            break;
        case NODE_CLASS_DEF:
            free(node->data.class_def.name);
            free(node->data.class_def.superclass);
            for (int i = 0; i < node->data.class_def.method_count; i++) {
                free(node->data.class_def.method_names[i]);
                ast_free(node->data.class_def.method_bodies[i]);
            }
            free(node->data.class_def.method_names);
            free(node->data.class_def.method_bodies);
            break;
        default:
            break;
    }
    free(node);
}
