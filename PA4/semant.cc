/*
 * semant.cc  —  Analisador Semântico para COOL  (PA4, versão C++)
 *
 * Estrutura geral:
 *   1. Símbolos pré-definidos
 *   2. Implementação do ClassTable (erros, classes básicas, construtor)
 *   3. Verificação de acicidade no grafo de herança
 *   4. Construção das tabelas de métodos/atributos próprios
 *   5. Acessores e utilidades (is_subtype, join)
 *   6. Inferência/verificação de tipos: tc_expr, tc_attr, tc_method
 *   7. check_classes  — passa principal
 *   8. program_class::semant()  — ponto de entrada
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include "semant.h"
#include "utilities.h"

extern int   semant_debug;
extern char *curr_filename;

/* ======================================================================
 * 1. SÍMBOLOS PRÉ-DEFINIDOS
 * ====================================================================== */

static Symbol
    arg, arg2,
    Bool,
    concat,
    cool_abort,
    copy_sym,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name_sym,
    val;

static bool constants_initialized = false;

static void initialize_constants() {
    if (constants_initialized) return;
    constants_initialized = true;

    arg           = idtable.add_string("arg");
    arg2          = idtable.add_string("arg2");
    Bool          = idtable.add_string("Bool");
    concat        = idtable.add_string("concat");
    cool_abort    = idtable.add_string("abort");
    copy_sym      = idtable.add_string("copy");
    Int           = idtable.add_string("Int");
    in_int        = idtable.add_string("in_int");
    in_string     = idtable.add_string("in_string");
    IO            = idtable.add_string("IO");
    length        = idtable.add_string("length");
    Main          = idtable.add_string("Main");
    main_meth     = idtable.add_string("main");
    No_class      = idtable.add_string("_no_class");
    No_type       = idtable.add_string("_no_type");
    Object        = idtable.add_string("Object");
    out_int       = idtable.add_string("out_int");
    out_string    = idtable.add_string("out_string");
    prim_slot     = idtable.add_string("_prim_slot");
    self          = idtable.add_string("self");
    SELF_TYPE     = idtable.add_string("SELF_TYPE");
    Str           = idtable.add_string("String");
    str_field     = idtable.add_string("_str_field");
    substr        = idtable.add_string("substr");
    type_name_sym = idtable.add_string("type_name");
    val           = idtable.add_string("_val");
}

/* ======================================================================
 * 2. RELATÓRIO DE ERROS
 * ====================================================================== */

ostream& ClassTable::semant_error() {
    semant_errors++;
    return error_stream;
}

ostream& ClassTable::semant_error(Class_ c) {
    semant_errors++;
    return error_stream << c->get_filename() << ":" << c->get_line_number() << ": ";
}

ostream& ClassTable::semant_error(Symbol filename, tree_node *t) {
    semant_errors++;
    return error_stream << filename << ":" << t->get_line_number() << ": ";
}

/* ======================================================================
 * 3. CLASSES BÁSICAS
 * ====================================================================== */

