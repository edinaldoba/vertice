//================================================== QUESTÃO 1 ==================================================//
void Q1() {
    int i;
    // Dados para sorteio
    int dado1[] = {1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000};
    int dado2[] = {10, 20, 30};
    int dado3[] = {2};

    // Inicializar ponteiros como NULL para evitar erro no primeiro free()
    int *r1 = NULL, *r2 = NULL, *r3 = NULL;

    // Alocação das alternativas
    char **alt = (char**) calloc(5, sizeof(char*));
    for (i = 0; i < 5; i++) {
        alt[i] = (char*) calloc(1000, sizeof(char));
    }

    do {
        // Liberar memória das permutações da iteração anterior
        if (r1) free(r1);
        if (r2) free(r2);
        if (r3) free(r3);

        r1 = randperm(9);
        r2 = randperm(3);
        r3 = randperm(1);

        for (i = 0; i < 5; i++) {

            int valor = dado1[r1[i]] / 100 * (100 + dado2[r2[i]]) * (100 + dado2[r2[i]]) / 100;

            sprintf(alt[i], "R\\$ $%d{,}00$\n", valor);
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
    fprintf(p, "Um capital de R\\$\\,$%d{,}00$ é aplicado a juros compostos de %d\\%% ao ano. Qual será o montante final após %d anos?}\n",
            dado1[r1[0]], dado2[r2[0]], dado3[r3[0]]);

    fprintf(p, "\\vspace{2mm}\n\\\\\\parbox{\\linewidth}{\n\n%% ALTERNATIVAS\n");

    for (i = 0; i < 5; i++) {
        fputs(alt[i], p);
    }

    fputs("\n}\n\n", p);
    fclose(p);

    // Limpeza final de memória
    if (r1) free(r1);
    if (r2) free(r2);
    if (r3) free(r3);
    for (i = 0; i < 5; i++) {
        free(alt[i]);
    }
    free(alt);
}





//================================================== QUESTÃO 2 ==================================================//
void Q2() {
    int i;
    // Dados para sorteio
    int dado1[] = {1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000};
    int dado2[] = {10, 20, 30};
    int dado3[] = {2};

    // Inicializar ponteiros como NULL para evitar erro no primeiro free()
    int *r1 = NULL, *r2 = NULL, *r3 = NULL;

    // Alocação das alternativas
    char **alt = (char**) calloc(5, sizeof(char*));
    for (i = 0; i < 5; i++) {
        alt[i] = (char*) calloc(1000, sizeof(char));
    }

    do {
        // Liberar memória das permutações da iteração anterior
        if (r1) free(r1);
        if (r2) free(r2);
        if (r3) free(r3);

        r1 = randperm(9);
        r2 = randperm(3);
        r3 = randperm(1);

        for (i = 0; i < 5; i++) {

            int valor = dado1[r1[i]];

            sprintf(alt[i], "R\\$ $%d{,}00$\n", valor);
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
    fprintf(p, "Um investidor deseja obter um montante de R\\$\\,$%d{,}00$ ao final de %d meses. Sabendo que a taxa de juros compostos da aplicação é de %d\\%% ao mês, qual deve ser o capital inicial aplicado por ele?}\n",
            dado1[r1[0]] / 100 * (100 + dado2[r2[0]]) * (100 + dado2[r2[0]]) / 100, dado3[r3[0]], dado2[r2[0]] );

    fprintf(p, "\\vspace{2mm}\n\\\\\\parbox{\\linewidth}{\n\n%% ALTERNATIVAS\n");

    for (i = 0; i < 5; i++) {
        fputs(alt[i], p);
    }

    fputs("\n}\n\n", p);
    fclose(p);

    // Limpeza final de memória
    if (r1) free(r1);
    if (r2) free(r2);
    if (r3) free(r3);
    for (i = 0; i < 5; i++) {
        free(alt[i]);
    }
    free(alt);
}




