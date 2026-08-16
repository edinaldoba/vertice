//================================================== QUESTÃO 1 ==================================================//
void Q1() {
    int i;
    // Dados para sorteio
    int dado1[] = {200, 300, 400, 500, 600, 700, 800, 900};
    int dado2[] = {11, 12, 13, 14, 15, 16, 17, 18, 19};

    // Inicializar ponteiros como NULL para evitar erro no primeiro free()
    int *r1 = NULL, *r2 = NULL;

    // Alocação das alternativas
    char **alt = (char**) calloc(5, sizeof(char*));
    for (i = 0; i < 5; i++) {
        alt[i] = (char*) calloc(1000, sizeof(char));
    }

    do {
        // Liberar memória das permutações da iteração anterior
        if (r1) free(r1);
        if (r2) free(r2);

        r1 = randperm(8); // Gera permutação para os valores
        r2 = randperm(9); // Gera permutação para as taxas

        for (i = 0; i < 5; i++) {
            // CÁLCULO: Se a pergunta pede o VALOR DO AUMENTO:
            // Valor = (Preço * Taxa) / 100
            int valor_aumento = (dado1[r1[i]] * dado2[r2[i]]) / 100;

            sprintf(alt[i], "R\\$ $%d{,}00$\n", valor_aumento);
        }

        // dist_alt garante que as alternativas geradas sejam diferentes entre si
    } while(dist_alt(alt, strlen(alt[0])));

    // Geração do arquivo LaTeX
    FILE *p = fopen("Q1.tex", "w");
    if (p == NULL) {
        printf("Erro ao abrir arquivo!\n");
        return;
    }

    fputs("\\parbox{\\linewidth}{\n\n% QUESTAO\n", p);

    // Note que usamos r1[0] e r2[0] para a pergunta, garantindo que a Resposta Correta (alt[0]) bata com os dados
    fprintf(p, "Um produto custa R\\$ %d,00 e sofre um reajuste de %d\\%%. Qual o valor do aumento em reais?}\n",
            dado1[r1[0]], dado2[r2[0]]);

    fprintf(p, "\\vspace{2mm}\n\\\\\\parbox{\\linewidth}{\n\n%% ALTERNATIVAS\n");

    for (i = 0; i < 5; i++) {
        fputs(alt[i], p);
    }

    fputs("\n}\n\n", p);
    fclose(p);

    // Limpeza final de memória
    if (r1) free(r1);
    if (r2) free(r2);
    for (i = 0; i < 5; i++) {
        free(alt[i]);
    }
    free(alt);
}





//================================================== QUESTÃO 2 ==================================================//
void Q2() {
    int i;
    // Dados para sorteio
    int dado1[] = {20, 30, 40, 50, 60};
    int dado2[] = {10, 20, 30, 40};

    // Inicializar ponteiros como NULL para evitar erro no primeiro free()
    int *r1 = NULL, *r2 = NULL;

    // Alocação das alternativas
    char **alt = (char**) calloc(5, sizeof(char*));
    for (i = 0; i < 5; i++) {
        alt[i] = (char*) calloc(1000, sizeof(char));
    }

    do {
        // Liberar memória das permutações da iteração anterior
        if (r1) free(r1);
        if (r2) free(r2);

        r1 = randperm(5); // Gera permutação para os valores
        r2 = randperm(4); // Gera permutação para as taxas

        for (i = 0; i < 5; i++) {
            // CÁLCULO: Se a pergunta pede QUANTOS ALUNOS FORAM APROVADOS:
            // Aprovados = (Alunos * (100 - Taxa_Reprovados)) / 100
            int numero_aprovados = (dado1[r1[i]] * (100 - dado2[r2[i]])) / 100;

            sprintf(alt[i], "%d alunos\n", numero_aprovados);
        }

        // dist_alt garante que as alternativas geradas sejam diferentes entre si
    } while(dist_alt(alt, strlen(alt[0])));

    // Geração do arquivo LaTeX
    FILE *p = fopen("Q2.tex", "w");
    if (p == NULL) {
        printf("Erro ao abrir arquivo!\n");
        return;
    }

    fputs("\\parbox{\\linewidth}{\n\n% QUESTAO\n", p);

    // Note que usamos r1[0] e r2[0] para a pergunta, garantindo que a Resposta Correta (alt[0]) bata com os dados
    fprintf(p, "Em uma turma de %d alunos, %d\\%% foram reprovados em um exame. Quantos alunos foram aprovados?}\n",
            dado1[r1[0]], dado2[r2[0]]);

    fprintf(p, "\\vspace{2mm}\n\\\\\\parbox{\\linewidth}{\n\n%% ALTERNATIVAS\n");

    for (i = 0; i < 5; i++) {
        fputs(alt[i], p);
    }

    fputs("\n}\n\n", p);
    fclose(p);

    // Limpeza final de memória
    if (r1) free(r1);
    if (r2) free(r2);
    for (i = 0; i < 5; i++) {
        free(alt[i]);
    }
    free(alt);
}





