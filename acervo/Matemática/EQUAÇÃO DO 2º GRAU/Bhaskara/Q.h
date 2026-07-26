/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, b, c, x1, x2, *rnd;
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    rnd = randperm(9);
    
    for( i=4; i>=0; i-- ){
        x1 = 1+rnd[i];
        if( rand()%2 ) x1=-x1;
        x2 = x1;
        sprintf( alt[i], "$x=%d$ ou $x=%d$\n", x1, x2 );
    }
    
    free(rnd);
    
    b = -(x1+x2);
    c = x1*x2;
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Dada a equação do 2º grau\n\\[x^2%+dx%+d=0\\]\nUse a fórmula de Bhaskara para determinar suas raízes.\n\n", b, c );

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



/******************************** QUESTÃO 2 ********************************/
void Q2(){
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
        if( rand()%2 ) x2=-x2;
        sprintf( alt[i], "$x=%d$ ou $x=%d$\n", x2, x1 );
    }
    
    free(rnd);
    
    b = (x1+x2);
    c = -x1*x2;
    
    if( b==1 )       sprintf( sb, "+" );
    else if( b==-1 ) sprintf( sb, "-" );
    else             sprintf( sb, "%+d", b );
    
    
    FILE *p = fopen( "Q2.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Use a fórmula de Bhaskara\n" );
    fprintf( p, "\\[x=\\frac{-b\\pm\\sqrt{\\Delta}}{2a}\\]\n" );
    fprintf( p, "em que\n" );
    fprintf( p, "\\[\\Delta=b^2-4ac\\]\n" );
    fprintf( p, "para resolver a equação do 2º grau\n\\[-x^2%sx%+d=0\\]\n", sb, c );
    fprintf( p, "\\vspace{-7mm}\n\n" );

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



/******************************** QUESTÃO 3 ********************************/
void Q3(){
    int i, a, b, c, Delta, D, *rnd;
    
    char sa[10], sb[10],
        *str[4] = { "nenhuma raiz", "nenhuma raiz", "duas raízes iguais", "duas raízes distintas" };
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    b = 1+rand()%7;
    a = 1+rand()%7;
    c = 1+b*b/(4*a)+rand()%3;
    
    if(rand()%2) { b=-b; }
    if(rand()%2) { a=-a; c=-c; }
    
    if( a==1 )       strncpy( sa, "", sizeof sa);
    else if( a==-1 ) sprintf( sa, "-"      );
    else             sprintf( sa, "%d", a  );
    
    if( b==1 )       sprintf( sb, "+"      );
    else if( b==-1 ) sprintf( sb, "-"      );
    else             sprintf( sb, "%+d", b );

    
    Delta = b*b-4*a*c;
    sprintf( alt[0], "$\\Delta=%d$ e %s\n", Delta, str[0] );
    
    for( i=1; i<5; i++ ){
        
        do{ D=rand()%169-84; } while( D==Delta );
        
        sprintf( alt[i], "$\\Delta=%d$ e %s\n", D, str[rand()%4] );
    }
    
    
    FILE *p = fopen( "Q3.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Dada a equação do 2º grau\n\\[%sx^2%sx%+d=0\\]\nDetermine o discriminante $(\\Delta)$ e o consequente número de raízes reais dessa equação.\n\n", sa, sb, c );

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
