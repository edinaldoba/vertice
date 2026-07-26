/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

void Q1(int id_questao) {
    int i, i1, i3;

    // Arrays de dados
    char *perguntas[] = {"qual é o tipo", "quais são os possíveis valores"};
    char *variaveis[] = {"cor", "preço", "número de portas", "cilindrada", "forma de pagamento", "estado de conservação"};

    char *tipo_correto[] = {
        "Qualitativa nominal", "Quantitativa contínua", "Quantitativa discreta",
        "Quantitativa contínua", "Qualitativa nominal", "Qualitativa ordinal"
    };

    char *valores[] = {
        "branca, vermelha ou azul", "$\\geq$ 50 mil ou $<$ 50 mil",
        "duas ou quatro", "1.0 ou 1.6", "à vista ou financiado", "novo ou usado"
    };

    char *t1[] = {"Qualitativa", "Quantitativa"};
    char *t2[] = {"nominal", "ordinal", "discreta", "contínua"};

    char **alt = alocar_alternativas(5, 1000);

    // 1. SORTEIO DOS ÍNDICES
    i1 = rand() % 2; // 0: Pergunta tipo, 1: Pergunta valores
    i3 = rand() % 6; // Sorteia qual variável será usada

    // 2. DEFINIÇÃO DA RESPOSTA CORRETA (alt[0])
    if (i1 == 0)
        sprintf(alt[0], "%s\n", tipo_correto[i3]);
    else
        sprintf(alt[0], "%s\n", valores[i3]);

    // 3. GERAÇÃO DOS DISTRATORES
    int tentativas = 0;
    do {
        for (i = 1; i < 5; i++) {
            if (i1 == 0) {
                // Gera combinações aleatórias de tipos (ex: Qualitativa discreta)
                sprintf(alt[i], "%s %s\n", t1[rand() % 2], t2[rand() % 4]);
            } else {
                // Sorteia outros valores da lista
                sprintf(alt[i], "%s\n", valores[rand() % 6]);
            }
        }
        tentativas++;
    } while (tem_alternativa_repetida(alt) && tentativas < 1000);

    // 4. GRAVAÇÃO DO ARQUIVO .TEX
    char nome_arq[20];
    sprintf(nome_arq, "Q%d.tex", id_questao);
    FILE *p = fopen(nome_arq, "w");

    fprintf(p, "%% QUESTAO\n");
    fprintf(p, "Uma concessionária de automóveis possui $3500$ clientes cadastrados e realizou uma pesquisa de preferência de compra considerando as seguintes variáveis: cor (branca, vermelha ou azul), preço (a partir de 50 mil ou abaixo de 50 mil), número de portas (duas ou quatro), cilindrada (1.0 ou 1.6), forma de pagamento (à vista ou financiado) e estado de conservação (novo ou usado). Foram consultados $210$ clientes. Diante dessas informações, responda: %s da variável %s?\n\n", perguntas[i1], variaveis[i3]);

    fprintf(p, "%% ALTERNATIVAS\n");
    for (i = 0; i < 5; i++) fprintf(p, "%s", alt[i]);

    fclose(p);
    liberar_alternativas(alt, 5);
}







