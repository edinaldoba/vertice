/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, n, p;
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            n = 10 + rand()%16;
            p = 2 + rand()%3;
            sprintf( alt[i], "$%d$ maneiras\n", combinacao(n,p) );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    
    FILE *p0 = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p0 );
    fprintf( p0, "Uma escola enviará a um congresso $%d$ de seus $%d$ professores. De quantas maneira distintas pode ser formado o grupo de professores que participará do congresso?\n\n", p, n );

    fputs( "\% ALTERNATIVAS 3\n", p0 );
    fputs( alt[0], p0 );
    fputs( alt[1], p0 );
    fputs( alt[2], p0 );
    fputs( alt[3], p0 );
    fputs( alt[4], p0 );
    fputs( "\n\n", p0 );
    fclose(p0);

}
/***************************************************************************/



/******************************** QUESTÃO 2 ********************************/
void Q2(){
    int i, n, pp=2, comb;
        
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    
    do{
        for( i=4; i>=0; i-- ){
            n = 11+rand()%20;
            comb = combinacao( n, pp );
            sprintf( alt[i], "$%d$ pessoas\n", n );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    
    FILE *p = fopen( "Q2.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Duas pessoas de um grupo serão escolhidas para representá-lo. Sabendo que essa escolha pode ser feita de $%d$ maneiras distintas, quantas pessoas formam esse grupo?\n\n", comb );

    fputs( "\% ALTERNATIVAS 3\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n}\n\n", p );
    fclose(p);

}
/***************************************************************************/



/******************************** QUESTÃO 2 ********************************/
void Q3(){
    int i, j, n, r, m, cm, cr;
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            n = 12+rand()%9;
            m = (n/4)+rand()%(n/2+1);
            r = n - m;
            cm = combinacao( m, 3 );
            cr = combinacao( r, 2 );
            sprintf( alt[i], "$%d$ comissões\n", cm*cr );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    FILE *p = fopen( "Q3.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Em uma sala de aula do 2º ano do Ensino Médio, há $%d$ alunos, sendo $%d$ moças e $%d$ rapazes. Quantas comissões de $5$ pessoas, sendo que, dessas, $3$ serão moças e $2$ serão rapazes, podemos formar?\n\n", n, m, r );

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
