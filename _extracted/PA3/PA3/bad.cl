class A {
};

(* error:  b is not a type identifier *)
Class b inherits A {
};

(* error:  a is not a type identifier *)
Class C inherits a {
};

(* error:  keyword inherits is misspelled *)
Class D inherts A {
};

(* error:  closing brace is missing *)
Class E inherits A {
;


-- Teste 1: Erro dentro de uma lista de Features
class F inherits A {

    atributo_bom : Int;
    isso_aqui_ta_tudo_errado 42 + "lixo" ;
    metodo_bom() : Object { 1 };
    metodo_ruim(a Int, b : String) : Int { 2 };
};

-- Teste 2: Erro dentro de um Bloco de Expressões { ... }
class G {
    teste_bloco() : Object {
        {
            
            out_string("Linha 1 OK\n");
            lixo_total + / * - 42 ;
            out_string("Sobreviveu ao erro do bloco!\n");
        }
    };
};

-- Teste 3: Erro no meio da declaração de variáveis
class H {
    teste_let() : Int {
        let 
            x : Int <- 1,
            y errado sem dois pontos ,
            z : Int <- 2 
        in
            x + z
    };
};