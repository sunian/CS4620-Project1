/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
        
         
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
}

VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}

void VarDecl::Check(Node* parent) {
    GetParent()->scope[getName()] = this;
    if (parent->isOfType("ClassDecl")) {//is instance field
        ClassDecl* parentClass = (ClassDecl*)parent;
        heapOffset = parentClass->heapSize++;
    }
}

Location* VarDecl::Emit(Node* parent) {
    if (parent->isOfType("FnDecl")) {//is local var
        FnDecl* parentFunc = (FnDecl*)parent;
        memLoc = new Location(fpRelative, -4 * (parentFunc->frameSize++) - 8, getName());
        return memLoc;
    } else if (parent->isOfType("ClassDecl")) {//is instance field

    }
    
    return NULL;
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
    heapSize = 0;
}

ClassDecl *ClassDecl::getSuperClass(){
    if (parent && extends) {
        Decl *superDecl = parent->scope[extends->getName()];
        if (superDecl != NULL && superDecl->isOfType("ClassDecl")) {
            return (ClassDecl*)superDecl;
        }
    }
    return NULL;
}

void ClassDecl::Check(Node* parent) {
    parent->scope[getName()] = this;
    for (int i = 0; i < members->NumElements(); i++) {
        members->Nth(i)->Check(this);
    }
}

Location* ClassDecl::Emit(Node* parent) {
    for (int i = 0; i < members->NumElements(); i++) {
        members->Nth(i)->Emit(this);
    }
    return NULL;
}

void ClassDecl::makeVTables() {
    if (methods != NULL) return;
    ClassDecl* superDecl = getSuperClass();
    methods = new List<FnDecl*>;
    if (superDecl != NULL) {
        superDecl->makeVTables();
        for (int i = 0; i < superDecl->methods->NumElements(); i++) {
            methods->Append(superDecl->methods->Nth(i));
            if (scope[superDecl->methods->Nth(i)->getName()] == NULL){
                scope[superDecl->methods->Nth(i)->getName()] = superDecl->methods->Nth(i);
            }
        }
        for (int i = 0; i < members->NumElements(); i++) {
            if (members->Nth(i)->isOfType("VarDecl")) {
                ((VarDecl*)(members->Nth(i)))->heapOffset += superDecl->heapSize;
            }
        }
        heapSize += superDecl->heapSize;
    }
    for (int i = 0; i < members->NumElements(); i++) {
        if (!members->Nth(i)->isOfType("FnDecl")) continue;
        FnDecl *fnDecl = (FnDecl*)(members->Nth(i));
        if (superDecl == NULL) {
            fnDecl->vOffset = methods->NumElements();
            methods->Append(fnDecl);
            continue;
        }
        Decl *superMethod = superDecl->scope[fnDecl->getName()];
        if (superMethod == NULL) {
            fnDecl->vOffset = methods->NumElements();
            methods->Append(fnDecl);
            continue;
        }
        fnDecl->vOffset = ((FnDecl*)superMethod)->vOffset;
        methods->RemoveAt(fnDecl->vOffset);
        methods->InsertAt(fnDecl, fnDecl->vOffset);
    }
}

InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

	
FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
    frameSize = 0;
}

void FnDecl::SetFunctionBody(Stmt *b) { 
    (body=b)->SetParent(this);
}

void FnDecl::Check(Node* parent) {
    parent->scope[getName()] = this;
    for (int i = 0; i < formals->NumElements(); i++) {
        scope[formals->Nth(i)->getName()] = formals->Nth(i);
    }
    body->Check(this);
}

Location* FnDecl::Emit(Node* parent) {
    // printf("Function: %s\n", getLabel());
    generator->currentFrame = this;
    for (int i = 0; i < formals->NumElements(); i++) {
        formals->Nth(i)->memLoc = new Location(fpRelative, 4 * i + 8, formals->Nth(i)->getName());
    }
    generator->GenLabel(getLabel());
    BeginFunc* funcHeader = generator->GenBeginFunc();
    body->Emit(this);
    funcHeader->SetFrameSize(frameSize * 4);
    generator->GenEndFunc();
    // printf("End Function: %s\n", getLabel());
    return NULL;
}

const char *FnDecl::getLabel() {
    if (GetParent()->isOfType("Program")) {
        if (strcmp(getName(), "main") == 0) {
            return "main";
        }
        snprintf(label, sizeof label, "_%s", getName());
        return label;
    }
    snprintf(label, sizeof label, "%s_%s",((ClassDecl*)GetParent())->getName(), getName());
    return label;
}