void ClassTable::install_basic_classes() {
    Symbol filename = stringtable.add_string("<basic class>");

    /*
     * Object: abort(), type_name() : String, copy() : SELF_TYPE
     */
    Class_ Object_class =
        class_(Object, No_class,
            append_Features(
                append_Features(
                    single_Features(method(cool_abort,    nil_Formals(),                     Object,    no_expr())),
                    single_Features(method(type_name_sym, nil_Formals(),                     Str,       no_expr()))),
                single_Features(method(copy_sym,          nil_Formals(),                     SELF_TYPE, no_expr()))),
            filename);

    /*
     * IO: out_string(String) : SELF_TYPE, out_int(Int) : SELF_TYPE,
     *     in_string() : String, in_int() : Int
     */
    Class_ IO_class =
        class_(IO, Object,
            append_Features(
                append_Features(
                    append_Features(
                        single_Features(method(out_string, single_Formals(formal(arg, Str)), SELF_TYPE, no_expr())),
                        single_Features(method(out_int,    single_Formals(formal(arg, Int)), SELF_TYPE, no_expr()))),
                    single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
                single_Features(method(in_int,    nil_Formals(), Int, no_expr()))),
            filename);

    /* Int: atributo interno _val */
    Class_ Int_class =
        class_(Int, Object,
            single_Features(attr(val, prim_slot, no_expr())),
            filename);

    /* Bool: atributo interno _val */
    Class_ Bool_class =
        class_(Bool, Object,
            single_Features(attr(val, prim_slot, no_expr())),
            filename);

    /*
     * String: length() : Int, concat(String) : String,
     *         substr(Int,Int) : String
     */
    Class_ Str_class =
        class_(Str, Object,
            append_Features(
                append_Features(
                    append_Features(
                        append_Features(
                            single_Features(attr(val,       Int,       no_expr())),
                            single_Features(attr(str_field, prim_slot, no_expr()))),
                        single_Features(method(length, nil_Formals(), Int, no_expr()))),
                    single_Features(method(concat,
                        single_Formals(formal(arg, Str)), Str, no_expr()))),
                single_Features(method(substr,
                    append_Formals(
                        single_Formals(formal(arg,  Int)),
                        single_Formals(formal(arg2, Int))),
                    Str, no_expr()))),
            filename);

    class_map[Object] = Object_class;
    class_map[IO]     = IO_class;
    class_map[Int]    = Int_class;
    class_map[Bool]   = Bool_class;
    class_map[Str]    = Str_class;

    parent_map[Object] = No_class;
    parent_map[IO]     = Object;
    parent_map[Int]    = Object;
    parent_map[Bool]   = Object;
    parent_map[Str]    = Object;
}

/* ======================================================================
 * 4. CONSTRUTOR DO ClassTable
 * ====================================================================== */

static bool is_basic_class(Symbol name) {
    return name == Object || name == IO || name == Int ||
           name == Bool   || name == Str;
}

ClassTable::ClassTable(Classes classes)
    : semant_errors(0), error_stream(cerr)
{
    initialize_constants();
    install_basic_classes();

    /* ---- Registrar classes do usuário ---- */
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        Class_ c    = classes->nth(i);
        Symbol name = c->get_name();
        Symbol par  = c->get_parent();

        if (is_basic_class(name)) {
            semant_error(c) << "Redefinition of basic class " << name << ".\n";
            continue;
        }
        if (name == SELF_TYPE) {
            semant_error(c) << "Redefinition of basic class SELF_TYPE.\n";
            continue;
        }
        if (class_map.count(name)) {
            semant_error(c) << "Class " << name << " was previously defined.\n";
            continue;
        }

        class_map[name]  = c;
        parent_map[name] = par;
    }

    /* ---- Validar parents ---- */
    for (auto &kv : class_map) {
        Symbol name = kv.first;
        if (is_basic_class(name)) continue;

        Symbol par = parent_map[name];

        if (par == Int || par == Bool || par == Str) {
            semant_error(class_map[name])
                << "Class " << name << " cannot inherit class " << par << ".\n";
            parent_map[name] = Object;
            continue;
        }
        if (par == SELF_TYPE) {
            semant_error(class_map[name])
                << "Class " << name << " cannot inherit from SELF_TYPE.\n";
            parent_map[name] = Object;
            continue;
        }
        if (par != No_class && !class_map.count(par)) {
            semant_error(class_map[name])
                << "Class " << name << " inherits from an undefined class " << par << ".\n";
            parent_map[name] = Object;
        }
    }

    /* ---- Verificar acicidade ---- */
    check_acyclic();

    /* ---- Verificar existência de Main com main() ---- */
    if (!class_map.count(Main)) {
        semant_error() << "Class Main is not defined.\n";
    } else {
        Class_   mc    = class_map[Main];
        Features feats = mc->get_features();
        bool found_main = false;
        for (int i = feats->first(); feats->more(i); i = feats->next(i)) {
            method_class *m = dynamic_cast<method_class*>(feats->nth(i));
            if (m && m->get_name() == main_meth) {
                found_main = true;
                if (m->get_formals()->len() != 0)
                    semant_error(mc)
                        << "'main' method in class Main should have no arguments.\n";
                break;
            }
        }
        if (!found_main)
            semant_error(mc) << "No 'main' method in class Main.\n";
    }
}

/* ======================================================================
 * 5. VERIFICAÇÃO DE ACICIDADE
 * ====================================================================== */

bool ClassTable::check_acyclic() {
    bool ok = true;
    for (auto &kv : class_map) {
        Symbol start = kv.first;
        if (is_basic_class(start)) continue;

        std::set<Symbol> visited;
        Symbol c = start;
        while (c != No_class && c != Object) {
            if (!class_map.count(c)) break;
            if (visited.count(c)) {
                semant_error(class_map[start])
                    << "Class " << start
                    << ", or an ancestor of " << start
                    << ", is involved in an inheritance cycle.\n";
                ok = false;
                /* Corte para evitar recursão infinita em build_tables */
                parent_map[start] = Object;
                break;
            }
            visited.insert(c);
            c = parent_map.count(c) ? parent_map[c] : No_class;
        }
    }
    return ok;
}

/* ======================================================================
 * 6. CONSTRUÇÃO DAS TABELAS (métodos e atributos PRÓPRIOS)
 * ====================================================================== */

void ClassTable::build_method_table_for(Symbol cname, std::set<Symbol>& visiting) {
    if (own_methods.count(cname)) return;
    if (visiting.count(cname))   return;   /* segurança contra ciclo residual */

    visiting.insert(cname);
    own_methods[cname] = {};               /* marca como em construção */

    /* Herdar métodos do pai */
    Symbol par = parent_map.count(cname) ? parent_map[cname] : No_class;
    if (par != No_class && class_map.count(par)) {
        build_method_table_for(par, visiting);
        /* own_methods[cname] parte com os métodos herdados */
        own_methods[cname] = own_methods[par];
    }

    /* Sobrescrever/adicionar métodos próprios */
    if (!class_map.count(cname)) return;
    Features feats = class_map[cname]->get_features();
    for (int i = feats->first(); feats->more(i); i = feats->next(i)) {
        method_class *m = dynamic_cast<method_class*>(feats->nth(i));
        if (!m) continue;

        MethodInfo info;
        info.return_type = m->get_return_type();
        Formals fms = m->get_formals();
        for (int j = fms->first(); fms->more(j); j = fms->next(j)) {
            formal_class *fm = (formal_class*)fms->nth(j);
            info.param_types.push_back(fm->get_type_decl());
        }
        own_methods[cname][m->get_name()] = info;
    }
    /* Remover da lista de visitação para permitir acesso ao resultado */
    visiting.erase(cname);
}

void ClassTable::build_attr_table_for(Symbol cname) {
    if (own_attrs.count(cname)) return;
    own_attrs[cname] = {};

    if (!class_map.count(cname)) return;
    Features feats = class_map[cname]->get_features();
    for (int i = feats->first(); feats->more(i); i = feats->next(i)) {
        attr_class *a = dynamic_cast<attr_class*>(feats->nth(i));
        if (!a) continue;
        own_attrs[cname][a->get_name()] = a->get_type_decl();
    }
}

