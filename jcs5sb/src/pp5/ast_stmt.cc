/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::makeVTables() {
    for (int i = 0; i < decls->NumElements(); i++) {
        if (decls->Nth(i)->isOfType("ClassDecl")) {
            ((ClassDecl*)decls->Nth(i))->makeVTables();
        }
    }
}

void Program::Check(Node* parent) {
    for (int i = 0; i < decls->NumElements(); i++) {
        decls->Nth(i)->Check(this);
    }
}

Location* Program::Emit(Node* parent) {
    /* pp5: here is where the code generation is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, generating instructions as you go.
     *      Each node can have its own way of translating itself,
     *      which makes for a great use of inheritance and
     *      polymorphism in the node classes.
     */
    for (int i = 0; i < decls->NumElements(); i++) {
        decls->Nth(i)->Emit(this);
    }
    // generator->GenLabel("main");
    // BeginFunc* funcHeader = generator->GenBeginFunc();
    // funcHeader->SetFrameSize(16);
    // Location* fortytwo = generator->GenLoadLabel("MyClass");
    // fortytwo = generator->GenLoad(fortytwo, 0);
    // Location* delim = generator->GenLoadConstant(-7);
    // generator->GenBuiltInCall(PrintInt, fortytwo);
    // generator->GenBuiltInCall(PrintInt, delim);
    // fortytwo = generator->GenLoadLabel("meth1");
    // generator->GenBuiltInCall(PrintInt, fortytwo);
    // generator->GenBuiltInCall(PrintInt, delim);
    // generator->GenACall(fortytwo, false);
    // generator->GenEndFunc();
    // generator->GenLabel("meth1");
    // funcHeader = generator->GenBeginFunc();
    // funcHeader->SetFrameSize(16);
    // fortytwo = generator->GenLoadConstant(42);
    // generator->GenBuiltInCall(PrintInt, fortytwo);
    // generator->GenEndFunc();
    // List<const char*>* methods = new List<const char*>;
    // methods->Append("meth1");
    // generator->GenVTable("MyClass", methods);
    generator->DoFinalCodeGen();
    return NULL;
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

void StmtBlock::Check(Node* parent) {
    for (int i = 0; i < decls->NumElements(); i++) {
        decls->Nth(i)->Check(parent);
    }
    for (int i = 0; i < stmts->NumElements(); i++) {
        stmts->Nth(i)->Check(parent);
    }
}

Location* StmtBlock::Emit(Node* parent) {
    for (int i = 0; i < decls->NumElements(); i++) {
        decls->Nth(i)->Emit(parent);
    }
    for (int i = 0; i < stmts->NumElements(); i++) {
        stmts->Nth(i)->Emit(parent);
    }
    return NULL;
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}


ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}

Location* ReturnStmt::Emit(Node* parent) {
    generator->GenReturn(expr->Emit(parent));
    return NULL;
}
  
PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

Location* PrintStmt::Emit(Node* parent) {
    for (int i = 0; i < args->NumElements(); i++) {
        if (args->Nth(i)->getType() == Type::intType) {
            generator->GenBuiltInCall(PrintInt, args->Nth(i)->Emit(parent));
        }
        
    }
    return NULL;
}

