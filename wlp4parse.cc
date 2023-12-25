#include <iostream> 
#include <sstream>

#include "structs.h"
#include "wlp4data.h"
#include "wlp4scan.h"
#include "wlp4parse.h"

using namespace std;

TreeNode* parseTokens() {
    SLR_DFA* slrDFA;

    try {
        stringstream cfg(WLP4_CFG);
        vector<Rule> CFG = createCFG(cfg);

        stringstream wlp4dfa(WLP4_DFA);
        slrDFA = createSLRDFA(wlp4dfa);

        deque<Token> tokens = scanTokens();
        vector<TreeNode*> treeStack;
        vector<int> stateStack = {0};

        TreeNode* parseTree = parse(CFG, slrDFA, tokens, treeStack, stateStack);

        delete(slrDFA);
        return parseTree;
    }
    catch (runtime_error &e) {
        delete(slrDFA);
        throw runtime_error(e.what());
    }
}

vector<Rule> createCFG(istream &cin) {
    vector<Rule> rules;
    string line;

    getline(cin, line);

    while(getline(cin, line)) {
        int nextSpace = line.find(" ");
        vector<string> symbols;

        while(nextSpace != int(string::npos)) {
            symbols.push_back(line.substr(0, nextSpace));
            line = line.substr(nextSpace + 1);
            nextSpace = line.find(" ");
        }
        if(line != ".EMPTY") symbols.push_back(line);

        string lhs = symbols.at(0);
        symbols.erase(symbols.begin());

        Rule newRule(lhs, symbols);
        rules.push_back(newRule);
    }

    return rules;
}

SLR_DFA* createSLRDFA(istream &cin) {
    map<pair<int, string>, int> transitions;
    map<pair<int, string>, int> reductions;

    string line;

    getline(cin, line);

    while(getline(cin, line) && line != ".REDUCTIONS") {
        int fromState = stoi(line.substr(0, line.find(" ")));
        line = line.substr(line.find(" ") + 1);
        string symbol = line.substr(0, line.find(" "));
        line = line.substr(line.find(" ") + 1);
        int toState = stoi(line);

        transitions[{fromState, symbol}] = toState;
    }

    while(getline(cin, line)) {
        int stateNum = stoi(line.substr(0, line.find(" ")));
        line = line.substr(line.find(" ") + 1);
        int ruleNum = stoi(line.substr(0, line.find(" ")));
        line = line.substr(line.find(" ") + 1);
        string tag = line;

        reductions[{stateNum, tag}] = ruleNum;
    }

    SLR_DFA* newSLRDFA = new SLR_DFA(transitions, reductions);
    return newSLRDFA;
}

void reduceTrees(Rule &rule, vector<TreeNode*> &treeStack) {
    TreeNode* newNode = new TreeNode(rule);
    int len = rule.rhs.size();
    for(int i = treeStack.size() - len; i < int(treeStack.size()); i++) newNode->children.push_back(treeStack.at(i));
    for(int i = 0; i < len; i++) treeStack.pop_back();
    treeStack.push_back(newNode);
}

void reduceStates(Rule &rule, vector<int> &stateStack, SLR_DFA* &dfa) {
    int len = rule.rhs.size();
    for(int i = 0; i < len; i++) stateStack.pop_back();
    int nextState = dfa->transitions[{stateStack.back(), rule.lhs}];
    stateStack.push_back(nextState);
}

void shift(deque<Token> &tokens, vector<TreeNode*> &treeStack, vector<int> &stateStack, SLR_DFA* &dfa) {
    Token firstToken = tokens.front();
    TreeNode* newNode = new TreeNode(firstToken);
    treeStack.push_back(newNode);

    if(dfa->transitions.find({stateStack.back(), firstToken.kind}) != dfa->transitions.end()) {
        stateStack.push_back(dfa->transitions[{stateStack.back(), firstToken.kind}]);
    }
    else {
        for(TreeNode* node : treeStack) {
            delete(node);
        }
        throw runtime_error("Could not parse properly.");
    }

    tokens.pop_front();
}

TreeNode* parse(vector<Rule> &CFG, SLR_DFA* &dfa, deque<Token> &tokens, vector<TreeNode*> &treeStack, vector<int> &stateStack) {
    while(!tokens.empty()) {
        while(dfa->reductions.find({stateStack.back(), tokens.front().kind}) != dfa->reductions.end()) {
            reduceTrees(CFG.at(dfa->reductions[{stateStack.back(), tokens.front().kind}]), treeStack);
            reduceStates(CFG.at(dfa->reductions[{stateStack.back(), tokens.front().kind}]), stateStack, dfa);
        }
        shift(tokens, treeStack, stateStack, dfa);   
    }

    reduceTrees(CFG.at(0), treeStack);

    return treeStack.at(0);
}

void printTree(TreeNode* node) {
    if(node->terminal) {
        cout << node->token.kind << " " << node->token.lexeme << "\n";
    }
    else {
        cout << node->rule.lhs << " "; 
        if(node->rule.rhs.empty()) {
            cout << ".EMPTY";
        }
        else {
            for(int i = 0; i < int(node->rule.rhs.size()); i++) {
                cout << node->rule.rhs.at(i);
                if(i != int(node->rule.rhs.size()) - 1) {
                    cout << " ";
                }
            }
        }
        cout << "\n";
    }

    for(TreeNode* tree : node->children) {
        printTree(tree);
    }
}