void ClassTable::build_tables() {
    std::set<Symbol> visiting;
    for (auto &kv : class_map) {
        build_method_table_for(kv.first, visiting);
        build_attr_table_for  (kv.first);
    }
}

/* ======================================================================
 * 7. ACESSORES E UTILIDADES
 * ====================================================================== */

bool ClassTable::class_exists(Symbol name) const {
    if (name == SELF_TYPE) return true;
    return class_map.count(name) > 0;
}

Class_ ClassTable::get_class(Symbol name) const {
    auto it = class_map.find(name);
    return (it == class_map.end()) ? (Class_)NULL : it->second;
}

Symbol ClassTable::get_parent(Symbol name) const {
    auto it = parent_map.find(name);
    return (it == parent_map.end()) ? No_class : it->second;
}

bool ClassTable::get_method(Symbol cname, Symbol mname, MethodInfo &out) const {
    /* Busca na tabela pré-construída (que já incluiu herança via recursão) */
    auto ct = own_methods.find(cname);
    if (ct != own_methods.end()) {
        auto mt = ct->second.find(mname);
        if (mt != ct->second.end()) { out = mt->second; return true; }
    }
    /* Fallback: subir na cadeia manualmente */
    Symbol c = cname;
    while (c != No_class) {
        auto pt = parent_map.find(c);
        if (pt == parent_map.end()) break;
        c = pt->second;
        auto ct2 = own_methods.find(c);
        if (ct2 != own_methods.end()) {
            auto mt2 = ct2->second.find(mname);
            if (mt2 != ct2->second.end()) { out = mt2->second; return true; }
        }
    }
    return false;
}

Symbol ClassTable::get_attr_type(Symbol cname, Symbol aname) const {
    Symbol c = cname;
    while (c != No_class) {
        auto at = own_attrs.find(c);
        if (at != own_attrs.end()) {
            auto ait = at->second.find(aname);
            if (ait != at->second.end()) return ait->second;
        }
        auto pt = parent_map.find(c);
        if (pt == parent_map.end()) break;
        c = pt->second;
    }
    return (Symbol)NULL;
}

/* ---- is_subtype: T1 <= T2 no contexto da classe curr_class ---- */
bool ClassTable::is_subtype(Symbol t1, Symbol t2, Symbol curr_class) const {
    if (t1 == No_type) return true;   /* No_type é o tipo bottom */
    if (t1 == t2)      return true;

    /* Resolver SELF_TYPE */
    if (t1 == SELF_TYPE && t2 == SELF_TYPE) return true;
    if (t1 == SELF_TYPE)
        return is_subtype(curr_class, t2, curr_class);
    if (t2 == SELF_TYPE)
        return false;   /* T <= SELF_TYPE_C  só se T == SELF_TYPE_C */

    /* Subir na hierarquia de t1 até encontrar t2 */
    Symbol c = t1;
    while (c != No_class) {
        if (c == t2) return true;
        auto pt = parent_map.find(c);
        if (pt == parent_map.end()) break;
        c = pt->second;
    }
    return false;
}

/* ---- join: menor limitante superior ---- */
Symbol ClassTable::join(Symbol t1, Symbol t2, Symbol curr_class) const {
    if (t1 == No_type) return t2;
    if (t2 == No_type) return t1;
    if (t1 == t2)      return t1;   /* inclui SELF_TYPE == SELF_TYPE */

    Symbol s1 = (t1 == SELF_TYPE) ? curr_class : t1;
    Symbol s2 = (t2 == SELF_TYPE) ? curr_class : t2;
    if (s1 == s2) return s1;

    /* Coleta ancestrais de s1 */
    std::set<Symbol> anc1;
    Symbol c = s1;
    while (c != No_class) {
        anc1.insert(c);
        auto pt = parent_map.find(c);
        if (pt == parent_map.end()) break;
        c = pt->second;
    }

    /* Sobe s2 até achar um ancestral comum */
    c = s2;
    while (c != No_class) {
        if (anc1.count(c)) return c;
        auto pt = parent_map.find(c);
        if (pt == parent_map.end()) break;
        c = pt->second;
    }
    return Object;
}

/* ======================================================================
 * 8. INFERÊNCIA DE TIPOS: tc_expr
 * ====================================================================== */

/* Verifica se o tipo t é um tipo válido (classe existente ou SELF_TYPE) */
static bool valid_type(Symbol t, ClassTableP ct) {
    return t == SELF_TYPE || ct->class_exists(t);
}

/* Retorna o nome de arquivo da classe curr_class para mensagens de erro */
static Symbol class_filename(ClassTableP ct, Symbol curr_class) {
    Class_ c = ct->get_class(curr_class);
    if (!c) return stringtable.add_string("<unknown>");
    return c->get_filename();
}

/*
 * tc_expr: infere o tipo de uma expressão, anota expr->type e retorna o tipo.
 *
 * Parâmetros:
 *   expr       — nó de expressão da AST
 *   ct         — tabela de classes
 *   curr_class — classe onde a expressão aparece (para resolver SELF_TYPE)
 *   env        — ambiente de objetos (escopado, id → tipo)
 */
static Symbol tc_expr(Expression expr, ClassTableP ct,
                      Symbol curr_class, ObjEnv &env);

/* ---- no_expr ---- */
static Symbol tc_no_expr(no_expr_class* /*e*/, Expression expr,
                         ClassTableP /*ct*/, Symbol /*cc*/, ObjEnv& /*env*/) {
    expr->set_type(No_type);
    return No_type;
}

