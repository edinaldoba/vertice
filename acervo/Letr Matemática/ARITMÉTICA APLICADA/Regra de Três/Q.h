//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, ii, n, *rnd,
        a, a1=30,  a2=75,  // Velocidade
        b, b1=90,  b2=180, // Velocidade
        c, c1=6,   c2=24,  // Tempo
        d, d1=2,   d2=10;  // Tempo
        
    n=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*c==b*d && a*c>=360 && a*c<=440 ){
                        n++;
                    }
                }
            }
        }
    }
//     printf("%d\n",n);
    struct{ int a, b, c, d; } D[n];
    i=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*c==b*d && a*c>=360 && a*c<=440 ){
                        D[i].a = a;
                        D[i].b = b;
                        D[i].c = c;
                        D[i].d = d;
                        i++;
                    }
                }
            }
        }
    }
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    do{ 
        ii = rand()%n;
        sprintf( alt[0], "Em $%d$ horas.\n", D[ii].d );
        
        rnd = randperm( d2-d1+1 );
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "Em $%d$ horas.\n", d1 + rnd[i] );
        }
        free(rnd);
        
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Antigamente, o trajeto São Paulo\\,-\\,Rio podia ser feito por trem. À velocidade constante de $%d$\\,km/h, o trem fazia essa viagem em $%d$ horas. Se ele desenvolvesse a velocidade de $%d$\\,km/h, em quanto tempo faria o mesmo trajeto?\n\n", D[ii].a, D[ii].c, D[ii].b  );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//





//############################################ Questão 02 ############################################//
void Q2(){
    
    int i, ii, n, *rnd,
        a, a1=5 , a2=15,  // Número de pessoas
        b, b1=16, b2=25,  // Número de pessoas
        c, c1=31, c2=50,  // Número de dias
        d, d1=10, d2=30;  // Número de dias
        
    n=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*c==b*d ){
                        n++;
                    }
                }
            }
        }
    }
//     printf("%d\n",n);
    struct{ int a, b, c, d; } D[n];
    i=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*c==b*d ){
                        D[i].a = a;
                        D[i].b = b;
                        D[i].c = c;
                        D[i].d = d;
                        i++;
                    }
                }
            }
        }
    }
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    do{ 
        ii = rand()%n;
        sprintf( alt[0], "Para $%d$ dias.\n", D[ii].d );
        
        rnd = randperm( d2-d1+1 );
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "Para $%d$ dias.\n", d1 + rnd[i] );
        }
        free(rnd);
        
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    FILE *p = fopen( "Q2.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Um navio foi abastecido com comida suficiente para alimentar $%d$ pessoas durante $%d$ dias. Se $%d$ pessoas embarcarem nesse navio, para quantos dias, no máximo, as reservas de alimentos serão suficientes?\n\n", D[ii].a, D[ii].c, D[ii].b  );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//





//############################################ Questão 03 ############################################//
void Q3(){
    
    int i, ii, n, *rnd,
        a, a1=2    , a2=5    ,  // Número de torneiras
        b, b1=6    , b2=9    ,  // Número de torneiras
        c, c1=61   , c2=108  ,  // Tempo em minutos
        d, d1=20*60, d2=59*60;  // Tempo em segundos
    
    struct tempo t;
    
    char str[100], *snum[4]={"Duas","Três","Quatro","Cinco"};
        
    n=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( 60*a*c==b*d && d%60!=0 ){
                        n++;
                    }
                }
            }
        }
    }
//     printf("%d\n",n);
    struct{ int a, b, c, d; } D[n];
    i=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( 60*a*c==b*d && d%60!=0 ){
                        D[i].a = a;
                        D[i].b = b;
                        D[i].c = c;
                        D[i].d = d;
                        i++;
                    }
                }
            }
        }
    }
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    do{ 
        ii = rand()%n;
        t = segundos( D[ii].d );
        hms( t, str );
        sprintf( alt[0], "%s.\n", str );
        
        rnd = randperm( d2-d1+1 );
        for( i=1; i<5; i++ ){
            t = segundos( d1 + rnd[i] );
            hms( t, str );
            sprintf( alt[i], "%s.\n", str );
        }
        free(rnd);
        
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    t = minutos( D[ii].c );
    hms( t, str );
    
    
    FILE *p = fopen( "Q3.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );    
    fprintf( p, "%s torneiras idênticas, abertas completamente, enchem um tanque de água em %s. Se em vez de $%d$ torneiras fossem $%d$, quanto tempo elas levariam para encher o mesmo tanque?\n\n", snum[D[ii].a-a1], str, D[ii].a, D[ii].b  );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//





//############################################ Questão 04 ############################################//
void Q4(){
    
    int i, ii, n, *rnd,
        a, a1=6,   a2=50,  // Segundos
        d, d1=200, d2=360, // Dias
        b, b1=4,   b2=30,  // Minutos
        c, c1=5,   c2=10;  // Dias
        
    n=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*d==b*60*c ){
                        n++;
                    }
                }
            }
        }
    }
