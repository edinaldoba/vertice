/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, j, aux, num[4], alt[5];
    
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
    fprintf( p, "Calcule o valor da expressão \\[\\frac{\%d!\\,\%d!}{\%d!\\,\%d!}\\] e assinale a alternativa correta.\n\n", num[0], num[1], num[2], num[3] );
    fputs( "\% ALTERNATIVAS 6\n", p );
    fprintf( p, "%d\n", alt[0] );
    fprintf( p, "%d\n", alt[1] );
    fprintf( p, "%d\n", alt[2] );
    fprintf( p, "%d\n", alt[3] );
    fprintf( p, "%d\n", alt[4] );
    fputs( "\n\n\n", p );
    fclose(p);
    
}
/***************************************************************************/

