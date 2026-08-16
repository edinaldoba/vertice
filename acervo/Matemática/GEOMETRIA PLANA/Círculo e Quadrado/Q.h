/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, j, ii, n=0, *rnd, P;
    
    unsigned A;
    
    for( P=56; P<1000; P+=8 ){
        if( P%10==0 ) continue;
        n++;
    }
    
    
//     printf("%d\n",n);
    struct{ int P; } D[n];
    i=0;
    for( P=56; P<1000; P+=8 ){
        if( P%10==0 ) continue;
        D[i].P = P;
        i++;
    }
    
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    
    rnd = randperm(n);
    ii = rnd[0];
    
    int a = D[ii].P*D[ii].P/16,
        b = D[ii].P*D[ii].P/64;
        
    sprintf( alt[0], "$%d-%d\\pi$ cm$^2$\n", a, b );
    do{
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "$%d-%d\\pi$ cm$^2$\n", a - rand()%(a/4) + a/8, b - rand()%(b/4) + b/8 );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    free(rnd);
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );
    
    fputs( "\% QUESTAO\n", p );
    

    fprintf( p, "O perímetro do quadrado $ABCD$ da figura abaixo é $%d$\\,cm. Calcule a área da região na cor cinza dessa figura.\n\n", D[ii].P );
    
    
    
    fputs( "\\parbox{0.55\\linewidth}{\n\n", p );
    
    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    
    fputs( "\n}\\parbox{0.45\\linewidth}{\n\n", p );
    
    fprintf( p, "\\tikz[thick]{\n" );
    fprintf( p, "\\draw[dashed,fill=gray!50] (0,0) -- (2,0) -- (2,2) -- (0,2) -- cycle;\n" );
    fprintf( p, "\\draw[fill=white] (0,0) circle (1) (2,0) circle (1) (2,2) circle (1) (0,2) circle (1);\n" );
    fprintf( p, "\\draw[dashed] (0,0) -- (2,0) -- (2,2) -- (0,2) -- cycle;\n" );
    fprintf( p, "\\fill (0,0) circle (2pt) (2,0) circle (2pt) (2,2) circle (2pt) (0,2) circle (2pt);\n" );
    fprintf( p, "\\node at (-0.25,-0.25) {$A$};\n" );
    fprintf( p, "\\node at (2.25,2.25) {$C$};\n" );
    fprintf( p, "\\node at (2.25,-0.25) {$B$};\n" );
    fprintf( p, "\\node at (-0.25,2.25) {$D$};\n" );
    fprintf( p, "} }\n\n\n}\n\n\n" );
    

    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//

