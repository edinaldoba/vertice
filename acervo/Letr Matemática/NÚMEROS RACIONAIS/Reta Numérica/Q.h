/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, j, *rnd;
    
    float num[] = {-5./3,-4./3,-2./3,-1./3, 1./3, 2./3, 4./3, 5./3, -3./2, -1./2, 1./2, 3./2};
    
    char *snum[12] = { "-\\frac{5}{3}", "-\\frac{4}{3}", "-\\frac{2}{3}", "-\\frac{1}{3}", "\\frac{1}{3}", "\\frac{2}{3}", "\\frac{4}{3}", "\\frac{5}{3}", "-1{,}5", "-0{,}5", "0{,}5", "1{,}5" };
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            rnd = randperm(12);
            sprintf( alt[i], "$A\\left(%s\\right),\\,\\,B\\left(%s\\right)\\,\\,\\textnormal{e}\\,\\,\\,C\\left(%s\\right)$\n", snum[rnd[0]], snum[rnd[1]], snum[rnd[2]] );
        }
    } while( dist_alt( alt ) );
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "A figura a seguir traz um segmento de reta numerado de $-2$ a $2$\n\n" );
    fprintf( p, "\\vspace{-4mm}\\begin{center}\n" );
    fprintf( p, "\\noindent\\tikz[scale=1.33,thick]{\n" );
    fprintf( p, "\\coordinate (A) at (%f,0);\n", num[rnd[0]] );
    fprintf( p, "\\coordinate (B) at (%f,0);\n", num[rnd[1]] );
    fprintf( p, "\\coordinate (C) at (%f,0);\n", num[rnd[2]] );
    fprintf( p, "\\draw (-2,0) -- (2,0);\n" );
    fprintf( p, "\\foreach \\i in { -2,...,2 }{\n" );
    fprintf( p, "\\draw (\\i,-0.1) -- (\\i,0.1);\n" );
    fprintf( p, "\\node[below] at (\\i,-0.1) {$\\i$};}\n" );
    fprintf( p, "\\node[above] at (A) {$A$};\n" );
    fprintf( p, "\\fill (A) circle (1.3pt);\n" );
    fprintf( p, "\\node[above] at (B) {$B$};\n" );
    fprintf( p, "\\fill (B) circle (1.3pt);\n" );
    fprintf( p, "\\node[above] at (C) {$C$};\n" );
    fprintf( p, "\\fill (C) circle (1.3pt);\n" );
    fprintf( p, "}\n" );
    fprintf( p, "}\\end{center}\\vspace{-6mm}\n" );
    fprintf( p, "Determine os números racionais localizados nos pontos $A$, $B$ e $C$.\n\n" );

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
