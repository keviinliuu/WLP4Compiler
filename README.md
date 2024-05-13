# WLP4 Compiler

This is a compiler written in C++ for a C-like language called [WLP4](https://student.cs.uwaterloo.ca/~cs241/wlp4/WLP4tutorial.html). The language is a subset of C++, with features including functions, parameter passing, if-statements, while loops, arrays, and pointers.

Compiling code into Assembly (MIPS) takes multiple steps, and each step is split up into separate files. Here's what each file does:

- wlp4scan - **Tokenizing**: Takes in code and tokenizes it, storing it in a deque. Syntax errors are detected in this step. 
- wlp4parse - **Parsing**: Turns the set of tokens into a parse tree for further processing. A bottom-up parsing algorithm is used in this step.
- wlp4type - **Semantic Analysis**: Performs semantic analysis of the parsed code for name errors (duplicate declarations, used but not declared variables) and type errors. Traverses the parse tree to detect these errors.
- wlp4gen - **Code Generation**: Finally outputs assembly language (MIPS, in this case) by traversing the parse tree. Makes use of the stack for storing and retrieving variables.
