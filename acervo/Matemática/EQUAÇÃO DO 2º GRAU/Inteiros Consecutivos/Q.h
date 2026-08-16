/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, x1, x2, *rnd;
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    rnd = randperm(8);
    
    for( i=4; i>=0; i-- ){
        x1 = 1+rnd[i];
        x2 = -(x1+1);
        sprintf( alt[i], "$%d$ e $%d$ ou, $%d$ e $%d$\n", x1, x1+1, x2, x2+1 );
    }
    
    free(rnd);
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Determine dois números inteiros e consecutivos tais que a soma de seus quadrados seja $%d$.\\\\{\\bf Obs: }{\\it Considere esses números como sendo $x$ e $x+1$}.\n\n", x1*x1 + (x1+1)*(x1+1) );

    fputs( "\% ALTERNATIVAS 7\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
    free(alt);

}
/***************************************************************************/