void Q2(int id_questao) {
    int i, a, b, c, d, n = 0;
    int total = (rand() % 2 == 0) ? 20 : 25; // Define o total (20 ou 25)
    char *times[] = {"Santos", "Palmeiras", "Corinthians", "São Paulo"};

    // Palitinhos de contagem em TikZ
    char *contagem[10] = {
        "\\tikz[scale=0.4,thick]{\\draw (0,1)--(0,0);}",
        "\\tikz[scale=0.4,thick]{\\draw (0,1)--(0,0)--(1,0);}",
        "\\tikz[scale=0.4,thick]{\\draw (0,1)--(0,0)--(1,0)--(1,1);}",
        "\\tikz[scale=0.4,thick]{\\draw (0,0)rectangle(1,1);}",
        "\\tikz[scale=0.4,thick]{\\draw (0,0)rectangle(1,1)--(0,0);}",
        "\\tikz[scale=0.4,thick]{\\draw (0,0)rectangle(1,1)--(0,0)(1.3,1)--(1.3,0);}",
        "\\tikz[scale=0.4,thick]{\\draw (0,0)rectangle(1,1)--(0,0)(1.3,1)--(1.3,0)--(2.3,0);}",
        "\\tikz[scale=0.4,thick]{\\draw (0,0)rectangle(1,1)--(0,0)(1.3,1)--(1.3,0)--(2.3,0)--(2.3,1);}",
        "\\tikz[scale=0.4,thick]{\\draw (0,0)rectangle(1,1)--(0,0)(1.3,0)rectangle(2.3,1);}",
        "\\tikz[scale=0.4,thick]{\\draw (0,0)rectangle(1,1)--(0,0)(1.3,0)rectangle(2.3,1)--(1.3,0);}"
    };

    // 1. CONTAGEM E ALOCAÇÃO
    for(a=1; a<=10; a++)
        for(b=1; b<=10; b++)
            for(c=1; c<=10; c++)
                for(d=1; d<=10; d++)
                    if(a!=b && a!=c && a!=d && b!=c && b!=d && c!=d && a+b+c+d==total) n++;

    typedef struct { int fa[4]; float fr[4]; } Registro;
    Registro *D = (Registro*) malloc(n * sizeof(Registro));

    int idx = 0;
    for(a=1; a<=10; a++)
        for(b=1; b<=10; b++)
            for(c=1; c<=10; c++)
                for(d=1; d<=10; d++)
                    if(a!=b && a!=c && a!=d && b!=c && b!=d && c!=d && a+b+c+d==total) {
                        D[idx].fa[0]=a; D[idx].fa[1]=b; D[idx].fa[2]=c; D[idx].fa[3]=d;
                        for(int k=0; k<4; k++) D[idx].fr[k] = (100.0 * D[idx].fa[k]) / total;
                        idx++;
                    }

    char **alt = alocar_alternativas(5, 1000);
    int ii = rand() % n;
    int jj = rand() % 4;

    // 2. RESPOSTA CORRETA
    sprintf(alt[0], "$f_a = %d$ e $f_r = %.0f\\%%$\n", D[ii].fa[jj], D[ii].fr[jj]);

    // 3. DISTRATORES
    int tentativas = 0;
    do {
        for (i = 1; i < 5; i++) {
            int fa_falsa = 1 + rand() % 12;
            float fr_falsa = (100.0 * fa_falsa) / ((rand() % 2 == 0) ? 20 : 25);
            sprintf(alt[i], "$f_a = %d$ e $f_r = %.0f\\%%$\n", fa_falsa, fr_falsa);
        }
        tentativas++;
    } while (tem_alternativa_repetida(alt) && tentativas < 1000);

    // 4. GRAVAÇÃO DO ARQUIVO .TEX
    char nome_arq[20];
    sprintf(nome_arq, "Q%d.tex", id_questao);
    FILE *p = fopen(nome_arq, "w");

    fprintf(p, "%% QUESTAO\n");
    fprintf( p, "Um grupo de alunos foi consultado sobre o time paulista de sua preferência, e os votos foram registrados assim: Santos %s; Palmeiras %s; Corinthians %s; São Paulo %s. Qual é a frequência absoluta do valor ``%s'', dessa variável? E a frequência relativa?\n\n", contagem[D[ii].fa[0]-1], contagem[D[ii].fa[1]-1], contagem[D[ii].fa[2]-1], contagem[D[ii].fa[3]-1], times[jj] );

    fprintf(p, "%% ALTERNATIVAS\n");
    for (i = 0; i < 5; i++) fprintf(p, "%s", alt[i]);

    fclose(p);
    free(D);
    liberar_alternativas(alt, 5);
}







