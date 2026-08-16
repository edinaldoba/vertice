//================================================== QUESTÃO 1 ==================================================//
void Q1() {
    int i;
    // Dados para sorteio
    int dado1[] = {200, 300, 400, 500, 600, 700, 800, 900};
    int dado2[] = {10, 20, 30};
    int dado3[] = {10, 20, 30};

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

        r1 = randperm(8);
        r2 = randperm(3);
        r3 = randperm(3);

        for (i = 0; i < 5; i++) {

            int valor = dado1[r1[i]] * (100 + dado2[r2[i]]) / 100 * (100 + dado3[r3[i]]) / 100;

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
    fprintf(p, "Um casaco de R\\$\\,$%d{,}00$ recebe um aumento de %d\\%% e, logo depois, um novo aumento de %d\\%% sobre o valor já reajustado. Qual o preço final?}\n",
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
    int dado1[] = {300, 400, 500, 600, 700, 800, 900};
    int dado2[] = {10, 20, 30};
    int dado3[] = {10, 20, 30};

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

        r1 = randperm(7);
        r2 = randperm(3);
        r3 = randperm(3);

        for (i = 0; i < 5; i++) {

            int valor = dado1[r1[i]] * (100 - dado2[r2[i]]) / 100 * (100 - dado3[r3[i]]) / 100;

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
    fprintf(p, "Uma loja dá um desconto de %d\\%% em um tênis de R\\$\\,$%d{,}00$. Para quem paga à vista, a loja oferece mais %d\\%% de desconto sobre o valor já reduzido. Quanto custará o tênis à vista?}\n",
            dado2[r2[0]], dado1[r1[0]], dado3[r3[0]]);

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



