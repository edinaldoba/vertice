/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

// Gera a string do gráfico TikZ para a alternativa
void formatar_grafico_nota(char *destino, int n1, int n2, int n3, int n4) {
    // Aqui usamos a CorSerie que definimos no LaTeX
    sprintf(destino,
        "\\parbox{\\linewidth}{\\centering"
        "\\tikz[scale=0.3]{"
        "\\draw[CorSerie!30, xstep=3, ystep=1] (-0.5,-0.5) grid (9.5,3.5);" // Grid suave de fundo
        "\\draw[ultra thick, CorSerie] (0,%d)--(3,%d)--(6,%d)--(9,%d);" // Linha da nota
        "\\draw[thick, CorSerie] (-0.5,-0.5) rectangle (9.5,3.5);" // Borda
        "\\node[right, CorSerie] at (9.5,0) {\\footnotesize 6};"
        "\\node[right, CorSerie] at (9.5,3) {\\footnotesize 9};"
        "}}\n", n1, n2, n3, n4);
}


void Q1(int id_questao) {
    int notas[4], n[4], i;
    char **alt = alocar_alternativas(5, 1000); // Função auxiliar para evitar repetição de malloc

    // 1. SORTEIO DOS DADOS DA QUESTÃO
    do {
        for(i=0; i<4; i++) notas[i] = 6 + rand() % 4;
    } while ((notas[0]==notas[1] && notas[1]==notas[2]) || (notas[1]==notas[2] && notas[2]==notas[3]));

    // 2. GERAÇÃO DA RESPOSTA CORRETA (Sempre em alt[0] antes de embaralhar)
    formatar_grafico_nota(alt[0], notas[0]-6, notas[1]-6, notas[2]-6, notas[3]-6);

    // 3. GERAÇÃO DAS DISTRATORES (Alternativas Erradas)
    int tentativas = 0;
    do {
        for(i = 1; i < 5; i++) {
            do {
                for(int j=0; j<4; j++) n[j] = rand() % 4;
            } while ((n[0]==n[1] && n[1]==n[2]) || (n[2]==n[3] && n[3]==n[4]) ||
                     (n[0]==notas[0]-6 && n[1]==notas[1]-6 && n[2]==notas[2]-6 && n[3]==notas[3]-6));

            formatar_grafico_nota(alt[i], n[0], n[1], n[2], n[3]);
        }
        tentativas++;
    } while (tem_alternativa_repetida(alt) && tentativas < 1000);

    // 4. EMBARALHAMENTO E GRAVAÇÃO DO GABARITO
    // Aqui você chamaria sua função de embaralhar e salvaria qual letra ficou a correta
    // shuffle_e_salva_gabarito(alt, id_questao);

    // 5. GRAVAÇÃO DO ARQUIVO .TEX
    char nome_arq[20];
    sprintf(nome_arq, "Q%d.tex", id_questao);
    FILE *p = fopen(nome_arq, "w");

    fprintf(p, "%% QUESTAO\n");
    fprintf(p, "Um aluno apresentou durante o ano letivo o seguinte aproveitamento: "
               "1º Período: nota %d; 2º Período: nota %d; 3º Período: nota %d; e 4º Período: nota %d. "
               "Qual gráfico de linhas melhor representa essa situação?\n\n",
               notas[0], notas[1], notas[2], notas[3]);

    fprintf(p, "%% ALTERNATIVAS\n");
    for(i=0; i<5; i++) fprintf(p, "%s", alt[i]);

    fputs( "\n\n", p );

    fclose(p);
    liberar_alternativas(alt, 5);
}