/* ---- Constantes ---- */
static Symbol tc_int_const(int_const_class* /*e*/, Expression expr,
                            ClassTableP /*ct*/, Symbol /*cc*/, ObjEnv& /*env*/) {
    expr->set_type(Int); return Int;
}
static Symbol tc_bool_const(bool_const_class* /*e*/, Expression expr,
                             ClassTableP /*ct*/, Symbol /*cc*/, ObjEnv& /*env*/) {
    expr->set_type(Bool); return Bool;
}
static Symbol tc_string_const(string_const_class* /*e*/, Expression expr,
                               ClassTableP /*ct*/, Symbol /*cc*/, ObjEnv& /*env*/) {
    expr->set_type(Str); return Str;
}

/* ---- object(id) ---- */
static Symbol tc_object(object_class *e, Expression expr,
                        ClassTableP ct, Symbol cc, ObjEnv &env) {
    Symbol name = e->get_name();
    Symbol t;
    if (name == self) {
        t = SELF_TYPE;
    } else {
        Symbol *tp = env.lookup(name);
        if (!tp) {
            ct->semant_error(class_filename(ct, cc), expr)
                << "Undeclared identifier " << name << ".\n";
            t = Object;
        } else {
            t = *tp;
        }
    }
    expr->set_type(t);
    return t;
}

/* ---- assign(id, e) ---- */
static Symbol tc_assign(assign_class *e, Expression expr,
                        ClassTableP ct, Symbol cc, ObjEnv &env) {
    Symbol name     = e->get_name();
    Symbol rhs_type = tc_expr(e->get_expr(), ct, cc, env);

    if (name == self) {
        ct->semant_error(class_filename(ct, cc), expr)
            << "Cannot assign to 'self'.\n";
        expr->set_type(rhs_type);
        return rhs_type;
    }
    Symbol *ltp = env.lookup(name);
    if (!ltp) {
        ct->semant_error(class_filename(ct, cc), expr)
            << "Assignment to undeclared variable " << name << ".\n";
        expr->set_type(rhs_type);
        return rhs_type;
    }
    Symbol lhs_type = *ltp;
    if (!ct->is_subtype(rhs_type, lhs_type, cc)) {
        ct->semant_error(class_filename(ct, cc), expr)
            << "Type " << rhs_type
            << " of assigned expression does not conform to declared type "
            << lhs_type << " of identifier " << name << ".\n";
    }
    expr->set_type(rhs_type);
    return rhs_type;
}

/* ---- new_(T) ---- */
static Symbol tc_new(new__class *e, Expression expr,
                     ClassTableP ct, Symbol cc, ObjEnv& /*env*/) {
    Symbol t = e->get_type_name();
    Symbol res;
    if (t == SELF_TYPE) {
        res = SELF_TYPE;
    } else if (!ct->class_exists(t)) {
        ct->semant_error(class_filename(ct, cc), expr)
            << "'" << t << "' used in 'new' but is not a defined class.\n";
        res = Object;
    } else {
        res = t;
    }
    expr->set_type(res);
    return res;
}

/* ---- isvoid(e) ---- */
static Symbol tc_isvoid(isvoid_class *e, Expression expr,
                        ClassTableP ct, Symbol cc, ObjEnv &env) {
    tc_expr(e->get_e1(), ct, cc, env);
    expr->set_type(Bool);
    return Bool;
}

/* ---- ~e (neg. inteiro) ---- */
static Symbol tc_neg(neg_class *e, Expression expr,
                     ClassTableP ct, Symbol cc, ObjEnv &env) {
    Symbol t = tc_expr(e->get_e1(), ct, cc, env);
    if (t != Int)
        ct->semant_error(class_filename(ct, cc), expr)
            << "Argument of '~' has type " << t << " instead of Int.\n";
    expr->set_type(Int);
    return Int;
}

/* ---- not e (compl. lógico) ---- */
static Symbol tc_comp(comp_class *e, Expression expr,
                      ClassTableP ct, Symbol cc, ObjEnv &env) {
    Symbol t = tc_expr(e->get_e1(), ct, cc, env);
    if (t != Bool)
        ct->semant_error(class_filename(ct, cc), expr)
            << "Argument of 'not' has type " << t << " instead of Bool.\n";
    expr->set_type(Bool);
    return Bool;
}

/* ---- operadores aritméticos binários ---- */
static Symbol tc_arith(Expression e1_node, Expression e2_node,
                       const char *op_str, Expression expr,
                       ClassTableP ct, Symbol cc, ObjEnv &env) {
    Symbol t1 = tc_expr(e1_node, ct, cc, env);
    Symbol t2 = tc_expr(e2_node, ct, cc, env);
    if (t1 != Int || t2 != Int)
        ct->semant_error(class_filename(ct, cc), expr)
            << "non-Int arguments: " << t1 << " " << op_str << " " << t2 << "\n";
    expr->set_type(Int);
    return Int;
}

/* ---- operadores de comparação < e <= ---- */
static Symbol tc_cmp_int(Expression e1_node, Expression e2_node,
                         const char *op_str, Expression expr,
                         ClassTableP ct, Symbol cc, ObjEnv &env) {
    Symbol t1 = tc_expr(e1_node, ct, cc, env);
    Symbol t2 = tc_expr(e2_node, ct, cc, env);
    if (t1 != Int || t2 != Int)
        ct->semant_error(class_filename(ct, cc), expr)
            << "non-Int arguments: " << t1 << " " << op_str << " " << t2 << "\n";
    expr->set_type(Bool);
    return Bool;
}

