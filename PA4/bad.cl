-- bad.cl  —  Casos de teste semânticos INVÁLIDOS para PA4
-- Cada bloco demonstra um tipo diferente de erro semântico.
-- O analisador deve reportar todos os erros e tentar se recuperar.

-- -----------------------------------------------------------------------
-- ERRO 1: Herdar de Int (classe básica não herdável)
-- -----------------------------------------------------------------------
class HerdaInt inherits Int {
    x : Int <- 0;
};

-- -----------------------------------------------------------------------
-- ERRO 2: Herdar de classe indefinida
-- -----------------------------------------------------------------------
class HerdaUndefined inherits ClasseQueNaoExiste {
    y : String <- "ops";
};

-- -----------------------------------------------------------------------
-- ERRO 3: Classe Main sem método main()
-- (Se Main for redefinida sem main, o compilador deve reclamar.
--  Aqui usamos outra classe que omite a verificação, pois Main
--  válida é definida ao final. Testamos falta de main em Main
--  comentando a classe Main real e usando esta.)
-- -----------------------------------------------------------------------

-- -----------------------------------------------------------------------
-- ERRO 4: Redefinição de classe
-- -----------------------------------------------------------------------
class Duplicada { x : Int <- 1; };
class Duplicada { x : Int <- 2; };   -- erro: redefinição

-- -----------------------------------------------------------------------
-- ERRO 5: Atributo chamado 'self'
-- -----------------------------------------------------------------------
class AtribSelf {
    self : Int <- 0;   -- erro
};

-- -----------------------------------------------------------------------
-- ERRO 6: Tipo de atributo indefinido
-- -----------------------------------------------------------------------
class TipoAtribInvalido {
    x : TipoQueNaoExiste <- 0;  -- erro: tipo desconhecido
};

-- -----------------------------------------------------------------------
-- ERRO 7: Redefinição de atributo na própria classe
-- -----------------------------------------------------------------------
class AtribDuplo {
    x : Int <- 1;
    x : Int <- 2;   -- erro: x definido duas vezes
};

-- -----------------------------------------------------------------------
-- ERRO 8: Atributo herdado redefinido na subclasse
-- -----------------------------------------------------------------------
class Pai {
    valor : Int <- 10;
};
class FilhoRedefineAtrib inherits Pai {
    valor : Int <- 20;  -- erro: atributo herdado
};

-- -----------------------------------------------------------------------
-- ERRO 9: Tipo de retorno de método indefinido
-- -----------------------------------------------------------------------
class RetornoInvalido {
    metodo() : TipoFantasma { 0 };  -- erro: TipoFantasma não existe
};

-- -----------------------------------------------------------------------
-- ERRO 10: Parâmetro formal chamado 'self'
-- -----------------------------------------------------------------------
class FormalSelf {
    metodo(self : Int) : Int { self };  -- erro: formal 'self'
};

-- -----------------------------------------------------------------------
-- ERRO 11: Override com assinatura incompatível (tipo de parâmetro diferente)
-- -----------------------------------------------------------------------
class BaseSig {
    metodo(x : Int) : Int { x };
};
class FilhoSigErrada inherits BaseSig {
    metodo(x : String) : Int { 0 };  -- erro: tipo do parâmetro diferente
};

-- -----------------------------------------------------------------------
-- ERRO 12: Override com tipo de retorno diferente
-- -----------------------------------------------------------------------
class BaseRet {
    get() : Int { 0 };
};
class FilhoRetErrado inherits BaseRet {
    get() : String { "ops" };  -- erro: tipo de retorno diferente
};

-- -----------------------------------------------------------------------
-- ERRO 13: Corpo do método não conforma ao tipo de retorno
-- -----------------------------------------------------------------------
class CorpoErrado {
    metodo() : Int { "isso nao e um Int" };  -- erro: String não conforma a Int
};

-- -----------------------------------------------------------------------
-- ERRO 14: Inicialização de atributo não conforma ao tipo
-- -----------------------------------------------------------------------
class InitErrada {
    x : Int <- "nao e inteiro";  -- erro: String não conforma a Int
};

