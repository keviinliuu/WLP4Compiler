#include <iostream>
#include <sstream>

#include "structs.h"
#include "wlp4parse.h"
#include "wlp4type.h"

using namespace std;

TreeNode* createTree() {
    TreeNode* tree;
    ProcedureTable* procTable;

    try {
        tree = parseTokens();
        
        procTable = new ProcedureTable();
        TreeNode* procNode = tree->getChild("procedures");
        collectProcedures(procTable, procNode);

        delete(procTable);
        return tree;
    }
    catch (runtime_error &e) {
        delete(tree);
        delete(procTable);
        throw runtime_error(e.what());
    }
}

void collectProcedures(ProcedureTable* &procTable, TreeNode* &tree) {
    if(tree->children.size() == 1) {
        TreeNode* mainNode = tree->getChild("main");
        Procedure mainProc(mainNode);

        procTable->addProcedure(mainProc);
        annotateTypes(mainNode, mainProc, procTable);

        if(mainNode->getChild("expr")->type != "int") {
            throw runtime_error("wain must return int.");
        }

        checkStatements(mainNode);
    }
    else {
        TreeNode* procNode = tree->getChild("procedure");
        Procedure proc(procNode);

        procTable->addProcedure(proc);
        annotateTypes(procNode, proc, procTable);

        if(procNode->getChild("expr")->type != "int") {
            throw runtime_error("Procedure must return int.");
        }

        checkStatements(procNode);

        TreeNode* procsNode = tree->getChild("procedures");
        collectProcedures(procTable, procsNode);
    }
}

void annotateTypes(TreeNode* &tree, Procedure &proc, ProcedureTable* &procTable) {
    for(TreeNode* child : tree->children) {
        annotateTypes(child, proc, procTable);
    }

    string ruleRhs = tree->rule.getRHSString();

    if(tree->rule.lhs == "factor") {
        if(ruleRhs == "NUM") {
            tree->type = "int";
        }
        else if(ruleRhs == "NULL") {
            tree->type = "int*";
        }
        else if(ruleRhs == "ID") {
            tree->type = proc.localTable.getVariable(tree->getChild("ID")->token.lexeme).type;
        }
        else if(ruleRhs == "LPAREN expr RPAREN") {
            tree->type = tree->getChild("expr")->type;
        }
        else if(ruleRhs == "AMP lvalue") {
            if(tree->getChild("lvalue")->type != "int") {
                throw runtime_error("Address can only be taken of an integer l-value.");
            }
            tree->type = "int*";
        }
        else if(ruleRhs == "STAR factor") {
            if(tree->getChild("factor")->type != "int*") {
                throw runtime_error("Cannot dereference a non-pointer.");
            }
            tree->type = "int";
        }
        else if(ruleRhs == "NEW INT LBRACK expr RBRACK") {
            if(tree->getChild("expr")->type != "int") {
                throw runtime_error("Argument of new int[] must be an integer.");
            }
            tree->type = "int*";
        }
        else if(ruleRhs == "ID LPAREN RPAREN") {
            string idName = tree->getChild("ID")->token.lexeme;

            if(proc.localTable.table.find(idName) != proc.localTable.table.end()) {
                throw runtime_error("Variable " + idName + " is not a function.");
            }

            if(!procTable->getProcedure(idName).signature.empty()) {
                throw runtime_error("Function " + idName + " expects arguments.");
            }

            tree->type = "int";
        }
        else if(ruleRhs == "ID LPAREN arglist RPAREN") {
            string idName = tree->getChild("ID")->token.lexeme;

            if(proc.localTable.table.find(idName) != proc.localTable.table.end()) {
                throw runtime_error("Variable " + idName + " is not a function.");
            }

            vector<string> args;
            TreeNode* argsNode = tree->getChild("arglist");

            while(argsNode->children.size() == 3) {
                args.push_back(argsNode->getChild("expr")->type);
                argsNode = argsNode->getChild("arglist");
            }

            args.push_back(argsNode->getChild("expr")->type);

            if(args.size() != procTable->getProcedure(idName).signature.size()) {
                throw runtime_error("Function " + idName + " given wrong number of args.");
            }

            for(int i = 0; i < int(args.size()); i++) {
                if(args.at(i) != procTable->getProcedure(idName).signature.at(i)) {
                    throw runtime_error("Function " + idName + " argument type mismatch.");
                }
            }
            
            tree->type = "int";
        }
    }
    else if(tree->rule.lhs == "expr") {
        if(ruleRhs == "term") {
            tree->type = tree->getChild("term")->type;
        }
        else if(ruleRhs == "expr PLUS term") {
            if(tree->getChild("expr")->type == "int" && tree->getChild("term")->type == "int") {
                tree->type = "int";
            }
            else if(tree->getChild("expr")->type == "int*" && tree->getChild("term")->type == "int") {
                tree->type = "int*";
            }
            else if(tree->getChild("expr")->type == "int" && tree->getChild("term")->type == "int*") {
                tree->type = "int*";
            }
            else {
                throw runtime_error("Incompatible operands to +.");
            }
        }
        else if(ruleRhs == "expr MINUS term") {
            if(tree->getChild("expr")->type == "int" && tree->getChild("term")->type == "int") {
                tree->type = "int";
            }
            else if(tree->getChild("expr")->type == "int*" && tree->getChild("term")->type == "int") {
                tree->type = "int*";
            }
            else if(tree->getChild("expr")->type == "int*" && tree->getChild("term")->type == "int*") {
                tree->type = "int";
            }
            else {
                throw runtime_error("Incompatible operands to -.");
            }
        }
    }
    else if(tree->rule.lhs == "lvalue") {
        if(ruleRhs == "ID") {
            tree->type = proc.localTable.getVariable(tree->getChild("ID")->token.lexeme).type;
        }
        else if(ruleRhs == "LPAREN lvalue RPAREN") {
            tree->type = tree->getChild("lvalue")->type;
        }
        else if(ruleRhs == "STAR factor") {
            if(tree->getChild("factor")->type != "int*") {
                throw runtime_error("Cannot dereference a non-pointer.");
            }
            tree->type = "int";
        }
    }
    else if(tree->rule.lhs == "term") {
        if(ruleRhs == "factor") {
            tree->type = tree->getChild("factor")->type;
        }
        else {
            if(!(tree->getChild("term")->type == "int" && tree->getChild("factor")->type == "int")) {
                throw runtime_error("Operands of * / % must be ints.");
            }
            tree->type = "int";
        }
    }
}

void checkStatements(TreeNode* &tree) {
    for(TreeNode* child : tree->children) {
        checkStatements(child);
    }

    string ruleRhs = tree->rule.getRHSString();
    if(tree->rule.lhs == "statement") {
        if(ruleRhs == "lvalue BECOMES expr SEMI") {
            if(tree->getChild("lvalue")->type != tree->getChild("expr")->type) {
                throw runtime_error("Assignment between incompatible types.");
            }
        }
        else if(ruleRhs == "PRINTLN LPAREN expr RPAREN SEMI") {
            if(tree->getChild("expr")->type != "int") {
                throw runtime_error("println() requires an int.");
            }
        }
        else if(ruleRhs == "DELETE LBRACK RBRACK expr SEMI") {
            if(tree->getChild("expr")->type != "int*") {
                throw runtime_error("delete [] requires a int*.");
            }
        }
    }
    else if(tree->rule.lhs == "test") {
        if(tree->getChild("expr", 1)->type != tree->getChild("expr", 2)->type) {
            throw runtime_error("Cannot compare int* to int.");
        }
    }
}