void gerar_tikz_eleicao(FILE *p, int v0h, int v0m, int v1h, int v1m, int v2h, int v2m) {
    fprintf(p, "\\begin{tikzpicture}[>=latex, xscale=0.4, yscale=0.6]\n");

    // Grid suave de fundo
    fprintf(p, "\\draw[CorSerie!20, xstep=1, ystep=0.5] (0,0) grid (10,7);\n" );

    // Eixos com a CorSerie
    fprintf(p, "  \\draw[<->, CorSerie, thick] (0,7) node[right, text=CorSerie] {\\bf\\scriptsize Votos} "
               "-- (0,0) -- (11.5,0) node[above, text=CorSerie] {\\bf\\scriptsize Candidatos};\n");

    // Escala no eixo Y
    fprintf(p, "  \\foreach \\y in {2,4,...,12} { "
               "\\draw[CorSerie!40] (0.1,\\y/2) -- (-0.1,\\y/2) node[left, text=black] {\\scriptsize \\y}; }\n");

    // Cores para Homens (H) e Mulheres (M)
    // Usamos tons diferentes da mesma CorSerie ou cores complementares da paleta
    char *corH = "AzulProfessor!70";
    char *corM = "VinhoDestaque!70";

    // Barras do Ricardo
    fprintf(p, "  \\draw[fill=%s,text=white] (1,0) rectangle node {\\bf\\scriptsize H} (2,%.2f);\n", corH, v0h/2.0);
    fprintf(p, "  \\draw[fill=%s,text=white] (2,0) rectangle node {\\bf\\scriptsize M} (3,%.2f);\n", corM, v0m/2.0);
    fprintf(p, "  \\node[below, scale=0.8] at (2,0) {Ricardo};\n");

    // Barras do Fausto
    fprintf(p, "  \\draw[fill=%s,text=white] (4,0) rectangle node {\\bf\\scriptsize H} (5,%.2f);\n", corH, v1h/2.0);
    fprintf(p, "  \\draw[fill=%s,text=white] (5,0) rectangle node {\\bf\\scriptsize M} (6,%.2f);\n", corM, v1m/2.0);
    fprintf(p, "  \\node[below, scale=0.8] at (5,0) {Fausto};\n");

    // Barras da Paula
    fprintf(p, "  \\draw[fill=%s,text=white] (7,0) rectangle node {\\bf\\scriptsize H} (8,%.2f);\n", corH, v2h/2.0);
    fprintf(p, "  \\draw[fill=%s,text=white] (8,0) rectangle node {\\bf\\scriptsize M} (9,%.2f);\n", corM, v2m/2.0);
    fprintf(p, "  \\node[below, scale=0.8] at (8,0) {Paula};\n");

    fprintf(p, "\\end{tikzpicture}\n\n");
}


void Q2(int id_questao) {
    char **alt = alocar_alternativas(5, 1000);
    int i, ii, jj;
    struct { int m, h, total; char *nome; } c[3];

    c[0].nome = "Ricardo"; c[1].nome = "Fausto"; c[2].nome = "Paula";

    // 1. SORTEIO E CÁLCULO
    do {
        for(i=0; i<3; i++) {
            c[i].m = 2 + rand() % 11;
            c[i].h = 2 + rand() % 11;
            c[i].total = c[i].m + c[i].h;
        }
    } while (c[0].total == c[1].total || c[0].total == c[2].total || c[1].total == c[2].total);

    // 2. DETERMINAR VENCEDOR (ii) E VICE (jj)
    // Uma forma simples: ordenar ou apenas comparar
    int v[3] = {0, 1, 2};
    // Bubble sort simples para 3 elementos (ordem decrescente)
    for(i=0; i<2; i++)
        for(int k=i+1; k<3; k++)
            if(c[v[k]].total > c[v[i]].total) { int t=v[i]; v[i]=v[k]; v[k]=t; }

    ii = v[0]; // Primeiro
    jj = v[1]; // Segundo

    // 3. ALTERNATIVAS
    sprintf(alt[0], "Repr.: %s; Vice: %s\n", c[ii].nome, c[jj].nome);

    char *vendedores_possiveis[] = {
        "Repr.: Ricardo; Vice: Fausto", "Repr.: Fausto; Vice: Ricardo",
        "Repr.: Ricardo; Vice: Paula",  "Repr.: Paula; Vice: Ricardo",
        "Repr.: Fausto; Vice: Paula",   "Repr.: Paula; Vice: Fausto"
    };

    int *rnd = randperm(6);
    int count = 1, r_idx = 0;
    while(count < 5) {
        char buffer[100];
        sprintf(buffer, "%s\n", vendedores_possiveis[rnd[r_idx]]);
        if(strcmp(buffer, alt[0]) != 0) {
            strcpy(alt[count], buffer);
            count++;
        }
        r_idx++;
    }
    free(rnd);

    // 4. GRAVAÇÃO
    char nome_arq[20];
    sprintf(nome_arq, "Q%d.tex", id_questao);
    FILE *p = fopen(nome_arq, "w");

    fprintf(p, "%% QUESTAO\n");
    fprintf(p, "Em uma eleição para representante de classe, os candidatos foram Ricardo, Paula e Fausto. "
               "Observe o resultado da votação no gráfico, onde estão especificados os votos de homens (H) e mulheres (M).\n\n");

    gerar_tikz_eleicao(p, c[0].h, c[0].m, c[1].h, c[1].m, c[2].h, c[2].m);

    fprintf(p, "Com base nos dados, quem foi eleito representante e vice-representante, respectivamente?\n\n");

    fprintf(p, "%% ALTERNATIVAS\n");
    for(i=0; i<5; i++) fprintf(p, "%s", alt[i]);

    fclose(p);
    liberar_alternativas(alt, 5);
}
/***************************************************************************/