-- -----------------------------------------------------------------------
-- ERRO 15: Atribuição a 'self'
-- -----------------------------------------------------------------------
class AssignSelf {
    metodo() : Object {
        self <- new Object   -- erro: não se pode atribuir a self
    };
};

-- -----------------------------------------------------------------------
-- ERRO 16: Variável não declarada
-- -----------------------------------------------------------------------
class VarNaoDeclarada {
    metodo() : Int {
        variavel_inexistente + 1   -- erro: variável não declarada
    };
};

-- -----------------------------------------------------------------------
-- ERRO 17: Aritmética com tipos não-Int
-- -----------------------------------------------------------------------
class AritmeticaErrada {
    metodo() : Int {
        "texto" + 1   -- erro: operando não é Int
    };
};

-- -----------------------------------------------------------------------
-- ERRO 18: Comparação ilegal entre tipos básicos distintos
-- -----------------------------------------------------------------------
class ComparacaoIlegal {
    metodo() : Bool {
        1 = "um"   -- erro: Int comparado com String
    };
};

-- -----------------------------------------------------------------------
-- ERRO 19: Predicado de if não é Bool
-- -----------------------------------------------------------------------
class IfNaoBool {
    metodo() : Int {
        if 42 then 1 else 2 fi   -- erro: predicado não é Bool
    };
};

-- -----------------------------------------------------------------------
-- ERRO 20: Predicado de while não é Bool
-- -----------------------------------------------------------------------
class WhileNaoBool {
    metodo() : Object {
        while 0 loop out_string("loop\n") pool   -- erro: 0 é Int, não Bool
    };
};

-- -----------------------------------------------------------------------
-- ERRO 21: Dispatch a método inexistente
-- -----------------------------------------------------------------------
class DispatchInexistente {
    metodo() : Object {
        (new Object).metodo_que_nao_existe()  -- erro: método indefinido
    };
};

-- -----------------------------------------------------------------------
-- ERRO 22: Número errado de argumentos no dispatch
-- -----------------------------------------------------------------------
class ArgWrong {
    soma(a : Int, b : Int) : Int { a + b };
    teste() : Int {
        soma(1)   -- erro: faltou um argumento
    };
};

-- -----------------------------------------------------------------------
-- ERRO 23: Tipo de argumento não conforma ao parâmetro
-- -----------------------------------------------------------------------
class ArgTipoErrado {
    double(x : Int) : Int { x * 2 };
    teste() : Int {
        double("texto")   -- erro: String não conforma a Int
    };
};

-- -----------------------------------------------------------------------
-- ERRO 24: 'self' ligado em let
-- -----------------------------------------------------------------------
class LetSelf {
    metodo() : Object {
        let self : Int <- 5 in self   -- erro: 'self' não pode ser ligado em let
    };
};

-- -----------------------------------------------------------------------
-- ERRO 25: Inicialização de let não conforma ao tipo declarado
-- -----------------------------------------------------------------------
class LetInitErrada {
    metodo() : Object {
        let x : Int <- "texto" in x   -- erro: String não conforma a Int
    };
};

-- -----------------------------------------------------------------------
-- ERRO 26: Tipo duplicado em ramos de case
-- -----------------------------------------------------------------------
class CaseDuplicado {
    metodo(x : Object) : String {
        case x of
            a : Int => "int";
            b : Int => "int de novo";   -- erro: tipo duplicado
            c : Object => "obj";
        esac
    };
};

-- -----------------------------------------------------------------------
-- ERRO 27: dispatch estático para tipo não existente
-- -----------------------------------------------------------------------
class StaticDispatchInvalido {
    metodo() : Object {
        self@TipoFantasma.toString()   -- erro: tipo não existe
    };
};

-- -----------------------------------------------------------------------
-- ERRO 28: new com tipo indefinido
-- -----------------------------------------------------------------------
class NewInvalido {
    metodo() : Object {
        new ClasseFantasma   -- erro: classe não existe
    };
};

-- -----------------------------------------------------------------------
-- Classe Main válida (necessária para o compilador não dar erro de Main)
-- -----------------------------------------------------------------------
class Main {
    main() : Object {
        out_string("bad.cl loaded\n")
    };
};
