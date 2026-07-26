//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, ii, h, base, Base, c, b, k, A;
    
    struct{ int c, b, a, k; } terno[4] = { {3,4,5,10}, {5,12,13,4}, {8,15,17,3}, {7,24,25,2} };
    
//     printf( "%d  %d  %d\n", terno[0].a, terno[0].b, terno[0].c );
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    
    
    ii = rand()%4;
    do{
        k = 1 + rand() % terno[ii].k;
        c = k * terno[ii].c;
        b = k * terno[ii].b;
    
        Base = k * terno[ii].a;
        base = 4*Base/5 - rand() % (Base/2);
        h = 3*b/4 + rand() % (b/2);
        
        A = c * b + (Base + base) * h;
        
    } while( A%2==1 );
    
    A /= 2;
    
    sprintf( alt[0], "$%d$ m$^2$\n", A );
    
    
    do{
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "$%d$ m$^2$\n", A + rand() % (9*A/10) - 9*A/20 );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );
    
    fputs( "\% QUESTAO\n", p );
    

    fprintf( p, "Feito o levantamento de um terreno, foram determinados os dados indicados na figura abaixo. Nessas condições, qual é a área desse terreno?\n\n" );
    
    fprintf( p, "\\parbox{0.34\\linewidth}{\n\n" );
    
    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );

    fprintf( p, "\n\n} \\parbox{0.6\\linewidth}{\n" );

    fprintf( p, "\\tikz[scale=0.5]{\n" );
    fprintf( p, "\\draw[thick,fill=gray!30] (0,0) -- (0,4) -- (3.2,6.4) -- (5,4) -- (3.6,0) -- cycle;\n" );
    fprintf( p, "\\draw[dashed] (0,4) -- (5,4);\n" );

    fprintf( p, "\\draw[thick] ($(3.2,6.4)+({-asin(0.8)}:0.5)$) -- ($(3.2,6.4)+({-asin(0.8)}:0.5)+({-asin(0.8)-90}:0.5)$) -- ($(3.2,6.4)+({-asin(0.8)-90}:0.5)$);\n" );

    fprintf( p, "\\draw[thick] (0,0) rectangle (0.5,0.5) (0,3.5) rectangle (0.5,4);\n" );

    fprintf( p, "\\fill  (0.25,0.25) circle (2pt) (0.25,3.75) circle (2pt) ($($(3.2,6.4)+({-asin(0.8)}:0.5)$)!0.5!($(3.2,6.4)+({-asin(0.8)-90}:0.5)$)$) circle (2pt);\n" );

    fprintf( p, "\\node[below] at (1.8,0) {$%d$\\,m};\n", base );
    fprintf( p, "\\node[left] at (0,2) {$%d$\\,m};\n", h );
    fprintf( p, "\\node at (0.7,5.6) {$%d$\\,m};\n", b );
    fprintf( p, "\\node at (5.1,5.4) {$%d$\\,m};} }\n\n\n}\n\n\n", c );

    
    
    

    
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//