//     printf("%d\n",n);
    struct{ int a, b, c, d; } D[n];
    i=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*d==b*60*c ){
                        D[i].a = a;
                        D[i].b = b;
                        D[i].c = c;
                        D[i].d = d;
                        i++;
                    }
                }
            }
        }
    }
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    do{ 
        ii = rand()%n;
        sprintf( alt[0], "$%d$ minutos.\n", D[ii].b );
        
        rnd = randperm( b2-b1+1 );
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "$%d$ minutos.\n", b1 + rnd[i] );
        }
        free(rnd);
        
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    
    FILE *p = fopen( "Q4.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "O relógio da igreja matriz adianta $%d$ segundos a cada $%d$ dias. Quanto adianta de $%d$ dias?\n\n", D[ii].a, D[ii].c, D[ii].d  );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//





//############################################ Questão 05 ############################################//
void Q5(){
    
    int i, ii, n, *rnd,
        a, a1=20,    a2=40,       // Quilogramas
        b, b1=40,    b2=60;       // Quilogramas
        
    float preco;
        
    n=441;
    struct{ int a, b; float c, d; } D[n];
    i=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            preco = 18.00 + 1e-2*(rand()%101 - 50);
            D[i].a = a;
            D[i].b = b;
            D[i].c = a*preco;
            D[i].d = b*preco;
            i++;
        }
    }
//     printf("%d\n",i);
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    do{ 
        ii = rand()%n;
        sprintf( alt[0], "$%d$ kg.\n", D[ii].b );
        
        rnd = randperm( b2-b1+1 );
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "$%d$ kg.\n", b1 + rnd[i] );
        }
        free(rnd);
        
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    FILE *p = fopen( "Q5.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Uma churrascaria comprou $%d$ quilogramas de alcatra por R\\$\\,$%d{,}%.2d$. Quanto dessa carne poderia comprar com R\\$\\,$%d{,}%.2d$?\n\n", D[ii].a, (int)D[ii].c, (int)((long int)(100*D[ii].c)%100), (int)D[ii].d, (int)((long int)(100*D[ii].d)%100) );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//





//############################################ Questão 06 ############################################//
void Q6(){
    
    int i, ii, n, *rnd,
        a, a1=5000, a2=5125, // Exemplares
        b, b1=4875, b2=4999, // Exemplares
        c, c1=2051, c2=2101, // Quilogramas de papel
        d, d1=2000, d2=2050; // Quilogramas de papel
        
    n=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*d==b*c ){
                        n++;
                    }
                }
            }
        }
    }
//     printf("%d\n",n);
    struct{ int a, b, c, d; } D[n];
    i=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*d==b*c ){
                        D[i].a = a;
                        D[i].b = b;
                        D[i].c = c;
                        D[i].d = d;
                        i++;
                    }
                }
            }
        }
    }
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    do{ 
        ii = rand()%n;
        sprintf( alt[0], "$%d$ exemplares.\n", D[ii].b );
        
        rnd = randperm( b2-b1+1 );
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "$%d$ exemplares.\n", b1 + rnd[i] );
        }
        free(rnd);
        
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    FILE *p = fopen( "Q6.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Para imprimir $%d$ exemplares de certo livro foram usados $%d$ quilogramas de papel. Quantos exemplares desse livro podem ser impressos com $%d$ quilogramas do mesmo papel?\n\n", D[ii].a, D[ii].c, D[ii].d );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//





//############################################ Questão 07 ############################################//
void Q7(){
    
    int i, ii, n, *rnd,
        a, a1=20  , a2=58,   // Tempo em segundos
        b, b1=2000, b2=2180, // Tempo em segundos
        c, c1=10  , c2=29,   // Capacidade em litros
        d, d1=1000, d2=1090; // Capacidade em litros
        
    char str[100];
    
    struct tempo t;
        
    n=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*d==b*c && a%c!=0 && b%60!=0 ){
                        n++;
                    }
                }
            }
        }
    }
//     printf("%d\n",n);
    struct{ int a, b, c, d; } D[n];
    i=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*d==b*c && a%c!=0 && b%60!=0 ){
                        D[i].a = a;
                        D[i].b = b;
                        D[i].c = c;
                        D[i].d = d;
                        i++;
                    }
                }
            }
        }
    }
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    do{ 
        ii = rand()%n;
        t = segundos( D[ii].b );
        hms( t, str );
        sprintf( alt[0], "%s.\n", str );
        
        rnd = randperm( b2-b1+1 );
        for( i=1; i<5; i++ ){
            t = segundos( b1 + rnd[i] );
            hms( t, str );
            sprintf( alt[i], "%s.\n", str );
        }
        free(rnd);
        
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    FILE *p = fopen( "Q7.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Uma torneira, completamente aberta, leva $%d$ segundos para encher um balde com capacidade de $%d$\\,L. Quanto tempo seria necessário para essa torneira encher um recipiente com capacidade para $%d$\\,L?\n\n", D[ii].a, D[ii].c, D[ii].d );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//





//############################################ Questão 08 ############################################//
void Q8(){
    
    int i, ii, n, *rnd,
        a, a1=25  , a2=28,     // Litros de água
        b, b1=1401, b2=1499,   // Litros de água
        c, c1=8901, c2=9899,   // Gramas de sal
        d, d1=4988, d2=5299;   // Centenas de gramas de sal
        
    n=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( 1e2*a*d==b*c && d%10!=0 ){
                        n++;
                    }
                }
            }
        }
    }