/* ---- = (igualdade) ---- */
static Symbol tc_eq(eq_class *e, Expression expr,
                    ClassTableP ct, Symbol cc, ObjEnv &env) {
    Symbol t1 = tc_expr(e->get_e1(), ct, cc, env);
    Symbol t2 = tc_expr(e->get_e2(), ct, cc, env);
    /* Int, Bool e String só podem ser comparados com o mesmo tipo */
    if ((t1 == Int || t1 == Bool || t1 == Str ||
         t2 == Int || t2 == Bool || t2 == Str) && t1 != t2)
        ct->semant_error(class_filename(ct, cc), expr)
            << "Illegal comparison with a basic type.\n";
    expr->set_type(Bool);
    return Bool;
}

/* ---- if pred then t else e fi ---- */
static Symbol tc_cond(cond_class *e, Expression expr,
                      ClassTableP ct, Symbol cc, ObjEnv &env) {
    Symbol pred_t = tc_expr(e->get_pred(),     ct, cc, env);
    Symbol then_t = tc_expr(e->get_then_exp(), ct, cc, env);
    Symbol else_t = tc_expr(e->get_else_exp(), ct, cc, env);
    if (pred_t != Bool)
        ct->semant_error(class_filename(ct, cc), expr)
            << "Predicate of 'if' does not have type Bool.\n";
    Symbol res = ct->join(then_t, else_t, cc);
    expr->set_type(res);
    return res;
}

/* ---- while pred loop body pool ---- */
static Symbol tc_loop(loop_class *e, Expression expr,
                      ClassTableP ct, Symbol cc, ObjEnv &env) {
    Symbol pred_t = tc_expr(e->get_pred(), ct, cc, env);
    if (pred_t != Bool)
        ct->semant_error(class_filename(ct, cc), expr)
            << "Loop condition does not have type Bool.\n";
    tc_expr(e->get_body(), ct, cc, env);
    expr->set_type(Object);
    return Object;
}

/* ---- { e1; e2; ... ; en; } ---- */
static Symbol tc_block(block_class *e, Expression expr,
                       ClassTableP ct, Symbol cc, ObjEnv &env) {
    Expressions body  = e->get_body();
    Symbol      last  = Object;
    for (int i = body->first(); body->more(i); i = body->next(i))
        last = tc_expr(body->nth(i), ct, cc, env);
    expr->set_type(last);
    return last;
}

/* ---- let id:T [<- init] in body ---- */
static Symbol tc_let(let_class *e, Expression expr,
                     ClassTableP ct, Symbol cc, ObjEnv &env) {
    Symbol id        = e->get_identifier();
    Symbol decl_type = e->get_type_decl();
    Expression init  = e->get_init();
    Expression body  = e->get_body();

    if (id == self)
        ct->semant_error(class_filename(ct, cc), expr)
            << "'self' cannot be bound in a 'let' expression.\n";

    if (!valid_type(decl_type, ct)) {
        ct->semant_error(class_filename(ct, cc), expr)
            << "Class " << decl_type << " of let-bound identifier "
            << id << " is undefined.\n";
        decl_type = Object;
    }

    /* Verificar tipo da inicialização (se houver) */
    if (!dynamic_cast<no_expr_class*>(init)) {
        Symbol init_t = tc_expr(init, ct, cc, env);
        if (!ct->is_subtype(init_t, decl_type, cc))
            ct->semant_error(class_filename(ct, cc), expr)
                << "Inferred type " << init_t
                << " of initialization of " << id
                << " does not conform to identifier's declared type "
                << decl_type << ".\n";
    }

    env.enterscope();
    env.addid(id, decl_type);
    Symbol res = tc_expr(body, ct, cc, env);
    env.exitscope();

    expr->set_type(res);
    return res;
}

/* ---- case e of xi:Ti => ei esac ---- */
static Symbol tc_typcase(typcase_class *e, Expression expr,
                         ClassTableP ct, Symbol cc, ObjEnv &env) {
    tc_expr(e->get_expr(), ct, cc, env);

    Cases  cases  = e->get_cases();
    Symbol lub    = No_type;
    std::set<Symbol> seen_types;

    for (int i = cases->first(); cases->more(i); i = cases->next(i)) {
        branch_class *b     = (branch_class*)cases->nth(i);
        Symbol        bname = b->get_name();
        Symbol        btype = b->get_type_decl();

        if (bname == self)
            ct->semant_error(class_filename(ct, cc), expr)
                << "'self' bound in 'case'.\n";
        if (btype == SELF_TYPE)
            ct->semant_error(class_filename(ct, cc), expr)
                << "Identifier " << bname
                << " declared with type SELF_TYPE in case branch.\n";
        if (!ct->class_exists(btype)) {
            ct->semant_error(class_filename(ct, cc), expr)
                << "Class " << btype << " of case branch is undefined.\n";
            btype = Object;
        }
        if (seen_types.count(btype))
            ct->semant_error(class_filename(ct, cc), expr)
                << "Duplicate branch " << btype << " in case statement.\n";
        seen_types.insert(btype);

        env.enterscope();
        env.addid(bname, btype);
        Symbol branch_t = tc_expr(b->get_expr(), ct, cc, env);
        env.exitscope();

        lub = ct->join(lub, branch_t, cc);
    }
    expr->set_type(lub);
    return lub;
}

