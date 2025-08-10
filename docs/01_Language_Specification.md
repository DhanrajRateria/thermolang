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

declaration ::= let_stmt | function_decl | type_alias | annotation ;

annotation ::= "@" identifier [ "(" [ annotation_param { "," annotation_param } ] ")" ] declaration ;
annotation_param ::= identifier "=" literal ;

function_decl ::= [ "stochastic" | "energy" ] "fn" identifier "(" [param_list] ")" "->" type_expr "{" block "}" ;

type_alias ::= "type" identifier "=" type_expr ";" ;

statement ::= let_stmt | expr_stmt | return_stmt | if_stmt | while_stmt | block | thermal_stmt | parallel_stmt ;

thermal_stmt ::= "thermal" block ;
parallel_stmt ::= "parallel" block ;

block ::= "{" { statement } "}" ;

param_list ::= param { "," param } ;
param ::= identifier ":" type ;

type ::= identifier | "distribution" "<" type { "," type } ">" ;

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
```

## 4. Semantics of Domain-Specific Blocks
### 4.1. thermal blocks

A thermal block defines a computational context operating at a specific temperature. The compiler and runtime use this context to manage noise levels and energy calculations.
Temperature Scope: The temperature is constant within the block unless explicitly modified by a standard library function.
Operations: Operations within a thermal block, especially sample functions, are influenced by the ambient temperature, affecting the probability distributions.
### 4.2 thermal_anneal function

The thermal_anneal function is a high-level operation that instructs the system to find the minimum energy configuration of a given energy function.
Signature: thermal_anneal(energy_func: E, schedule: CoolingSchedule) -> Configuration
energy_func: A valid energy function. Its parameters define the variables of the system (e.g., spins). It must return a single float representing the system's energy.
schedule: A data structure defining the annealing process, including initial temperature, cooling rate, and number of steps.
Configuration: The return value is a data structure holding the state of the variables that resulted in the lowest observed energy.