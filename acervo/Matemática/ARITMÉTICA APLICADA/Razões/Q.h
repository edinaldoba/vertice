/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, j, ii, n=0, *rnd,
    a, a1=2,  a2=19,
    b, b1=2,  b2=19,
    c, c1=10, c2=50,
    d;
    
    char *str[2] = { "maior", "menor" };
    
    
    for( a=a1; a<=a2; a++ ){
        for( b=b1; b<=b2; b++ ){
            for( c=c1; c<=c2; c++ ){
                if( a!=b && a!=c && mdc(a,b)==1 && (b*c)%a==0 && b*c/a<=c2 && b*c/a>=c1 ){
                    n++;
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
                if( a!=b && a!=c && mdc(a,b)==1 && (b*c)%a==0 && b*c/a<=c2 && b*c/a>=c1 ){
                    D[i].a = a;
                    D[i].b = b;
                    D[i].c = c;
                    D[i].d = b*c/a;
                    i++;
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
        
    sprintf( alt[0], "O %s número é $%d$\n", str[(int)(D[ii].d<D[ii].c)], D[ii].d );
    do{
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "O %s número é $%d$\n", str[(int)(D[ii].d<D[ii].c)], D[ii].d + rand()%19 - 9 );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    free(rnd);
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\parbox{\\linewidth}{\n\n\% QUESTAO\n", p );
    
    fprintf( p, "A razão entre dois números é $\\frac{%d}{%d}$, e o %s deles é $%d$. Qual é o %s?}\\vspace{2mm}\\\\\\parbox{\\linewidth}{\n\n", D[ii].a, D[ii].b, str[(int)(D[ii].d>D[ii].c)], D[ii].c, str[(int)(D[ii].d<D[ii].c)]  );
    
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

