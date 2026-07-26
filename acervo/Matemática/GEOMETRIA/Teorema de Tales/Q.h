/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, x[5], k1, k2, c1, c2, a, b, c, d, p, rnd;
    
    char valores[4][10];
        
    FILE *p0 = fopen( "Q1.tex", "w+" );

    do{
        for( i=4; i>=0; i-- ){
            do{
                k1 = 2+rand()%4;
                k2 = 2+rand()%4;
            } while( k1==k2 );
            do{
                c1 = 1+rand()%4;
                c2 = 1+rand()%4;
            } while( c1==c2 );
            a = k1*c1;
            b = k1*c2;
            c = k2*c1;
            d = k2*c2;
            p = a+b+c+d;
            sprintf( valores[0], "%d", a );
            sprintf( valores[1], "%d", b );
            sprintf( valores[2], "%d", c );
            sprintf( valores[3], "%d", d );
            rnd = rand()%4;
            x[i] = atoi( valores[rnd] );
            strncpy( valores[rnd], "x", sizeof valores[rnd] );
        }
    } while( dist_alt(x) );

    float escala = 5.4/1.2/p;

    fputs( "\% QUESTAO\n", p0 );
    fputs( "Na figura\n\n", p0 );

    fprintf( p0, "\\tikz[>=latex,scale=%f,rotate=0,thick]{\n", escala );
    fprintf( p0, "\\pgfmathsetmacro{\\a}{%d};\n", a );
    fprintf( p0, "\\pgfmathsetmacro{\\b}{%d};\n", b );
    fprintf( p0, "\\pgfmathsetmacro{\\c}{%d};\n", c );
    fprintf( p0, "\\pgfmathsetmacro{\\d}{%d};\n", d );
    fprintf( p0, "\\pgfmathsetmacro{\\p}{%d};\n", p );

    fputs( "\\pgfmathsetmacro{\\kr}{0.15*\\p};\n", p0 );
    fputs( "\\pgfmathsetmacro{\\angr}{90-\\a-\\b};\n", p0 );
    fputs( "\\pgfmathsetmacro{\\ar}{0.15*\\p/sin(\\angr)};\n", p0 );
    fputs( "\\pgfmathsetmacro{\\br}{0.65*\\p/sin(\\angr)};\n", p0 );

    fputs( "\\pgfmathsetmacro{\\ks}{1.05*\\p};\n", p0 );
    fputs( "\\pgfmathsetmacro{\\angs}{90+\\c+\\d};\n", p0 );
    fputs( "\\pgfmathsetmacro{\\as}{0.15*\\p/sin(\\angs)};\n", p0 );
    fputs( "\\pgfmathsetmacro{\\bs}{0.65*\\p/sin(\\angs)};\n", p0 );

    fputs( "\\draw[<->] (0,0)--(1.2*\\p,0);\n", p0 );
    fputs( "\\node[above] at (0,0) {$c$};\n", p0 );
    fputs( "\\draw[<->] (0,\\b/2+\\d/2)--(1.2*\\p,\\b/2+\\d/2);\n", p0 );
    fputs( "\\node[above] at (0,\\b/2+\\d/2) {$b$};\n", p0 );
    fputs( "\\draw[<->] (0,\\p/2)--(1.2*\\p,\\p/2);\n", p0 );
    fputs( "\\node[above] at (0,\\p/2) {$a$};\n", p0 );

    fputs( "\\draw[<->] ($(\\kr,0)+(\\angr-180:\\ar)$)--($(\\kr,0)+(\\angr:\\br)$);\n", p0 );
    fputs( "\\node[left] at ($(\\kr,0)+(\\angr:\\br)$) {$r$};\n", p0 );
    fputs( "\\pgfmathsetmacro{\\br}{0.25*(\\b+\\d)/sin(\\angr)};\n", p0 );
    fprintf( p0, "\\node[left] at ($(\\kr,0)+(\\angr:\\br)$) {$%s$};\n", valores[1] );
    fputs( "\\pgfmathsetmacro{\\br}{(0.5*(\\b+\\d)+0.25*(\\a+\\c))/sin(\\angr)};\n", p0 );
    fprintf( p0, "\\node[left] at ($(\\kr,0)+(\\angr:\\br)$) {$%s$};\n", valores[0] );

    fputs( "\\draw[<->] ($(\\ks,0)+(\\angs-180:\\as)$)--($(\\ks,0)+(\\angs:\\bs)$);\n", p0 );
    fputs( "\\node[right] at ($(\\ks,0)+(\\angs:\\bs)$) {$s$};\n", p0 );
    fputs( "\\pgfmathsetmacro{\\bs}{0.25*(\\b+\\d)/sin(\\angs)};\n", p0 );
    fprintf( p0, "\\node[right] at ($(\\ks,0)+(\\angs:\\bs)$) {$%s$};\n", valores[3] );
    fputs( "\\pgfmathsetmacro{\\bs}{(0.5*(\\b+\\d)+0.25*(\\a+\\c))/sin(\\angs)};\n", p0 );
    fprintf( p0, "\\node[right] at ($(\\ks,0)+(\\angs:\\bs)$) {$%s$};\n", valores[2] );

    fputs( "}\n\n", p0 );

    fputs( "temos $a\\,//\\,b\\,//\\,c$. Calcule $x$.\n\n", p0 );
    
    fputs( "\% ALTERNATIVAS 6\n", p0 );
    fprintf( p0, "$x=%d$\n", x[0] );
    fprintf( p0, "$x=%d$\n", x[1] );
    fprintf( p0, "$x=%d$\n", x[2] );
    fprintf( p0, "$x=%d$\n", x[3] );
    fprintf( p0, "$x=%d$\n", x[4] );
    fputs( "\n\n\n", p0 );

    fclose(p0);
    
}
/***************************************************************************/



