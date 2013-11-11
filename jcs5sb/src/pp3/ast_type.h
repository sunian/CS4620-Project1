/* File: ast_type.h
 * ----------------
 * In our parse tree, Type nodes are used to represent and
 * store type information. The base Type class is used
 * for built-in types, the NamedType for classes and interfaces,
 * and the ArrayType for arrays of other types.  
 */
 
#ifndef _H_ast_type
#define _H_ast_type

#include "ast.h"
#include "list.h"
#include <string.h>
using namespace std;

class Type : public Node 
{
  protected:
    char *typeName;

  public :
    static Type *intType, *doubleType, *boolType, *voidType,
                *nullType, *stringType, *errorType;

    Type(yyltype loc) : Node(loc) {}
    Type(const char *str);
    
    const char *GetPrintNameForNode() { return "Type"; }
    void PrintChildren(int indentLevel);
    char *getName() { return typeName; }
    virtual bool sameTypeAs(Type *type) {return type->nodeIsOfType("Type") && 0 == strcmp(typeName, type->typeName); }
};

class NamedType : public Type 
{
  protected:
    Identifier *id;
    
  public:
    NamedType(Identifier *i);
    
    const char *GetPrintNameForNode() { return "NamedType"; }
    void PrintChildren(int indentLevel);
    char *getName() { return id->getName(); }
    Identifier *getIdent() {return id;}
    bool sameTypeAs(Type *type) {return type->nodeIsOfType("NamedType") && 0 == strcmp(getName(), ((NamedType*)type)->getName()); }
};

class ArrayType : public Type 
{
  protected:
    Type *elemType;

  public:
    ArrayType(yyltype loc, Type *elemType);
    
    const char *GetPrintNameForNode() { return "ArrayType"; }
    void PrintChildren(int indentLevel);
    Type *getElemType() {return elemType;}
    bool sameTypeAs(Type *type) {return type->nodeIsOfType("ArrayType") && elemType->sameTypeAs(((ArrayType*)type)->elemType); }
};

 
#endif
