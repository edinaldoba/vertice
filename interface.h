#ifndef INTERFACE_H
#define INTERFACE_H

#include <gtk/gtk.h>
#include <stdbool.h>

#include "comum.h"

#define NTI 10


typedef struct {
   GtkWidget *ano;
   GtkWidget *disciplina;
   GtkWidget *escola;
   GtkWidget *turma;
   GtkWidget *periodo;
   GtkWidget *alunos;

   GtkWidget *tema;
   GtkWidget *tema_espelho;
   GtkWidget *cor_destaque;
   GtkWidget *decoracao_estilo;
} InterfaceEntry;




typedef struct {
   GtkWidget *avaliacao[3];               // Índice 0: 1ª Prova, 1: 2ª Prova, 2: 3ª Prova
   GtkWidget *qtd_paginas[2];             // Antes: npaginas[2]
   GtkWidget *qtd_colunas[2];             // Antes: colunas[2]
   GtkWidget *separadores[2];             // Mantido (Excelente nome)
   GtkWidget *cabecalho_tipo[2];        // Antes: cabecalho[2]
   GtkWidget *fonte_latex[2];           // Antes: fonte[2]
   GtkWidget *interface_style[3];
} InterfaceOpcao;






typedef struct {
   GtkWidget *expor_dados;         // Ativa exibição de logs detalhados de depuração

   // =========================================================================
   // VALIDAÇÃO DE INTEGRIDADE DOS CARTÕES-RESPOSTA
   // =========================================================================
   GtkWidget *validar_ciclos;      // Antes: bcruz ("Números Cruzados")
   // Ativa a detecção de permutações cíclicas na identificação dos alunos

   GtkWidget *nao_presencial;      // Aplica regras de correção para alunos em regime remoto
} InterfaceCheck;




typedef struct {
   // =========================================================================
   // GERENCIAMENTO DO Acervo
   // =========================================================================
   GtkWidget *executar_gcc_acervo;      // Compila ferramentas de apoio em C
   GtkWidget *compilar_latex_acervo;    // Renderiza o compilado LaTeX de questões
   GtkWidget *abrir_pdf_acervo;         // Visualiza o catálogo de questões em PDF

   // =========================================================================
   // GESTÃO PEDAGÓGICA: DIÁRIO DE CLASSE E RELATÓRIOS
   // =========================================================================
   GtkWidget *frequencia;              // Exporta/Gerencia chamadas e assiduidade
   GtkWidget *conteudos;               // Exporta/Gerencia o registro de aulas dadas
   GtkWidget *avaliacoes;              // Consolida as notas do período letivo
   GtkWidget *abrir;                   // Carrega arquivos base (.dat) do período ativo
   GtkWidget *relatorio_final;         // Consolida o fechamento anual/período

   // =========================================================================
   // CICLO DE PRODUÇÃO: GERAÇÃO E CORREÇÃO AUTOMÁTICA DE PROVAS
   // =========================================================================
   GtkWidget *carregar_dados;          // Sincroniza informações com o sistema escolar
   GtkWidget *gerar_prova;             // Dispara a montagem do documento LaTeX final
   GtkWidget *corrigir_prova;        // Interface para leitura automatizada de cartões-resposta
   GtkWidget *processamento_img;       // Processa os escaneamentos via rotinas de Visão Computacional

} InterfaceBotao;




typedef struct {
   int *qtd_subtemas;
   ItemCombo *subtemas;
   ItemCombo *temas;
   ItemCombo *anos;
   ItemCombo *escolas;
   ItemCombo *turmas;
   ItemCombo *disciplinas;

   // Ponteiros para as constantes
   const ItemCombo *periodos;
   const ItemCombo *cores_destaque;
   const ItemCombo *decoracoes_estilo;
   const ItemCombo *provas_sequencia;
} InterfaceListas;




// 1. Tipo para as coordenadas selecionadas na interface
typedef struct {
   int ano;
   int escola;
   int turma;
   int disciplina;
   int periodo;
   int aluno;
   int cor_destaque;
   int decoracao_estilo;
   int tema;
} FocoCoordenadas;

// 2. Tipo para as quantidades máximas físicas (limites de disco)
typedef struct {
   int anos;
   int escolas;
   int turmas;
   int disciplinas;
   int periodos;
   int alunos;
   int cores_destaque;
   int decoracoes_estilo;
   int temas;
   int subtemas;
} LimitesFiltro;

