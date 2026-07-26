//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, j, ii, n=0, *rnd, l;
    
    for( l=10; l<=30; l++ ){
        n++;
    }
    
    
    // printf("%d\n",n);
    struct{ int l, A; } D[n];
    i=0;
    for( l=10; l<=30; l++ ){
        D[i].l = l;
        D[i].A = l*l;
        i++;
    }
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    
    rnd = randperm(n);
    do{
        for( i=4; i>=0; i-- ){
            ii = rnd[i];
            sprintf( alt[i], "$%d$ m$^2$\n", D[ii].A );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    free(rnd);
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );
    
    fputs( "\% QUESTAO\n", p );
    

    fprintf( p, "Um terreno tem a forma de um quadrado cuja diagonal mede $%d\\sqrt{2}$ m. Determine a área desse terreno.\n\n", D[ii].l );
    
    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n}\n\n\n", p );
    
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
//####################################################################################################//