/* ---- e.f(args)  e OBJECTID(args) ---- */
static Symbol tc_dispatch(dispatch_class *e, Expression expr,
                          ClassTableP ct, Symbol cc, ObjEnv &env) {
    Symbol recv_t = tc_expr(e->get_expr(), ct, cc, env);
    Symbol mname  = e->get_name();
    Expressions actuals = e->get_actual();

    /* Classe real para busca do método */
    Symbol dispatch_cls = (recv_t == SELF_TYPE) ? cc : recv_t;

    MethodInfo minfo;
    if (!ct->get_method(dispatch_cls, mname, minfo)) {
        ct->semant_error(class_filename(ct, cc), expr)
            << "Dispatch to undefined method " << mname << ".\n";
        expr->set_type(Object);
        /* Ainda type-check os argumentos para evitar erros em cascata */
        for (int i = actuals->first(); actuals->more(i); i = actuals->next(i))
            tc_expr(actuals->nth(i), ct, cc, env);
        return Object;
    }

    int nact   = actuals->len();
    int npar   = (int)minfo.param_types.size();
    if (nact != npar) {
        ct->semant_error(class_filename(ct, cc), expr)
            << "Method " << mname << " called with wrong number of arguments.\n";
        for (int i = actuals->first(); actuals->more(i); i = actuals->next(i))
            tc_expr(actuals->nth(i), ct, cc, env);
    } else {
        for (int j = 0; j < nact; j++) {
            Symbol arg_t   = tc_expr(actuals->nth(j), ct, cc, env);
            Symbol param_t = minfo.param_types[j];
            if (!ct->is_subtype(arg_t, param_t, cc))
                ct->semant_error(class_filename(ct, cc), expr)
                    << "In call of method " << mname
                    << ", type " << arg_t
                    << " of parameter #" << (j+1)
                    << " does not conform to declared type "
                    << param_t << ".\n";
        }
    }

    /* Tipo de retorno: se SELF_TYPE, propagar o tipo do receptor */
    Symbol res = (minfo.return_type == SELF_TYPE) ? recv_t : minfo.return_type;
    expr->set_type(res);
    return res;
}

/* ---- e@T.f(args) ---- */
static Symbol tc_static_dispatch(static_dispatch_class *e, Expression expr,
                                  ClassTableP ct, Symbol cc, ObjEnv &env) {
    Symbol recv_t  = tc_expr(e->get_expr(), ct, cc, env);
    Symbol stype   = e->get_type_name();
    Symbol mname   = e->get_name();
    Expressions actuals = e->get_actual();

    if (stype == SELF_TYPE) {
        ct->semant_error(class_filename(ct, cc), expr)
            << "Static dispatch to type SELF_TYPE.\n";
        expr->set_type(Object);
        for (int i = actuals->first(); actuals->more(i); i = actuals->next(i))
            tc_expr(actuals->nth(i), ct, cc, env);
        return Object;
    }
    if (!ct->class_exists(stype)) {
        ct->semant_error(class_filename(ct, cc), expr)
            << "Static dispatch to undefined class " << stype << ".\n";
        expr->set_type(Object);
        for (int i = actuals->first(); actuals->more(i); i = actuals->next(i))
            tc_expr(actuals->nth(i), ct, cc, env);
        return Object;
    }
    if (!ct->is_subtype(recv_t, stype, cc)) {
        ct->semant_error(class_filename(ct, cc), expr)
            << "Expression type " << recv_t
            << " does not conform to declared static dispatch type "
            << stype << ".\n";
    }

    MethodInfo minfo;
    if (!ct->get_method(stype, mname, minfo)) {
        ct->semant_error(class_filename(ct, cc), expr)
            << "Static dispatch to undefined method " << mname << ".\n";
        expr->set_type(Object);
        for (int i = actuals->first(); actuals->more(i); i = actuals->next(i))
            tc_expr(actuals->nth(i), ct, cc, env);
        return Object;
    }

    int nact = actuals->len();
    int npar = (int)minfo.param_types.size();
    if (nact != npar) {
        ct->semant_error(class_filename(ct, cc), expr)
            << "Method " << mname << " called with wrong number of arguments.\n";
        for (int i = actuals->first(); actuals->more(i); i = actuals->next(i))
            tc_expr(actuals->nth(i), ct, cc, env);
    } else {
        for (int j = 0; j < nact; j++) {
            Symbol arg_t   = tc_expr(actuals->nth(j), ct, cc, env);
            Symbol param_t = minfo.param_types[j];
            if (!ct->is_subtype(arg_t, param_t, cc))
                ct->semant_error(class_filename(ct, cc), expr)
                    << "In call of method " << mname
                    << ", type " << arg_t
                    << " of parameter #" << (j+1)
                    << " does not conform to declared type "
                    << param_t << ".\n";
        }
    }

    Symbol res = (minfo.return_type == SELF_TYPE) ? recv_t : minfo.return_type;
    expr->set_type(res);
    return res;
}

