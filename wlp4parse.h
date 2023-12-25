#ifndef WLP4PARSE_H
#define WLP4PARSE_H

#include <deque>
#include "structs.h"

TreeNode* parseTokens();
vector<Rule> createCFG(istream &cin);
SLR_DFA* createSLRDFA(istream &cin);
void reduceTrees(Rule &rule, vector<TreeNode*> &treeStack);
void reduceStates(Rule &rule, vector<int> &stateStack, SLR_DFA* &dfa);
void shift(deque<Token> &tokens, vector<TreeNode*> &treeStack, vector<int> &stateStack, SLR_DFA* &dfa);
TreeNode* parse(vector<Rule> &CFG, SLR_DFA* &dfa, deque<Token> &tokens, vector<TreeNode*> &treeStack, vector<int> &stateStack);
void printTree(TreeNode* node);

#endif