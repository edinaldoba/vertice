//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, j, ii, n=0, *rnd,
    a,   a1=2,  a2=6,
    b,          b2=6,
    c,          c2=6,
    kn, kn1=1, kn2=9,
    kd, kd1=2, kd2=9;
    
    
    for( a=a1; a<=a2; a++ ){
        for( b=a+1; b<=b2; b++ ){
            for( c=b+1; c<=c2; c++ ){
                for( kn=kn1; kn<=kn2; kn++ ){
                    for( kd=kd1; kd<=kd2; kd++ ){
                        if( kn%kd!=0 ){
                            n++;
                        }
                    }
                }
            }
        }
    }
    
    
//     printf("%d\n",n);
    struct{ int a, b, c, kn, kd; } D[n];
    i=0;
    for( a=a1; a<=a2; a++ ){
        for( b=a+1; b<=b2; b++ ){
            for( c=b+1; c<=c2; c++ ){
                for( kn=kn1; kn<=kn2; kn++ ){
                    for( kd=kd1; kd<=kd2; kd++ ){
                        if( kn%kd!=0 ){
                            D[i].a = a;
                            D[i].b = b;
                            D[i].c = c;
                            D[i].kn = kn;
                            D[i].kd = kd;
                            i++;
                        }
                    }
                }
            }
        }
    }
    
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    
    rnd = randperm(n);
    ii = rnd[0];
    j = mdc( D[ii].kn, D[ii].kd );
        
    sprintf( alt[0], "$k=\\frac{%d}{%d}$\n", D[ii].kn/j,  D[ii].kd/j );
    do{
        for( i=1; i<5; i++ ){
            do{
            kn = kn1 + rand()%(kn2-kn1);
            kd = kd1 + rand()%(kd2-kd1);
            } while( mdc(kn,kd)!=1 );
            sprintf( alt[i], "$k=\\frac{%d}{%d}$\n", kn, kd  );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    free(rnd);
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\parbox{\\linewidth}{\n\n\% QUESTAO\n", p );
    
    fprintf( p, "São dadas as sucessões de números diretamente proporcionais $%d, %d, %d$\\, \\,e\\, \\,$%d, %d,%d$. Qual é o fator de proporcionalidade $(k)$ entre elas?}\\vspace{2mm}\\\\\\parbox{\\linewidth}{\n\n", D[ii].a*D[ii].kn, D[ii].b*D[ii].kn, D[ii].c*D[ii].kn, D[ii].a*D[ii].kd, D[ii].b*D[ii].kd, D[ii].c*D[ii].kd  );
    
    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n}\n\n", p );
    
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//







//############################################ Questão 02 ############################################//
void Q2(){
    
    int i, j, ii, n=0, *rnd,
    a, a1=2, a2=25,
    b,       b2=25,
    c,       c2=25,
    m;
    
    
    
    for( a=a1; a<=a2; a++ ){
        for( b=a+1; b<=b2; b++ ){
            for( c=b+1; c<=c2; c++ ){
                m = mmc(mmc(a,b),c);
                if( a!=b && a!=c && b!=c && m/b!=c && m<100 ){
                    n++;
                }
            }
        }
    }
    
    
//     printf("%d\n",n);
    struct{ int a, b, c, m; } D[n];
    i=0;
    for( a=a1; a<=a2; a++ ){
        for( b=a+1; b<=b2; b++ ){
            for( c=b+1; c<=c2; c++ ){
                m = mmc(mmc(a,b),c);
                if( a!=b && a!=c && b!=c && m/b!=c && m<100 ){
                    D[i].a = a;
                    D[i].b = b;
                    D[i].c = c;
                    D[i].m = m;
                    i++;
                }
            }
        }
    }
    
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    
    
    ii = rand() % n;
    
    int x, y;
        
    sprintf( alt[0], "$x=%d$ e $y=%d$\n", D[ii].b, D[ii].c );
    do{
        for( i=1; i<5; i++ ){
            do{
                x = D[ii].b + rand()%11 - 5;
                y = D[ii].c + rand()%11 - 5;
            } while( x==y || x==D[ii].b || y==D[ii].c );
            sprintf( alt[i], "$x=%d$ e $y=%d$\n", x, y );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    
    FILE *p = fopen( "Q2.tex", "w+" );
    
    fputs( "\\parbox{\\linewidth}{\n\n\% QUESTAO\n", p );
    
    fprintf( p, "Calcule $x$ e $y$, sabendo que os números da sucessão $%d, x, y$ são inversamente proporcionais aos da sucessão $%d, %d, %d$.}\\vspace{2mm}\\\\\\parbox{\\linewidth}{\n\n", D[ii].a, D[ii].m/D[ii].a, D[ii].m/D[ii].b, D[ii].m/D[ii].c );
    
    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n}\n\n", p );
    
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//








//############################################ Questão 03 ############################################//
void Q3(){
    
    int i, j, ii, n=0, k, A,
    a,   a1=2,  a2=30,
    b,          b2=30;
    
    
    for( a=a1; a<=a2; a++ ){
        for( b=a+1; b<=b2; b++ ){
            if( mdc(a,b)!=1 ){
                n++;
            }
        }
    }
    
    
//     printf("%d\n",n);
    struct{ int a, b; } D[n];
    i=0;
    for( a=a1; a<=a2; a++ ){
        for( b=a+1; b<=b2; b++ ){
            if( mdc(a,b)!=1 ){
                D[i].a = a;
                D[i].b = b;
                i++;
            }
        }
    }
    
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    
    ii = rand()%n;
    
    k = mdc( D[ii].a, D[ii].b );
        
    sprintf( alt[0], "Área $=%d$ m$^2$\n", D[ii].a * D[ii].b );
    do{
        for( i=1; i<5; i++ ){
            do{
                A = D[ii].a * D[ii].b + rand()%21 - 10;
            } while( A <= 0 );
            sprintf( alt[i], "Área $=%d$ m$^2$\n", A );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    
    FILE *p = fopen( "Q3.tex", "w+" );
    
    fputs( "\\parbox{\\linewidth}{\n\n\% QUESTAO\n", p );
    
    fprintf( p, "Calcule a área de um retângulo que tem perímetro $%d$\\,m, sabendo que a razão entre seu comprimento e sua largura é $\\frac{%d}{%d}$.}\\vspace{2mm}\\\\\\parbox{\\linewidth}{\n\n", 2*(D[ii].a+D[ii].b), D[ii].a/k, D[ii].b/k );
    
    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n}\n\n", p );
    
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//

