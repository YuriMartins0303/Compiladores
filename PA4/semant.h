#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

/*
 * ObjEnv: tabela de símbolos escopada para mapear nomes de identificadores
 * aos seus tipos declarados.
 *
 * SymbolTable<K, V> armazena V* (ponteiros). Como Symbol = Entry*, usamos
 * SymbolTable<Symbol, Entry>:
 *   addid(id_sym, type_sym)   — armazena type_sym como Entry*
 *   lookup(id_sym)            — retorna Entry* (= Symbol)
 */
typedef SymbolTable<Symbol, Entry> ObjEnv;

/* Assinatura de um método: tipos dos parâmetros + tipo de retorno */
struct MethodInfo {
    std::vector<Symbol> param_types;
    Symbol              return_type;
};

class ClassTable {
private:
    int     semant_errors;
    ostream& error_stream;

    /* Tabela principal de classes (nome → nó AST) */
    std::map<Symbol, Class_> class_map;
    /* Grafo de herança (nome → nome do pai) */
    std::map<Symbol, Symbol> parent_map;

    /*
     * Tabelas de métodos/atributos PRÓPRIOS de cada classe
     * (não incluem herança; lookup usa get_method/get_attr_type que sobem a cadeia)
     */
    std::map<Symbol, std::map<Symbol, MethodInfo>> own_methods;
    std::map<Symbol, std::map<Symbol, Symbol>>     own_attrs;

    /* Auxiliares internos */
    bool check_acyclic();
    void build_method_table_for(Symbol cname, std::set<Symbol>& visiting);
    void build_attr_table_for  (Symbol cname);

public:
    ClassTable(Classes);

    int      errors() { return semant_errors; }
    ostream& semant_error();
    ostream& semant_error(Class_ c);
    ostream& semant_error(Symbol filename, tree_node *t);

    void install_basic_classes();

    /* Constrói own_methods e own_attrs — chamar após verificar erros de hierarquia */
    void build_tables();

    /* Consultas */
    bool    class_exists(Symbol name) const;
    Class_  get_class   (Symbol name) const;
    Symbol  get_parent  (Symbol name) const;

    /*
     * is_subtype(T1, T2, C): T1 <= T2 no contexto da classe C
     * (C é necessário para resolver SELF_TYPE)
     */
    bool   is_subtype(Symbol t1, Symbol t2, Symbol curr_class) const;

    /* Menor limitante superior (join/lub) */
    Symbol join(Symbol t1, Symbol t2, Symbol curr_class) const;

    /* Busca de método na cadeia de herança; retorna false se não encontrado */
    bool   get_method   (Symbol cname, Symbol mname, MethodInfo &out) const;
    /* Tipo de atributo na cadeia de herança; retorna NULL se não encontrado */
    Symbol get_attr_type(Symbol cname, Symbol aname) const;

    /* Passa de verificação de tipos em todas as classes do usuário */
    void check_classes();
};

#endif