void gerar_tikz_pizza(FILE *p) {
    // Definimos as cores para cada fatia para facilitar a associação visual
    // Usamos as cores da nossa paleta harmonizada
    fprintf(p, "\\begin{tikzpicture}[scale=1.5]\n");

    // Fatias coloridas com transparência para não carregar o toner
    fprintf(p, "  \\fill[AzulProfessor!70] (0,0) -- (0:1) arc (0:36:1) -- cycle;\n");      // 10%
    fprintf(p, "  \\fill[VerdeEscola!70] (0,0) -- (36:1) arc (36:90:1) -- cycle;\n");     // 15%
    fprintf(p, "  \\fill[LaranjaAlerta!70] (0,0) -- (90:1) arc (90:162:1) -- cycle;\n");   // 20%
    fprintf(p, "  \\fill[RoxoElegante!70] (0,0) -- (162:1) arc (162:270:1) -- cycle;\n"); // 30%
    fprintf(p, "  \\fill[VinhoDestaque!70] (0,0) -- (270:1) arc (270:360:1) -- cycle;\n"); // 25%

    // Contorno e divisões
    fprintf(p, "  \\foreach \\a in {0, 36, 90, 162, 270} {\\draw[CorSerie!50] (0,0) -- (\\a:1);}\n");
    fprintf(p, "  \\draw[CorSerie, thick] (0,0) circle (1);\n");


    // Porcentagens internas
    fprintf(p, "  \\node[white] at (18:0.7)  {\\scriptsize\\bf 10\\%%};\n");
    fprintf(p, "  \\node[white] at (63:0.7)  {\\scriptsize\\bf 15\\%%};\n");
    fprintf(p, "  \\node[white] at (126:0.7) {\\scriptsize\\bf 20\\%%};\n");
    fprintf(p, "  \\node[white] at (216:0.7) {\\scriptsize\\bf 30\\%%};\n");
    fprintf(p, "  \\node[white] at (315:0.7) {\\scriptsize\\bf 25\\%%};\n");

    // Rótulos (Labels) externos
    fprintf(p, "  \\node[right] at (18:1) {\\footnotesize\\bf comer};\n");
    fprintf(p, "  \\node[above right] at (63:1) {\\footnotesize\\bf estudar};\n");
    fprintf(p, "  \\node[above left] at (126:1) {\\footnotesize\\bf outras};\n");
    fprintf(p, "  \\node[left] at (216:1) {\\footnotesize\\bf dormir};\n");
    fprintf(p, "  \\node[below right] at (315:1) {\\footnotesize\\bf escola};\n");

    fprintf(p, "\\end{tikzpicture}\n\n");
}


