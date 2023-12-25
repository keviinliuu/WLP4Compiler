#ifndef STRUCTS_H
#define STRUCTS_H

#include <map>
#include <vector>
#include <string> 
#include <stdexcept>

using namespace std;

struct Token {
    string kind;
    string lexeme;

    Token() : kind{""}, lexeme{""} {};
    Token(string kind, string lexeme) : kind{kind}, lexeme{lexeme} {};
};

struct DFA {
    vector<string> states;
    map<string, bool> accepting;
    map<string, map<char, string>> arrows;

    DFA(vector<string> states, map<string, bool> accepting, map<string, map<char, string>> arrows) : states{states}, accepting{accepting}, arrows{arrows} {}; 
};

struct Rule {
    string lhs;
    vector<string> rhs;

    Rule() : lhs{""}, rhs{} {};
    Rule(string lhs, vector<string> rhs) : lhs{lhs}, rhs{rhs} {};

    string getRHSString() {
        string str = "";
        for(auto s : rhs) {
            str += s + " ";
        }

        if(!str.empty()) {
            str.pop_back();
        }

        return str;
    }
};

struct SLR_DFA {
    map<pair<int, string>, int> transitions;
    map<pair<int, string>, int> reductions;

    SLR_DFA(map<pair<int, string>, int> transitions, map<pair<int, string>, int> reductions) : transitions{transitions}, reductions{reductions} {};
};

struct TreeNode {
    Rule rule;
    Token token;
    string type;
    bool terminal;
    vector<TreeNode*> children;

    TreeNode(Rule rule) : rule{rule}, token{}, terminal{false} {};
    TreeNode(Token token) : rule{}, token{token}, terminal{true} {};

    TreeNode* getChild(string symbol, int n) {
        int index = 0;

        for(string c : rule.rhs) {
            if(c == symbol) n--;
            if(n == 0) break;
            index++;
        }

        return children.at(index);
    }

    TreeNode* getChild(string symbol) {
        return getChild(symbol, 1);
    }

    ~TreeNode() {
        for(auto &c : children) {
            delete(c);
        }
    }
};

struct Variable {
    string name;
    string type;

    Variable() : name(""), type("") {}
    Variable(TreeNode* tree) {
        TreeNode* typeNode = tree->getChild("type");
        TreeNode* idNode = tree->getChild("ID");

        if(typeNode->children.size() == 1) {
            type = "int";
        }
        else if(typeNode->children.size() == 2) {
            type = "int*";
        }

        name = idNode->token.lexeme;
    }
};

struct VariableTable {
    map<string, Variable> table;

    void addVariable(Variable &var) {
        if(table.find(var.name) != table.end()) {
            throw runtime_error("Duplicate declaration of " + var.name + ".");
        }

        table.insert({var.name, var});
    }

    Variable getVariable(string varName) {
        if(table.find(varName) == table.end()) {
            throw runtime_error("Undeclared variable: " + varName + ".");
        }
        
        return table[varName];
    }
};

struct Procedure {
    string name;
    vector<string> signature;
    VariableTable localTable;

    Procedure() : name(""), signature(), localTable() {};
    Procedure(TreeNode* tree) {
        string procStruct = tree->rule.lhs;

        if(procStruct == "procedure") {
            name = tree->getChild("ID")->token.lexeme;

            TreeNode* paramsNode = tree->getChild("params");

            if(!paramsNode->children.empty()) {
                TreeNode* paramlistNode = paramsNode->getChild("paramlist");

                while(paramlistNode->children.size() == 3) {
                    Variable newVar(paramlistNode->getChild("dcl"));
                    signature.push_back(newVar.type);
                    localTable.addVariable(newVar);
                    paramlistNode = paramlistNode->getChild("paramlist");
                }

                Variable newVar(paramlistNode->getChild("dcl"));
                signature.push_back(newVar.type);
                localTable.addVariable(newVar);
            }

            TreeNode* dclsNode = tree->getChild("dcls");
            processDcls(dclsNode);
        }
        else {
            name = "wain";

            Variable paramOne(tree->getChild("dcl", 1));
            Variable paramTwo(tree->getChild("dcl", 2));
            signature.push_back(paramOne.type);
            signature.push_back(paramTwo.type);
            localTable.addVariable(paramOne);
            localTable.addVariable(paramTwo);

            if(paramTwo.type != "int") {
                throw runtime_error("Second argument of wain must be of type int.");
            }

            TreeNode* dclsNode = tree->getChild("dcls");
            processDcls(dclsNode);
        }
    }

    void processDcls(TreeNode* &dclsNode) {
        if(dclsNode->children.empty()) return;
        TreeNode* dclsNode2 = dclsNode->getChild("dcls");
        processDcls(dclsNode2);

        Variable newVar(dclsNode->getChild("dcl"));
        string ruleRhs = dclsNode->rule.getRHSString();
        
        if(ruleRhs == "dcls dcl BECOMES NUM SEMI" && newVar.type != "int") {
            throw runtime_error("Cannot initialize int* with an integer.");
        }

        if(ruleRhs == "dcls dcl BECOMES NULL SEMI" && newVar.type != "int*") {
            throw runtime_error("Cannot initialize int with NULL.");
        }

        localTable.addVariable(newVar);
        return;
    }
};

struct ProcedureTable {
    map<string, Procedure> table;

    void addProcedure(Procedure &proc) {
        if(table.find(proc.name) != table.end()) {
            throw runtime_error("Duplicate declaration of function " + proc.name + ".");
        }

        table.insert({proc.name, proc});
    }

    Procedure getProcedure(string procName) {
        if(table.find(procName) == table.end()) {
            throw runtime_error("Function " + procName + " has not been declared.");
        }

        return table[procName];
    }
};

#endif