class Animal {
    nome : String;
    idade : Int <- 0;

    
    init(n : String, a : Int) : Animal {
        {
            nome <- n;
            idade <- a;
            self;
        }
    };

    fala() : String {
        "..."
    };
};


class Cachorro inherits Animal {
    bom_menino : Bool <- true; 

    fala() : String {
        "Au Au!"
    };

    calcular_petiscos(base : Int, bonus : Int) : Int {
        let total : Int <- base + bonus,
            multiplicador : Int <- 2,
            dummy_var : String
        in
            total * multiplicador
    };
};


class Main inherits IO {
    pet : Cachorro;

    main() : Object {
        {
            pet <- new Cachorro;
            if isvoid pet then
                out_string("Nenhum pet encontrado!\n")
            else {
                pet.init("Rex", 3);
                out_string(pet.fala());
                out_string(pet@Animal.fala()); 
            }
            fi;

            let contador : Int <- 0 in
                while contador < 5 loop
                    {
                        contador <- contador + 1;
                        
                        if ~(contador) <= ~3 then
                            out_string("Contador esta alto\n")
                        else
                            out_string("Contador esta baixo\n")
                        fi;
                    }
                pool;

            case pet of
                d : Cachorro => out_string("É um cachorro!\n");
                a : Animal => out_string("É um animal!\n");
                o : Object => out_string("É um objeto!\n");
            esac;

            if not (1 = 2) then
                out_string("Certo!\n")
            else
                out_string("Se cair aqui eu sou burro!\n")
            fi;
        }
    };
};