void Q3(int id_questao) {
    char **alt = alocar_alternativas(5, 1000);
    int i, ii, jj, *rnd;

    // 1. DADOS UNIFICADOS (5 atividades)
    int fr[5] = { 15, 10, 25, 30, 20 }; // Total 100%
    char *atividades[5] = {
        "estudando",
        "comendo",
        "na escola",
        "dormindo",
        "com outras atividades"
    };

    struct tempo t1, t2;
    char str1[100], str2[100];

    // 2. SORTEIO DAS COMBINAÇÕES (C(5,2))
    // randperm(5) gera, por exemplo, {3, 1, 0, 4, 2}
    // Pegamos os dois primeiros: ii=3 e jj=1. Nunca serão iguais.
    rnd = randperm(5);
    ii = rnd[0];
    jj = rnd[1];
    free(rnd);

    // 3. RESPOSTA CORRETA (alt[0])
    // Calculamos o tempo exato para as atividades sorteados ii e jj
    t1 = horas(0.01 * fr[ii] * 24);
    t2 = horas(0.01 * fr[jj] * 24);

    hms(t1, str1);
    hms(t2, str2);
    sprintf(alt[0], "%s e %s\n", str1, str2);

    // 4. GERAÇÃO DOS DISTRATORES
    int tentativas = 0;
    do {
        for(i = 1; i < 5; i++) {
            // Sorteia valores plausíveis (múltiplos de 0.5h ou 1h) para confundir
            rnd = randperm(16);
            float h_falsa1 = (float)(2 + rnd[0]) * 0.5;
            float h_falsa2 = (float)(2 + rnd[1]) * 0.5;
            free(rnd);

            t1 = horas(h_falsa1);
            t2 = horas(h_falsa2);

            hms(t1, str1);
            hms(t2, str2);
            sprintf(alt[i], "%s e %s\n", str1, str2);
        }
        tentativas++;
    } while (tem_alternativa_repetida(alt) && tentativas < 1000);

    // 5. GRAVAÇÃO DO ARQUIVO .TEX
    char nome_arq[20];
    sprintf(nome_arq, "Q%d.tex", id_questao);
    FILE *p = fopen(nome_arq, "w");

    fprintf(p, "%% QUESTAO\n");
    fprintf(p, "Luísa construiu um gráfico de setores para organizar seu tempo diário (24h).\n\n");

    // O gráfico deve ser gerado com os valores de fr[5] na ordem correta
    gerar_tikz_pizza(p);

    // A mágica da combinação C(5,2) acontece aqui:
    // ii e jj vêm do randperm, logo são sempre diferentes e limitados a 5 elementos.
    fprintf(p, "Com base no gráfico, quanto tempo Luísa gasta %s? E quanto tempo gasta %s?\n\n",
            atividades[ii], atividades[jj]);

    fprintf(p, "%% ALTERNATIVAS\n");
    for(i=0; i<5; i++) fprintf(p, "%s", alt[i]);

    fclose(p);
    liberar_alternativas(alt, 5);
}









void formatar_histograma(char *destino, int fa[]) {
    sprintf(destino,
        "\\parbox{\\linewidth}{\\centering"
        "\\tikz[xscale=0.35, yscale=0.2]{"
        "\\draw[CorSerie!20, step=1] (-0.2,0) grid (5.2,7.5);"
        "\\foreach \\i [count=\\j] in {%d,%d,%d,%d,%d} {"
        "\\pgfmathsetmacro{\\int}{30 + \\j*15}"
        "\\draw[thick, fill=CorSerie!\\int] (\\j-1,0) rectangle (\\j, \\i);"
        "}"
        "\\draw[thick, CorSerie] (0,0) -- (5,0);"
        "\\draw[thick, CorSerie] (0,0) -- (0,7.5);"
        "}}\n", fa[0], fa[1], fa[2], fa[3], fa[4]);
}


