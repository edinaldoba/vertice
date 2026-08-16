/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, j, ii, A[228], n=0, *rnd, x, y, b, B, h;
    bool t;
    
    for( y=5; y<10; y++ ){
        for( x=y+1; x<15; x++ ){
            for( h=y+2; h<15; h++ ){
                for( b=x+2; b<20; b++ ){
                    for( B=b+1; B<25; B++ ){
                        if( ((B+b)*h)%2==1 ) continue;
                        A[n] = ( B + b ) * h / 2 - x * y;
                        t = false;
                        for( i=0; i<n; i++ ){
                            if( A[n] == A[i] ){
                                t = true;
                                break;
                            }
                        }
                        if( t ) continue;
                        n++;
                    }
                }
            }
        }
    }
    
//     printf("%d\n",n);
    struct{ int x, y, b, B, h, A; } D[n];
    i=0;
    for( y=5; y<10; y++ ){
        for( x=y+1; x<15; x++ ){
            for( h=y+2; h<15; h++ ){
                for( b=x+2; b<20; b++ ){
                    for( B=b+1; B<25; B++ ){
                        if( ((B+b)*h)%2==1 ) continue;
                        A[i] = ( B + b ) * h / 2 - x * y;
                        t = false;
                        for( j=0; j<i; j++ ){
                            if( A[i] == A[j] ){
                                t = true;
                                break;
                            }
                        }
                        if( t ) continue;
                        D[i].x = x;
                        D[i].y = y;
                        D[i].B = B;
                        D[i].b = b;
                        D[i].h = h;
                        D[i].A = A[i];
                        i++;
                    }
                }
            }
        }
    }
//     printf("%d\n",i);
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    
    do{
        rnd = randperm(n);
        for( i=4; i>=0; i-- ){
            ii = rnd[i];
            sprintf( alt[i], "$%d$ m$^2$\n", D[ii].A );
        }
        free(rnd);
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );
    
    fputs( "\% QUESTAO\n", p );
    
    fprintf( p, "Um terreno tem a forma de um trapézio de bases $%d$\\,m e $%d$\\,m, e altura $%d$\\,m. Nesse terreno, construiu-se uma piscina retangular de $%d$\\,m por $%d$\\,m. No restante do terreno foram colocadas pedras mineiras. Qual foi a área onde se colocou pedras?\n\n", D[ii].B, D[ii].b, D[ii].h, D[ii].x, D[ii].y );
    
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

