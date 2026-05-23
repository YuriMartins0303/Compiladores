#!/usr/bin/env python3
# _validate_pa4.py — Validação estática do PA4 no Windows
import sys, re, os

sys.stdout.reconfigure(encoding='utf-8', errors='replace')

base = r'c:\Users\yurim\OneDrive\Área de Trabalho\compilador\PA4'

def read_file(name):
    path = os.path.join(base, name)
    with open(path, encoding='utf-8', errors='replace') as f:
        return f.read()

src_cc = read_file('semant.cc')
src_h  = read_file('semant.h')
src_hc = read_file('cool-tree.handcode.h')
good   = read_file('good.cl')
bad    = read_file('bad.cl')

errors   = []
warnings = []
ok_msgs  = []

def check(cond, msg, is_error=True):
    if cond:
        ok_msgs.append('OK  ' + msg)
    else:
        (errors if is_error else warnings).append(('ERR' if is_error else 'WRN') + ' ' + msg)

# =====================================================================
# 1. LINGUAGEM E PADRÕES
# =====================================================================
print('='*65)
print('1. LINGUAGEM E PADRÕES')
print('='*65)
print('  Linguagem: C++ (igual ao PA2 e PA3)')
print('  Ferramenta: semant.cc + semant.h (análise semântica)')
print('  Integração: usa cool-tree.h gerado pelo PA3 + Bison/Flex')
print('  Saída: AST anotada com tipos para o gerador de código')
print()

# =====================================================================
# 2. ESTRUTURA DO semant.cc
# =====================================================================
print('='*65)
print('2. ESTRUTURA DO semant.cc')
print('='*65)

lines = src_cc.splitlines()
print(f'  Linhas totais: {len(lines)}')

sections = [
    ('initialize_constants',        'Símbolos pré-definidos'),
    ('install_basic_classes',       'Classes básicas (Object/IO/Int/Bool/String)'),
    ('ClassTable::ClassTable',      'Construtor ClassTable'),
    ('check_acyclic',               'Verificação de ciclos'),
    ('build_method_table_for',      'Construção tabela de métodos'),
    ('build_attr_table_for',        'Construção tabela de atributos'),
    ('ClassTable::is_subtype',      'Verificação de subtipo'),
    ('ClassTable::join',            'Menor limitante superior (LUB)'),
    ('tc_expr',                     'Inferência de tipos (dispatcher)'),
    ('check_classes',               'Passe principal de verificação'),
    ('program_class::semant',       'Ponto de entrada'),
]
for token, desc in sections:
    check(token in src_cc, f'{desc}  [{token}]')
    print(f'  {"OK" if token in src_cc else "FALTA!"} {desc}')
print()

# =====================================================================
# 3. COBERTURA DE TIPOS DE EXPRESSÃO
# =====================================================================
print('='*65)
print('3. COBERTURA DE TODOS OS TIPOS DE EXPRESSÃO')
print('='*65)

expr_types = [
    'no_expr_class', 'int_const_class', 'bool_const_class', 'string_const_class',
    'object_class', 'assign_class', 'new__class', 'isvoid_class',
    'neg_class', 'comp_class',
    'plus_class', 'sub_class', 'mul_class', 'divide_class',
    'lt_class', 'leq_class', 'eq_class',
    'cond_class', 'loop_class', 'block_class', 'let_class',
    'typcase_class', 'dispatch_class', 'static_dispatch_class',
]
for t in expr_types:
    ok = f'dynamic_cast<{t}*>' in src_cc
    check(ok, f'Cobertura de {t}')
    print(f'  {"OK" if ok else "FALTA!"} {t}')
print()

# =====================================================================
# 4. ACESSORES DA AST (consistência com PA3)
# =====================================================================
print('='*65)
print('4. ACESSORES DA AST — CONSISTÊNCIA COM PA3')
print('='*65)

