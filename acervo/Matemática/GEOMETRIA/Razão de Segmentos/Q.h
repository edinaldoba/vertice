/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, x[5], y[5], k, c1, c2;
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    do{
        for( i=4; i>=0; i-- ){
            k = 2+rand()%5; // Constante de proporcionalidade
            do{
                c1 = 2+rand()%5;
                c2 = 2+rand()%5;
            } while( c1==c2 );
            x[i] = k*c1;
            y[i] = k*c2;
        }
    } while( dist_alt(x) || dist_alt(y) );
    
    float escala = 5.4/(x[0]+y[0]);
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Determine a medida dos segmentos $\\overline{AB}$ e $\\overline{BC}$ da figura\n\n\\tikz[scale=%f]{\\draw[thick] (0,0) -- (%d,0);\\fill (0,0) circle (%fpt);\\node[above] at (0,0) {$A$};\\fill (%d,0) circle (%fpt);\\node[above] at (%d,0) {$B$};\\fill (%d,0) circle (%fpt);\\node[above] at (%d,0) {$C$};\\node[below] at (%f,0) {$x$};\\node[below] at (%f,0) {$y$};}\n\nsabendo que $\\frac{AB}{BC}=\\frac{%d}{%d}$ e $AC=%d$\\,cm.\n\n", escala, x[0]+y[0], 2./escala, x[0], 2./escala, x[0], x[0]+y[0], 2./escala, x[0]+y[0], 0.5*x[0], x[0]+0.5*y[0], c1, c2, x[0]+y[0] );
    fputs( "\% ALTERNATIVAS 8\n", p );
    fprintf( p, "$x=%d$\\,cm e $y=%d$\\,cm\n", x[0], y[0] );
    fprintf( p, "$x=%d$\\,cm e $y=%d$\\,cm\n", x[1], y[1] );
    fprintf( p, "$x=%d$\\,cm e $y=%d$\\,cm\n", x[2], y[2] );
    fprintf( p, "$x=%d$\\,cm e $y=%d$\\,cm\n", x[3], y[3] );
    fprintf( p, "$x=%d$\\,cm e $y=%d$\\,cm\n", x[4], y[4] );
    fputs( "\n\n\n", p );
    
    fclose(p);
    
}
/***************************************************************************/



/******************************** QUESTÃO 2 ********************************/
void Q2(){
    int i, x[5], y[5], k, c1, c2;
    
    FILE *p = fopen( "Q2.tex", "w+" );
    
    do{
        for( i=4; i>=0; i-- ){
            k = 2+rand()%5; // Constante de proporcionalidade
            do{
                c1 = 2+rand()%5;
                c2 = 2+rand()%5;
            } while( c1==c2 );
            x[i] = k*(c1+c2);
            y[i] = k*c1;
        }
    } while( dist_alt(x) || dist_alt(y) );
    
    float escala = 5.4/x[0];
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Na figura\n\n\\tikz[scale=%f,rotate=-5]{\\draw[thick] (0,0) -- (%d,0);\\fill (0,0) circle (%fpt);\\node[above] at (0,0) {$A$};\\fill (%d,0) circle (%fpt);\\node[above] at (%d,0) {$B$};\\fill (%d,0) circle (%fpt);\\node[above] at (%d,0) {$C$};\\node at (%f,%f) {$x$};\\node[below] at (%f,0) {$y$};\\draw[decorate,decoration={brace,raise=3pt}] (%d,%f) -- (0,%f);\\node[below] at (%f,0) {$%d$\\,cm}}\n\n$\\frac{AB}{AC}=\\frac{%d}{%d}$ e $BC=%d$\\,cm. Determine os valores de $x$ e $y$\n\n", escala, x[0], 2./escala, y[0], 2./escala, y[0], x[0], 2./escala, x[0], 0.5*x[0], -0.8/escala, 0.5*y[0], x[0], -0.4/escala, -0.4/escala, y[0]+0.5*(x[0]-y[0]), x[0]-y[0], c1, c1+c2, x[0]-y[0] );
    fputs( "\% ALTERNATIVAS 8\n", p );
    fprintf( p, "$x=%d$\\,cm e $y=%d$\\,cm\n", x[0], y[0] );
    fprintf( p, "$x=%d$\\,cm e $y=%d$\\,cm\n", x[1], y[1] );
    fprintf( p, "$x=%d$\\,cm e $y=%d$\\,cm\n", x[2], y[2] );
    fprintf( p, "$x=%d$\\,cm e $y=%d$\\,cm\n", x[3], y[3] );
    fprintf( p, "$x=%d$\\,cm e $y=%d$\\,cm\n", x[4], y[4] );
    fputs( "\n\n\n", p );
    
    fclose(p);
    
}
/***************************************************************************/



/******************************** QUESTÃO 3 ********************************/
void Q3(){
    int i, P[5], b, h, k, c1, c2;
    
    FILE *p = fopen( "Q3.tex", "w+" );
    
    do{
        for( i=4; i>=0; i-- ){
            k = 2+rand()%5; // Constante de proporcionalidade
            do{
                c1 = 2+rand()%5;
                c2 = 2+rand()%5;
            } while( c1<=c2 );
            b = k*c1;
            h = k*c2;
            P[i] = 2*b + 2*h;
        }
    } while( dist_alt(P) );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "A razão entre a base e a altura de um retângulo é $\\frac{%d}{%d}$. Se a base mede $%d$\\,cm, determine o perímetro do retângulo.\n\n", c1, c2, b );
    fputs( "\% ALTERNATIVAS 8\n", p );
    fprintf( p, "Perímetro $=%d$\\,cm\n", P[0] );
    fprintf( p, "Perímetro $=%d$\\,cm\n", P[1] );
    fprintf( p, "Perímetro $=%d$\\,cm\n", P[2] );
    fprintf( p, "Perímetro $=%d$\\,cm\n", P[3] );
    fprintf( p, "Perímetro $=%d$\\,cm\n", P[4] );
    fputs( "\n\n\n", p );
    
    fclose(p);
    
}
/***************************************************************************/
