/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, j, *rnd, ii[] = { rand()%5, rand()%4, rand()%8, rand()%8, rand()%16 };
    
    float aux,
        num1[] = { -2., -1., 0., 1., 2. },
        num2[] = { -1.5, -0.5, 0.5, 1.5 },
        num3[] = { -5./3, -4./3, -2./3, -1./3, 1./3, 2./3, 4./3, 5./3 },
        num4[] = { -1.75, -1.25, -0.75, -0.25, 0.25, 0.75, 1.25, 1.75 },
        num5[] = { -1.8, -1.6, -1.4, -1.2, -0.8, -0.6, -0.4, -0.2, 0.2, 0.4, 0.6, 0.8, 1.2, 1.4, 1.6, 1.8 },
        num[] = { num1[ii[0]], num2[ii[1]], num3[ii[2]], num4[ii[3]], num5[ii[4]] };
        
    char snum[5][100], saux[100],
        *snum1[5 ] = { "$-2$", "$-1$", "$0$", "$1$", "$2$" },
        *snum2[4 ] = { "$-1{,}5$", "$-0{,}5$", "$0{,}5$", "$1{,}5$" },
        *snum3[8 ] = { "$-\\frac{5}{3}$", "$-\\frac{4}{3}$", "$-\\frac{2}{3}$", "$-\\frac{1}{3}$", "$\\frac{1}{3}$", "$\\frac{2}{3}$", "$\\frac{4}{3}$", "$\\frac{5}{3}$" },
        *snum4[8 ] = {  "$-\\frac{7}{4}$", "$-\\frac{5}{4}$", "$-\\frac{3}{4}$", "$-\\frac{1}{4}$", "$\\frac{1}{4}$", "$\\frac{3}{4}$", "$\\frac{5}{4}$", "$\\frac{7}{4}$" },
        *snum5[16] = { "$-1{,}8$", "$-1{,}6$", "$-1{,}4$", "$-1{,}2$", "$-0{,}8$", "$-0{,}6$", "$-0{,}4$", "$-0{,}2$", "$0{,}2$", "$0{,}4$", "$0{,}6$", "$0{,}8$", "$1{,}2$", "$1{,}4$", "$1{,}6$", "$1{,}8$" };
    
    
    strncpy( snum[0], snum1[ii[0]], sizeof snum[0] );
    strncpy( snum[1], snum2[ii[1]], sizeof snum[1] );
    strncpy( snum[2], snum3[ii[2]], sizeof snum[2] );
    strncpy( snum[3], snum4[ii[3]], sizeof snum[3] );
    strncpy( snum[4], snum5[ii[4]], sizeof snum[4] );
    
    
    for( i=0; i<5; i++ ){
        for( j=i+1; j<5; j++ ){
            if( num[i] > num[j] ){
                aux = num[i];
                num[i] = num[j];
                num[j] = aux;
                strncpy( saux, snum[i], sizeof saux );
                strncpy( snum[i], snum[j], sizeof snum[i] );
                strncpy( snum[j], saux   , sizeof snum[j] );
            }
        }
    }
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    sprintf( alt[0], "%s;\\,\\, %s;\\,\\, %s;\\,\\, %s\\,\\, e\\,\\, %s\n", snum[0], snum[1], snum[2], snum[3], snum[4] );
    
    do{
        for( i=1; i<5; i++ ){
            rnd = randperm(5);
            sprintf( alt[i], "%s;\\,\\, %s;\\,\\, %s;\\,\\, %s\\,\\, e\\,\\, %s\n", snum[rnd[0]], snum[rnd[1]], snum[rnd[2]], snum[rnd[3]], snum[rnd[4]] );
        }
    } while( dist_alt( alt ) );
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Observe os números racionais\n\n" );
    fprintf( p, "\\vspace{-4mm}\\begin{center}\n" );
    fprintf( p, "%s;\\,\\, %s;\\,\\, %s;\\,\\, %s\\,\\, e\\,\\, %s\n\n", snum1[ii[0]], snum2[ii[1]], snum3[ii[2]], snum4[ii[3]], snum5[ii[4]] );
    fprintf( p, "\\end{center}\\vspace{-4mm}\n" );
    fprintf( p, "Compare-os e coloque-os em ordem crescente, ou seja, do menor para o maior.\n\n" );

    fputs( "\% ALTERNATIVAS 3\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);

}
/***************************************************************************/