void Q4(int id_questao) {
    int i, j, fa[5];
    int alturas_brutas[20];
    char lista_str[1200] = "";
    char **alt = alocar_alternativas(5, 1200);

    // 1. SORTEIO DAS FREQUÊNCIAS (Soma = 20)
    int *rnd = randperm(3);
    int modelo = rand() % 4;
    if (modelo == 0)      { fa[0]=rnd[0]+2; fa[1]=5; fa[2]=6; fa[3]=rnd[1]+2; fa[4]=rnd[2]+2; }
    else if (modelo == 1) { fa[0]=rnd[0]+2; fa[1]=6; fa[2]=5; fa[3]=rnd[1]+2; fa[4]=rnd[2]+2; }
    else if (modelo == 2) { fa[0]=rnd[0]+2; fa[1]=rnd[1]+2; fa[2]=5; fa[3]=6; fa[4]=rnd[2]+2; }
    else                  { fa[0]=rnd[0]+2; fa[1]=rnd[1]+2; fa[2]=6; fa[3]=5; fa[4]=rnd[2]+2; }
    free(rnd);

    // 2. GERAÇÃO DOS DADOS BRUTOS (Critério: 140cm a 189cm)
    int k = 0;
    for (i = 0; i < 5; i++) {
        for (j = 0; j < fa[i]; j++) {
            int base = 140 + (i * 10);
            alturas_brutas[k] = base + (rand() % 10);

            // Garantir o limite inferior (140cm) na primeira classe
            if (i == 0 && j == 0) alturas_brutas[k] = 140;

            // Garantir o limite superior (186-189cm) na última classe
            if (i == 4 && j == 0) alturas_brutas[k] = 186 + (rand() % 4);

            k++;
        }
    }
    intperm(alturas_brutas, 20); // Embaralha a lista para a prova

    // Converte para string
    for (i = 0; i < 20; i++) {
        char temp[15];
        if (i == 19) sprintf(temp, "e %d", alturas_brutas[i]);
        else sprintf(temp, "%d, ", alturas_brutas[i]);
        strcat(lista_str, temp);
    }

    // 3. ALTERNATIVA CORRETA
    formatar_histograma(alt[0], fa);

    // 4. DISTRATORES
    int tentativas = 0;
    int base_fa[] = {2, 3, 4, 5, 6};
    do {
        for (i = 1; i < 5; i++) {
            intperm(base_fa, 5);
            formatar_histograma(alt[i], base_fa);
        }
        tentativas++;
    } while (tem_alternativa_repetida(alt) && tentativas < 1000);

    // 5. GRAVAÇÃO
    char nome_arq[20];
    sprintf(nome_arq, "Q%d.tex", id_questao);
    FILE *p = fopen(nome_arq, "w");

    fprintf(p, "%% QUESTAO\n");
    fprintf(p, "No levantamento das alturas de vinte alunos de uma turma, foram obtidos os seguintes valores (em cm): "
               "%s. Qual histograma melhor representa essa distribuição de frequências?\n\n", lista_str);

    fprintf(p, "%% ALTERNATIVAS\n");
    for (i = 0; i < 5; i++) fprintf(p, "%s", alt[i]);

    fclose(p);
    liberar_alternativas(alt, 5);
}