/* ---- Dispatcher principal ---- */
static Symbol tc_expr(Expression expr, ClassTableP ct,
                      Symbol curr_class, ObjEnv &env)
{
    if (no_expr_class          *e = dynamic_cast<no_expr_class*>          (expr)) return tc_no_expr    (e, expr, ct, curr_class, env);
    if (int_const_class        *e = dynamic_cast<int_const_class*>        (expr)) return tc_int_const  (e, expr, ct, curr_class, env);
    if (bool_const_class       *e = dynamic_cast<bool_const_class*>       (expr)) return tc_bool_const (e, expr, ct, curr_class, env);
    if (string_const_class     *e = dynamic_cast<string_const_class*>     (expr)) return tc_string_const(e, expr, ct, curr_class, env);
    if (object_class           *e = dynamic_cast<object_class*>           (expr)) return tc_object     (e, expr, ct, curr_class, env);
    if (assign_class           *e = dynamic_cast<assign_class*>           (expr)) return tc_assign     (e, expr, ct, curr_class, env);
    if (new__class             *e = dynamic_cast<new__class*>             (expr)) return tc_new        (e, expr, ct, curr_class, env);
    if (isvoid_class           *e = dynamic_cast<isvoid_class*>           (expr)) return tc_isvoid     (e, expr, ct, curr_class, env);
    if (neg_class              *e = dynamic_cast<neg_class*>              (expr)) return tc_neg        (e, expr, ct, curr_class, env);
    if (comp_class             *e = dynamic_cast<comp_class*>             (expr)) return tc_comp       (e, expr, ct, curr_class, env);
    if (plus_class             *e = dynamic_cast<plus_class*>             (expr)) return tc_arith(e->get_e1(), e->get_e2(), "+", expr, ct, curr_class, env);
    if (sub_class              *e = dynamic_cast<sub_class*>              (expr)) return tc_arith(e->get_e1(), e->get_e2(), "-", expr, ct, curr_class, env);
    if (mul_class              *e = dynamic_cast<mul_class*>              (expr)) return tc_arith(e->get_e1(), e->get_e2(), "*", expr, ct, curr_class, env);
    if (divide_class           *e = dynamic_cast<divide_class*>           (expr)) return tc_arith(e->get_e1(), e->get_e2(), "/", expr, ct, curr_class, env);
    if (lt_class               *e = dynamic_cast<lt_class*>               (expr)) return tc_cmp_int(e->get_e1(), e->get_e2(), "<",  expr, ct, curr_class, env);
    if (leq_class              *e = dynamic_cast<leq_class*>              (expr)) return tc_cmp_int(e->get_e1(), e->get_e2(), "<=", expr, ct, curr_class, env);
    if (eq_class               *e = dynamic_cast<eq_class*>               (expr)) return tc_eq         (e, expr, ct, curr_class, env);
    if (cond_class             *e = dynamic_cast<cond_class*>             (expr)) return tc_cond       (e, expr, ct, curr_class, env);
    if (loop_class             *e = dynamic_cast<loop_class*>             (expr)) return tc_loop       (e, expr, ct, curr_class, env);
    if (block_class            *e = dynamic_cast<block_class*>            (expr)) return tc_block      (e, expr, ct, curr_class, env);
    if (let_class              *e = dynamic_cast<let_class*>              (expr)) return tc_let        (e, expr, ct, curr_class, env);
    if (typcase_class          *e = dynamic_cast<typcase_class*>          (expr)) return tc_typcase    (e, expr, ct, curr_class, env);
    if (dispatch_class         *e = dynamic_cast<dispatch_class*>         (expr)) return tc_dispatch   (e, expr, ct, curr_class, env);
    if (static_dispatch_class  *e = dynamic_cast<static_dispatch_class*>  (expr)) return tc_static_dispatch(e, expr, ct, curr_class, env);

    /* Não deveria chegar aqui */
    expr->set_type(Object);
    return Object;
}

/* ======================================================================
 * 9. VERIFICAÇÃO DE FEATURES
 * ====================================================================== */

/* ---- Atributo ---- */
static void tc_attr(attr_class *a, ClassTableP ct,
                    Symbol curr_class, ObjEnv &env,
                    Symbol cls_filename) {
    Symbol name      = a->get_name();
    Symbol decl_type = a->get_type_decl();
    Expression init  = a->get_init();

    if (name == self) {
        ct->semant_error(cls_filename, a)
            << "'self' cannot be the name of an attribute.\n";
        return;
    }
    if (!valid_type(decl_type, ct)) {
        ct->semant_error(cls_filename, a)
            << "Class " << decl_type << " of attribute "
            << name << " is undefined.\n";
        /* Recuperação: tratar como Object */
        decl_type = Object;
    }

    if (!dynamic_cast<no_expr_class*>(init)) {
        Symbol init_t = tc_expr(init, ct, curr_class, env);
        if (!ct->is_subtype(init_t, decl_type, curr_class))
            ct->semant_error(cls_filename, a)
                << "Inferred type " << init_t
                << " of initialization of attribute " << name
                << " does not conform to declared type " << decl_type << ".\n";
    }
}

