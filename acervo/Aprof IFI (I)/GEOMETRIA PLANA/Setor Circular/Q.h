//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, j, ii, n=0, *rnd, l, r, A;
    
    
    for( r=3; r<=15; r++ ){
        for( l=r+1; l<=15; l++ ){
            if( l==r || (l*r)%2==1 ) continue;
            n++;
        }
    }
    
    
//     printf("%d\n",n);
    struct{ int l, r; } D[n];
    i=0;
    for( r=3; r<=15; r++ ){
        for( l=r+1; l<=15; l++ ){
            if( l==r || (l*r)%2==1 ) continue;
            D[i].l = l;
            D[i].r = r;
            i++;
        }
    }
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    
    
    do{
        rnd = randperm(n);
        ii = rnd[0];
        for( i=0; i<5; i++ ){
            A = D[rnd[i]].l * D[rnd[i]].r / 2;
            sprintf( alt[i], "$%d$ cm$^2$\n", A );
        }
        free(rnd);
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );
    
    fputs( "\% QUESTAO\n", p );
    fputs( "Calcule a área do setor circular da figura abaixo.\n", p );
    
    fprintf( p, "\\parbox{0.4\\linewidth}{\n\n" );
    
    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    
    fprintf( p, "\n}\\parbox{0.5\\linewidth}{\n" );
    fprintf( p, "\\tikz[thick,scale=1.5,>=latex]{\n" );
    fprintf( p, "\\draw (0,0) circle (1);\n" );
    fprintf( p, "\\draw[fill=gray!50] (0,0) -- (10:1) arc (10:60:1) -- cycle;\n" );
    fprintf( p, "\\draw[<->] (10:1.15) arc (10:60:1.15);\n" );
    fprintf( p, "\\node[rotate=-52] at (37:1.4) {$%d$\\,cm};\n", D[ii].l );
    fprintf( p, "\\node[rotate=10,below] at (10:0.5) {$%d$\\,cm};\n", D[ii].r );
    fprintf( p, "\\node[left] at (0,0) {O};\n" );
    fprintf( p, "}}\n\n\n}\n\n\n" );
    
    
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//

