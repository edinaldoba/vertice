/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, k, a, b, r, rd, *rnd;
    
    char *sinal = " -";
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    rnd = randperm(12);
    
    for( i=4; i>=0; i-- ){
        
        r = 2+rand()%4;
        
        a = r*(1+rnd[i]  );
        b = r*(1+rnd[i+1]);
        
        rd = rand()%4;
        
        if( b%a==0 ){
            sprintf( alt[i], "$x=0$ ou $x=%c%d$\n", sinal[(rd==2)||(rd==3)], b/a );
        }
        else{
            k = mdc( b, a );
            sprintf( alt[i], "$x=0$ ou $x=%c\\frac{%d}{%d}$\n", sinal[(rd==2)||(rd==3)], b/k, a/k );
        }
    }
    
    
    if( rd==0 )       a=-a;
    else if( rd==1 )        b=-b;
    else if( rd==2 ){ a=-a; b=-b; }
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Dada a equação do 2º grau\n\\[%dx^2%+dx=0\\]\nem que $c=0$. Coloque o fator comum em evidência e resolva a equação produto resultante para obter as raízes dessa equação.\\\\{\\bf Obs: }{\\it A fórmula de Bhaskara NÃO deve ser utilizada}.\n\n", a, b );

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

