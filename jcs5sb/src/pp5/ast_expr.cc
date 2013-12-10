/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>



IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
}

Location* IntConstant::Emit(Node* parent) {
    return generator->GenLoadConstant(value);
}

DoubleConstant::DoubleConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}

StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}
CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r) 
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this); 
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r) 
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL; 
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}

Location* CompoundExpr::Emit(Node* parent) {
    if (left != NULL) {
        return generator->GenBinaryOp(op->getOpName(), left->Emit(parent), right->Emit(parent));
    }
    return NULL;
}
 
Location* AssignExpr::Emit(Node* parent) {
    // printf("Assign %s\n", parent->GetPrintNameForNode());
    Location* loc = left->Emit(parent);
    generator->GenAssign(loc, right->Emit(parent));
    return loc;
}
  
ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}
     
FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}

Location* FieldAccess::Emit(Node* parent) {
    Decl* decl = searchScope(field->getName());
    if (decl != NULL && decl->isOfType("VarDecl")) {
        VarDecl* varDecl = (VarDecl*)decl;
        if (varDecl->memLoc != NULL) return varDecl->memLoc;

    }
    return NULL;
}

Type *FieldAccess::getType() {
    return ((VarDecl*)searchScope(field->getName()))->getType();
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}

Type *Call::getType() {
    FnDecl* fnDecl = (FnDecl*)searchScope(field->getName());
    return fnDecl->getReturnType();
}

Location* Call::Emit(Node* parent) {
    FnDecl* fnDecl;
    Location* returnVal;
    if (base != NULL) {
        Type* baseType = base->getType();
        if (baseType != NULL && baseType->isOfType("NamedType")) {
            NamedType* baseClass = (NamedType*)baseType;
            ClassDecl* baseDecl = (ClassDecl*)(parent->searchScope(baseClass->getName()));
            Location* vTable = generator->GenLoadLabel(baseDecl->getName());
            fnDecl = (FnDecl*)(baseDecl->scope[field->getName()]);
            vTable = generator->GenLoad(vTable, fnDecl->vOffset * 4);
            for (int i = actuals->NumElements() - 1; i >= 0; i--) {
                generator->GenPushParam(actuals->Nth(i)->Emit(parent));
            }
            generator->GenPushParam(base->Emit(parent));
            returnVal = generator->GenACall(vTable, fnDecl->getReturnType() != Type::voidType);
        }
    } else {
        fnDecl = (FnDecl*)searchScope(field->getName());

        for (int i = actuals->NumElements() - 1; i >= 0; i--) {
            generator->GenPushParam(actuals->Nth(i)->Emit(parent));
        }
        generator->GenPushParam(CodeGenerator::ThisPtr);
        returnVal = generator->GenLCall(fnDecl->getLabel(), fnDecl->getReturnType() != Type::voidType);
    }
    
    generator->GenPopParams(4 * actuals->NumElements() + 4);
    return returnVal;
} 

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}

Location* NewExpr::Emit(Node* parent) {
    Decl* decl = parent->searchScope(cType->getIdent()->getName());
    // printf("%d\n", decl);
    if (decl != NULL && decl->isOfType("ClassDecl")) {
        ClassDecl* classDecl = (ClassDecl*)decl;
        Location* heapSize = generator->GenLoadConstant((classDecl->heapSize + 1) * 4);
        return generator->GenBuiltInCall(Alloc, heapSize);
    }
    return NULL;
}

NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

       
