/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, D, d, q;
    
    char sD[10], sd[10], sq[10];
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            
            do{ d = 32 + rand()%68; }while(d%10==0);
            sprintf( sd, "%3.1f", 0.1*d  );
            
            do{ q = 32 + rand()%68; }while(q%10==0);
            sprintf( sq, "%3.1f", 0.1*q  );
            
            D = d * q;
            sprintf( sD, "%5.2f", 0.01*D );
            
            sprintf( alt[i], "$%c{,}%c$\n", sq[0], sq[2] );
        }
        
    } while( dist_alt( alt ) );
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Considere uma divisão cujo dividendo seja $%.2s{,}%s$ e o divisor $%c{,}%c$. Calcule o quociente.\n\n", sD, &sD[3], sd[0], sd[2] );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);

}
/***************************************************************************/
