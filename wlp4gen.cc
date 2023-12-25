#include <iostream>
#include <sstream>

#include "structs.h"
#include "wlp4type.h"
#include "wlp4gen.h"

using namespace std;

int labelCounter = 0;
bool print = false;
bool memAlloc = false;

int main() {
    TreeNode* tree;
    try {
        tree = createTree();
        print = hasPrint(tree);
        memAlloc = hasMemAlloc(tree);
        generateCode(tree->getChild("procedures"));
        delete(tree);
    }
    catch (runtime_error &e) {
        delete(tree);
        cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

void generateCode(TreeNode* tree) {
    if(tree->children.size() == 1) {
        TreeNode* wainNode = tree->getChild("main");
        generateWain(wainNode);
    }
    else {
        TreeNode* procsNode = tree->getChild("procedures");
        generateCode(procsNode);
        TreeNode* procNode = tree->getChild("procedure");
        generateProc(procNode);
    }
}

void generateWain(TreeNode* tree) {
    if(print) import("print");
    if(memAlloc) {
        import("init");
        import("new");
        import("delete");
    }

    Lis(4);
    Word(4);
    Lis(11);
    Word(1);

    if(memAlloc) {
        if(tree->getChild("dcl", 1)->getChild("type")->children.size() == 1) {
            push(2);
            Add(2, 0, 0);
            push(31);
            callProc("init");
            pop(31);
            pop(2);
        }
        else {
            push(31);
            callProc("init");
            pop(31);
        }
    }

    map<string, int> offsetTable;
    int counter = 0;

    TreeNode* firstParam = tree->getChild("dcl", 1);
    offsetTable.insert({firstParam->getChild("ID")->token.lexeme, offset(counter, 2)});
    counter++;
    push(1);

    TreeNode* secondParam = tree->getChild("dcl", 2);
    offsetTable.insert({secondParam->getChild("ID")->token.lexeme, offset(counter, 2)});
    counter++;
    push(2);

    Sub(29, 30, 4);
    
    TreeNode* dclsNode = tree->getChild("dcls");
    generateDcls(dclsNode, offsetTable, counter, 2);

    TreeNode* statementsNode = tree->getChild("statements");
    generateStatements(statementsNode, offsetTable);

    TreeNode* exprNode = tree->getChild("expr");
    generateGeneral(exprNode, offsetTable);

    for(int i = 0; i < counter; i++) {
        pop();
    }

    Jr(31);
    cout << "\n";
}

void generateProc(TreeNode* tree) {
    map<string, int> offsetTable;
    int counter = 0;
    int paramCount = 0;

    string procName = tree->getChild("ID")->token.lexeme;
    Label("PROC" + procName);

    TreeNode* paramsNode = tree->getChild("params");

    if(!paramsNode->children.empty()) {
        TreeNode* paramlistNode = paramsNode->getChild("paramlist");

        while(paramlistNode->children.size() == 3) {
            paramCount++;
            paramlistNode = paramlistNode->getChild("paramlist");
        }
        paramCount++;

        paramlistNode = paramsNode->getChild("paramlist");

        while(paramlistNode->children.size() == 3) {
            TreeNode* paramNode = paramlistNode->getChild("dcl");
            offsetTable.insert({paramNode->getChild("ID")->token.lexeme, offset(counter, paramCount)});
            counter++;
            paramlistNode = paramlistNode->getChild("paramlist");
        }
        
        TreeNode* paramNode = paramlistNode->getChild("dcl");
        offsetTable.insert({paramNode->getChild("ID")->token.lexeme, offset(counter, paramCount)});
        counter++;
    }

    Sub(29, 30, 4);

    TreeNode* dclsNode = tree->getChild("dcls");
    generateDcls(dclsNode, offsetTable, counter, paramCount);

    TreeNode* statementsNode = tree->getChild("statements");
    generateStatements(statementsNode, offsetTable);

    TreeNode* exprNode = tree->getChild("expr");
    generateGeneral(exprNode, offsetTable);

    for(int i = 0; i < counter - paramCount; i++) {
        pop();
    }

    Jr(31);
    cout << "\n";
}

void generateDcls(TreeNode* tree, map<string, int> &offsetTable, int &counter, int n) {
    if(tree->children.empty()) return;
        
    generateDcls(tree->getChild("dcls"), offsetTable, counter, n);

    TreeNode* dclNode = tree->getChild("dcl");
    string varName = dclNode->getChild("ID")->token.lexeme;

    if(tree->children.at(3)->token.kind == "NUM") {
        int num = stoi(tree->getChild("NUM")->token.lexeme);
        constant(num);
        push(3);
    }
    else if(tree->children.at(3)->token.kind == "NULL"){
        push(11);
    }

    offsetTable.insert({varName, offset(counter, n)});
    counter++;
}

void generateStatements(TreeNode* tree, map<string, int> &offsetTable) {
    string ruleRHS = tree->rule.getRHSString();

    if(tree->rule.lhs == "statements") {
        if(ruleRHS.empty()) {
            return;
        }
        else if(ruleRHS == "statements statement") {
            generateStatements(tree->getChild("statements"), offsetTable);
            generateStatements(tree->getChild("statement"), offsetTable);
        }
    }
    else if(tree->rule.lhs == "statement") {
        if(ruleRHS == "lvalue BECOMES expr SEMI") {
            generateLvalues(tree, offsetTable);
        }
        else if(ruleRHS == "PRINTLN LPAREN expr RPAREN SEMI") {
            generateGeneral(tree->getChild("expr"), offsetTable);
            Add(1, 3, 0);
            push(31);
            callProc("print");
            pop(31);
        }
        else if(ruleRHS == "IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE") {
            generateIf(tree, offsetTable);
        }
        else if(ruleRHS == "WHILE LPAREN test RPAREN LBRACE statements RBRACE") {
            generateWhile(tree, offsetTable);
        }
        else if(ruleRHS == "DELETE LBRACK RBRACK expr SEMI") {
            generateGeneral(tree->getChild("expr"), offsetTable);
            Add(1, 3, 0);
            Beq(1, 11, "skipDelete" + to_string(labelCounter));
            push(31);
            callProc("delete");
            pop(31);
            Label("skipDelete" + to_string(labelCounter));
            labelCounter++;
        }
    }
}

void generateGeneral(TreeNode* tree, map<string, int> &offsetTable) {
    string ruleRHS = tree->rule.getRHSString();

    if(tree->rule.lhs == "expr") {
        if(ruleRHS == "term") {
            generateGeneral(tree->getChild("term"), offsetTable);
        }
        else if(ruleRHS == "expr PLUS term") {
            if(tree->getChild("expr")->type == "int" && tree->getChild("term")->type == "int") {
                generateGeneral(tree->getChild("expr"), offsetTable);
                push(3);
                generateGeneral(tree->getChild("term"), offsetTable);
                pop(5);
                Add(3, 5, 3);
            }
            else {
                if(tree->getChild("expr")->type == "int*") {
                    generateGeneral(tree->getChild("expr"), offsetTable);
                    push(3);
                    generateGeneral(tree->getChild("term"), offsetTable);
                    Mult(3, 4);
                    Mflo(3);
                    pop(5);
                    Add(3, 5, 3);
                }
                else if(tree->getChild("term")->type == "int*") {
                    generateGeneral(tree->getChild("term"), offsetTable);
                    push(3);
                    generateGeneral(tree->getChild("expr"), offsetTable);
                    Mult(3, 4);
                    Mflo(3);
                    pop(5);
                    Add(3, 5, 3);
                }
            }
        }
        else if(ruleRHS == "expr MINUS term") {
            if(tree->getChild("expr")->type == "int" && tree->getChild("term")->type == "int") {
                generateGeneral(tree->getChild("expr"), offsetTable);
                push(3);
                generateGeneral(tree->getChild("term"), offsetTable);
                pop(5);
                Sub(3, 5, 3);
            }
            else {
                if(tree->getChild("term")->type == "int") {
                    generateGeneral(tree->getChild("expr"), offsetTable);
                    push(3);
                    generateGeneral(tree->getChild("term"), offsetTable);
                    Mult(3, 4);
                    Mflo(3);
                    pop(5);
                    Sub(3, 5, 3);
                }
                else if(tree->getChild("term")->type == "int*") {
                    generateGeneral(tree->getChild("expr"), offsetTable);
                    push(3);
                    generateGeneral(tree->getChild("term"), offsetTable);
                    pop(5);
                    Sub(3, 5, 3);
                    Div(3, 4);
                    Mflo(3);
                }
            }
        }
    }
    else if(tree->rule.lhs == "term") {
        if(ruleRHS == "factor") {
            generateGeneral(tree->getChild("factor"), offsetTable);
        }
        else if(ruleRHS == "term STAR factor") {
            generateGeneral(tree->getChild("term"), offsetTable);
            push(3);
            generateGeneral(tree->getChild("factor"), offsetTable);
            pop(5);
            Mult(3, 5);
            Mflo(3);
        }
        else if(ruleRHS == "term SLASH factor") {
            generateGeneral(tree->getChild("term"), offsetTable);
            push(3);
            generateGeneral(tree->getChild("factor"), offsetTable);
            pop(5);
            Div(5, 3);
            Mflo(3);
        }
        else if(ruleRHS == "term PCT factor") {
            generateGeneral(tree->getChild("term"), offsetTable);
            push(3);
            generateGeneral(tree->getChild("factor"), offsetTable);
            Lw(5, 0, 30);
            Div(5, 3);
            Mflo(5);
            Mult(3, 5);
            Mflo(3);
            pop(5);
            Sub(3, 5, 3);
        }
    }
    else if(tree->rule.lhs == "factor") {
        if(ruleRHS == "ID") {
            int offset = offsetTable[tree->getChild("ID")->token.lexeme];
            Lw(3, offset, 29);
        }
        else if(ruleRHS == "NUM") {
            int num = stoi(tree->getChild("NUM")->token.lexeme);
            constant(num);
        } 
        else if(ruleRHS == "NULL") {
            Add(3, 11, 0);
        }
        else if(ruleRHS == "LPAREN expr RPAREN") {
            generateGeneral(tree->getChild("expr"), offsetTable);
        }
        else if(ruleRHS == "AMP lvalue") {
            generateLvalues(tree, offsetTable);
        }
        else if(ruleRHS == "STAR factor") {
            generateGeneral(tree->getChild("factor"), offsetTable);
            Lw(3, 0, 3);
        }
        else if(ruleRHS == "NEW INT LBRACK expr RBRACK") {
            generateGeneral(tree->getChild("expr"), offsetTable);
            Add(1, 3, 0);
            push(31);
            callProc("new");
            pop(31);
            Bne(3, 0, "newSuccess" + to_string(labelCounter));
            Add(3, 11, 0);
            Label("newSuccess" + to_string(labelCounter));
            labelCounter++;        
        }
        else if(ruleRHS == "ID LPAREN RPAREN") {
            push(29);
            push(31);

            string procName = tree->getChild("ID")->token.lexeme;
            callProc("PROC" + procName);

            pop(31);
            pop(29);
        }
        else if(ruleRHS == "ID LPAREN arglist RPAREN") {
            push(29);
            push(31);

            int argCount = 0;
            TreeNode* arglistNode = tree->getChild("arglist");

            while(arglistNode->children.size() == 3) {
                TreeNode* exprNode = arglistNode->getChild("expr");
                generateGeneral(exprNode, offsetTable);
                argCount++;
                push(3);
                arglistNode = arglistNode->getChild("arglist");
            }

            TreeNode* exprNode = arglistNode->getChild("expr");
            generateGeneral(exprNode, offsetTable);
            argCount++;
            push(3);

            string procName = tree->getChild("ID")->token.lexeme;
            callProc("PROC" + procName);

            for(int i = 0; i < argCount; i++) {
                pop();
            }

            pop(31);
            pop(29);
        }
    }
}

void generateLvalues(TreeNode* tree, map<string, int> &offsetTable) {
    TreeNode* lvalueNode = tree->getChild("lvalue");

    while(lvalueNode->children.size() == 3) {
        lvalueNode = lvalueNode->getChild("lvalue");
    }

    string ruleRHS = lvalueNode->rule.getRHSString();

    if(ruleRHS == "ID") {
        int offset = offsetTable[lvalueNode->getChild("ID")->token.lexeme];
        if(tree->rule.lhs == "factor") {
            constant(offset);
            Add(3, 29, 3);
        }
        else if(tree->rule.lhs == "statement") {
            generateGeneral(tree->getChild("expr"), offsetTable);
            Sw(3, offset, 29);
       }
    }
    else if(ruleRHS == "STAR factor") {
        generateGeneral(lvalueNode->getChild("factor"), offsetTable);

        if(tree->rule.lhs == "statement") {
            push(3);
            generateGeneral(tree->getChild("expr"), offsetTable);
            pop(5);
            Sw(3, 0, 5);
        }
    }
}

void generateWhile(TreeNode* tree, map<string, int> &offsetTable) {
    pair<string, string> labels = createLoopLabel();

    Label(labels.first);

    TreeNode* testNode = tree->getChild("test");
    generateTest(testNode, labels.second, offsetTable);

    TreeNode* statementsNode = tree->getChild("statements");
    generateStatements(statementsNode, offsetTable);

    Beq(0, 0, labels.first);
    Label(labels.second);
}

void generateIf(TreeNode* tree, map<string, int> &offsetTable) {
    vector<string> labels = createIfLabel();
    
    Label(labels.at(0));

    TreeNode* testNode = tree->getChild("test");
    generateTest(testNode, labels.at(1), offsetTable);

    TreeNode* ifStatementsNode = tree->getChild("statements", 1);
    generateStatements(ifStatementsNode, offsetTable);
    Beq(0, 0, labels.at(2));

    Label(labels.at(1));

    TreeNode* elseStatementsNode = tree->getChild("statements", 2);
    generateStatements(elseStatementsNode, offsetTable);

    Label(labels.at(2));
}

void generateTest(TreeNode* tree, string endLabel, map<string, int> &offsetTable) {
    string ruleRHS = tree->rule.getRHSString();

    generateGeneral(tree->getChild("expr", 1), offsetTable);
    push(3);
    generateGeneral(tree->getChild("expr", 2), offsetTable);
    pop(5);

    if(ruleRHS == "expr EQ expr") {
        Bne(3, 5, endLabel);
    }
    else if(ruleRHS == "expr NE expr") {
        Beq(3, 5, endLabel);
    }
    else if(ruleRHS == "expr LT expr") {
        if(tree->getChild("expr", 1)->type == "int") {
            Slt(3, 5, 3);
        }
        else {
            Sltu(3, 5, 3);
        }
        Beq(3, 0, endLabel);
    }
    else if(ruleRHS == "expr GT expr") {
        if(tree->getChild("expr", 1)->type == "int") {
            Slt(3, 3, 5);
        }
        else {
            Sltu(3, 3, 5);
        }
        Beq(3, 0, endLabel);
    }
    else if(ruleRHS == "expr LE expr") {
        if(tree->getChild("expr", 1)->type == "int") {
            Slt(3, 3, 5);
        }
        else {
            Sltu(3, 3, 5);
        }
        Bne(3, 0, endLabel);
    }
    else if(ruleRHS == "expr GE expr") {
        if(tree->getChild("expr", 1)->type == "int") {
            Slt(3, 5, 3);
        }
        else {
            Sltu(3, 5, 3);
        }
        Bne(3, 0, endLabel);
    }
}

void Add(int d, int s, int t) {
    cout << "add $" << d << ", $" << s << ", $" << t << "\n";
}

void Sub(int d, int s, int t) {
    cout << "sub $" << d << ", $" << s << ", $" << t << "\n";
}

void Mult(int s, int t) {
    cout << "mult $" << s << ", $" << t << "\n";
}

void Multu(int s, int t) {
    cout << "multu $" << s << ", $" << t << "\n";
}

void Div(int s, int t) {
    cout << "div $" << s << ", $" << t << "\n";
}

void Divu(int s, int t) {
    cout << "divu $" << s << ", $" << t << "\n";
}

void Mfhi(int d) {
    cout << "mfhi $" << d  << "\n";
}

void Mflo(int d) {
    cout << "mflo $" << d  << "\n";
}

void Lis(int d) {
    cout << "lis $" << d  << "\n";
}

void Slt(int d, int s, int t) {
    cout << "slt $" << d << ", $" << s << ", $" << t << "\n";
}

void Sltu(int d, int s, int t) {
    cout << "sltu $" << d << ", $" << s << ", $" << t << "\n";
}

void Jr(int s) {
    cout << "jr $" << s << "\n";
}

void Jalr(int s) {
    cout << "jalr $" << s << "\n";
}

void Beq(int s, int t, string label) {
    cout << "beq $" << s << ", $" << t << ", " << label << "\n"; 
}

void Bne(int s, int t, string label) {
    cout << "bne $" << s << ", $" << t << ", " << label << "\n"; 
}

void Lw(int t, int i, int s) {
    cout << "lw $" << t << ", " << i << "($" << s << ")" << "\n";
}

void Sw(int t, int i, int s) {
    cout << "sw $" << t << ", " << i << "($" << s << ")" << "\n";
}

void Word(int i) {
    cout << ".word " << i << "\n";
}

void Word(string label) {
    cout << ".word " << label << "\n";
}

void Label(string label) {
    cout << label << ":" << "\n";
}

void push(int s) {
    Sw(s, -4, 30);
    Sub(30, 30, 4);
}

void pop(int d) {
    pop();
    Lw(d, -4, 30);
}

void pop() {
    Add(30, 30, 4);
}

void constant(int i) {
    Lis(3);
    Word(i);
}

void import(string label) {
    cout << ".import " << label << "\n";
}

void callProc(string proc) {
    Lis(5);
    Word(proc);
    Jalr(5);
}

int offset(int count, int n) {
    return (count - n) * -4;
}

bool hasPrint(TreeNode* tree) {
    if(tree == nullptr) {
        return false;
    }

    if(tree->rule.getRHSString().find("PRINTLN") == 0) {
        return true;
    }
    
    for(TreeNode* child : tree->children) {
        if(hasPrint(child)) {
            return true;
        }
    }

    return false;
}

bool hasMemAlloc(TreeNode* tree) {
    if(tree == nullptr) {
        return false;
    }

    if(tree->rule.getRHSString().find("NEW") == 0 || tree->rule.getRHSString().find("DELETE") == 0) {
        return true;
    }
    
    for(TreeNode* child : tree->children) {
        if(hasMemAlloc(child)) {
            return true;
        }
    }

    return false;
}

pair<string, string> createLoopLabel() {
    string begin = "loopLabel" + to_string(labelCounter);
    string end = "loopEndLabel" + to_string(labelCounter);
    labelCounter++;
    return {begin, end};
}

vector<string> createIfLabel() {
    string ifCond = "ifLabel" + to_string(labelCounter);
    string elseCond = "elseLabel" + to_string(labelCounter);
    string end = "ifEndLabel" + to_string(labelCounter);
    labelCounter++;
    return {ifCond, elseCond, end};
}