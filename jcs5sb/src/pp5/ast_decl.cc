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

Location* VarDecl::Emit(Node* parent) {
    if (parent->isOfType("FnDecl")) {//is local var
        FnDecl* parentFunc = (FnDecl*)parent;
        memLoc = new Location(fpRelative, -4 * (parentFunc->frameSize++) - 8, getName());
        GetParent()->scope[getName()] = this;
        return memLoc;
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

Location* FnDecl::Emit(Node* parent) {
    generator->GenLabel(getLabel());
    BeginFunc* funcHeader = generator->GenBeginFunc();
    body->Emit(this);
    funcHeader->SetFrameSize(frameSize * 4);
    generator->GenEndFunc();
    
    return NULL;
}

const char *FnDecl::getLabel() {
    generator->currentFrame = this;
    if (GetParent()->isOfType("Program")) {
        if (strcmp(getName(), "main") == 0) {
            return "main";
        }
        snprintf(label, sizeof label, "_%s", getName());
        return label;
    }
    return "";
}