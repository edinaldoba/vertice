/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, b, c, x1, x2, *rnd;
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    rnd = randperm(9);
    
    for( i=4; i>=0; i-- ){
        
        x1 = 11+rnd[i];
        x2 = 11+rnd[i+1];
        
        if( rand()%2 ) x1=-x1;
        else           x2=-x2;
        
        sprintf( alt[i], "$x=%d$ ou $x=%d$\n", x1, x2 );
    }
    
    free(rnd);
    
    b = -(x1+x2);
    c = x1*x2;
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Dada a equação do 2º grau\n\\[x\\cdot (x%+d)=%d\\]\nObtenha a sua forma reduzida e calcule suas raízes pelo método que achar melhor.\n\n", b, -c );

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