/******************************** QUESTÃO 3 ********************************/
void Q3(int id_questao){
    int i, j, ii, jj, total, T[]={20,25,50}, n=0, *rnd, a, b, c, d, inicio, fim;

    total = T[rand()%3];

    inicio = (total==50) ? total/12 : total/8;
    fim    = (total==50) ? total/3 : total/2;


    for( a=inicio; a<=fim; a++ )
        for( b=inicio; b<=fim; b++ )
            for( c=inicio; c<=fim; c++ )
                for( d=inicio; d<=fim; d++ )
                    if( a!=b && a!=c && a!=d && b!=c && b!=d && c!=d && a+b+c+d==total )
                        n++;


    struct{ int fa[4], fr[4]; } D[n];
    i=0;
    for( a=inicio; a<=fim; a++ )
        for( b=inicio; b<=fim; b++ )
            for( c=inicio; c<=fim; c++ )
                for( d=inicio; d<=fim; d++ )
                    if( a!=b && a!=c && a!=d && b!=c && b!=d && c!=d && a+b+c+d==total ){
                        D[i].fa[0] = a;
                        D[i].fa[1] = b;
                        D[i].fa[2] = c;
                        D[i].fa[3] = d;
                        D[i].fr[0] = 100/total*a;
                        D[i].fr[1] = 100/total*b;
                        D[i].fr[2] = 100/total*c;
                        D[i].fr[3] = 100/total*d;
                        i++;
                    }



    char **alt = alocar_alternativas(5, 1000);



    ii = rand()%n;
    jj = rand()%4;


    sprintf( alt[0], "$fa=%d$\n", D[ii].fa[2] );


    int tentativas = 0;
    do{
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "$fa=%d$\n", inicio + rand()%(fim-inicio) );
        }
        tentativas++;
    } while (tem_alternativa_repetida(alt) && tentativas < 1000);



    // 4. GRAVAÇÃO DO ARQUIVO .TEX
    char nome_arq[20];
    sprintf(nome_arq, "Q%d.tex", id_questao);
    FILE *p = fopen(nome_arq, "w");

    fprintf(p, "%% QUESTAO\n");
    fprintf( p, "A tabela a seguir apresenta os dados de uma pesquisa sobre os gêneros musicais mais vendidos em uma loja de CDs em um único dia.\n\n" );


    fprintf(p, "    \\renewcommand{\\arraystretch}{1.3} %% Dá um respiro vertical para as frações\n");
    fprintf(p, "    \\arrayrulecolor{CorSerie}    %% Linhas da tabela na cor do tema\n");
    fprintf(p, "    \\noindent{\\small \\begin{tabular}{|L{13mm}|C{5mm}|C{5mm}|C{6mm}|C{8mm}|}\n");
    fprintf(p, "        \\hline\n");
    fprintf(p, "        \\rowcolor{CorSerie!70} \n");
    fprintf(p, "        \\textbf{\\color{white}Gênero} & \n");
    fprintf(p, "        \\textbf{\\color{white}FA} & \n");
    fprintf(p, "        \\textbf{\\color{white}FR} & \n");
    fprintf(p, "        \\textbf{\\color{white}FR} & \n");
    fprintf(p, "        \\textbf{\\color{white}FR} \\\\\\hline\n");
    fprintf(p, "        \n");
    fprintf(p, "        \\rowcolor{CorSerie!5} %% Linha com azul quase branco\n");
    fprintf(p, "        Sertanejo & & & & %d\\%% \\\\\\hline\n", D[ii].fr[0]);
    fprintf(p, "        \n");

    char fr_str[50];
    simplificar_fracao(D[ii].fa[1], total, fr_str);
    fprintf(p, "MPB & & $%s$ & & \\\\\\hline\n", fr_str);
    // fprintf(p, "        MPB & & $\\frac{%d}{%d}$ & & \\\\\\hline\n", D[ii].fr[1]/mdc(D[ii].fr[1],100), 100/mdc(D[ii].fr[1],100));

    fprintf(p, "        \n");
    fprintf(p, "        \\rowcolor{CorSerie!5}\n");
    fprintf(p, "        Rock & & & & \\\\\\hline\n");
    fprintf(p, "        \n");
    fprintf(p, "        Clássico & & & 0,%.2d&\\\\\\hline\n", D[ii].fr[3] );
    fprintf(p, "        \n");
    fprintf(p, "        \\rowcolor{CorSerie!15} %% Destaque para o total\n");
    fprintf(p, "        \\textbf{Total} & \\textbf{%d} &  &  &  \\\\\\hline\n", total);
    fprintf(p, "    \\end{tabular}}\n\n");



    fprintf( p, "Preencha os campos em branco na tabela e determine a frequência absoluta ($fa$) da categoria rock.\n\n" );


    fprintf(p, "%% ALTERNATIVAS\n");
    for (i = 0; i < 5; i++) fprintf(p, "%s", alt[i]);


    fclose(p);

    liberar_alternativas(alt, 5);

}
/***************************************************************************/