/******************************** QUESTÃO 2 ********************************/
void Q2(){
    int i, x[5], k1, k2, c1, c2, a, b, c, d, p, rnd;
    
    char valores[4][10];
        
    FILE *p0 = fopen( "Q2.tex", "w+" );

    do{
        for( i=4; i>=0; i-- ){
            do{
                k1 = 2+rand()%4;
                k2 = 2+rand()%4;
            } while( k1==k2 );
            do{
                c1 = 1+rand()%4;
                c2 = 1+rand()%4;
            } while( c1==c2 );
            a = k1*c1;
            b = k1*c2;
            c = k2*c1;
            d = k2*c2;
            p = a+b+c+d;
            sprintf( valores[0], "%d", a );
            sprintf( valores[1], "%d", b );
            sprintf( valores[2], "%d", c );
            sprintf( valores[3], "%d", d );
            rnd = rand()%4;
            x[i] = atoi( valores[rnd] );
            strncpy( valores[rnd], "x", sizeof valores[rnd] );
        }
    } while( dist_alt(x) );

    float escala = 5.4/1.1/p;

    fputs( "\% QUESTAO\n", p0 );
    fputs( "No triângulo $ABC$\n\n", p0 );

    fprintf( p0, "\\hspace{-1mm}\\tikz[>=latex,scale=%f,rotate=0,thick]{\n", escala );
    fprintf( p0, "\\pgfmathsetmacro{\\a}{%d};\n", a );
    fprintf( p0, "\\pgfmathsetmacro{\\b}{%d};\n", b );
    fprintf( p0, "\\pgfmathsetmacro{\\c}{%d};\n", c );
    fprintf( p0, "\\pgfmathsetmacro{\\d}{%d};\n", d );
    fprintf( p0, "\\pgfmathsetmacro{\\p}{%d};\n", p );
    
    fputs( "\\coordinate (A) at (0,0);\n", p0 );
    fputs( "\\coordinate (B) at (\\p,0);\n", p0 );
    fputs( "\\coordinate (C) at ({\\a/(\\a+\\c)*\\p},0.5*\\p);\n", p0 );
    fputs( "\\coordinate (D) at ($(A)!\\b/(\\a+\\b)!(C)$);\n", p0 );
    fputs( "\\coordinate (E) at ($(B)!\\b/(\\a+\\b)!(C)$);\n", p0 );
    
    fputs( "\\draw (A)--(B)--(C)--cycle (D)--(E);\n", p0 );
    
    fputs( "\\node[above] at (C) {$A$};\n", p0 );
    fputs( "\\node[left] at (A) {$B$};\n", p0 );
    fputs( "\\node[right] at (B) {$C$};\n", p0 );
    fputs( "\\node[left] at (D) {$D$};\n", p0 );
    fputs( "\\node[right] at (E) {$E$};\n", p0 );
    
    fprintf( p0, "\\node[left]  at ($(C)!0.4!(D)$) {$%s$};\n", valores[0] );
    fprintf( p0, "\\node[left]  at ($(A)!0.6!(D)$) {$%s$};\n", valores[1] );
    fprintf( p0, "\\node[right] at ($(C)!0.4!(E)$) {$%s$};\n", valores[2] );
    fprintf( p0, "\\node[right] at ($(B)!0.6!(E)$) {$%s$};\n", valores[3] );

    fputs( "}\n\n", p0 );

    fputs( "temos $\\overline{BC}\\,//\\,\\overline{DE}$. Calcule $x$.\n\n", p0 );
    
    fputs( "\% ALTERNATIVAS 6\n", p0 );
    fprintf( p0, "$x=%d$\n", x[0] );
    fprintf( p0, "$x=%d$\n", x[1] );
    fprintf( p0, "$x=%d$\n", x[2] );
    fprintf( p0, "$x=%d$\n", x[3] );
    fprintf( p0, "$x=%d$\n", x[4] );
    fputs( "\n\n\n", p0 );

    fclose(p0);

}
/***************************************************************************/



