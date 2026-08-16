/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, j, aux, num[4], alt[5];
    
    char *str = "AEOUGDRST";
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    num[0] = 10+rand()%10;
    num[1] = 20+rand()%10;
    num[2] = num[0] - 2;
    num[3] = num[1] - 1;
    
    alt[0] = num[0]*(num[0]-1)*num[1];
    
    do{
        for( i=1; i<5; i++ ){
            aux = 10+rand()%10;
            alt[i] = aux*(aux-1)*20+rand()%10;
        }
    } while( dist_alt( alt ) );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Considerando todas as palavras de $5$ letras, com ou sem significado, que podem ser escritas com\n\n" );
    fputs( "\% ALTERNATIVAS\n", p );
    fprintf( p, "%d\n", alt[0] );
    fprintf( p, "%d\n", alt[1] );
    fprintf( p, "%d\n", alt[2] );
    fprintf( p, "%d\n", alt[3] );
    fprintf( p, "%d\n", alt[4] );
    fputs( "\n\n\n", p );
    fclose(p);
    
}
/***************************************************************************/

