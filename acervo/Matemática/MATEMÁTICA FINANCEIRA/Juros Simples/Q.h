//================================================== QUESTÃO 1 ==================================================//
void Q1() {
    int i;
    // Dados para sorteio
    int dado1[] = {1000, 2000};
    int dado2[] = {1, 2, 3, 4};
    int dado3[] = {2, 3, 4, 5, 6};

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

        r1 = randperm(2);
        r2 = randperm(4);
        r3 = randperm(5);

        for (i = 0; i < 5; i++) {

            int valor = dado1[r1[i]] * dado2[r2[i]] * dado3[r3[i]] / 100;

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
    fprintf(p, "Qual o juro simples produzido por um capital de R\\$\\,$%d{,}00$ aplicado a uma taxa de %d\\%% ao mês, durante %d meses?}\n",
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
    int dado1[] = {600, 700, 800, 900};
    int dado2[] = {4 ,5, 6, 7};
    int dado3[] = {2, 3, 4, 5};

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

        r1 = randperm(4);
        r2 = randperm(4);
        r3 = randperm(4);

        for (i = 0; i < 5; i++) {

            int valor = dado1[r1[i]] * (100 + dado2[r2[i]] * dado3[r3[i]] ) / 100;

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
    fprintf(p, "Um capital de R\\$\\,$%d{,}00$ foi aplicado a juros simples de %d\\%% ao ano por %d anos. Qual o montante (capital + juros) ao final do período?}\n",
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





//================================================== QUESTÃO 3 ==================================================//
void Q3() {
    int i;
    // Dados para sorteio
    int dado1[] = {1000, 2000, 3000}; // Capital
    int dado2[] = {2, 3, 4, 5, 6}; // Taxa
    int dado3[] = {2, 3, 4, 5}; // Tempo

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

        r1 = randperm(3);
        r2 = randperm(5);
        r3 = randperm(4);

        for (i = 0; i < 5; i++) {

            int valor = dado2[r2[i]];

            sprintf(alt[i], "%d\\%% ao mês\n", valor);
        }

        // dist_alt garante que as alternativas geradas sejam diferentes entre si
    } while(dist_alt(alt, strlen(alt[0])));

    // Geração do arquivo LaTeX
    FILE *p = fopen("Q3.tex", "w");
    if (p == NULL) {
        printf("Erro ao abrir arquivo!\n");
        return;
    }

    fputs("\\parbox{\\linewidth}{\n\n% QUESTAO\n", p);

    // Note que usamos r1[0] e r2[0] para a pergunta, garantindo que a Resposta Correta (alt[0]) bata com os dados
    fprintf(p, "Se um empréstimo de R\\$\\,$%d{,}00$ a juros simples gerou R\\$\\,$%d{,}00$ de juros em %d meses, qual foi a taxa mensal aplicada?}\n",
            dado1[r1[0]], dado1[r1[0]] * dado2[r2[0]] * dado3[r3[0]] / 100, dado3[r3[0]]);

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



//================================================== QUESTÃO 4 ==================================================//
void Q4() {
    int i;
    // Dados para sorteio
    int dado1[] = {1100, 1200, 1300, 1400, 1500}; // Capital
    int dado2[] = {2, 3, 4, 5, 6}; // Taxa
    int dado3[] = {2, 3, 4, 5, 6}; // Tempo

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

        r1 = randperm(5);
        r2 = randperm(5);
        r3 = randperm(5);

        for (i = 0; i < 5; i++) {

            int valor = dado3[r3[i]];

            sprintf(alt[i], "%d meses\n", valor);
        }

        // dist_alt garante que as alternativas geradas sejam diferentes entre si
    } while(dist_alt(alt, strlen(alt[0])));

    // Geração do arquivo LaTeX
    FILE *p = fopen("Q4.tex", "w");
    if (p == NULL) {
        printf("Erro ao abrir arquivo!\n");
        return;
    }

    fputs("\\parbox{\\linewidth}{\n\n% QUESTAO\n", p);

    // Note que usamos r1[0] e r2[0] para a pergunta, garantindo que a Resposta Correta (alt[0]) bata com os dados
    fprintf(p, "Durante quanto tempo um capital de R\\$\\,$%d{,}00$ deve ser aplicado a juros simples de %d\\%% ao mês para render R\\$\\,$%d{,}00$ de juros?}\n",
            dado1[r1[0]], dado2[r2[0]], dado1[r1[0]] * dado2[r2[0]] * dado3[r3[0]] / 100);

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



