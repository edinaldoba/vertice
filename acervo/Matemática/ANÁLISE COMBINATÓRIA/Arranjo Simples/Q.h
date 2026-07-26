/******************************** QUESTÃO 1 ********************************/
void Q1(){
    int i, j, alt[5], ii[5], jj[5];
    
    int num[] = {1,2,3,4,5,6};
    char
        *smus[3] = { "tecladistas", "guitarristas", "vocalistas" },
        *snum[6] = { "um", "dois", "três", "quatro", "cinco", "seis" },
        *s[2] = {"","s"};
    
    FILE *p = fopen( "Q1.tex", "w+" );
    
    for( i=0; i<5; i++ ){
        ii[i] = 2 + rand()%4;
        jj[i] = 0;
    }
    j = 2+rand()%3;
    jj[j] = 1;
    
    alt[0] = num[ii[0]] * num[ii[1]] * num[ii[2]] * num[ii[3]] * num[ii[4]];
    for( i=2; i<5; i++ ){
        if( jj[i]==1 )
            alt[0] *= (num[ii[i]]-1);
    }
    
    do{
        for( i=1; i<5; i++ ){
            alt[i] = num[2+rand()%4] * num[2+rand()%4] * num[2+rand()%4] * num[2+rand()%4] * num[2+rand()%4] * (num[2+rand()%4]-1);
        }
    } while( dist_alt( alt ) );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Um concurso promovido por uma emissora de televisão vai formar uma nova banda de {\\it rock}. Classificaram-se para a etapa final \%s bateristas, \%s baixistas, \%s tecladistas, \%s guitarristas e \%s vocalistas. A banda vencedora do concurso será formada por \%s baterista, \%s baixista, \%s tecladista\%s \%s guitarrista\%s e \%s vocalista\%s.\\\\De quantas maneiras pode-se formar a banda, a partir dos candidatos finalistas, sabendo que os \%s possuem funções diferentes nessa banda?\n\n", snum[ii[0]], snum[ii[1]], snum[ii[2]], snum[ii[3]], snum[ii[4]], snum[jj[0]], snum[jj[1]], snum[jj[2]], s[jj[2]==1], snum[jj[3]], s[jj[3]==1], snum[jj[4]], s[jj[4]==1], smus[j-2] );
    fputs( "\% ALTERNATIVAS 7\n", p );
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
    int i, j, alt1[5], alt2[5], nequipes, njogos, aux;
    
    FILE *p = fopen( "Q2.tex", "w+" );
    
    nequipes = 12+rand()%20;
    njogos = nequipes * (nequipes-1);
    
    alt1[0] = nequipes;
    alt2[0] = 2*(nequipes-1);
    
    
    do{
        for( i=1; i<5; i++ ){
            aux = 12+rand()%20;
            alt1[i] = aux;
            alt2[i] = 2*(aux-1);
        }
    } while( dist_alt(alt1) );
    
    fputs( "\% QUESTAO\n", p );
    fprintf( p, "Em certa temporada da uma liga de basquete, os jogos da fase classificatória foram disputados em turno e returno, de maneira que cada equipe realizasse dois jogos contra cada uma das outras equipes, uma em ``casa'' e outra na ``casa'' do adversário. Sabendo que ao todo foram realizados %d jogos, resolva quantas equipes disputaram esse campeonato de basquete e, quantas partidas na fase classificatória cada equipe disputou?\n\n", njogos );
    fputs( "\% ALTERNATIVAS 10\n", p );
    fprintf( p, "Foram %d equipes e cada equipe disputou %d partidas.\n", alt1[0], alt2[0] );
    fprintf( p, "Foram %d equipes e cada equipe disputou %d partidas.\n", alt1[1], alt2[1] );
    fprintf( p, "Foram %d equipes e cada equipe disputou %d partidas.\n", alt1[2], alt2[2] );
    fprintf( p, "Foram %d equipes e cada equipe disputou %d partidas.\n", alt1[3], alt2[3] );
    fprintf( p, "Foram %d equipes e cada equipe disputou %d partidas.\n", alt1[4], alt2[4] );
    fputs( "\n\n\n", p );
    fclose(p);
    
}
/***************************************************************************/

