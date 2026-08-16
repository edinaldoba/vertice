//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, j, ii, n=0, *rnd, d, e;
    
    unsigned A;
    
    for( d=2; d<20; d++ ){
        for( e=101; e<200; e++ ){
            if( d==10 || (d*d*e*e)%200>0 ) continue;
//             printf( "%4d%4d  -  %12.5f\n", e, d, 0.005*d*d*e*e );
            n++;
        }
    }
    
    
//     printf("%d\n",n);
    struct{ int e, d; } D[n];
    i=0;
    for( d=2; d<20; d++ ){
        for( e=101; e<200; e++ ){
            if( d==10 || (d*d*e*e)%200>0 ) continue;
            D[i].e = e;
            D[i].d = d;
            i++;
        }
    }
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    
    rnd = randperm(n);
    ii = rnd[0];
    A = D[ii].d*D[ii].d*D[ii].e*D[ii].e/200;
    sprintf( alt[0], "$%d\\sqrt{3}$ km$^2$\n", A );
    do{
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "$%d\\sqrt{3}$ km$^2$\n", A+(rand()%(A/5)-A/10) );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    free(rnd);
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );
    
    fputs( "\% QUESTAO\n", p );
    

    fprintf( p, "(PUC-SP) Um mapa é feito em uma escala de $1$\\,cm para cada $%d$\\,km. O município onde se encontra a capital de certo estado está representado, nesse mapa, por um losango que tem um ângulo de $120^\\circ$ e cuja diagonal menor mede $%d{,}%d$\\,cm. Determine a área desse município.\n\n", D[ii].e, D[ii].d/10, D[ii].d%10 );
    
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

