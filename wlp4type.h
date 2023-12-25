#ifndef WLP4TYPE_H
#define WL4TYPE_H

TreeNode* createTree();
void collectProcedures(ProcedureTable* &procTable, TreeNode* &tree);
void annotateTypes(TreeNode* &tree, Procedure &proc, ProcedureTable* &procTable);
void checkStatements(TreeNode* &tree);

#endif