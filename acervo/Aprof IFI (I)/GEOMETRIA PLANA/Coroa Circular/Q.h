//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, j, ii, n=0, *rnd, l, A;
    
    for( l=10; l<100; l+=2 ){
        n++;
    }
    
    
//     printf("%d\n",n);
    struct{ int l; } D[n];
    i=0;
    for( l=10; l<100; l+=2 ){
        D[i].l = l;
        i++;
    }
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    
    rnd = randperm(n);
    ii = rnd[0];
    A = D[ii].l * D[ii].l / 4;
    sprintf( alt[0], "$%d\\pi$ cm$^2$\n", A );
    do{
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "$%d\\pi$ cm$^2$\n", A - rand()%(A/3) + A/6 );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    free(rnd);
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );
    
    fputs( "\% QUESTAO\n", p );
    

    fprintf( p, "(UFU-MG) Dado um triângulo equilátero de lado $%d$\\,cm, qual a área da coroa circular limitada entre as circunferências inscrita e circunscrita nesse triângulo?\n\n", D[ii].l );
    
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

