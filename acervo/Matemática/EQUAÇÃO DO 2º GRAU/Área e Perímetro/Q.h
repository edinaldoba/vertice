/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, x, comp, larg, tela, *rnd;
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    rnd = randperm(7);
    x = rnd[0]+3;
    comp = 60+10*(rand()%4);
    larg = comp-10-10*(rand()%3);
    tela = (comp-2*x)*(larg-2*x);
    
    sprintf( alt[0], "$x=%d$\\,cm\n", x );
    
    for( i=1; i<5; i++ ){
        sprintf( alt[i], "$x=%d$\\,cm\n", rnd[i]+3 );
    }
    free(rnd);
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Um quadro tem forma retangular de dimensões externas $%d\\times %d$\\,cm. A moldura tem largura $x$ uniforme. Calcule a largura, sabendo que a área da tela é $%d$\\,cm$^2$.\n", comp, larg, tela );
    fprintf( p, "\\begin{center}\n" );
    fprintf( p, "\\begin{tikzpicture}[scale=%f,thick]\n", 5.e1/(comp+larg) );
    fprintf( p, "\\draw[fill=gray!50] (0,0) rectangle (%d,%d);\n", comp/10, larg/10 );
    fprintf( p, "\\draw               (0,0) rectangle (%d,%d);\n", comp/10, larg/10 );
    fprintf( p, "\\node[below] at (%.1f, 0  ) {$%d$\\,cm};\n", 0.05*comp, comp );
    fprintf( p, "\\node[right] at (%d  ,%.1f) {$%d$\\,cm};\n", comp/10, 0.05*larg, larg );
    fprintf( p, "\\draw[thick,fill=gray!0] (%.1f,%.1f) rectangle (%.1f,%.1f);\n", 0.005*(comp+larg), 0.005*(comp+larg), 0.1*comp-0.005*(comp+larg), 0.1*larg-0.005*(comp+larg) );
    fprintf( p, "\\node at (%.1f,%.1f) {\\large TELA};\n", 5.e-2*comp, 5.e-2*larg );
    fprintf( p, "\\end{tikzpicture}\n" );
    fprintf( p, "\\end{center}\n\n" );

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