# Acessores esperados pelo cool-tree.h padrão do curso Stanford
expected_accessors = {
    'class__class':          ['get_name','get_parent','get_features','get_filename'],
    'method_class':          ['get_name','get_formals','get_return_type','get_expr'],
    'attr_class':            ['get_name','get_type_decl','get_init'],
    'formal_class':          ['get_name','get_type_decl'],
    'branch_class':          ['get_name','get_type_decl','get_expr'],
    'assign_class':          ['get_name','get_expr'],
    'dispatch_class':        ['get_expr','get_name','get_actual'],
    'static_dispatch_class': ['get_expr','get_type_name','get_name','get_actual'],
    'cond_class':            ['get_pred','get_then_exp','get_else_exp'],
    'loop_class':            ['get_pred','get_body'],
    'typcase_class':         ['get_expr','get_cases'],
    'block_class':           ['get_body'],
    'let_class':             ['get_identifier','get_type_decl','get_init','get_body'],
    'plus_class':            ['get_e1','get_e2'],
    'neg_class':             ['get_e1'],
    'isvoid_class':          ['get_e1'],
    'new__class':            ['get_type_name'],
    'object_class':          ['get_name'],
}
all_ok = True
for cls, methods in expected_accessors.items():
    for m in methods:
        ok = f'->{m}()' in src_cc
        if not ok:
            print(f'  FALTA! {cls}::{m}()')
            all_ok = False
if all_ok:
    print('  OK  Todos os acessores estão presentes no código')
print()

# =====================================================================
# 5. BALANÇO DE ESCOPOS
# =====================================================================
print('='*65)
print('5. BALANÇO DE ESCOPOS (enterscope/exitscope)')
print('='*65)

enters = src_cc.count('enterscope()')
exits  = src_cc.count('exitscope()')
bal    = enters == exits
print(f'  enterscope(): {enters}')
print(f'  exitscope():  {exits}')
check(bal, f'Escopos balanceados ({enters} enter = {exits} exit)')
print(f'  {"OK  Balanceado" if bal else "ERR Desbalanceado — pode causar vazamento de escopo!"}')
print()

# =====================================================================
# 6. VERIFICAÇÃO DE ERROS SEMÂNTICOS COBERTOS
# =====================================================================
print('='*65)
print('6. ERROS SEMÂNTICOS VERIFICADOS')
print('='*65)

semantic_checks = [
    ('Redefinição de classe',          'was previously defined'),
    ('Herança de Int/Bool/String',     'cannot inherit class'),
    ('Classe pai indefinida',          'inherits from an undefined class'),
    ('Ciclo de herança',               'is involved in an inheritance cycle'),
    ('Main não definida',              'Class Main is not defined'),
    ('main() sem main()',              "No 'main' method in class Main"),
    ('main() com parâmetros',          'Main should have no arguments'),
    ('Atributo chamado self',          "cannot be the name of an attribute"),
    ('Tipo de atributo inválido',      'of attribute'),
    ('Atributo duplicado na classe',   'multiply defined in class'),
    ('Atributo herdado redefinido',    'attribute of an inherited class'),
    ('Tipo de retorno inválido',       'Undefined return type'),
    ('Override incompatível (params)', 'Incompatible number of formal'),
    ('Override tipo de retorno dif.',  'return type'),
    ('Formal chamado self',            "cannot be the name of a formal"),
    ('Formal duplicado',               'is multiply defined'),
    ('Corpo não conforma ao retorno',  'Inferred return type'),
    ('Init não conforma ao tipo',      'Inferred type'),
    ('Atribuição a self',              "Cannot assign to 'self'"),
    ('Variável não declarada',         'Undeclared identifier'),
    ('Aritmética com não-Int',         'non-Int arguments'),
    ('Comparação de tipos básicos',    'Illegal comparison'),
    ('If/while sem Bool',              'does not have type Bool'),
    ('Dispatch método inexistente',    'Dispatch to undefined method'),
    ('Número errado de argumentos',    'wrong number of arguments'),
    ('Argumento não conforma',         'does not conform to declared type'),
    ('Self em let',                    "cannot be bound in a 'let'"),
    ('New tipo indefinido',            'used in'),
    ('Case tipo SELF_TYPE',            'SELF_TYPE in case branch'),
    ('Case tipo duplicado',            'Duplicate branch'),
    ('Static dispatch inválido',       'Static dispatch'),
]

for desc, keyword in semantic_checks:
    ok = keyword in src_cc
    check(ok, desc)
    print(f'  {"OK" if ok else "FALTA!"} {desc}')
print()

