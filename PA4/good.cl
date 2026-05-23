-- good.cl  —  Casos de teste semânticos VÁLIDOS para PA4
-- Cobre: herança, SELF_TYPE, dispatch, let, case, aritmética,
--        comparações, blocos, isvoid, override de métodos, IO.

-- -----------------------------------------------------------------------
-- Classe base: Animal (herda de IO)
-- -----------------------------------------------------------------------
class Animal inherits IO {
    nome : String;
    idade : Int <- 0;

    init(n : String, i : Int) : SELF_TYPE {
        {
            nome  <- n;
            idade <- i;
            self;
        }
    };

    falar() : String { "..." };

    apresentar() : Object {
        out_string(nome.concat(" tem ").concat(
            (new A2I).i2a(idade)).concat(" anos.\n"))
    };

    get_idade() : Int { idade };
    get_nome()  : String { nome };
};

-- -----------------------------------------------------------------------
-- Subclasse: Cachorro
-- -----------------------------------------------------------------------
class Cachorro inherits Animal {
    raca : String <- "Vira-lata";

    -- Override de falar()
    falar() : String { "Au au!" };

    set_raca(r : String) : SELF_TYPE {
        { raca <- r; self; }
    };

    get_raca() : String { raca };
};

-- -----------------------------------------------------------------------
-- Subclasse: Gato
-- -----------------------------------------------------------------------
class Gato inherits Animal {
    falar() : String { "Miau!" };
};

-- -----------------------------------------------------------------------
-- Testando SELF_TYPE em new e retorno
-- -----------------------------------------------------------------------
class Copiavel {
    copia() : SELF_TYPE { new SELF_TYPE };
};

class CopiavelEsp inherits Copiavel {
    valor : Int <- 42;
    get_valor() : Int { valor };
};

-- -----------------------------------------------------------------------
-- Teste de let aninhado, case, blocos e operadores
-- -----------------------------------------------------------------------
class Calculadora inherits IO {

    soma(a : Int, b : Int) : Int { a + b };
    sub (a : Int, b : Int) : Int { a - b };
    mul (a : Int, b : Int) : Int { a * b };
    div (a : Int, b : Int) : Int { a / b };

    abs_val(x : Int) : Int {
        if x < 0 then ~x else x fi
    };

    fatorial(n : Int) : Int {
        let acc : Int <- 1 in
            let i : Int <- n in
                {
                    while 0 < i loop
                    {
                        acc <- acc * i;
                        i <- i - 1;
                    }
                    pool;
                    acc;
                }
    };

    max(a : Int, b : Int) : Int {
        if b < a then a else b fi
    };

    -- Testa case com herança
    descricao_animal(a : Animal) : String {
        case a of
            c : Cachorro => c.falar();
            g : Gato     => g.falar();
            x : Animal   => x.falar();
        esac
    };

    -- Testa isvoid
    nulo_ou_nao(x : Object) : Bool {
        isvoid x
    };

    -- Testa igualdade de tipos básicos
    compara_strings(s1 : String, s2 : String) : Bool {
        s1 = s2
    };

    compara_ints(i1 : Int, i2 : Int) : Bool {
        i1 = i2
    };

    compara_bools(b1 : Bool, b2 : Bool) : Bool {
        b1 = b2
    };
};

-- -----------------------------------------------------------------------
-- Classe com herança múltipla (via cadeia)
-- -----------------------------------------------------------------------
class Veiculo {
    velocidade : Int <- 0;

    acelerar(v : Int) : SELF_TYPE {
        { velocidade <- velocidade + v; self; }
    };

    get_vel() : Int { velocidade };
};

class Carro inherits Veiculo {
    marca : String <- "Generico";

    set_marca(m : String) : SELF_TYPE {
        { marca <- m; self; }
    };

    get_marca() : String { marca };
};

class CarroEsportivo inherits Carro {
    turbo : Bool <- false;

    ligar_turbo() : SELF_TYPE {
        { turbo <- true; self; }
    };

    is_turbo() : Bool { turbo };
};

-- -----------------------------------------------------------------------
-- Teste de dispatch estático
-- -----------------------------------------------------------------------
class Base {
    metodo() : String { "Base" };
};

class Derivada inherits Base {
    metodo() : String { "Derivada" };

    chamar_base() : String {
        self@Base.metodo()   -- dispatch estático para Base
    };
};

-- -----------------------------------------------------------------------
-- Classe principal
-- -----------------------------------------------------------------------
class Main inherits IO {
    main() : Object {
        let c    : Cachorro   <- (new Cachorro).init("Rex",  3) in
        let g    : Gato       <- (new Gato).init("Mimi", 2) in
        let calc : Calculadora <- new Calculadora in
        let d    : Derivada    <- new Derivada    in
        {
            out_string(c.falar().concat("\n"));
            out_string(g.falar().concat("\n"));

            out_string((new A2I).i2a(calc.fatorial(5)).concat("\n"));

            -- SELF_TYPE: copia retorna CopiavelEsp
            let ce : CopiavelEsp <- new CopiavelEsp in
            let cp : Copiavel    <- ce.copia() in
                out_string("copia ok\n");

            -- Dispatch estático
            out_string(d.chamar_base().concat("\n"));

            -- isvoid
            if calc.nulo_ou_nao(new Object) then
                out_string("void\n")
            else
                out_string("nao void\n")
            fi;

            -- case em cadeia de tipos
            out_string(calc.descricao_animal(c).concat("\n"));
            out_string(calc.descricao_animal(g).concat("\n"));
        }
    };
};

-- -----------------------------------------------------------------------
-- Classe auxiliar: conversão Int <-> String (versão mínima)
-- -----------------------------------------------------------------------
class A2I {
    i2a(i : Int) : String {
        if i = 0 then "0"
        else if i < 0 then "-".concat(i2a_aux(~i))
        else i2a_aux(i)
        fi fi
    };

    i2a_aux(i : Int) : String {
        if i = 0 then ""
        else (i2a_aux(i/10)).concat(
                (let r : Int <- i - (i/10*10) in
                    if      r = 0 then "0"
                    else if r = 1 then "1"
                    else if r = 2 then "2"
                    else if r = 3 then "3"
                    else if r = 4 then "4"
                    else if r = 5 then "5"
                    else if r = 6 then "6"
                    else if r = 7 then "7"
                    else if r = 8 then "8"
                    else               "9"
                    fi fi fi fi fi fi fi fi fi)
             )
        fi
    };
};
