/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, b, c, x1, x2, *rnd;
    char sb[10];
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    rnd = randperm(9);
    
    for( i=4; i>=0; i-- ){
        
        x1 = 1+rnd[i];
        x2 = 1+rnd[i+1];
        
        if( rand()%2 ) x1=-x1;
        else           x2=-x2;
        
        sprintf( alt[i], "$x=%d$ ou $x=%d$\n", x1, x2 );
    }
    
    free(rnd);
    
    b = -(x1+x2);
    c = x1*x2;
    
    if( b==1 )       sprintf( sb, "+" );
    else if( b==-1 ) sprintf( sb, "-" );
    else             sprintf( sb, "%+d", b );
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Dada a equação do 2º grau\n\\[x^2%sx%+d=0\\]\nDescubra as raízes por meio da soma e do produto delas.\\\\{\\bf Obs: }{\\it A fórmula de Bhaskara NÃO deve ser utilizada.}\n\n", sb, c );
    
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

