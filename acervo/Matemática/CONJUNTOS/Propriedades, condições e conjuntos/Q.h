//############################################ Questão 01 ############################################//
void Q1(){

    int i, j, ii, n=0, *rnd, x1, x2;

    for( x1=-5; x1<=5; x1++ ){
        if( x1==0 ) continue;
        for( x2=-5; x2<=5; x2++ ){
            if( x2==0 ) continue;
            if( x1!=x2 && x1!=-x2 ){
                n++;
            }
        }
    }


    // printf("%d\n",n);
    struct{ int b, c, x1, x2; } D[n];
    i=0;
    for( x1=-5; x1<=5; x1++ ){
        if( x1==0 ) continue;
        for( x2=-5; x2<=5; x2++ ){
            if( x2==0 ) continue;
            if( x1!=x2 && x1!=-x2 ){
                D[i].x1 = x1;
                D[i].x2 = x2;
                D[i].b = -(x1+x2);
                D[i].c = x1*x2;
                i++;
            }
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
            sprintf( alt[i], "$S=\\{%d,%d\\}$\n", D[ii].x1, D[ii].x2 );
        }
        free(rnd);
    } while( dist_alt( alt, strlen(alt[0]) ) );



    FILE *p = fopen( "Q1.tex", "w+" );

    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );

    fputs( "\% QUESTAO\n", p );

    if( D[ii].b==1 )
        fprintf( p, "Escreva um conjunto dado pela condição ``$x$ é um número inteiro tal que $x^2+x%+d=0$''.\n\n", D[ii].c );
    else if( D[ii].b==-1 )
        fprintf( p, "Escreva um conjunto dado pela condição ``$x$ é um número inteiro tal que $x^2-x%+d=0$''.\n\n", D[ii].c );
    else
        fprintf( p, "Escreva um conjunto dado pela condição ``$x$ é um número inteiro tal que $x^2%+dx%+d=0$''.\n\n", D[ii].b, D[ii].c );

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

