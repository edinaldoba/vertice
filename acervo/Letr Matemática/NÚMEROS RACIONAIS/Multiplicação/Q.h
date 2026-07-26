/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, k, a, b, c, d, *rnd, s1, s2, num[6], den[6], primos[]={2,2,2,2,3,3,3,5,5,7};
    
    char sinal[10]; // Sinais para as frações
    strncpy( sinal, "- ", sizeof sinal );
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            
            rnd = randperm(10);
            
            num[0] = primos[rnd[0]];
            num[1] = primos[rnd[1]];
            num[2] = primos[rnd[2]];
            num[3] = primos[rnd[3]];
            num[4] = primos[rnd[4]];
            num[5] = primos[rnd[5]];
            
            den[0] = primos[rnd[0]];
            den[1] = primos[rnd[1]];
            den[2] = primos[rnd[3]];
            den[3] = primos[rnd[4]];
            den[4] = primos[rnd[6]];
            den[5] = primos[rnd[7]];
            
            free(rnd);
            
            a = prod(  num   , 3 );
            b = prod(  den   , 3 );
            c = prod( &num[3], 3 );
            d = prod( &den[3], 3 );
            
            s1 = rand()%2; // Sinal da primeira fração (índice lógico)
            s2 = rand()%2; // Sinal da segunda fração (índice lógico)
            
            if( (num[2]*num[5])%(den[4]*den[5])==0 ){
                sprintf( alt[i], "$%c%d$\n", sinal[(!s1||s2)&&(s1||!s2)], num[2]*num[5]/den[4]/den[5] );
            }
            else{
                k = mdc( num[2]*num[5], den[4]*den[5] );
                sprintf( alt[i], "$%c\\frac{%d}{%d}$\n", sinal[(!s1||s2)&&(s1||!s2)], num[2]*num[5]/k, den[4]*den[5]/k );
            }
        }
        
    } while( a==b||a==c||a==d||b==c||b==d||c==d|| dist_alt(alt) );
    
    
    strncpy( sinal, "-+", sizeof sinal );
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Multiplicando abaixo, números racionais na forma fracionária\n\\[\\left(%c\\frac{%d}{%d}\\right)\\cdot\\left(%c\\frac{%d}{%d}\\right)\\]\nSimplifique e calcule.\n\n", sinal[s1], a, b, sinal[s2], c, d );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);

}
/***************************************************************************/