/* ---- Método ---- */
static void tc_method(method_class *m, ClassTableP ct,
                      Symbol curr_class, ObjEnv &env,
                      Symbol cls_filename) {
    Symbol mname       = m->get_name();
    Symbol return_type = m->get_return_type();
    Formals formals    = m->get_formals();
    Expression body    = m->get_expr();

    /* Tipo de retorno deve existir */
    if (!valid_type(return_type, ct))
        ct->semant_error(cls_filename, m)
            << "Undefined return type " << return_type
            << " in method " << mname << ".\n";

    /* Verificação de override: assinatura deve ser idêntica à da superclasse */
    Symbol parent = ct->get_parent(curr_class);
    if (parent != No_class) {
        MethodInfo pinfo;
        if (ct->get_method(parent, mname, pinfo)) {
            int nf = formals->len();
            if (nf != (int)pinfo.param_types.size()) {
                ct->semant_error(cls_filename, m)
                    << "Incompatible number of formal parameters in redefined method "
                    << mname << ".\n";
            } else {
                int j = 0;
                for (int i = formals->first(); formals->more(i);
                     i = formals->next(i), j++) {
                    formal_class *fm = (formal_class*)formals->nth(i);
                    if (fm->get_type_decl() != pinfo.param_types[j])
                        ct->semant_error(cls_filename, m)
                            << "In redefined method " << mname
                            << ", parameter type " << fm->get_type_decl()
                            << " is different from original type "
                            << pinfo.param_types[j] << ".\n";
                }
            }
            if (return_type != pinfo.return_type)
                ct->semant_error(cls_filename, m)
                    << "In redefined method " << mname
                    << ", return type " << return_type
                    << " is different from original return type "
                    << pinfo.return_type << ".\n";
        }
    }

    /* Escopo local para os parâmetros formais */
    env.enterscope();
    std::set<Symbol> seen;
    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
        formal_class *fm    = (formal_class*)formals->nth(i);
        Symbol        fname = fm->get_name();
        Symbol        ftype = fm->get_type_decl();

        if (fname == self) {
            ct->semant_error(cls_filename, m)
                << "'self' cannot be the name of a formal parameter.\n";
            continue;
        }
        if (ftype == SELF_TYPE)
            ct->semant_error(cls_filename, m)
                << "Formal parameter " << fname
                << " cannot have type SELF_TYPE.\n";
        if (!valid_type(ftype, ct))
            ct->semant_error(cls_filename, m)
                << "Class " << ftype << " of formal parameter "
                << fname << " is undefined.\n";
        if (seen.count(fname)) {
            ct->semant_error(cls_filename, m)
                << "Formal parameter " << fname << " is multiply defined.\n";
            continue;
        }
        seen.insert(fname);
        env.addid(fname, ftype);
    }

    /* Verificar corpo */
    Symbol body_t = tc_expr(body, ct, curr_class, env);
    if (!ct->is_subtype(body_t, return_type, curr_class))
        ct->semant_error(cls_filename, m)
            << "Inferred return type " << body_t << " of method " << mname
            << " does not conform to declared return type "
            << return_type << ".\n";

    env.exitscope();
}

/* ======================================================================
 * 10. PASSE PRINCIPAL: check_classes
 * ====================================================================== */

void ClassTable::check_classes() {
    for (auto &kv : class_map) {
        Symbol cname = kv.first;
        Class_ cls   = kv.second;

        /* Pular classes básicas — já são corretas por construção */
        if (is_basic_class(cname)) continue;

        Symbol cls_fn = cls->get_filename();
        Features feats = cls->get_features();

        /* ---- Verificar duplicatas e restrições de atributos ---- */
        std::set<Symbol> own_attr_names;
        std::set<Symbol> own_meth_names;

        for (int i = feats->first(); feats->more(i); i = feats->next(i)) {
            Feature f = feats->nth(i);

            if (attr_class *a = dynamic_cast<attr_class*>(f)) {
                Symbol aname = a->get_name();
                if (aname == self) continue; /* erro reportado em tc_attr */

                /* Atributo não pode ser herdado (COOL proíbe redefini-lo) */
                Symbol par = get_parent(cname);
                while (par != No_class) {
                    if (own_attrs.count(par) && own_attrs[par].count(aname)) {
                        semant_error(cls_fn, f)
                            << "Attribute " << aname
                            << " is an attribute of an inherited class.\n";
                        break;
                    }
                    par = get_parent(par);
                }
                if (own_attr_names.count(aname))
                    semant_error(cls_fn, f)
                        << "Attribute " << aname
                        << " is multiply defined in class.\n";
                own_attr_names.insert(aname);

            } else if (method_class *m = dynamic_cast<method_class*>(f)) {
                Symbol mname = m->get_name();
                if (own_meth_names.count(mname))
                    semant_error(cls_fn, f)
                        << "Method " << mname
                        << " is multiply defined.\n";
                own_meth_names.insert(mname);
            }
        }

        /* ---- Construir ObjEnv com self + todos os atributos visíveis ---- */
        ObjEnv env;
        env.enterscope();
        env.addid(self, SELF_TYPE);

        /*
         * Percorrer a cadeia de ancestrais (do mais antigo para cname)
         * e adicionar os atributos próprios de cada um.
         * Atributos da subclasse ficam sobre os do pai → sombreamento
         * correto em caso de redefinição (que já foi reportada acima).
         */
        std::vector<Symbol> ancestry;
        for (Symbol c = cname; c != No_class; c = get_parent(c))
            ancestry.push_back(c);
        std::reverse(ancestry.begin(), ancestry.end());

        for (Symbol anc : ancestry) {
            auto it = own_attrs.find(anc);
            if (it == own_attrs.end()) continue;
            for (auto &av : it->second)
                env.addid(av.first, av.second);
        }

        /* ---- Type-check de cada feature ---- */
        for (int i = feats->first(); feats->more(i); i = feats->next(i)) {
            Feature f = feats->nth(i);
            if (attr_class   *a = dynamic_cast<attr_class*>  (f))
                tc_attr  (a, this, cname, env, cls_fn);
            else if (method_class *m = dynamic_cast<method_class*>(f))
                tc_method(m, this, cname, env, cls_fn);
        }

        env.exitscope();
    }
}

/* ======================================================================
 * 11. PONTO DE ENTRADA
 * ====================================================================== */

void program_class::semant() {
    initialize_constants();

    /* Fase 1: construir e validar hierarquia de classes */
    ClassTable *ct = new ClassTable(classes);

    if (ct->errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }

    /* Fase 2: construir tabelas de métodos e atributos */
    ct->build_tables();

    /* Fase 3: verificar tipos em todas as classes */
    ct->check_classes();

    if (ct->errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }

    delete ct;
}