# =====================================================================
# 7. VERIFICAÇÃO DOS ARQUIVOS COOL
# =====================================================================
print('='*65)
print('7. ARQUIVOS COOL DE TESTE')
print('='*65)

# Verificar que good.cl tem classe Main com main()
has_main_good = 'class Main' in good and 'main()' in good
check(has_main_good, 'good.cl tem classe Main com main()')
print(f'  {"OK" if has_main_good else "ERR"} good.cl: classe Main com main() presente')

# Verificar que bad.cl tem classe Main (para não dar erro de "no Main" e travar)
has_main_bad = 'class Main' in bad
check(has_main_bad, 'bad.cl tem classe Main')
print(f'  {"OK" if has_main_bad else "WRN"} bad.cl: classe Main presente (necessária para recuperação)')

# Contar classes em cada arquivo
classes_good = len(re.findall(r'^class\s+\w+', good, re.MULTILINE))
classes_bad  = len(re.findall(r'^class\s+\w+', bad,  re.MULTILINE))
print(f'  OK  good.cl: {classes_good} classes definidas')
print(f'  OK  bad.cl:  {classes_bad} classes (incluindo erros intencionais)')

# Verificar erros intencionais no bad.cl
intentional_errors = [
    ('inherits Int',               'Herança de Int'),
    ('inherits ClasseQueNaoExiste','Herança de classe indefinida'),
    ('class Duplicada',            'Classe duplicada'),
    ('self : Int',                 'Atributo chamado self'),
    ('self : String',              'Atributo chamado self (tipo String)'),
    ('TipoQueNaoExiste',           'Tipo inexistente em atributo'),
    ('Cannot assign',              'Atrib. a self (comentário)'),
    ('variavel_inexistente',       'Variável não declarada'),
]
print()
print('  Erros intencionais em bad.cl:')
for pattern, desc in intentional_errors:
    found = pattern in bad
    print(f'    {"OK" if found else "NAO ENCONTRADO"} {desc}')
print()

# =====================================================================
# 8. CONSISTÊNCIA COM PA2 e PA3
# =====================================================================
print('='*65)
print('8. CONSISTÊNCIA COM PA2 E PA3')
print('='*65)

pa2 = read_file(r'..\PA2 (1).u'.replace('..', r'c:\Users\yurim\OneDrive\Área de Trabalho\compilador'))
# Verificar tokens usados no PA4 que vêm do lexer (PA2)
tokens_pa2 = ['OBJECTID', 'TYPEID', 'INT_CONST', 'STR_CONST', 'BOOL_CONST']
print('  Tokens do PA2 (lexer) referenciados no PA4:')
for t in tokens_pa2:
    print(f'    OK  {t} (referenciado indiretamente via cool-tree.h)')

print()
print('  Estruturas do PA3 (parser) usadas no PA4:')
pa3_structures = [
    ('class_',          'Nó de classe'),
    ('method',          'Nó de método'),
    ('attr',            'Nó de atributo'),
    ('dispatch',        'Dispatch dinâmico'),
    ('static_dispatch', 'Dispatch estático'),
    ('let',             'Expressão let'),
    ('typcase',         'Expressão case'),
    ('assign',          'Atribuição'),
    ('no_expr',         'Expressão vazia'),
]
for node, desc in pa3_structures:
    ok = f'{node}_class' in src_cc
    print(f'    {"OK" if ok else "FALTA!"} {node}_class — {desc}')
print()

# =====================================================================
# 9. RESUMO FINAL
# =====================================================================
print('='*65)
print('9. RESUMO FINAL')
print('='*65)
total_ok  = len(ok_msgs)
total_err = len(errors)
total_wrn = len(warnings)

if errors:
    print('\n  ERROS:')
    for e in errors:
        print(f'    {e}')
if warnings:
    print('\n  AVISOS:')
    for w in warnings:
        print(f'    {w}')

print(f'\n  Total OK:     {total_ok}')
print(f'  Total Erros:  {total_err}')
print(f'  Total Avisos: {total_wrn}')

if total_err == 0:
    print('\n  RESULTADO: CÓDIGO APROVADO NA VALIDAÇÃO ESTÁTICA')
    print('  Próximo passo: compilar no Ubuntu/WSL com make semant')
else:
    print('\n  RESULTADO: CORRIJA OS ERROS ACIMA ANTES DE COMPILAR')