//     printf("%d\n",n);
    struct{ int a, b, c, d; } D[n];
    i=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( 1e2*a*d==b*c && d%10!=0 ){
                        D[i].a = a;
                        D[i].b = b;
                        D[i].c = c;
                        D[i].d = d;
                        i++;
                    }
                }
            }
        }
    }
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    do{ 
        ii = rand()%n;
        sprintf( alt[0], "$%d{,}%d$ quilogramas\n", D[ii].d/10, D[ii].d%10 );
        
        rnd = randperm( d2-d1+1 );
        for( i=1; i<5; i++ ){
            if( (d1+rnd[i])%10!=0 )
                sprintf( alt[i], "$%d{,}%d$ quilogramas\n", (d1+rnd[i])/10, (d1+rnd[i])%10 );
            else
                sprintf( alt[i], "$%d$ quilogramas\n", (d1+rnd[i])/10 );
        }
        free(rnd);
        
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    FILE *p = fopen( "Q8.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Em $%d$ litros de água, à temperatura ambiente, é possível dissolver até $%d$ gramas de sal. Qual a quantidade máxima de sal que pode ser dissolvida em $%d$ litros de água?\n\n", D[ii].a, D[ii].c, D[ii].b );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//





//############################################ Questão 09 ############################################//
void Q9(){
    
    int i, ii, n, *rnd,
        a, a1=1100, a2=1300,  // Número de veículos
        b, b1=314 , b2=650,   // Número de veículos
        c, c1=7   , c2=12,    // Horas por dia
        d, d1=2   , d2=6;     // Horas por dia
        
    n=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*d==b*c && c%d!=0 ){
                        n++;
                    }
                }
            }
        }
    }
//     printf("%d\n",n);
    struct{ int a, b, c, d; } D[n];
    i=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*d==b*c && c%d!=0 ){
                        D[i].a = a;
                        D[i].b = b;
                        D[i].c = c;
                        D[i].d = d;
                        i++;
                    }
                }
            }
        }
    }
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    do{ 
        ii = rand()%n;
        sprintf( alt[0], "$%d$ veículos\n", D[ii].b );
        
        rnd = randperm( b2-b1+1 );
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "$%d$ veículos\n", b1+rnd[i] );
        }
        free(rnd);
        
    } while( dist_alt( alt, 5 ) );
    
    
    FILE *p = fopen( "Q9.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Uma montadora de automóveis produz mensalmente $%d$ veículos de certo modelo, se a linha de montagem operar $%d$ horas por dia. Quantos veículos serão produzidos se a linha de montagem operar diariamente durante $%d$ horas?\n\n", D[ii].a, D[ii].c, D[ii].d );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//





//############################################ Questão 10 ############################################//
void Q10(){
    
    int i, ii, n, *rnd,
        a, a1=5 , a2=9,    // Número de pedreiros
        b, b1=2 , b2=4,    // Número de pedreiros
        c, c1=10, c2=40,   // Número de dias
        d, d1=25, d2=90;   // Número de dias
        
    char *snum[5] = {"Cinco","Seis","Sete","Oito","Nove"};
        
    n=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*c==b*d && c!=d ){
                        n++;
                    }
                }
            }
        }
    }
//     printf("%d\n",n);
    struct{ int a, b, c, d; } D[n];
    i=0;
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                for( d=d1; d<=d2; d++ ){
                    if( a*c==b*d && c!=d ){
                        D[i].a = a;
                        D[i].b = b;
                        D[i].c = c;
                        D[i].d = d;
                        i++;
                    }
                }
            }
        }
    }
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    do{ 
        ii = rand()%n;
        sprintf( alt[0], "$%d$ dias\n", D[ii].d );
        
        rnd = randperm( d2-d1+1 );
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "$%d$ dias\n", d1+rnd[i] );
        }
        free(rnd);
        
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    FILE *p = fopen( "Q10.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "%s pedreiros, com a mesma capacidade de trabalho, levam $%d$ dias para concluir certa obra. Com apenas $%d$ desses pedreiros, em quanto tempo a obra seria concluída?\n\n", snum[D[ii].a-5], D[ii].c, D[ii].b );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//





