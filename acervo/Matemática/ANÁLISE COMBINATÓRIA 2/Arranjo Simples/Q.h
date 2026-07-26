/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, n, pp=4-rand()%2;
    
    char *vice[2] = {"",", um vice-presidente"};
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            n = 20+rand()%11;
            sprintf( alt[i], "$%d$ comissões\n", arranjo( n, pp ) );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Um clube tem $%d$ membros. A diretoria é formada por um presidente%s, um secretário e um tesoureiro. Se uma pessoa pode ocupar apenas um desses cargos, de quantas maneiras é possível formar a diretoria?\n\n", n, vice[pp-3] );
    
    fputs( "\% ALTERNATIVAS 5\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
}
/***************************************************************************/



/******************************** QUESTÃO 2 ********************************/
void Q2(){
    int i, n, pp;
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            n = 5+rand()%5;
            pp = 2+rand()%3;
            sprintf( alt[i], "$%d$ maneiras\n", arranjo( n, pp ) );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    FILE *p = fopen( "Q2.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "De quantas maneiras $%d$ meninos podem sentar-se num banco que tem apenas $%d$ lugares?\n\n", n, pp );
    
    fputs( "\% ALTERNATIVAS 5\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
}
/***************************************************************************/




void Q3(){
    int i, r, n, pp, arr;
    
    char *ip[2] = { "pares", "ímpares" };
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            n = 5+rand()%5;
            pp = 2+rand()%3;
            r = rand()%2;
            sprintf( alt[i], "$%d$ números\n", (n/2+r*n%2)*arranjo(n-1,pp-1) );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    FILE *p = fopen( "Q3.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Quantos números %s de $%d$ algarismos distintos podemos formar com os algarismos de $1$ a $%d$?\n\n", ip[r], pp, n );
    
    fputs( "\% ALTERNATIVAS 5\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
    for( i=0; i<5; i++ )
        free(alt[i]);
    free(alt);
    
}







/******************************** QUESTÃO 2 ********************************/
void Q4(){
    int i, j, n, pp;
    
    char *palavra[3] = {"PICOLE","LUMINAR","CONTAGEM"};
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            j = rand()%3;
            n = 6+j;
            pp = 2+rand()%4;
            sprintf( alt[i], "$%d$ maneiras\n", arranjo( n, pp ) );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    FILE *p = fopen( "Q4.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Quantas ``palavras'' de %d letras distintas podemos formar com as letras da palavra %s?\n\n", pp, palavra[j] );
    
    fputs( "\% ALTERNATIVAS 5\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
}
/***************************************************************************/



/******************************** QUESTÃO 2 ********************************/
void Q5(){
    int i, n;
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            n = 12+rand()%19;
            sprintf( alt[i], "$%d$ maneiras\n", arranjo( n, 2 ) );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    FILE *p = fopen( "Q5.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "De quantas maneiras podemos escolher um pivô e um ala num grupo de %d jogadores de basquete?\n\n", n );
    
    fputs( "\% ALTERNATIVAS 5\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
}
/***************************************************************************/



/******************************** QUESTÃO 2 ********************************/
void Q6(){
    int i, n;
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            n = 6+rand()%15;
            sprintf( alt[i], "$%d$ maneiras\n", arranjo( n, 4 ) );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    FILE *p = fopen( "Q6.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Um estudante tem %d lápis de cores diferentes. De quantas maneiras ele pode pintar os estados da região Sudeste do Brasil (São Paulo, Rio de Janeiro, Minas Gerais e Espírito Santo), cada um de uma cor?\n\n", n );
    
    fputs( "\% ALTERNATIVAS 5\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);
    
}
/***************************************************************************/
