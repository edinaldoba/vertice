//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, j, ii, n=0, *rnd, np, ap;
    
    for( np=4; np<=5; np++ ){
        for( ap=2000; ap<=4000; ap+=100 ){
            n++;
        }
    }
    
    
    // printf("%d\n",n);
    struct{ int np, ap, NP; } D[n];
    i=0;
    for( np=4; np<=5; np++ ){
        for( ap=2000; ap<=4000; ap+=100 ){
            D[i].ap = ap;
            D[i].np = np;
            D[i].NP = ap*np;
            i++;
        }
    }
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    

    do{
        rnd = randperm(n);
        for( i=4; i>=0; i-- ){
            ii = rnd[i];
            sprintf( alt[i], "$%d$ pessoas\n", D[ii].NP );
        }
        free(rnd);
    } while( dist_alt( alt, strlen(alt[0]) ) );

    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );
    
    fputs( "\% QUESTAO\n", p );
    

    fprintf( p, "(Unicamp-SP) Alguns jornais calculam o número de pessoas presentes em atos públicos considerando que cada metro quadrado é ocupado por $%d$ pessoas. Qual a estimativa do número de pessoas presentes numa praça de $%d$ m$^2$ que tenha ficado lotada para um comício, segundo essa avaliação?\n\n", D[ii].np, D[ii].ap );
    
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

