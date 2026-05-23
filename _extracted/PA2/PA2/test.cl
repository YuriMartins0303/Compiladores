(*
  test.cl - PA2 lexical tests

  Este arquivo contém casos "bons" e casos de erro.
*)

-- =========================================================
-- 1) Keywords (diferentes caixas) + IDs
-- =========================================================
ClAsS Main InHeRiTs IO {
  x : Int <- 123;
  y : Int <- 0;

  -- while/loop/pool
  testWhile() : Object {
    {
      WhIlE y < 10 LoOp
        y <- y + 1
      PoOl;
      0;
    }
  };

  -- if/then/else/fi
  testIf() : Object {
    {
      iF true tHeN
        x <- x + 1
      eLsE
        x <- x - 1
      fI;
      0;
    }
  };

  -- case/of/esac
  testCase(a : Object) : Object {
    case a of
      b : Int => b + 1;
      c : Object => 0;
    esac
  };

  -- let/in
  testLet() : Object {
    {
      let z : Int <- 5 in
        z + 1;
    }
  };

  -- new / isvoid / not
  testNew() : Object {
    {
      if isvoid (new Main) then 0 else 1 fi;
      if not false then 1 else 0 fi;
      0;
    }
  };
};

-- =========================================================
-- 2) BOOL_CONST: true/false começando em minúsculo
--    True/False devem virar IDs (OBJECTID/TYPEID)
-- =========================================================
true
false
True
False

-- =========================================================
-- 3) Inteiros e IDs
-- =========================================================
a
a_1
A
A_1
0123
999999

-- =========================================================
-- 4) Operadores multi-char e 1-char
-- =========================================================
=> <= <-
+ - * / = < . ~ , @ ; : ( ) { }

-- =========================================================
-- 5) Comentários aninhados
-- =========================================================
(*
  nível 1
  (*
    nível 2
  *)
  volta nível 1
*)

-- Unmatched close comment:
*)

-- =========================================================
-- 6) Strings OK + escapes
-- =========================================================
"abc"
"linha1\nlinha2"
"tab\tfim"
"backspace\bX"
"formfeed\fX"
"aspas: \""
"barra: \\"
"\q vira q"     -- regra \c -> c

-- =========================================================
-- 7) Strings com erro (cada uma força um erro específico)
-- =========================================================

-- Unterminated string constant (newline antes de fechar)
"nao fecha
x

-- EOF in string constant: deixe a última aspas aberta até EOF
"eof em string

-- =========================================================
-- 8) EOF in comment (comentário aberto até EOF)
-- =========================================================
(*
  comentario aberto até o fim do arquivo