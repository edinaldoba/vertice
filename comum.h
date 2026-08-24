#ifndef COMUM_H
#define COMUM_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <glib.h>


#define NTI 10



//=========================================================================================================//
//                                       E N U M E R A Ç Õ E S                                             //
//=========================================================================================================//
typedef enum {
   ARQUIVO_INEXISTENTE = 1 << 0,
   ARQUIVO_VAZIO       = 1 << 1,
   ARQUIVO_PRONTO      = 1 << 2
} EstadoArquivo;

typedef enum {
   SUCESSO = 1 << 0,
   AVISO   = 1 << 1,
   ERRO    = 1 << 2,
   INFO    = 1 << 3
} MensagemTipo;

typedef enum {
   STATUS_PROVA_OK            = 1 << 0,
   ERRO_NUMERO_ALUNO_INVALIDO = 1 << 1,
   ERRO_ID_GABARITO_INVALIDO  = 1 << 2,
   AVISO_ALUNO_INATIVO        = 1 << 3
} StatusMapeamento;

// char cor_latex[32];
// switch ( ficha[j].atipico ) {
//     case ALUNO_AUTISTA:    sprintf( cor_latex, "\\color{blue!70}" );   break;
//     case ALUNO_ADHD:       sprintf( cor_latex, "\\color{orange!80}" ); break;
//     case ALUNO_DEFICIENTE: sprintf( cor_latex, "\\color{green!60!black}" ); break;
//     case ALUNO_LAUDADO:    sprintf( cor_latex, "\\color{purple!70}" ); break;
//     case ALUNO_OBSERVACAO: sprintf( cor_latex, "\\color{black!50}" );  break;
//     default:               sprintf( cor_latex, "" ); break; // Aluno típico
// }
// // Na hora do fprintf do nome:
// fprintf( p, "%.2d & %s%.25s & ...", j + 1, cor_latex, ficha[j].aluno );
//=========================================================================================================//




//=========================================================================================================//
//              E S T R U T U R A S    D E    D A D O S    B Á S I C A S                                   //
//=========================================================================================================//
typedef struct {
   int i, j;
} IndiceMatriz;

typedef struct {
   float x, y;
} Ponto2D;

typedef struct {
   char str[8];
} StringNota;

typedef struct {
   char str[32];
} ItemTextoCurto;

typedef struct {
   uint8_t r, g, b;
} PixelRGB;

typedef struct {
   char str[64];
} ItemCombo;

typedef struct {
   int dia, mes, ano;
} CalendarioData;

typedef struct {
   char aluno[64];
   bool ativo;
} FichaAuxiliar;
//=========================================================================================================//






//=========================================================================================================//
//   E S T R U T U R A S    D E    D A D O S   -   E S T A D O    A T U A L    D A    I N T E R F A C E    //
//=========================================================================================================//
typedef struct {
   // 1. Textos do Cabeçalho da Prova
   char ano[64], disciplina[64], escola[64], turma[64], periodo[64], gestor[64], professor[64];

   // 2. Textos do Estilo Visual da Prova
   char tema[64], decoracao_estilo[64], cor_destaque[64], prova_sequencia[64];

   // 3. Configurações Numéricas e Estatísticas da Turma
   int  serie, iprova, qtd_alunos_ativos, qtd_alunos_total;

   // 4. Configurações de Layout do LaTeX
   int  qtd_paginas, qtd_colunas, separadores, cabecalho_tipo, fonte_latex;

   // 5. Matriz do Acervo
   int  qtd_questoes[NTI];
   int total_questoes; // Total de questões da prova (padrão: 10 questões)
   struct {
      char str[128];
   } temas_prova_sequencia[NTI];

   // 6. Flags de Controle
   bool expor, cruz, naopresencial;

   // 7. Estilo visual da interface
   int interface_style;

} InterfaceDados;
//=========================================================================================================//
typedef struct {
   // 1. Estética e Identidade Visual (O que pode mudar por prova)
   char cor_destaque[64], decoracao_estilo[64];

   // 2. Configurações de Layout do LaTeX
   int  qtd_paginas, qtd_colunas, separadores, cabecalho_tipo, fonte_latex;

   // 3. O Miolo da Avaliação: Matriz do Acervo
   int  qtd_questoes[NTI];
   struct {
      char str[128];
   } temas_prova_sequencia[NTI];

   // 4. Flags de Controle Estéticas
   bool expor, naopresencial;
} DadosRascunho;
//=========================================================================================================//



//-------------------------------------------------------------------------------------------//
//          P L A N I L H A S    S I A E P                                                   //
//-------------------------------------------------------------------------------------------//
typedef struct {
   char aluno[64];
   char sexo; // M ou F
   char nasc[16];
} __attribute__( ( packed ) ) FichaSiaep;

typedef struct {
   uint32_t cod_aluno;
   char sit[8];
} __attribute__( ( packed ) ) AcessoTurmas;

typedef enum {
   PRESENTE             = 1 << 0, // Aluno presente e em sala de aula
   AUSENTE              = 1 << 1, // Falta não justificada
   FALTA_JUSTIFICADA    = 1 << 2, // Ausência abonada por critério pedagógico ou atestado
   DISPENSADO           = 1 << 3, // Ausente da sala (atividade externa, reunião ou trânsito pelo prédio)
   FORA_DE_SALA         = 1 << 4, // Afastamento temporário por medida disciplinar da direção
   FOI_EMBORA           = 1 << 5, // Saída antecipada autorizada (saúde ou busca pelos responsáveis)
   ATIVIDADE_DOMICILIAR = 1 << 6,  // Evasão não autorizada do recinto escolar durante o período letivo
   SUSPENSO             = 1 << 7  // Evasão não autorizada do recinto escolar durante o período letivo
} __attribute__( ( packed ) ) StatusAssiduidade;

