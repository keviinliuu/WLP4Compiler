#ifndef WLP4GEN_H
#define WLP4GEN_H

#include "structs.h"

void generateCode(TreeNode* tree);
void generateWain(TreeNode* tree);
void generateProc(TreeNode* tree);
void generateDcls(TreeNode* tree, map<string, int> &offsetTable, int& counter, int n);
void generateStatements(TreeNode* tree, map<string, int> &offsetTable);
void generateGeneral(TreeNode* tree, map<string, int> &offsetTable);
void generateLvalues(TreeNode* tree, map<string, int> &offsetTable);
void generateWhile(TreeNode* tree, map<string, int> &offsetTable);
void generateIf(TreeNode* tree, map<string, int> &offsetTable);
void generateTest(TreeNode* tree, string endLabel, map<string, int> &offsetTable);
void Add(int d, int s, int t);
void Sub(int d, int s, int t);
void Mult(int s, int t);
void Multu(int s, int t);
void Div(int s, int t);
void Divu(int s, int t);
void Mfhi(int d);
void Mflo(int d);
void Lis(int d);
void Slt(int d, int s, int t);
void Sltu(int d, int s, int t);
void Jr(int s);
void Jalr(int s);
void Beq(int s, int t, string label);
void Bne(int s, int t, string label);
void Lw(int t, int i, int s);
void Sw(int t, int i, int s);
void Word(int i);
void Word(string label);
void Label(string label);
void push(int s);
void pop(int d);
void pop();
void constant(int i);
void import(string label);
void callProc(string proc);
int offset(int count, int n);
bool hasPrint(TreeNode* tree);
bool hasMemAlloc(TreeNode* tree);
pair<string, string> createLoopLabel();
vector<string> createIfLabel();

#endif