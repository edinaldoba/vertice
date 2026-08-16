/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, j, k, ii, n=0, *rnd, t, t1=1, t2=5;
    
    int C, C1=1000, C2=5000;
    
    char *sn[9] = {"um ano","dois anos","três anos","quatro anos","cinco anos","seis anos","sete anos","oito anos","nove anos"},
         *si[6] = {"","bimestre","trimestre","","","semestre"}, sm[100];
    
         
    for( t=t1; t<=t2; t++ ){
        for( C=C1; C<=C2; C+=1000 ){
            for( j=2; j<=6; j++ ){
                if( j==4 || j==5 ) continue;
                for( k=1; k<=9; k++ ){
                    n++;
                }
            }
        }
    }
    
    
//     printf("%d\n",n);
    struct{ int C; float M; int i, n; char si[100], sn[100]; } D[n];
    i=0;
    for( t=t1; t<=t2; t++ ){
        for( C=C1; C<=C2; C+=1000 ){
            for( j=2; j<=6; j++ ){
                if( j==4 || j==5 ) continue;
                for( k=1; k<=9; k++ ){
                    D[i].C = C;
                    D[i].i = t;
                    D[i].n = k * 12 / j;
                    D[i].M = D[i].C * powf( 1 + 0.01*D[i].i, 1.*D[i].n );
                    strncpy( D[i].si, si[j-1] , sizeof D[i].si );
                    strncpy( D[i].sn, sn[k-1] , sizeof D[i].sn );
                    i++;
                }
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
            sprintf( sm, "%10.2f", D[ii].M );
            sprintf( alt[i], "R\\$\\,$%.7s{,}%s$\n", sm, &sm[8] );
        }
        free(rnd);
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    fputs( "\\parbox{\\linewidth}{\n\n\% QUESTAO\n", p );
    fprintf( p, "Calcule o montante produzido por R\\$\\,$%d\\,000{,}00$ aplicado à taxa de $%d\\%%$ ao %s, após %s, no sistema de juros compostos.}\\vspace{2mm}\\\\\\parbox{\\linewidth}{\n\n", D[ii].C/1000, D[ii].i, D[ii].si, D[ii].sn );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n}\n\n\n", p );
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
/***************************************************************************/








/******************************** QUESTÃO 2 ********************************/
void Q2(){
    int i, j, k, ii, n=0, *rnd;
    
    char svp[100], si[100];
    
         
    for( i=0; i<=3; i++ ){
        for( j=1; j<10; j++ ){
            for( k=3; k<=6; k++ ){
                n++;
            }
        }
    }
    
    
//     printf("%d\n",n);
    struct{ float i, vp; int vf, n; } D[n];
    ii=0;
    for( i=0; i<=3; i++ ){
        for( j=1; j<10; j++ ){
            for( k=3; k<=6; k++ ){
                D[ii].i = i + 1.5;
                D[ii].vf = 1000 + 100*j;
                D[ii].n = k;
                D[ii].vp = D[ii].vf/2./powf(1+0.01*D[ii].i,1.*D[ii].n) + D[ii].vf/2./powf(1+0.01*D[ii].i,2.*D[ii].n);
                ii++;
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
            sprintf( svp, "%10.2f", D[ii].vp );
            sprintf( alt[i], "R\\$\\,$%.7s{,}%s$\n", svp, &svp[8] );
        }
        free(rnd);
    } while( dist_alt( alt, strlen(alt[0]) ) );
    
    
    sprintf( si, "%.2f", D[ii].i );
    
    
    FILE *p = fopen( "Q2.tex", "w+" );
    
    fputs( "\\parbox{\\linewidth}{\n\n\% QUESTAO\n", p );
    fprintf( p, "Qual é o valor presente de uma dívida de R\\$\\,$%d\\,%d00{,}00$ que deve ser paga em duas partes de R\\$\\,$%d{,}00$, sendo a primeira em $%d$ meses e a segunda em $%d$ meses? Vamos considerar o custo de oportunidade em $%c{,}%c\\%%$ a.m.}\\vspace{2mm}\\\\\\parbox{\\linewidth}{\n\n", D[ii].vf/1000, (D[ii].vf/100)%10, D[ii].vf/2, D[ii].n, 2*D[ii].n, si[0], si[2] );

    fputs( "\% ALTERNATIVAS 4\n", p );
    fputs( alt[0], p );
    fputs( alt[1], p );
    fputs( alt[2], p );
    fputs( alt[3], p );
    fputs( alt[4], p );
    fputs( "\n}\n\n", p );
    fclose(p);
    
    for( i=0; i<5; i++ ){
        free(alt[i]);
    }
    free(alt);

}
/***************************************************************************/


