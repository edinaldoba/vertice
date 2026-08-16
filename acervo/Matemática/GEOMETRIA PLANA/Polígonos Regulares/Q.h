/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

//############################################ Questão 01 ############################################//
void Q1( int **rnd ){
    
    int i, *rd, nn = 6 * (rnd[2][0] + 1);
    
    struct{ char str[100]; }
        poligono[3] = { "triângulo equilátero", "quadrado", "hexágono regular" },
        elemento[3] = { "lado", "apótema", "raio da circunferência circunscrita" },
        preposicao[3] = { "do", "do", "no" }, respostas[3][3][3];
        
        
        sprintf( respostas[0][0][1].str, "$%d\\sqrt{3}$ m\n", 2*nn );
        sprintf( respostas[0][0][2].str, "$%d\\sqrt{3}$ m\n", nn );
        sprintf( respostas[0][1][0].str, "$%d\\sqrt{3}$ m\n", nn/6 );
        sprintf( respostas[0][1][2].str, "$%d$ m\n", nn/2 );
        sprintf( respostas[0][2][0].str, "$%d\\sqrt{3}$ m\n", nn/3 );
        sprintf( respostas[0][2][1].str, "$%d$ m\n", 2*nn );
        
        sprintf( respostas[1][0][1].str, "$%d$ m\n", 2*nn );
        sprintf( respostas[1][0][2].str, "$%d\\sqrt{2}$ m\n", nn );
        sprintf( respostas[1][1][0].str, "$%d$ m\n", nn/2 );
        sprintf( respostas[1][1][2].str, "$%d\\sqrt{2}$ m\n", nn/2 );
        sprintf( respostas[1][2][0].str, "$%d\\sqrt{2}$ m\n", nn/2 );
        sprintf( respostas[1][2][1].str, "$%d\\sqrt{2}$ m\n", nn );
        
        sprintf( respostas[2][0][1].str, "$%d\\sqrt{3}$ m\n", 2*nn/3 );
        sprintf( respostas[2][0][2].str, "$%d$ m\n", nn );
        sprintf( respostas[2][1][0].str, "$%d\\sqrt{3}$ m\n", nn/2 );
        sprintf( respostas[2][1][2].str, "$%d\\sqrt{3}$ m\n", nn/2 );
        sprintf( respostas[2][2][0].str, "$%d$ m\n", nn );
        sprintf( respostas[2][2][1].str, "$%d\\sqrt{3}$ m\n", 2*nn/3 );
        
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    
    strncpy( alt[0], respostas[ rnd[1][0] ] [ rnd[0][0] ] [ rnd[0][1] ].str, 1000 );
    
    do{
        for( i=1; i<5; i++ ){
            rd = randperm(3);
            strncpy( alt[i], respostas[ rand()%3 ] [ rd[0] ] [ rd[1] ].str, 1000 );
            free(rd);
        }
        
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Determine o %s %s %s, sabendo que o %s mede $%d$\\,m.\n\n", elemento[rnd[0][0]].str, preposicao[rnd[0][0]].str, poligono[rnd[1][0]].str, elemento[rnd[0][1]].str, nn );

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





//############################################ Questão 01 ############################################//
void Q2( int **rnd ){
    
    int i, *rd, nn = 6 * (rnd[2][1] + 1);
    
    struct{ char str[100]; }
        poligono[3] = { "triângulo equilátero", "quadrado", "hexágono regular" },
        elemento[3] = { "lado", "apótema", "raio da circunferência circunscrita" },
        preposicao[3] = { "do", "do", "no" }, respostas[3][3][3];
        
        
        sprintf( respostas[0][0][1].str, "$%d\\sqrt{3}$ m\n", 2*nn );
        sprintf( respostas[0][0][2].str, "$%d\\sqrt{3}$ m\n", nn );
        sprintf( respostas[0][1][0].str, "$%d\\sqrt{3}$ m\n", nn/6 );
        sprintf( respostas[0][1][2].str, "$%d$ m\n", nn/2 );
        sprintf( respostas[0][2][0].str, "$%d\\sqrt{3}$ m\n", nn/3 );
        sprintf( respostas[0][2][1].str, "$%d$ m\n", 2*nn );
        
        sprintf( respostas[1][0][1].str, "$%d$ m\n", 2*nn );
        sprintf( respostas[1][0][2].str, "$%d\\sqrt{2}$ m\n", nn );
        sprintf( respostas[1][1][0].str, "$%d$ m\n", nn/2 );
        sprintf( respostas[1][1][2].str, "$%d\\sqrt{2}$ m\n", nn/2 );
        sprintf( respostas[1][2][0].str, "$%d\\sqrt{2}$ m\n", nn/2 );
        sprintf( respostas[1][2][1].str, "$%d\\sqrt{2}$ m\n", nn );
        
        sprintf( respostas[2][0][1].str, "$%d\\sqrt{3}$ m\n", 2*nn/3 );
        sprintf( respostas[2][0][2].str, "$%d$ m\n", nn );
        sprintf( respostas[2][1][0].str, "$%d\\sqrt{3}$ m\n", nn/2 );
        sprintf( respostas[2][1][2].str, "$%d\\sqrt{3}$ m\n", nn/2 );
        sprintf( respostas[2][2][0].str, "$%d$ m\n", nn );
        sprintf( respostas[2][2][1].str, "$%d\\sqrt{3}$ m\n", 2*nn/3 );
        
    
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ ){
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    }
    
    
    strncpy( alt[0], respostas[ rnd[1][1] ] [ rnd[0][1] ] [ rnd[0][2] ].str, 1000 );
    
    do{
        for( i=1; i<5; i++ ){
            rd = randperm(3);
            strncpy( alt[i], respostas[ rand()%3 ] [ rd[0] ] [ rd[1] ].str, 1000 );
            free(rd);
        }
        
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    FILE *p = fopen( "Q2.tex", "w+" );
    
    fputs( "\\noindent\\parbox{\\linewidth}{\n\n", p );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Determine o %s %s %s, sabendo que o %s mede $%d$\\,m.\n\n", elemento[rnd[0][1]].str, preposicao[rnd[0][1]].str, poligono[rnd[1][1]].str, elemento[rnd[0][2]].str, nn );

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
