/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, n, r, fat;
    
    char *palavra[8] = {"LUA","MEL","AMOR","CURA","LAPIS","OLHAR","ESCOLA","FRAUDE"};
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    r = rand()%8;
    n = r/2+3;
    fat = fatorial(n);
    sprintf( alt[0], "$%d$ anagramas\n", fat );
    
    do{
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "$%d$ anagramas\n", fat/3 + rand()%(3*fat/2) );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Quantos são os anagramas da palavra %s?\n\n", palavra[r] );
    
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


