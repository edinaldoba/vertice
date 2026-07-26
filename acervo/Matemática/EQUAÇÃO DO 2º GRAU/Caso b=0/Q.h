/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, j, k, a, c, n, d, *rnd;
    
    char sa[10];
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    rnd = randperm(9);
    
    for( i=4; i>=0; i-- ){
        
        n = 1+rnd[i];
        d = 1+rnd[i+1];
        
        if( n%d==0 ){
            sprintf( alt[i], "$x=%d$ ou $x=-%d$\n", n/d, n/d );
        }
        else{
            k = mdc( n, d );
            sprintf( alt[i], "$x=\\frac{%d}{%d}$ ou $x=-\\frac{%d}{%d}$\n", n/k, d/k, n/k, d/k );
        }
    }
    
    a = d*d;
    c = n*n;
    if( rand()%2 ) a=-a;
    else           c=-c;
    
    if( a==1 )       strncpy( sa, "", sizeof sa);
    else if( a==-1 ) sprintf( sa, "-" );
    else             sprintf( sa, "%d", a );
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Dada a equação do 2º grau\n\\[%sx^2%+d=0\\]\nem que $b=0$. Calcule as raízes dessa equação.\\\\{\\bf Obs: }{\\it A fórmula de Bhaskara NÃO deve ser utilizada}.\n\n", sa, c );

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

