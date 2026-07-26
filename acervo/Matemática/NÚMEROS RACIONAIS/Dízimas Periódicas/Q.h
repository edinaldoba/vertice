/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, num, *rnd;
    char snum[100];
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        do{ num=10+rand()%90; }while( num%10==0 || num%9==0 );
        
        sprintf( snum, "%.5f", 1.*num/90 );
        
        sprintf( alt[0], "Periodo $%c$\n", snum[3] );
        
        rnd = randperm(9);
        
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "Periodo $%d$\n", 1+rnd[i] );
        }
        
        free(rnd);
        
    } while( dist_alt( alt ) );
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Dada a fração geratriz $%d/90$. Determine o período do número decimal periódico resultante.\n\n", num );

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
