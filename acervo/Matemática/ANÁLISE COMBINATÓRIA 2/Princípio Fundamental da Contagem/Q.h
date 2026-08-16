/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, num[5], alt[5];
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    char *quant[4] = { "dois", "três", "quatro", "cinco" };
    num[0] = rand()%4;
    num[1] = rand()%4;
    num[2] = rand()%4;
    
    alt[0] = (num[0]+2)*(num[1]+2)*(num[2]+2);
    do{
        for( i=1; i<5; i++ ){
            alt[i] = (rand()%4+2)*(rand()%4+2)*(rand()%4+2);
        }
    } while( dist_alt( alt ) );
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Antes do início de uma partida de futebol, é verificado se as equipes utilizarão uniformes cujas cores os distingam claramente.\\\\Para certa partida de futebol, uma das equipes dispunha de %s modelos de camisa, %s de calção e %s de meião. De quantas maneiras distintas essa equipe pode compor seu uniforme?\n\n", quant[num[0]], quant[num[1]], quant[num[2]] );
    fputs( "\% ALTERNATIVAS 4\n", p );
    fprintf( p, "%d maneiras\n", alt[0] );
    fprintf( p, "%d maneiras\n", alt[1] );
    fprintf( p, "%d maneiras\n", alt[2] );
    fprintf( p, "%d maneiras\n", alt[3] );
    fprintf( p, "%d maneiras\n", alt[4] );
    fputs( "\n\n\n", p );
    fclose(p);
    
}
/***************************************************************************/



/******************************** QUESTÃO 2 ********************************/
void Q2(){
    int i, num[5], alt[5];
    
    FILE *p = fopen( "Q2.tex", "w+" );
    
    num[0] = rand()%7+2;
    num[1] = rand()%12+3;
    
    alt[0] = num[1];
    do{
        for( i=1; i<5; i++ ){
            alt[i] = rand()%12+3;
        }
    } while( dist_alt( alt ) );
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Uma companhia de transporte rodoviário intermunicipal estuda as %d possíveis rotas para realização de viagens do município $A$ ao município $C$, com passagem obrigatória pelo município $B$. Sabendo que de $A$ a $B$ existem %d possíveis trajetos, quantos trajetos existem entre $B$ e $C$?\n\n", num[0]*num[1], num[0] );
    fputs( "\% ALTERNATIVAS 7\n", p );
    fprintf( p, "%d trajetos\n", alt[0] );
    fprintf( p, "%d trajetos\n", alt[1] );
    fprintf( p, "%d trajetos\n", alt[2] );
    fprintf( p, "%d trajetos\n", alt[3] );
    fprintf( p, "%d trajetos\n", alt[4] );
    fputs( "\n\n\n", p );
    fclose(p);
    
}
/***************************************************************************/



/******************************** QUESTÃO 3 ********************************/
void Q3(){
    int i, alt[5];
    
    FILE *p = fopen( "Q3.tex", "w+" );
    
    char
        *detalhe1[3] = { "", "ímpares", "múltiplos de 5" },
        *detalhe2[2] = { "", "distintos" },
        *ordens[3] = {"três ordens, ou seja, centena (C), dezena (D) e unidade (U)",
            "quatro ordens, ou seja, unidade de milhar (UM), centena (C), dezena (D) e unidade (U)",
            "cinco ordens, ou seja, dezena de milhar (DM), unidade de milhar (UM), centena (C), dezena (D) e unidade (U)"};
        
    int
        j,
        n1 = rand()%3,
        n2 = (n1==0)*rand()%2,
        nalg = rand()%3+3,
        na1 = rand()%2,
        na2 = rand()%2+8;
    
    if( n2==0 ){
        alt[0] = na2-na1+(na1==1);
        for( i=1; i<nalg-1; i++ ){
            alt[0] *= (na2-na1+1);
        }
        if( n1==1 )
            alt[0] *= ( (na2==9)*5 + (na2==8)*4 );
        else if( n1==2 )
            alt[0] *= ( (na1==0)*2 + (na1==1) );
        else
            alt[0] *= (na2-na1+1);
    }
    else{
        alt[0] = na2-na1+(na1==1);
        for( i=1; i<nalg; i++ ){
            alt[0] *= (na2-na1+1-i);
        }
    }
    
    do{
        for( i=1; i<5; i++ ){
            alt[i] = 1;
            for( j=0; j<nalg; j++ )
                alt[i] *= rand()%5+5;
        }
    } while( dist_alt( alt ) );
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Com os algarismos de %d a %d, quantos números %s de %d algarismos %s podem ser formados?\\\\\n\\underline{\\bf Obs:} cada número formado terá %s.\n\n", na1, na2, detalhe1[n1], nalg, detalhe2[n2], ordens[nalg-3] );
    fputs( "\% ALTERNATIVAS 8\n", p );
    fprintf( p, "%d números\n", alt[0] );
    fprintf( p, "%d números\n", alt[1] );
    fprintf( p, "%d números\n", alt[2] );
    fprintf( p, "%d números\n", alt[3] );
    fprintf( p, "%d números\n", alt[4] );
    fputs( "\n\n\n", p );
    fclose(p);
    
    
}
/***************************************************************************/



/******************************** QUESTÃO 3 ********************************/
void Q4(){
    int i, alt[5];
    
    FILE *p = fopen( "Q4.tex", "w+" );
    
    char
        *detalhe1[3] = { "", "ímpares", "múltiplos de 5" },
        *detalhe2[2] = { "", "distintos" },
        *ordens[3] = {"três ordens, ou seja, centena (C), dezena (D) e unidade (U)",
            "quatro ordens, ou seja, unidade de milhar (UM), centena (C), dezena (D) e unidade (U)",
            "cinco ordens, ou seja, dezena de milhar (DM), unidade de milhar (UM), centena (C), dezena (D) e unidade (U)"};
        
    int
        j,
        n1 = rand()%3,
        n2 = (n1==0)*rand()%2,
        nalg = rand()%3+3,
        na1 = rand()%2,
        na2 = rand()%2+8;
    
    if( n2==0 ){
        alt[0] = na2-na1+(na1==1);
        for( i=1; i<nalg-1; i++ ){
            alt[0] *= (na2-na1+1);
        }
        if( n1==1 )
            alt[0] *= ( (na2==9)*5 + (na2==8)*4 );
        else if( n1==2 )
            alt[0] *= ( (na1==0)*2 + (na1==1) );
        else
            alt[0] *= (na2-na1+1);
    }
    else{
        alt[0] = na2-na1+(na1==1);
        for( i=1; i<nalg; i++ ){
            alt[0] *= (na2-na1+1-i);
        }
    }
    
    do{
        for( i=1; i<5; i++ ){
            alt[i] = 1;
            for( j=0; j<nalg; j++ )
                alt[i] *= rand()%5+5;
        }
    } while( dist_alt( alt ) );
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Numa lanchonete há 5 tipos de sanduíche, 4 tipos de refrigerante e 3 tipos de sorvete. De quantas maneiras podemos tomar um lanche composto por 1 sanduíche, 1 refrigerante e 1 sorvete?\n\n", na1, na2, detalhe1[n1], nalg, detalhe2[n2], ordens[nalg-3] );
    fputs( "\% ALTERNATIVAS 8\n", p );
    fprintf( p, "%d números\n", alt[0] );
    fprintf( p, "%d números\n", alt[1] );
    fprintf( p, "%d números\n", alt[2] );
    fprintf( p, "%d números\n", alt[3] );
    fprintf( p, "%d números\n", alt[4] );
    fputs( "\n\n\n", p );
    fclose(p);
    
    
}
/***************************************************************************/

