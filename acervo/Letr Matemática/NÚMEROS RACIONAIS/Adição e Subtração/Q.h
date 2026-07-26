/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, rd, a, A[]={1,3,5,7,9}, b=2, c, C[]={1,2,3,4,6,7,8,9}, d=5;
    
    char snum[10], sinal[10]; // Sinais para as frações
    
    char **alt = (char**) calloc( 5, sizeof(char*) );
    for( i=0; i<5; i++ )
        alt[i] = (char*) calloc( 1000, sizeof(char) );
    
    do{
        for( i=4; i>=0; i-- ){
            
            a = A[rand()%5];
            c = C[rand()%8];
            
            rd = rand()%4;
            if( rd==0 )      { strncpy(sinal," +",sizeof sinal); sprintf( snum,"%4.1f", 1.*a/b+1.*c/d ); }
            else if( rd==1 ) { strncpy(sinal," -",sizeof sinal); sprintf( snum,"%4.1f", 1.*a/b-1.*c/d ); }
            else if( rd==2 ) { strncpy(sinal,"-+",sizeof sinal); sprintf( snum,"%4.1f",-1.*a/b+1.*c/d ); }
            else if( rd==3 ) { strncpy(sinal,"--",sizeof sinal); sprintf( snum,"%4.1f",-1.*a/b-1.*c/d ); }
            
            sprintf( alt[i], "$%c%c{,}%c$\n", snum[0], snum[1], snum[3] );
        }
        
    } while( dist_alt( alt ) );
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Podemos transformar em fração um número na forma decimal e vice-versa. Assim, para a operação\n\\[%c\\frac{%d}{%d}%c\\frac{%d}{%d}\\]\nDetermine o seu valor na forma decimal.\n\n", sinal[0], a, b, sinal[1], c, d );

    fputs( "\% ALTERNATIVAS 3\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n\n\n", p );
    fclose(p);

}
/***************************************************************************/
