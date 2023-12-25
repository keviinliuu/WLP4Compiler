#include <iostream>
#include <sstream>

#include "structs.h"
#include "wlp4.h"
#include "wlp4scan.h"

using namespace std;

map<string, string> keywords = {{"int", "INT"}, {"wain", "WAIN"}, {"if", "IF"}, {"else", "ELSE"}, {"while", "WHILE"}, {"println", "PRINTLN"}, {"return", "RETURN"}, {"new", "NEW"}, {"delete", "DELETE"}, {"NULL", "NULL"}};

deque<Token> scanTokens() {
    DFA* dfa;

    try {
        stringstream s(wlp4string);
        dfa = createDFA(s);
        deque<Token> tokens = smm(dfa, cin);
        Token bof("BOF", "BOF");
        tokens.push_front(bof);
        Token eof("EOF", "EOF");
        tokens.push_back(eof);

        delete(dfa);
        return tokens;
    }
    catch (runtime_error &e) {
        delete(dfa);
        throw runtime_error(e.what());
    }
}

DFA* createDFA(istream &cin) {
    string s;
    vector<string> states;
    map<string, bool> acceptings;
    map<string, map<char, string>> arrows;

    while(true) {
        if(!getline(cin, s)) {
            throw runtime_error("Expected .STATES, but found end of input.");
        }

        s = squish(s);

        if(s == ".STATES") {
            break;
        }
        
        if(!s.empty()) {
            throw runtime_error("Expected .STATES, but found:" + s);
        }
    }

    while(true) {
        if(!(cin >> s)) {
            throw runtime_error
                ("Unexpected end of input while reading state set: .TRANSITIONS not found.");
        }

        if(s == ".TRANSITIONS") {
            break;
        }

        bool accepting = false;

        if(s.back() == '!' && s.length() > 2) {
            accepting = true;
            s.pop_back();
        }

        states.push_back(s);
        acceptings[s] = accepting;
    }

    getline(cin, s);

    while(true) {
        if(!getline(cin, s)) {
            break;
        }

        s = squish(s);

        if(s == ".INPUT") {
            break;
        }

        string lineStr = s;
        stringstream line(lineStr);
        vector<string> lineVec;
        while(line >> s) {
            lineVec.push_back(s);
        }
        if(lineVec.empty()) {
            continue;
        }
        if(lineVec.size() < 3) {
            throw runtime_error("Incomplete transition line: " + lineStr);
        }

        string fromState = lineVec.front();
        string toState = lineVec.back();

        for(int i = 1; i < (int)lineVec.size() - 1; i++) {
            string charOrRange = escape(lineVec[i]);

            if(isChar(charOrRange)) {
                char c = charOrRange[0];

                if(c < 0 || c > 127) {
                    throw runtime_error 
                        ("Invalid (non-ASCII) character in transition line: " + lineStr + "\n"
                        + "Character " + unescape(string(1, c)) + " is outside ASCII range");
                }

                arrows[fromState][c] = toState;
            }
            else if(isRange(charOrRange)) {
                for(char c = charOrRange[0]; charOrRange[0] <= c && c <= charOrRange[2]; c++) {
                    arrows[fromState][c] = toState;
                }
            }
            else {
                throw runtime_error
                    ("Expected character or range, but found " + charOrRange + " in transition line: " + lineStr);
            }
        }
    }

    DFA* newDFA = new DFA(states, acceptings, arrows);
    return newDFA;
}

bool checkAccepting(DFA* dfa, string state) {
    return dfa->accepting[state];
}

string getNextState(DFA* dfa, string fromString, char c) {
    if(dfa->arrows[fromString].find(c) != dfa->arrows[fromString].end()) {
        return dfa->arrows[fromString][c];
    }
    return "NO_NEXT_STATE";
}

