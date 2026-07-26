/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

//############################################ Questão 01 ############################################//
void Q1(){
    
    int i, j, ii, n=0, *rnd, l;
    
    for( l=6; l<=30; l++ ){
        if( (l*l)%4==0 ) n++;
    }
    
    
    // printf("%d\n",n);
    struct{ int l, p, A; } D[n];
    i=0;
    for( l=6; l<=30; l++ ){
        if( (l*l)%4==0 ){
            D[i].l = l;
            D[i].p = 3*l;
            D[i].A = l*l/4;
            i++;
        }
    }
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    
    rnd = randperm(n);
    do{
        for( i=4; i>=0; i-- ){
            ii = rnd[i];
            sprintf( alt[i], "$%d\\sqrt{3}$ cm$^2$\n", D[ii].A );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    free(rnd);
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );
    
    fputs( "\% QUESTAO\n", p );
    

    fprintf( p, "O perímetro de um triângulo equilátero é $%d$ cm. Calcule a área desse triângulo.\n\n", D[ii].p );
    
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








//############################################ Questão 02 ############################################//
void Q2(){

    int i, j, ii, n=0, *rnd, l;

    for( l=6; l<=30; l+=2 ){
        n++;
    }


    // printf("%d\n",n);
    struct{ int l, h; } D[n];
    i=0;
    for( l=6; l<=30; l+=2 ){
        D[i].l = l;
        D[i].h = l/2;
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
            sprintf( alt[i], "$%d\\sqrt{3}$ cm\n", D[ii].h );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    free(rnd);


    FILE *p = fopen( "Q2.tex", "w+" );

    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );

    fputs( "\% QUESTAO\n", p );


    fprintf( p, "De uma placa de alumínio foi recortada uma região triangular equilátera de lado $%d$ cm. Qual é a altura dessa região que foi recortada?\n\n", D[ii].l );

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