void Q5(int id_questao) {
    int i, a, b, c, n = 0;
    int total = 12 + 3 * (rand() % 3);
    char *categorias[3] = {"fácil", "mediana", "difícil"};
    char **alt = alocar_alternativas(5, 1000);

    // 1. CONTAGEM DE COMBINAÇÕES (Para alocação dinâmica segura)
    for (a = 1; a <= 10; a++) {
        for (b = 1; b <= 10; b++) {
            for (c = 1; c <= 10; c++) {
                if (a != b && a != c && b != c && a + b + c == total) n++;
            }
        }
    }

    typedef struct { int fa[3], ang[3]; } DadosQuestao;
    DadosQuestao *D = (DadosQuestao*) malloc(n * sizeof(DadosQuestao));

    // 2. PREENCHIMENTO DA ESTRUTURA
    int idx = 0;
    for (a = 1; a <= 10; a++) {
        for (b = 1; b <= 10; b++) {
            for (c = 1; c <= 10; c++) {
                if (a != b && a != c && b != c && a + b + c == total) {
                    D[idx].fa[0] = a; D[idx].fa[1] = b; D[idx].fa[2] = c;
                    D[idx].ang[0] = (360 * a) / total;
                    D[idx].ang[1] = (360 * b) / total;
                    D[idx].ang[2] = (360 * c) / total;
                    idx++;
                }
            }
        }
    }

    // 3. SORTEIO DA QUESTÃO E CATEGORIA
    int ii = rand() % n;
    int jj = rand() % 3;
    int angulo_correto = D[ii].ang[jj];

    // 4. GERAÇÃO DAS ALTERNATIVAS
    sprintf(alt[0], "%d$^{\\circ}$\n", angulo_correto);

    int tentativas = 0;
    do {
        for (i = 1; i < 5; i++) {
            int erro = (rand() % 2 == 0) ? (10 + rand() % 21) : -(10 + rand() % 21);
            sprintf(alt[i], "%d$^{\\circ}$\n", ((angulo_correto+erro)<3) ? 3+rand()%5 : angulo_correto+erro );
        }
        tentativas++;
    } while (tem_alternativa_repetida(alt) && tentativas < 1000);

    // 5. GRAVAÇÃO DO ARQUIVO .TEX
    char nome_arq[20];
    sprintf(nome_arq, "Q%d.tex", id_questao);
    FILE *p = fopen(nome_arq, "w");

    fprintf(p, "%% QUESTAO\n");
    fprintf(p, "As questões de Matemática foram classificadas em categorias quanto ao índice de facilidade, como mostra o gráfico abaixo.\n\n");

    // Gráfico de Barras Horizontais Colorido
    float xscale=0.3;
    fprintf(p, "\\begin{tikzpicture}[>=latex, xscale=%.2f, yscale=0.45]\n", xscale);
    fprintf(p, "  \\draw[CorSerie!20, xstep=1, ystep=1.5] (0,0) grid (11,5);\n"); // Nosso grid com steps diferentes!
    fprintf(p, "  \\draw[<->, CorSerie, thick] (0,5.5) node[above=-1mm] {\\bf\\tiny Categoria} -- (0,0) -- (13.5,0) node[below] {\\bf\\tiny Nº Quest.};\n");

    for(i = 1; i <= 10; i++)
        fprintf(p, "  \\draw[CorSerie!40] (%d,0.1) -- (%d,-0.1) node[below, text=black] {\\tiny %d};\n", i, i, i);

    // Barras com degradê usando a CorSerie
    fprintf(p, "  \\draw[fill=VinhoDestaque!70] (0,0.5) rectangle node[right=%.3fcm] {\\bf\\scriptsize %d} (%d,1.5);\n", xscale*0.5*D[ii].fa[0], D[ii].fa[0], D[ii].fa[0]);
    fprintf(p, "  \\draw[fill=LaranjaAlerta!70] (0,2.0) rectangle node[right=%.3fcm] {\\bf\\scriptsize %d} (%d,3.0);\n", xscale*0.5*D[ii].fa[1], D[ii].fa[1], D[ii].fa[1]);
    fprintf(p, "  \\draw[fill=VerdeEscola!70]   (0,3.5) rectangle node[right=%.3fcm] {\\bf\\scriptsize %d} (%d,4.5);\n", xscale*0.5*D[ii].fa[2], D[ii].fa[2], D[ii].fa[2]);

    fprintf(p, "  \\node[left] at (0,1) {\\scriptsize fácil};\n");
    fprintf(p, "  \\node[left] at (0,2.5) {\\scriptsize mediana};\n");
    fprintf(p, "  \\node[left] at (0,4) {\\scriptsize difícil};\n");
    fprintf(p, "\\end{tikzpicture}\n\n");

    fprintf(p, "Se essa classificação fosse apresentada em um gráfico de setores, o ângulo do setor referente à categoria ``\\textbf{%s}'' mediria:\n\n", categorias[jj]);

    fprintf(p, "%% ALTERNATIVAS\n");
    for (i = 0; i < 5; i++) fprintf(p, "%s", alt[i]);

    fclose(p);
    free(D);
    liberar_alternativas(alt, 5);
}







