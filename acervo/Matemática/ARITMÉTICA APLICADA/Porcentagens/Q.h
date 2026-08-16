/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, j, rx, rp, x, X[8], P[18];
    
    for( i=2; i<=19; i++ ){
        P[i-2] = i;
        if( i<=9 )
            X[i-2] = 100*i;
    }
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            rp = rand()%18;
            sprintf( alt[i], "$%d$\\%%\n", P[rp] );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    rx = rand()%8;
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\parbox{\\linewidth}{\n\n\% QUESTAO\n", p );
    fprintf( p, "Na venda de um tênis de R\\$\\,$%d{,}00$, um vendedor obteve uma comissão de R\\$\\,$%d{,}00$.\nEssa comissão representa quantos por cento do preço do produto?}\\vspace{2mm}\\\\\\parbox{\\linewidth}{\n\n", X[rx], P[rp]*X[rx]/100 );

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
void Q2(){
    int i, ii, n=0, A, B, *rnd,
        t, t1=6, t2=14,
        d, d1=1900 ,d2=2100;
        
        
    for( t=t1; t<=t2; t++ ){
        for( d=d1; d<=d2; d++ ){
            if( (100*d)%t==0 ){
                n++;
            }
        }
    }
     
     
//     printf("%d\n",n);
    struct{ int A, B, t, d; } D[n];
    i = 0;
    for( t=t1; t<=t2; t++ ){
        for( d=d1; d<=d2; d++ ){
            if( (100*d)%t==0 ){
                D[i].t = t;
                D[i].d = d;
                D[i].B = 100*d/t;
                D[i].A = D[i].B - d;
                i++;
            }
        }
    }
    
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    
    
    do{
        rnd = randperm(n);
        for( i=4; i>=0; i-- ){
            ii = rnd[i];
            sprintf( alt[i], "R\\$\\,$%d{,}%.2d$\n", D[ii].A/100, D[ii].A%100 );
        }
        free(rnd);
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    
    FILE *p = fopen( "Q2.tex", "w+" );
    
    fputs( "\\parbox{\\linewidth}{\n\n\% QUESTAO\n", p );
    fprintf( p, "Uma certa mercadoria é vendida nas lojas $A$ e $B$, sendo R\\$\\,$%d{,}%.2d$ mais cara em $B$. Se a loja $B$ oferecesse um desconto de $%d\\%%$, o preço nas duas lojas seria o mesmo. Qual é o preço na loja $A$?}\\vspace{2mm}\\\\\\parbox{\\linewidth}{\n\n", D[ii].d/100, D[ii].d%100, D[ii].t );

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
    int i, j, x, y, nn, rd;
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            
            rd = rand()%51;
            
            if( rd<7 ){
                nn = 30;
                do{ x = nn/5 + 3*(rand()%(3*nn/15)); }while(x==nn-x); // 7
            }
            else if( rd<(7+13) ){
                nn = 40;
                do{ x = nn/5 + 2*(rand()%(3*nn/10)); }while(x==nn-x); // 13
            }
            else{
                nn = 50;
                do{ x = nn/5 + rand()%(3*nn/5); }while(x==nn-x); // 31
            }
            
            y = nn - x;
            
            sprintf( alt[i], "$%d$\\%%\\,\\,\\,e\\,\\,\\,$%d$\\%%\n", 100*y/nn, 100*x/nn );
        }
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    FILE *p = fopen( "Q3.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Na Escola Educa Mais Força Aérea Brasileira, no turno vespertino, temos quatro salas de aula de 8º ano. Na turma %d temos, matriculados, %d meninos e %d meninas.\nCalcule o percentual de meninos e de meninas dessa turma, respectivamente.\n\n", 800+rand()%4, y, x );

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
