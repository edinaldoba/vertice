/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, num, rd, dia, taxa, capital;
    
    float juro;
    
    char sjuro[100];
    
    char *dias[3] = { "dois", "três", "quatro" };
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{ 
        for( i=4; i>=0; i-- ){
            
            dia = 2 + rand()%3;
            
            do{ taxa=5+rand()%15; }while(taxa%5==0);
            
            do{ capital=300+rand()%600; }while(capital%5==0);
            
            juro = 1e-2*capital*taxa*dia;
            
            sprintf( alt[i], "R\\$\\,$%d{,}00$\n", capital );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    sprintf( sjuro, "%6.2f", juro );
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\parbox{\\linewidth}{\n\n\% QUESTAO\n", p );
    fprintf( p, "Luis Roberto colocou parte de seu 13º salário em uma aplicação que rendia $%d$\\%% ao ano no regime de juros simples. Sabendo-se que após %s anos ele recebeu R\\$\\,$%.3s{,}%s$ de juro, qual foi a quantia que ele aplicou?}\\vspace{2mm}\\\\\\parbox{\\linewidth}{\n\n", taxa, dias[dia-2], sjuro, &sjuro[4] );

    fputs( "\% ALTERNATIVAS 4\n", p );
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
void Q2(){
    int i, num, rd, dia, capital;
    
    float taxa=0.3, juro, montante;
    
    char smontante[100];
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{ 
        for( i=4; i>=0; i-- ){
            
            do{ dia=10+rand()%15; }while(dia%10==0);

            capital=30+10*(rand()%7);
            
            juro = 1e-2*capital*taxa*dia;
            
            montante = capital + juro;
            
            sprintf( smontante, "%.2f", montante );
            
            sprintf( alt[i], "R\\$\\,$%.2s{,}%s$\n", smontante, &smontante[3] );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    
    FILE *p = fopen( "Q2.tex", "w+" );
    
    fputs( "\\parbox{\\linewidth}{\n\n\% QUESTAO\n", p );
    fprintf( p, "Certo banco cobra juros simples de $0{,}3$\\%% ao dia para contas pagas com atraso de até 30 dias. Pedro pagou uma conta de R\\$\\,$%d{,}00$ com atraso de %d dias. O valor pago por Pedro foi de:}\\vspace{2mm}\\\\\\parbox{\\linewidth}{\n\n", capital, dia );

    fputs( "\% ALTERNATIVAS 5\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n}\n\n", p );
    fclose(p);

}
/***************************************************************************/
