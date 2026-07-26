/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, ii, n, P, p, *rnd,
        a, a1=5, a2=24,
        b, b1=5, b2=24,
        c, c1=5, c2=24;
        
    unsigned A;
    
    char str[100];
        
    n=0;
    for( c=c1; c<c2; c++ ){
        for( b=c+1; b<b2; b++ ){
            for( a=b+1; a<=a2; a++ ){
                
                if( a>=b+c ) continue;
                
                p = a + b + c;
                if( p%2==1 ) continue;
                p /= 2;
                
                A = p * (p - a) * (p - b) * (p - c);
                if( !fatraiz( A, 2, str ) ) continue;
//                 A = sqrtf( A );
                
//                 printf( "%4d%4d%4d  -  %s\n", a, b, c, str );
                
                n++;
            }
        }
    }
    
//     printf("%d\n",n);
    
    struct{ int a, b, c; unsigned A; } D[n];
    i=0;
    for( c=c1; c<c2; c++ ){
        for( b=c+1; b<b2; b++ ){
            for( a=b+1; a<=a2; a++ ){
                
                if( a>=b+c ) continue;
                
                p = a + b + c;
                if( p%2==1 ) continue;
                p /= 2;
                
                A = p * (p - a) * (p - b) * (p - c);
                if( !fatraiz( A, 2, str ) ) continue;
//                 A = sqrtf( A );
                
                D[i].a = a;
                D[i].b = b;
                D[i].c = c;
                D[i].A = A;
                
                i++;
            }
        }
    }
    
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    
    do{
        rnd = randperm(n);
        for( i=4; i>=0; i-- ){
            ii = rnd[i];
            fatraiz( D[ii].A, 2, str );
            sprintf( alt[i], "$%s$ m$^2$\n", str );
        }
        free(rnd);
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    
    
    FILE *pp = fopen( "Q1.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", pp );
    
    fputs( "\% QUESTAO\n", pp );
    fprintf( pp, "Um terreno tem a forma triangular e as medidas de seus lados são $%d$\\,m, $%d$\\,m e $%d$\\,m. Qual é a área desse terreno?\n\n", D[ii].a, D[ii].b, D[ii].c );

    fputs( "\% ALTERNATIVAS 4\n", pp );
    fputs( alt[0], pp );
    fputs( alt[1], pp );
    fputs( alt[2], pp );
    fputs( alt[3], pp );
    fputs( alt[4], pp );
    fputs( "\n\n\n}\n\n\n", pp );
    fclose(pp);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//
