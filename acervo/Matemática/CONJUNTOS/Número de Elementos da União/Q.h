//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, j, ii, n=0, *rnd, a, ab, b, nao;

    char *pergunta[3] = { "Quantos alunos leram Iracema?", "Quantos alunos leram só Helena?", "Qual é o número de alunos nessa classe?" };

    int per = rand()%3;
    
    for( a=11; a<=20; a++ ){
        for( b=11; b<=20; b++ ){
            for( ab=5; ab<=10; ab++ ){
                for( nao=5; nao<=10; nao++ ){
                    n++;
                }
            }
        }
    }
    
    
    // printf("%d\n",n);
    struct{ int a, ab, b, nao, resp; } D[n];
    i=0;
    for( a=11; a<=20; a++ ){
        for( b=11; b<=20; b++ ){
            for( ab=5; ab<=10; ab++ ){
                for( nao=5; nao<=10; nao++ ){
                    D[i].a = a;
                    D[i].ab = ab;
                    D[i].b = b;
                    D[i].nao = nao;
                    if( per==0 )
                        D[i].resp = b;
                    else if(per==1)
                        D[i].resp = a-ab;
                    else if(per==2)
                        D[i].resp = a + b - ab + nao;
                    i++;
                }
            }
        }
    }
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    

    do{
        rnd = randperm(n);
        for( i=4; i>=0; i-- ){
            ii = rnd[i];
            sprintf( alt[i], "$%d$ alunos\n", D[ii].resp );
        }
        free(rnd);
    } while( dist_alt( alt, strlen(alt[0]) ) );

    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );
    
    fputs( "\% QUESTAO\n", p );
    

    fprintf( p, "Um professor de Português sugeriu em uma classe a leitura dos livros Helena, de Machado de Assis, e Iracemna, de José de Alencar, %d alunos leram Helena, %d leram só Iracema, %d leram os dois livros e %d não leram nenhum deles. %s\n\n", D[ii].a, D[ii].b - D[ii].ab, D[ii].ab, D[ii].nao, pergunta[per]  );
    
    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n}\n\n\n", p );
    
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//






//############################################ Questão 02 ############################################//
void Q2(){

    int i, j, ii, n=0, *rnd, a, ab, b, nao;

    for( a=11; a<=20; a++ ){
        for( b=11; b<=20; b++ ){
            for( ab=5; ab<=10; ab++ ){
                for( nao=1; nao<=20; nao++ ){
                    n++;
                }
            }
        }
    }


    // printf("%d\n",n);
    struct{ int a, ab, b, nao, resp; } D[n];
    i=0;
    for( a=11; a<=20; a++ ){
        for( b=11; b<=20; b++ ){
            for( ab=5; ab<=10; ab++ ){
                for( nao=1; nao<=20; nao++ ){
                    D[i].a = a;
                    D[i].ab = ab;
                    D[i].b = b;
                    D[i].nao = nao;
                    i++;
                }
            }
        }
    }



    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }



    do{
        rnd = randperm(n);
        for( i=4; i>=0; i-- ){
            ii = rnd[i];
            sprintf( alt[i], "$%d$ alunos\n", D[ii].nao );
        }
        free(rnd);
    } while( dist_alt( alt, strlen(alt[0]) ) );



    FILE *p = fopen( "Q2.tex", "w+" );

    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );

    fputs( "\% QUESTAO\n", p );


    fprintf( p, "Uma prova com duas questões foi dada a uma classe de %d alunos, %d alunos acertaram as duas questões, %d acertaram a primeira questão e %d acertaram a segunda questão. Quantos alunos erraram as duas questões?\n\n", D[ii].a + D[ii].b - D[ii].ab + D[ii].nao, D[ii].ab, D[ii].a, D[ii].b );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n}\n\n\n", p );

    fclose(p);

    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//