// 3. A estrutura unificada que consolida o painel de navegação
typedef struct {
   FocoCoordenadas foco;  // Instância do tipo CoordenadasFoco
   LimitesFiltro   limite; // Instância do tipo LimitesFiltro
} CascataControle;





typedef struct {
   gulong ano;
   gulong escola;
   gulong turma;
   gulong disciplina;
   gulong periodo;
   gulong alunos;
   gulong cor_destaque;
   gulong decoracao_estilo;
   gulong tema;
   gulong tema_espelho;
   gulong expor_dados;
   gulong nao_presencial;
} InterfaceHandlers;


typedef struct {
   GtkWidget *titulo, *subtitulo, *instrucao, *container, *cabecalho;
   gchar *format_titulo, *format_subtitulo, *format_instrucao, *format_cabecalho;
} InterfacePainel;


typedef struct {
   GtkWidget *professor;
   GtkWidget *gestor;
} InterfaceCabecalho;



typedef struct {
   GtkWidget *listbox_subtemas;
   GtkWidget *flowbox_selecionados;
   GtkWidget *scrolled_window;
   GtkWidget *btn_menos;
   GtkWidget *btn_mais;
   gulong **handler;
   gulong handler_scrolled;
   bool importar;
   int cont_add;
} InterfaceDinamica;

typedef struct {
   GtkWidget *listbox_subtemas;
} InterfaceLatex;


typedef struct {
   FichaAluno *diario;
   DataHoje data;
   CaminhoDiretorio caminho;

   GtkWidget *window;

   InterfaceBotao botao;
   InterfaceOpcao opcao;
   InterfaceCheck check;
   InterfaceEntry entry;

   InterfaceDados dados;

   InterfaceListas listas;

   GtkCssProvider *provider;
   CascataControle cascata; // O cérebro da navegação por filtros

   InterfaceHandlers handlers;

   InterfaceDinamica provas;

   InterfaceLatex latex;

   InterfacePainel painel;

   InterfaceCabecalho cabecalho;

   GRand *rand; // <- Ponteiro para o gerador de números aleatórios

} AppContext;





//==================== ESTRUTURAS DE DADOS AUXILIARES ========================
typedef struct {
   DadosRascunho   rascunho;
   FocoCoordenadas foco; // Mantém os focos para sincronizar os carrosséis estéticos
} PacotePersistencia;
//==================================================================






typedef void ( *ComboMapperFunc )( GtkListStore *store, GtkTreeIter *iter, const void *dados, int indice );
void popular_combo_box_generico( GtkWidget *combo, const void *dados, int limite, int foco,
                                 gulong handler_id, ComboMapperFunc mapper );


void interface_style( AppContext *ctx );

// Funções de carregamento e salvamento
void atualizar_booleanos_interface( const bool estado, const int categoria, AppContext *ctx );

void atualizar_generic_interface( AppContext *ctx, const int categoria, const int valor );

bool detectar_ubuntu( void );

bool carregar_estado_aplicativo( AppContext *ctx );

void atualizar_interface_com_dados( AppContext *ctx, const PacotePersistencia *pacote );

void atualizar_tema( AppContext *ctx, const char *tema );

void gerenciar_fluxo_gabaritos( GtkWidget *widget, InterfacePainel *painel, const AppContext *ctx );

gboolean mostrar_popup_confirmacao( GtkWindow *parent, const char *titulo, const char *mensagem );



void salvar_estado_aplicativo( const InterfaceDados *dados, const FocoCoordenadas *foco, const CaminhoDiretorio *caminho );

bool verificar_dados_da_interface( InterfacePainel *painel, const InterfaceDados *dados );

void popular_combo_box_text( GtkWidget *combo, const ItemCombo *lista, int foco, int limite, gulong handler_id );

void inicializar_estado_do_aplicativo( AppContext *ctx );

gboolean atualizar_ano_interface( AppContext *ctx, const char *novo_ano, gboolean forcar_atualizacao );
gboolean atualizar_escola_interface( AppContext *ctx, const char *nova_escola, gboolean forcar_atualizacao );
gboolean atualizar_turma_interface( AppContext *ctx, const char *nova_turma, gboolean forcar_atualizacao );
void atualizar_disciplina_interface( AppContext *ctx, const char *nova_disciplina, gboolean forcar_atualizacao );
void atualizar_periodo_interface( AppContext *ctx, const char *novo_periodo );


#endif
