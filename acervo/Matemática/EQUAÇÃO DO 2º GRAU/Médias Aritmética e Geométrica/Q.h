/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, j=0, n, n1, n2, x1, x2, *rnd, dif, difaux, primos[]={1,2,3,5,7};
    char sma[10], smg[50], str[10];
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    
    rnd = randperm(5);
    n1 = primos[ rnd[0] ];
    n2 = primos[ rnd[1] ];
    free(rnd);
    
    n = 2+rand()%5;
    x1 = n * n1;
    x2 = n * n2;
//     printf("\nx=%2d  ou  x=%2d\n",x1,x2);
    
    dif = ( (x1>x2) - (x1<x2) ) * (x1-x2);
    sprintf( alt[0], "$%d$ anos\n", dif );
    
    rnd = randperm(35);
    for( i=1; i<5; i++ ){
        do{ difaux=2+rnd[j]; j++; } while(difaux==dif);
        sprintf( alt[i], "$%d$ anos\n", difaux );
    }
    free(rnd);
    
    
    if( (x1+x2)%2==0 ){
        sprintf( sma, "%d", (x1+x2)/2 ); // Média aritmética
    }
    else{
        sprintf( str, "%4.1f", 0.5*(x1+x2) );  // Média aritmética
        sprintf( sma, "%.2s{,}5", str );
    }
    
    sprintf( smg, "%d\\sqrt{%d}", n, n1*n2 ); // Média geométrica
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Maria Clara e Ana são as irmãs mais velhas de Maria Isabel. As idades de Maria Clara e Maria Isabel têm média aritmética $%s$ anos e média geométrica $%s$ anos. Quantos anos Maria Clara tem a mais que Maria Isabel?\\\\\n", sma, smg );
    fprintf( p, "{\\bf Obs: }{\\it Considere}\\\\\n" );
    fprintf( p, "$x_1\\rightarrow$ {\\it idade de Maria Clara}\\\\\n" );
    fprintf( p, "$x_2\\rightarrow$ {\\it idade de Maria Isabel}\\\\\n" );
    fprintf( p, "{\\it Assim temos:}\\\\\n" );
    fprintf( p, "\\[\\dfrac{x_1+x_2}{2}=%s\\textnormal{\\,\\,e\\,\\,}\\sqrt{x_1\\cdot x_2}=%s \\]\n\n", sma, smg );
    

    fputs( "\% ALTERNATIVAS 7\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
    free(alt);

}
/***************************************************************************/

