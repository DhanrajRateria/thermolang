# ThermoLang Language Specification v0.1

This document defines the formal grammar and syntax of the ThermoLang programming language.

## 1. Overview

ThermoLang is a statically-typed language designed for expressing probabilistic algorithms that can be compiled to thermodynamic computing hardware. Its syntax is inspired by modern languages like Rust, Swift, and C++, with special constructs for stochastic operations.

## 2. Lexical Structure

*   **Identifiers:** `[a-zA-Z_][a-zA-Z0-9_]*`
*   **Keywords:** `type`, `distribution`, `function`, `stochastic`, `energy`, `thermal`, `parallel`, `if`, `else`, `while`, `return`, `let`, `const`, `true`, `false`.
*   **Literals:** Integer (`123`), Float (`3.14`), String (`"hello"`), Boolean (`true`).
*   **Operators:** `+`, `-`, `*`, `/`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `->`, `=>`.
*   **Punctuation:** `(`, `)`, `{`, `}`, `[`, `]`, `,`, `:`, `;`, `.`.
*   **Comments:** `//` for single-line, `/* ... */` for multi-line.

## 3. EBNF Grammar

Here is the Extended Backus-Naur Form (EBNF) grammar for ThermoLang.

```ebnf
program ::= { declaration } ;

declaration ::= function_decl | type_alias | const_decl ;

function_decl ::= "stochastic" "fn" identifier "(" [param_list] ")" "->" type "{" block "}"
                | "energy" "fn" identifier "(" [param_list] ")" "->" "real" "{" block "}"
                | "fn" identifier "(" [param_list] ")" "->" type "{" block "}" ;

param_list ::= param { "," param } ;
param ::= identifier ":" type ;

type ::= identifier | "distribution" "<" type { "," type } ">" ;

block ::= { statement } ;

statement ::= let_stmt | expr_stmt | return_stmt | if_stmt | while_stmt ;

let_stmt ::= ("let" | "const") identifier [":" type] "=" expression ";" ;
expr_stmt ::= expression ";" ;
return_stmt ::= "return" expression ";" ;

if_stmt ::= "if" "(" expression ")" "{" block "}" [ "else" "{" block "}" ] ;
while_stmt ::= "while" "(" expression ")" "{" block "}" ;

expression ::= assignment_expr ;
assignment_expr ::= logical_or_expr [ "=" assignment_expr ] ;
logical_or_expr ::= logical_and_expr { "||" logical_and_expr } ;
logical_and_expr ::= equality_expr { "&&" equality_expr } ;
equality_expr ::= comparison_expr { ("==" | "!=") comparison_expr } ;
comparison_expr ::= term { ("<" | ">" | "<=" | ">=") term } ;
term ::= factor { ("+" | "-") factor } ;
factor ::= unary { ("*" | "/") unary } ;
unary ::= ("!" | "-") unary | call_expr ;
call_expr ::= primary_expr { "(" [arg_list] ")" } ;
primary_expr ::= literal | identifier | "(" expression ")" ;

literal ::= integer_lit | float_lit | string_lit | bool_lit ;
arg_list ::= expression { "," expression } ;