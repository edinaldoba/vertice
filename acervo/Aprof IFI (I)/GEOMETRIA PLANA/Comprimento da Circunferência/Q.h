//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, ii, n, resp, *rnd,
        v, v1=200,  v2=900,  // Número de voltas
        d, d1=40,  d2=80;    // Diâmetro
    
    char sresp[100];
    
    struct{ char str[100]; } elemento[2] = { "raio", "diâmetro" };
        
    n=0;
    for( v=v1; v<=v2; v++ ){
        for( d=d1; d<=d2; d++ ){
            if( roundf(0.314*v*d) == 0.314*v*d ){
                n++;
            }
        }
    }
//     printf("%d\n",n);
    struct{ int v, d; } D[n];
    i=0;
    for( v=v1; v<=v2; v++ ){
        for( d=d1; d<=d2; d++ ){
            if( roundf(0.314*v*d) == 0.314*v*d ){
                D[i].v = v;
                D[i].d = d;
                i++;
            }
        }
    }
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    
    rnd = randperm(n);
    ii = rnd[0];
    sprintf( sresp, "%10.1f", D[ii].d * 0.0314 * D[ii].v );
    sprintf( alt[0], "$%.8s{,}%s$ metros\n", sresp, &sresp[9] );
    resp = D[ii].d * 0.0314 * D[ii].v;
    do{
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "$%d{,}%d$ metros\n", resp - rand()%(resp/4) + resp/8, rand()%10 );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    free(rnd);
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Uma roda de bicicleta tem %s de $%d$\\,cm. Qual é a distância percorrida pela bicicleta depois que a roda deu $%d$ voltas?\n\n", elemento[D[ii].d%2].str, D[ii].d/(2-D[ii].d%2), D[ii].v );

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
