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

void Decl::doPass(int pass, Node* parentScope) {
    if (pass == 1) {
        if (parentScope->scope[id->getName()]) {
            ReportError::DeclConflict(this, parentScope->scope[id->getName()]);
        } else {
            parentScope->scope[id->getName()] = this;
        }
    }
}

VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}
  
void VarDecl::PrintChildren(int indentLevel) { 
   type->Print(indentLevel+1);
   id->Print(indentLevel+1);
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
}

void ClassDecl::PrintChildren(int indentLevel) {
    id->Print(indentLevel+1);
    if (extends) extends->Print(indentLevel+1, "(extends) ");
    implements->PrintAll(indentLevel+1, "(implements) ");
    members->PrintAll(indentLevel+1);
}

ClassDecl *ClassDecl::getSuperClass(){
    if (parent && extends) {
        Decl *superDecl = parent->scope[extends->getName()];
        if (superDecl != NULL && superDecl->nodeIsOfType("ClassDecl")) {
            return (ClassDecl*)superDecl;
        }
    }
    return NULL;
}

List<InterfaceDecl*> *ClassDecl::getInterfaces() {
    List<InterfaceDecl*> *interfaces = new List<InterfaceDecl*>;
    if (parent) {
        for (int i = 0; i < implements->NumElements(); i++) {
            Decl *interface = parent->scope[implements->Nth(i)->getName()];
            if (interface != NULL && interface->nodeIsOfType("InterfaceDecl")) {
                interfaces->Append((InterfaceDecl*)interface);
            }
        }
    }
    return interfaces;
}

FnDecl *ClassDecl::getOverridenFn(char *name) {
    List<InterfaceDecl*> *interfaces = getInterfaces();
    for (int i = 0; i < interfaces->NumElements(); i++) {
        Decl *matchingDecl = interfaces->Nth(i)->scope[name];
        if (matchingDecl != NULL && matchingDecl->nodeIsOfType("FnDecl")) {
            return (FnDecl*) matchingDecl;
        }
    }
    ClassDecl *superClass = getSuperClass();
    if (superClass) {
        Decl *matchingDecl = superClass->scope[name];
        if (matchingDecl != NULL && matchingDecl->nodeIsOfType("FnDecl")) {
            return (FnDecl*) matchingDecl;
        }
        matchingDecl = superClass->getOverridenFn(name);
        if (matchingDecl) return (FnDecl*) matchingDecl;
    }
    return NULL;
}

void ClassDecl::doPass(int pass, Node* parentScope) {
    Decl::doPass(pass, parentScope);
    members->PassAll(pass, this);
}

InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

void InterfaceDecl::PrintChildren(int indentLevel) {
    id->Print(indentLevel+1);
    members->PrintAll(indentLevel+1);
}

void InterfaceDecl::doPass(int pass, Node* parentScope) {
    Decl::doPass(pass, parentScope);
    members->PassAll(pass, this);
}

FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
}

void FnDecl::SetFunctionBody(Stmt *b) { 
    (body=b)->SetParent(this);
}

void FnDecl::PrintChildren(int indentLevel) {
    returnType->Print(indentLevel+1, "(return type) ");
    id->Print(indentLevel+1);
    formals->PrintAll(indentLevel+1, "(formals) ");
    if (body) body->Print(indentLevel+1, "(body) ");
}

void FnDecl::doPass(int pass, Node* parentScope) {
    Decl::doPass(pass, parentScope);
    // printf("checking FnDecl\n");
    if (pass == 2) {
        // printf("%s\n", parent->GetPrintNameForNode());
        if (parent->nodeIsOfType("ClassDecl")) {
            FnDecl *inherited = ((ClassDecl*) parent)->getOverridenFn(id->getName());
            if (inherited) {
                if (returnType->sameTypeAs(inherited->returnType) && 
                    formals->NumElements() == inherited->formals->NumElements()) {
                    for (int i = 0; i < formals->NumElements(); i++) {
                        // printf("%s %s\n", formals->Nth(i)->getType()->GetPrintNameForNode(), inherited->formals->Nth(i)->getType()->GetPrintNameForNode());
                        if (!formals->Nth(i)->getType()->sameTypeAs(inherited->formals->Nth(i)->getType())) {
                            ReportError::OverrideMismatch(this);
                            break;
                        }
                    }
                } else {
                    ReportError::OverrideMismatch(this);
                }
            }
        }
    } else {
        if (body) {
            formals->PassAll(pass, body);
            body->doPass(pass, body);
        } else {
            formals->PassAll(pass, this);
        }
    }
    // printf("%s\n", body->GetPrintNameForNode());
}
