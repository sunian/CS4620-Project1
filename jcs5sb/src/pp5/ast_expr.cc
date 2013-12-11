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

Location* BoolConstant::Emit(Node* parent) {
    return generator->GenLoadConstant(value ? 1 : 0);
}

StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
}

Location* StringConstant::Emit(Node* parent) {
    return generator->GenLoadConstant(value);
}

Location* NullConstant::Emit(Node* parent) {
    return generator->GenLoadConstant((char*)NULL);
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
        if (strcmp(op->getOpName(), "<=") == 0) {
            Location* temp = generator->GenBinaryOp("<", right->Emit(parent), left->Emit(parent));
            return generator->GenBinaryOp("==", generator->GenLoadConstant(0), temp);
        }
        if (strcmp(op->getOpName(), ">=") == 0) {
            Location* temp = generator->GenBinaryOp("<", left->Emit(parent), right->Emit(parent));
            return generator->GenBinaryOp("==", generator->GenLoadConstant(0), temp);
        }
        if (strcmp(op->getOpName(), ">") == 0) {
            return generator->GenBinaryOp("<", right->Emit(parent), left->Emit(parent));
        }
        if (strcmp(op->getOpName(), "!=") == 0) {
            Location* temp = generator->GenBinaryOp("==", left->Emit(parent), right->Emit(parent));
            return generator->GenBinaryOp("==", generator->GenLoadConstant(0), temp);
        }
        return generator->GenBinaryOp(op->getOpName(), left->Emit(parent), right->Emit(parent));
    }
    if (strcmp(op->getOpName(), "-") == 0) {
        return generator->GenBinaryOp("*", generator->GenLoadConstant(-1), right->Emit(parent));
    }
    if (strcmp(op->getOpName(), "!") == 0) {
        return generator->GenBinaryOp("==", generator->GenLoadConstant(0), right->Emit(parent));
    }
    return NULL;
}
 
Location* AssignExpr::Emit(Node* parent) {
    // printf("Assign %s\n", parent->GetPrintNameForNode());
    Location* loc = left->Emit(this);
    if (loc->GetSegment() == thisRelative) {
        generator->GenStore(CodeGenerator::ThisPtr, right->Emit(parent), loc->GetOffset());
        return generator->GenLoad(CodeGenerator::ThisPtr, loc->GetOffset());
    }
    if (left->isOfType("ArrayAccess")) {
        generator->GenStore(loc, right->Emit(parent), 0);
        return generator->GenLoad(loc, 0);
    } 
    generator->GenAssign(loc, right->Emit(parent));
    return loc;
}
  
ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    Assert(b != NULL && s != NULL);
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

Location* ArrayAccess::Emit(Node* parent) {
    Location* loc = generator->GenBinaryOp("+", 
        base->Emit(parent), 
        generator->GenBinaryOp("*", generator->GenLoadConstant(4), subscript->Emit(parent)));
    if (parent->isOfType("AssignExpr")) {
        return loc;
        // return new Location(arrayRelative, 4 * subscript, "array");
    }
    return generator->GenLoad(loc, 0);
}

Type *ArrayAccess::getType() {
    Type* baseType = base->getType();
    if (baseType->isOfType("ArrayType")) {
        Type* elemType = ((ArrayType*)baseType)->getElemType();
        // printf("%s\n", elemType->getName());
        return elemType;
    }
    
    if (base->isOfType("FieldAccess")) {
        VarDecl* varDecl = ((FieldAccess*)base)->getVarDecl();
        // printf("var=%d\n", varDecl);
        return varDecl->getType();
    }
    return base->getType();
}
     
FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}

VarDecl* FieldAccess::getVarDecl() {
    Decl* decl;
    if (base == NULL) {
        // printf("var name=%s\n", field->getName());
        decl = searchScope(field->getName());
    } else if (base->isOfType("This")) {
        Node* parent = GetParent();
        // printf("Search for parent\n");
        while (!parent->isOfType("ClassDecl")) parent = parent->GetParent();
        // printf("Parent is %s\n", parent->GetPrintNameForNode());
        decl = parent->scope[field->getName()];
    }
    if (decl != NULL && decl->isOfType("VarDecl")) return (VarDecl*)decl;
    return NULL;
}

Location* FieldAccess::Emit(Node* parent) {
    VarDecl* varDecl = getVarDecl();
    if (varDecl != NULL) {
        if (varDecl->memLoc != NULL) return varDecl->memLoc;
        if (parent->isOfType("AssignExpr")) {
            return new Location(thisRelative, 4 * (varDecl->heapOffset + 1), varDecl->getName());
        }
        
        // printf("var offset=%d\n", varDecl->heapOffset);
        return generator->GenLoad(CodeGenerator::ThisPtr, 4 * (varDecl->heapOffset + 1));
        
    }
    return NULL;
}

Type *FieldAccess::getType() {
    VarDecl* varDecl = getVarDecl();
    if (varDecl != NULL) return varDecl->getType();
    return NULL;
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}

Type *Call::getType() {
    FnDecl* fnDecl;
    if (base != NULL && !base->isOfType("This")) {
        Type* baseType = base->getType();
        if (baseType != NULL && baseType->isOfType("NamedType")) {
            NamedType* baseClass = (NamedType*)baseType;
            ClassDecl* baseDecl = (ClassDecl*)(parent->searchScope(baseClass->getName()));
            fnDecl = (FnDecl*)(baseDecl->scope[field->getName()]);
        }
    } else {
        fnDecl = (FnDecl*)searchScope(field->getName());
    }
    return fnDecl->getReturnType();
}

Location* Call::Emit(Node* parent) {
    FnDecl* fnDecl;
    Location* returnVal;
    if (base != NULL && !base->isOfType("This")) {
        Type* baseType = base->getType();
        if (baseType != NULL && baseType->isOfType("NamedType")) {
            Location* baseLoc = base->Emit(parent);
            NamedType* baseClass = (NamedType*)baseType;
            ClassDecl* baseDecl = (ClassDecl*)(parent->searchScope(baseClass->getName()));
            Location* vTable = generator->GenLoad(baseLoc);
            fnDecl = (FnDecl*)(baseDecl->scope[field->getName()]);
            vTable = generator->GenLoad(vTable, fnDecl->vOffset * 4);
            for (int i = actuals->NumElements() - 1; i >= 0; i--) {
                generator->GenPushParam(actuals->Nth(i)->Emit(parent));
            }
            generator->GenPushParam(baseLoc);
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
        Location* newObj = generator->GenBuiltInCall(Alloc, heapSize);
        generator->GenStore(newObj, generator->GenLoadLabel(classDecl->getName()));
        return newObj;
    }
    return NULL;
}

NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

Location* NewArrayExpr::Emit(Node* parent) {
    return generator->GenBuiltInCall(Alloc, generator->GenBinaryOp("*", generator->GenLoadConstant(4), size->Emit(parent)));
}

Location* ReadIntegerExpr::Emit(Node* parent) {
    return generator->GenBuiltInCall(ReadInteger);
}

Location* ReadLineExpr::Emit(Node* parent) {
    return generator->GenBuiltInCall(ReadLine);
}