typedef struct {
   char data[16];
   int n_horarios;
   char tema[32];
   char descricao[128];
} __attribute__( ( packed ) ) DadosRegistroDiario;
//-------------------------------------------------------------------------------------------//


//=========================================================================================================//
//             E S T R U T U R A    D E    D A D O S   -   B O L E T I M    A L U N O                      //
//=========================================================================================================//
typedef struct {
   char aluno[64];
   char sexo[16];
   char nasc[16];
   char rg[32];
   char cpf[16];
   char pai[64];
   char mae[64];
} __attribute__( ( packed ) ) BiografiaAluno;

typedef struct {
   char resp[64]; // Responsável
   char celular[16];
   char email[64];
} __attribute__( ( packed ) ) ContatoAluno;

typedef enum {
   SEM_SITUACAO          = 0,
   MATRICULA_REGULAR     = 1 << 0, // Aluno matriculado no começo do ano letivo
   MATRICULA_INTERNA     = 1 << 1, // Aluno matriculado no meio do ano letivo vindo de outra turma da mesma escola
   MATRICULA_EXTERNA     = 1 << 2, // Aluno matriculado no meio do ano letivo vindo de outra escola
   TRANSFERENCIA_INTERNA = 1 << 3, // Aluno transferido da turma para outra turma da mesma escola
   TRANSFERENCIA_EXTERNA = 1 << 4, // Aluno transferido da turma para outra escola
   EVADIDO               = 1 << 5  // Aluno deixou de frequentar e não pediu transferência
} __attribute__( ( packed ) ) SituacaoAluno;

typedef enum {
   ALUNO_TIPICO     = 1 << 0, // 1  (0001)
   ALUNO_AUTISTA    = 1 << 1, // 2  (0010)
   ALUNO_TDAH       = 1 << 2, // 4  (0100)
   ALUNO_DEFICIENTE = 1 << 3, // 8  (1000)
   ALUNO_LAUDADO    = 1 << 4, // 16 (0001 0000)
   ALUNO_OBSERVACAO = 1 << 5  // 32 (0010 0000)
} __attribute__( ( packed ) ) TipoAtipico;

typedef struct {

   char aluno[64];
   char sexo[16]; // Masculino ou Feminino
   char nasc[16];
   SituacaoAluno sit;

   int limite_corte;    // Formatação de impressão
   int idx;             // Na Vértice sempre ordem alfabética (idx siaep de origem preservado)
   gboolean ativo;      // Status de matrícula global
   TipoAtipico atipico; // Condição de adaptação curricular

   struct {
      float av;
      float rec;
   } nota[4][5];         // [4 Períodos][5 Avaliações]

   StatusAssiduidade frequencia[4][128]; // [4 Períodos][5 Avaliações]

   float rec_final;
   float conselho;

   float relatorio[6];   // Médias dos 4 períodos + rec. final + conselho

   int presencas[4];     // [4 Períodos]
   int faltas[4];        // [4 Períodos]

} __attribute__( ( packed ) ) FichaAlunoAux;

typedef struct {
   char aluno[64];

   int limite_corte;        // Formatação de impressão
   int idx;                 // Na Vértice sempre ordem alfabética (idx siaep de origem preservado)
   bool ativo;              // Status de matrícula global

   // --- 6. Campos Legados (Para futura remoção) ---
   int avaliacoes[4][10];
   float media[4];
} FichaAluno;
//=========================================================================================================//






//=========================================================================================================//
//              E S T R U T U R A    D E    D A D O S   -   D I R E T Ó R I O S                            //
//=========================================================================================================//
typedef struct {

   char dados[512],            // Caminho para a pasta de dados da turma em um dado período
        gabaritos[512],        // Caminho para a pasta de gabaritos da turma em um dado período
        relatorios[512],       // Caminho para a pasta de relatórios do período da turma
        externo[512],          // Caminho para a pasta externa de dados da turma em um dado período
        relatorios_final[512], // Caminho para salvar o relatórios final na hierarquia interna
        externo_final[512],    // Caminho para salvar o relatorios final na hierarquia externa
        externo_escola[256],   // Caminho para a pasta externa de relatórios que serão enviados para escola
        banco_questoes[256],   // Caminho para a pasta do Acervo da disciplina assinalada na interface
        base[320],
        questoes_tex[512];

   const char *recursos_prefix;

} CaminhoDiretorio;
//=========================================================================================================//






//=========================================================================================================//
//          E S T R U T U R A S    D E    D A D O S   -   C O R R E Ç Ã O    D I G I T A L                 //
//=========================================================================================================//
typedef struct {

   int num;        // Número do aluno na lista de frequência
   char resp[32];  //  Guarda o quadro de respostas assinaladas por cada aluno
   char nome_img[32];

   char direcao; // direção horizontal 'h' ou vertical 'v'

   uint32_t payload;

   StatusMapeamento status;

   uint8_t id;
   uint8_t turma;
   uint8_t disc;
   uint8_t per;
   uint8_t seq;

   int nota;

} MapeamentoGabarito; //  Referências da imagem


typedef struct {
   char key[5];      // Identificador do formato (ex: "P2", "P5")
   int ncol, nrow;   // Dimensões da imagem (Largura e Altura)
   int max;          // Valor máximo de intensidade do pixel
   int **image;      // Matriz bidimensional de tons de cinza alocada dinamicamente
} ImagemCinza;


typedef struct {
   char key[5];         // Identificador do formato (ex: "P3", "P6")
   int ncol, nrow;      // Dimensões da imagem
   int max;             // Valor máximo para os canais de cor
   PixelRGB **image;    // Matriz bidimensional de estruturas de pixel RGB
} ImagemColorida;
//=========================================================================================================//




#endif