/******************************** QUESTÃO 4 ********************************/
void Q4(int id_questao){
    int i, total, amplitude, fr[4], faixas[5], *rnd;
    
    do{
        total = 10 * ( 2 + rand()%19 );
    } while ( total==100 );
    
    amplitude = 50 * ( 1 + rand()%15 );
    
    faixas[0] = 1000 + 100 * ( rand()%5 );
    faixas[1] = faixas[0] + amplitude;
    faixas[2] = faixas[1] + amplitude;
    faixas[3] = faixas[2] + amplitude;
    faixas[4] = faixas[3] + amplitude;
    
    rnd = randperm(5);
    struct{ char str[100]; } sfaixas[5] = {"\\rule{8mm}{0pt}", "\\rule{8mm}{0pt}", "\\rule{8mm}{0pt}", "\\rule{8mm}{0pt}", "\\rule{8mm}{0pt}"};
    sprintf( sfaixas[rnd[0]].str, "%d", faixas[rnd[0]] );
    sprintf( sfaixas[rnd[1]].str, "%d", faixas[rnd[1]] );
    free(rnd);
    
    
    rnd = randperm(4);
    fr[0] = 10 * ( 1 + rnd[0] );
    fr[1] = 10 * ( 1 + rnd[1] );
    fr[2] = 10 * ( 1 + rnd[2] );
    fr[3] = 10 * ( 1 + rnd[3] );
    free(rnd);
    
    
    char **alt = alocar_alternativas(5, 1000);
    
    
    sprintf( alt[0], "total $=%d$ e amplitude $=%d$\n", total, amplitude );
    
    
    int tentativas = 0;
    do{
        for( i=1; i<5; i++ ){
            sprintf( alt[i], "total $=%d$ e amplitude $=%d$\n", 10*(2+rand()%19), 50*(1+rand()%15) );
        }        
        tentativas++;
    } while (tem_alternativa_repetida(alt) && tentativas < 1000);



    // 4. GRAVAÇÃO DO ARQUIVO .TEX
    char nome_arq[20];
    sprintf(nome_arq, "Q%d.tex", id_questao);
    FILE *p = fopen(nome_arq, "w");


    fprintf(p, "%% QUESTAO\n");
    fprintf( p, "A tabela de frequências a seguir apresenta o levantamento salarial dos funcionários de uma empresa, com os valores agrupados em quatro intervalos.\n\n" );



    fprintf(p, "\\renewcommand{\\arraystretch}{1.3} %% Espaço extra para o aluno escrever os cálculos\n" );
	fprintf(p, "\\arrayrulecolor{CorSerie}    %% Linhas na cor do tema\n" );

    fprintf(p, "\\noindent\\begin{tabular}{|C{28mm}|C{9mm}|C{9mm}|}\n" );
    fprintf(p, "\\hline\n" );
    fprintf(p, "\\rowcolor{CorSerie!70} \n" );
    fprintf(p, "\\textbf{\\color{white}Salário (R\\$)} & \n" );
    fprintf(p, "\\textbf{\\color{white}FA} & \n" );
    fprintf(p, "\\textbf{\\color{white}FR} \\\\ \\hline\n" );

    fprintf(p, "%% Classe 1\n" );
    fprintf(p, "\\rowcolor{CorSerie!5}\n" );
    fprintf(p, "%s \\tikz[thick, CorSerie]{\\draw (0,-0.15)--(0,0.15) (0,0)--(0.7,0);} \\rule{8mm}{0pt} & & %d\\%% \\\\ \\hline\n", sfaixas[0].str, fr[0] );

    fprintf(p, "%% Classe 2\n" );
    fprintf(p, "%s \\tikz[thick, CorSerie]{\\draw (0,-0.15)--(0,0.15) (0,0)--(0.7,0);} \\rule{8mm}{0pt} & 15 & \\\\ \\hline\n", sfaixas[1].str );

    fprintf(p, "%% Classe 3\n" );
    fprintf(p, "\\rowcolor{CorSerie!5}\n" );
    fprintf(p, "%s \\tikz[thick, CorSerie]{\\draw (0,-0.15)--(0,0.15) (0,0)--(0.7,0);} %s &%d &%d\\%%\\\\\\hline\n", sfaixas[2].str, sfaixas[3].str, fr[2]*total/100, fr[2] );

    fprintf(p, "%% Classe 4\n" );
    fprintf(p, "\\rule{8mm}{0pt} \\tikz[thick, CorSerie]{\\draw (0,-0.15)--(0,0.15) (0,0)--(0.7,0);} %s &&\\\\\\hline\n", sfaixas[4].str );
    fprintf(p, "%% Total\n" );
    fprintf(p, "\\rowcolor{CorSerie!15}\n" );
    fprintf(p, "\\textbf{Total} & &  \\\\ \\hline\n" );
    fprintf(p, "\\end{tabular}\n\n" );



    fprintf( p, "Preencha os campos em branco na tabela e determine: qual é o total de funcionários da empresa e qual é a amplitude dos intervalos de classe?\n\n" );

    
    fprintf(p, "%% ALTERNATIVAS\n");
    for (i = 0; i < 5; i++) fprintf(p, "%s", alt[i]);
    
    
    fclose(p);
    
    liberar_alternativas(alt, 5);

}