void Q6(int id_questao) {
    int i, j, ii[7], i1, i2;
    char *dias[] = { "no domingo", "na segunda", "na terça", "na quarta", "na quinta", "na sexta", "no sábado" };
    char **alt = alocar_alternativas(5, 1000);

    int *recebidas = randperm(7);
    int *resolvidas = randperm(7);

    // 1. LÓGICA DE SORTEIO: Garante que exatamente 2 dias tenham resolvidas > recebidas
    do {
        j = 0;
        intperm(recebidas, 7);
        intperm(resolvidas, 7);
        for (i = 0; i < 7; i++) {
            if (resolvidas[i] > recebidas[i]) {
                ii[j] = i;
                j++;
            }
        }
    } while (j != 2);

    // 2. ALTERNATIVA CORRETA (alt[0])
    sprintf(alt[0], "%s e %s\n", dias[ii[0]], dias[ii[1]]);

    // 3. DISTRATORES
    int tentativas = 0;
    do {
        for (i = 1; i < 5; i++) {
            i1 = rand() % 7;
            i2 = rand() % 7;
            if (i1 == i2)
                sprintf(alt[i], "somente %s\n", dias[i1]);
            else
                sprintf(alt[i], "%s e %s\n", dias[i1], dias[i2]);
        }
        tentativas++;
    } while (tem_alternativa_repetida(alt) && tentativas < 1000);

    // 4. GRAVAÇÃO DO ARQUIVO .TEX
    char nome_arq[20];
    sprintf(nome_arq, "Q%d.tex", id_questao);
    FILE *p = fopen(nome_arq, "w");

    fprintf(p, "%% QUESTAO\n");
    fprintf(p, "O gráfico a seguir apresenta o movimento de reclamações no SAC de uma empresa em uma determinada semana. "
               "A linha tracejada informa o número de reclamações \\textbf{recebidas} e a linha contínua o número de reclamações \\textbf{resolvidas}.\n\n");

    // Início do TikZ
    fprintf(p, "\\begin{tikzpicture}[xscale=1, yscale=1]\n");

    // Grid de fundo usando o que aprendemos sobre steps
    fprintf(p, "  \\draw[CorSerie!40, xstep=5/6, ystep=0.5] (0,0) grid (5,3);\n");

    // Eixos
    fprintf(p, "\\foreach \\y in {0,5,10,15,20,25,30} {"
               "\\node[left=1mm, scale=0.7] at (0,0.1*\\y) {\\y}; }\n");

    char *siglas[] = {"dom", "seg", "ter", "qua", "qui", "sex", "sáb"};
    float ajustes[7] = {0, 0.04, 0, 0.04, 0.01, 0.01, 0}; // ajuste fino da posição dos dias da semana
    for(i=0; i<7; i++)
        fprintf(p, "\\node[scale=0.7] at (5/6*%d, -0.3-%.2f) {%s};\n", i, ajustes[i], siglas[i]);

    // Linha de Recebidas (Tracejada)
    fprintf(p, "  \\draw[ultra thick, dashed, VermelhoLaser] ");
    for(i=0; i<7; i++) fprintf(p, "(%d*5/6, %.1f) %s ", i, recebidas[i]/2.0, (i==6)?"": "--");
    fprintf(p, ";\n");

    // Linha de Resolvidas (Contínua - CorSerie)
    fprintf(p, "  \\draw[ultra thick, VerdeEscola] ");
    for(i=0; i<7; i++) fprintf(p, "(%d*5/6, %.1f) %s ", i, resolvidas[i]/2.0, (i==6)?"": "--");
    fprintf(p, ";\n");

    // Pontos (Nodes) para destaque
    for(i=0; i<7; i++) {
        fprintf(p, "  \\fill[VermelhoLaser] (%d*5/6, %.1f) circle (2.5pt);\n", i, recebidas[i]/2.0);
        fprintf(p, "  \\fill[VerdeEscola] (%d*5/6, %.1f) circle (2.5pt);\n", i, resolvidas[i]/2.0);
    }

    fprintf(p, "\\end{tikzpicture}\n\n");

    fprintf(p, "Em quais dias da semana o número de reclamações resolvidas \\textbf{excedeu} o de recebidas?\n\n");

    fprintf(p, "%% ALTERNATIVAS\n");
    for (i = 0; i < 5; i++) fprintf(p, "%s", alt[i]);

    fclose(p);
    free(recebidas); free(resolvidas);
    liberar_alternativas(alt, 5);
}


