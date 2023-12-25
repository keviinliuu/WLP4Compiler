#ifndef WLP4SCAN_H
#define WLP4SCAN_H

#include <deque>
#include "structs.h"

deque<Token> scanTokens();
DFA* createDFA(istream &cin);
bool checkAccepting(DFA* dfa, string state);
string getNextState(DFA* dfa, string fromState, char c);
deque<Token> smm(DFA* dfa, istream &input);
bool checkToken(string state, string lexeme);
void checkNum(string decimal);
Token createToken(string state, string lexeme);
string squish(string s);
bool isChar(string s);
bool isRange(string s);
int hexToNum(char c);
char numToHex(int d);
string escape(string s);
string unescape(string s);

#endif