deque<Token> smm(DFA* dfa, istream &cin) {
    deque<Token> tokens;
    string line;

    while(getline(cin, line)) {
        string state = "start";
        string curLexeme = "";

        if(line.empty()) continue;

        while(!line.empty()) {
            char c = line[0];
            string nextState = getNextState(dfa, state, c);

            if(state == "ZERO" && nextState == "NUM") throw runtime_error("Cannot have useless zeroes.");

            if(nextState != "NO_NEXT_STATE") {
                curLexeme += c;
                line.erase(0, 1);
                state = nextState;
            }
            else {
                if(checkAccepting(dfa, state)) {
                    if(checkToken(state, curLexeme)) {
                        tokens.push_back(createToken(state, curLexeme));
                    }
                    state = "start";
                    curLexeme = "";
                }
                else {
                    throw runtime_error("Invalid token: " + curLexeme);
                }
            }
        }

        if(checkAccepting(dfa, state)) {
            if(checkToken(state, curLexeme)) {
                tokens.push_back(createToken(state, curLexeme));
            }
        }
        else {
            throw runtime_error("Invalid token: " + curLexeme);
        }
    }

    return tokens;
}

Token createToken(string state, string lexeme) {
    if(state == "ID" && keywords.find(lexeme) != keywords.end()) {
        Token newToken(keywords[lexeme], lexeme);
        return newToken;
    }
    else if(state == "ZERO") {
        Token newToken("NUM", lexeme);
        return newToken;
    }
    else {
        Token newToken(state, lexeme);
        return newToken;
    }
}

bool checkToken(string state, string lexeme) {
    if(state[0] == '?') {
        return false;
    }
    else if(state == "NUM") {
        checkNum(lexeme);
    }

    return true;
}

void checkNum(string decimal) {
    try {
        stoi(decimal);
    }
    catch (const out_of_range& e) {
        throw runtime_error("Invalid decimal: " + decimal);
    }
}

string squish(string s) {
    stringstream ss(s);
    string token;
    string result;
    string space = "";
    while(ss >> token) {
        result += space;
        result += token;
        space = " ";
    }
    return result;
}

bool isChar(string s) {
    return s.length() == 1;
}

bool isRange(std::string s) {
    return s.length() == 3 && s[1] == '-';
}

std::string escape(std::string s) {
    std::string p;
    for (int i = 0; i < (int) s.length(); ++i) {
        if (s[i] == '\\' && i + 1 < (int) s.length()) {
            char c = s[i + 1];
            i = i + 1;
            if (c == 's') {
                p += ' ';
            } 
            else if (c == 'n') {
                p += '\n';
            } 
            else if (c == 'r') {
                p += '\r';
            } 
            else if (c == 't') {
                p += '\t';
            } 
            else if (c == 'x') {
                if (i + 2 < (int) s.length() && isxdigit(s[i + 1]) && isxdigit(s[i + 2])) {
                    if (hexToNum(s[i + 1]) > 8) {
                        throw std::runtime_error(
                            "Invalid escape sequence \\x" +
                            std::string(1, s[i + 1]) +
                            std::string(1, s[i + 2]) +
                            ": not in ASCII range (0x00 to 0x7F)");
                    }
                    char code = hexToNum(s[i + 1]) * 16 + hexToNum(s[i + 2]);
                    p += code;
                    i = i + 2;
                } 
                else {
                    p += c;
                }
            } 
            else if (isgraph(c)) {
                p += c;
            } 
            else {
                p += s[i];
            }
        } 
        else {
            p += s[i];
        }
    }
    return p;
}

std::string unescape(std::string s) {
    std::string p;
    for (int i = 0; i < (int) s.length(); ++i) {
        char c = s[i];
        if (c == ' ') {
            p += "\\s";
        } 
        else if (c == '\n') {
            p += "\\n";
        } 
        else if (c == '\r') {
            p += "\\r";
        } 
        else if (c == '\t') {
            p += "\\t";
        } 
        else if (!isgraph(c)) {
            std::string hex = "\\x";
            p += hex + numToHex((unsigned char) c / 16) + numToHex((unsigned char) c % 16);
        } 
        else {
            p += c;
        }
    }
    return p;
}

int hexToNum(char c) {
    if ('0' <= c && c <= '9') {
        return c - '0';
    } 
    else if ('a' <= c && c <= 'f') {
        return 10 + (c - 'a');
    } 
    else if ('A' <= c && c <= 'F') {
        return 10 + (c - 'A');
    }
    throw std::runtime_error("Invalid hex digit!");
}

char numToHex(int d) {
    return (d < 10 ? d + '0' : d - 10 + 'A');
}