#ifndef INTERFACE_H
#define INTERFACE_H

#include <gtk/gtk.h>


#include "comum.h"

#define NTI 10


/* ==========================================================================
 * VÉRTICE: ESTRUTURAS DE MAPEAMENTO DA INTERFACE GRÁFICA (GTK)
 * ==========================================================================
 * Esta seção define a árvore de estado visual da aplicação. Cada struct
 * espelha um contêiner específico (Aba, Popover ou Painel) do arquivo Glade,
 * isolando as responsabilidades e facilitando o rastreamento de ponteiros.
 * ========================================================================== */

/*** 1. CONTROLES GLOBAIS ***/

// Barra superior de filtros globais (Ano, Escola, Turma, etc.)
typedef struct {
   GtkWidget *combo_ano;
   GtkWidget *combo_disciplina;
   GtkWidget *combo_escola;
   GtkWidget *combo_turma;
   GtkWidget *combo_periodo;
   GtkWidget *combo_estilo;

   gulong handler_ano;
   gulong handler_escola;
   gulong handler_turma;
   gulong handler_disciplina;

   ItemCombo *anos;
   ItemCombo *escolas;
   ItemCombo *turmas;
   ItemCombo *disciplinas;

   int foco_ano;
   int foco_escola;
   int foco_turma;
   int foco_disciplina;
   int foco_periodo;

   int limite_anos;
   int limite_escolas;
   int limite_turmas;
   int limite_disciplinas;

   GtkWidget *btn_atualizar_alunos; // Sincroniza alunos a partir das planilhas SIAEP
} InterfaceCabecalho;

// Identificação institucional (Exibida logo abaixo do cabeçalho)
typedef struct {
   GtkWidget *professor;
   GtkWidget *gestor;
} InterfaceResponsavel;


/*** 2. MÓDULO DIÁRIO (Sub-páginas do GtkStack) ***/

// Sub-aba: Registro de Aulas e Conteúdos
typedef struct {
   GtkWidget *entry_data;
   GtkWidget *popover_calendario;
   GtkWidget *calendario_data;

   GtkWidget *stepper_menos;
   GtkWidget *stepper_mais;
   GtkWidget *qtd_aulas;

   GtkWidget *treeview;          // Tabela principal de conteúdos

   GtkWidget *tema;              // Entry do Tema
   GtkWidget *descricao;         // Entry da Descrição
   GtkWidget *tipo_registro;     // Combo (Normal, Pedagógico, Extra, etc.)
   GtkWidget *btn_salvar;
   GtkWidget *remover_registro;

   GtkTreeIter iter_em_edicao;
   gboolean editando;            // FALSE = Novo Registro, TRUE = Editando Existente
} InterfaceConteudo;

// Sub-aba: Chamada e Assiduidade
typedef struct {
   GtkWidget *combo_data;        // Filtra por data da aula
   GtkWidget *label_ch;
   GtkWidget *check_por_aluno;
   GtkWidget *combo_status;      // StatusAssiduidade (Presente, Falta Justificada, etc.)

   GtkWidget *btn_salvar;        // Aplica o status selecionado
   GtkWidget *btn_presente;      // Atalho para PRESENTE
   GtkWidget *btn_ausente;       // Atalho para AUSENTE
   GtkWidget *btn_justificada;   // Atalho para FALTA JUSTIFICADA

   GtkWidget *combo_alunos;
   GtkWidget *treeview;          // Tabela de assiduidade

   gulong handler_combo_alunos;
   gulong handler_combo_data;

   int limite_combo_alunos;
   int foco_combo_alunos;
} InterfaceFrequencia;

// Sub-aba: Lançamento de Notas e Recuperações
typedef struct {
   GtkWidget *combo_avaliacao;   // Seleciona a coluna ativa (0=Av1, 1=Rec1... 9=Rec5)
   GtkWidget *btn_nova_avaliacao;
   GtkWidget *btn_editar_avaliacao;

   // Popover de nomenclatura da avaliação
   GtkWidget *popover_nomear_avaliacao;
   GtkWidget *label_popover_avaliacao;
   GtkWidget *entry_popover_nomear;
   GtkWidget *btn_popover_nomear;

   GtkWidget *check_desativar_avaliacao;
   gulong handler_check_desativar;

   GtkWidget *btn_salvar;
   GtkWidget *treeview;          // Tabela de notas

   gulong handler_combo_avaliacao;
   int foco_avaliacao;           // Guardião do índice da avaliação ativa (0 a 9)
} InterfaceAvaliacao;

// Sub-aba: Fechamento de Bimestre e Situação Final
typedef struct {
   GtkWidget *treeview;          // Tabela resumo
   GtkWidget *btn_consolidar;    // Executa cálculos finais
   GtkWidget *check_pendencias;  // Filtra alunos em recuperação/dependência
} InterfaceRelatorio;

// Contêiner Mestre: Aba DIÁRIOS (Notebook Page)
typedef struct {
   GtkWidget *stack_pages;       // Gerencia a navegação entre as sub-abas

   gboolean botoes_travados;     // Controle de bloqueio de UI durante operações
   GtkWidget *botao_ativo;       // Ponteiro para o botão do stack em evidência

   InterfaceResponsavel responsavel;
   GtkWidget *check_expor_dados; // Exporta pdfs de relatório para a pasta da escola
   GtkWidget *check_modo_edicao;

   InterfaceConteudo  conteudo;
   InterfaceFrequencia frequencia;
   InterfaceAvaliacao  avaliacao;
   InterfaceRelatorio  relatorio;

   // Botões de navegação lateral/inferior do Stack
   GtkWidget *btn_conteudo;
   GtkWidget *btn_frequencia;
   GtkWidget *btn_avaliacao;
   GtkWidget *btn_relatorio;
} InterfaceDiario;


/*** 3. MÓDULO LATEX (Sandbox de Diagramação e Acervo) ***/

// Contêiner Mestre: Aba LATEX (Notebook Page)
typedef struct {
   GtkWidget *combo_tema_espelho;
   gulong handler_combo_tema_espelho;
   GtkWidget *listbox_subtemas;

   // Parâmetros do gerador de documento (Radio buttons agrupados em vetor)
   GtkWidget *radio_fonte_latex[2];
   GtkWidget *radio_qtd_paginas[2];
   GtkWidget *radio_separadores[2];
   GtkWidget *radio_qtd_colunas[2];
   GtkWidget *radio_cabecalho_tipo[2];

   GtkWidget *combo_cor_serie;
   GtkWidget *combo_decoracao;

   GtkWidget *btn_pdflatex;      // Visualiza o catálogo de questões selecionadas em PDF
   GtkWidget *btn_compilar;      // Renderiza o script LaTeX final
   GtkWidget *btn_diversificar;  // Compila e executa as rotinas geradoras em C
} InterfaceLatex;


/*** 4. MÓDULO PROVAS (Avaliações Customizadas e OMR) ***/

// Gerenciamento de itens de UI alocados em tempo de execução
typedef struct {
   GtkWidget *btn_menos;         // Criado dinâmicamente fora do Glade
   GtkWidget *btn_mais;          // Criado dinâmicamente fora do Glade
   gulong **handlers_check;
   gulong handler_scrolled;
   gboolean importar;
   int cont_add;
} InterfaceDinamica;

// Contêiner Mestre: Aba PROVAS (Notebook Page)
typedef struct {
   ItemCombo *temas;
   int foco_tema;
   int limite_temas;

   GtkWidget *combo_tema;
   gulong handler_combo_tema;

   ItemCombo *subtemas;
   int limite_subtemas;

   GtkWidget *listbox_subtemas;

   GtkWidget *combo_seq_prova;
   GtkWidget *check_nao_presencial; // Regras flexíveis para alunos remotos
   GtkWidget *check_validar_ciclos; // Detecção de fraudes cíclicas nos gabaritos

   GtkWidget *scrolled_window;      // Área de rolagem da montagem
   GtkWidget *flowbox_selecionados; // Grid reativo das questões selecionadas

   InterfaceDinamica montagem;      // Estrutura de widgets instanciados via código

   GtkWidget *btn_gerar_prova;      // Motor Estocástico e LaTeX
   GtkWidget *btn_processar;        // OCR / Visão Computacional de cartões
   GtkWidget *btn_corrigir;         // Lógica de cálculo de nota sobre a matriz extraída
   GtkWidget *btn_importar;         // Recupera último setup de prova gerada
} InterfaceProva;


/*** 5. COMPONENTES DE FEEDBACK E ESTADO GLOBAL ***/

// Notificações flutuantes assíncronas (Toast)
typedef struct {
   GtkWidget *eventbox_orelhinha;
   GtkWidget *eventbox_painel;
   GtkWidget *revealer_painel;
   GtkWidget *titulo, *subtitulo, *instrucao, *container, *cabecalho;

   gchar *format_titulo, *format_subtitulo, *format_instrucao, *format_cabecalho;
   guint timeout_id;
   gboolean mouse_hover;

   // Fila de mensagens encadeadas
   gboolean tem_proxima_mensagem;
   MensagemTipo proximo_tipo;
   char *proximo_titulo;
   char *proximo_subtitulo;
   char *proximo_instrucao;
} InterfacePainel;

// O DEUS DA APLICAÇÃO: O Contexto Mestre
typedef struct {
   // --- ARMAZENAMENTO EM MEMÓRIA RAM (Gerido pela GLib) ---
   GArray *diarios;    // Registros cronológicos de aula
   GArray *fichas;     // Perfil e configurações dos alunos
   GArray *avaliacoes; // Lista de avaliações por período

   // --- CONTROLE DE AUTOSAVE E DISCO ---
   gboolean dados_modificados; // Flag "dirty" (true = precisa salvar)
   guint autosave_timer_id;    // Timer loop em background

   // Caminhos estáticos mantidos em RAM para evitar perda de referência
   // durante a troca de contextos (ex: usuário muda de turma sem salvar)
   gchar *path_save_diario;
   gchar *path_save_avaliacao;
   gchar *path_save_fichas;

   // --- ÁRVORE DE INTERFACES (VIEW) ---
   InterfaceCabecalho cabecalho;
   InterfaceDiario    ui_diario;
   InterfaceLatex     ui_latex;
   InterfaceProva     ui_prova;
   InterfacePainel    painel;

   // --- ESTRUTURAS DE APOIO E CSS ---
   CalendarioData data;
   CaminhoDiretorio caminho;
   InterfaceDados dados;

   GtkWidget *window;            // Janela Pai (Top Level)
   GtkWidget *notebook;          // Navegação de 1º Nível (Diário | Latex | Provas)
   GtkCssProvider *provider;     // Motor de estilização CSS nativo

   // --- GERADORES NATIVOS ---
   GRand *rand;                  // Semente estocástica da GLib
} AppContext;







void parar_autosave_diario( AppContext *ctx );

void popular_datas( AppContext *ctx );

typedef void ( *ComboMapperFunc )( GtkListStore *store, GtkTreeIter *iter, const void *dados, int indice );
void popular_combo_box_generico( GtkWidget *combo, const void *dados, int limite, int foco,
                                 gulong handler_id, ComboMapperFunc mapper );


void interface_style( AppContext *ctx );

// Funções de carregamento e salvamento
void atualizar_gbooleaneanos_interface( const gboolean estado, const int categoria, AppContext *ctx );

void atualizar_generic_interface( AppContext *ctx, const int categoria, const int valor );

gboolean detectar_ubuntu( void );

gboolean carregar_estado_aplicativo( AppContext *ctx );

void atualizar_tema( AppContext *ctx, const char *tema );

void gerenciar_fluxo_gabaritos( GtkWidget *widget, InterfacePainel *painel, const AppContext *ctx );

gboolean mostrar_popup_confirmacao( GtkWindow *parent, const char *titulo, const char *mensagem );

void salvar_estado_aplicativo( const InterfaceDados *dados, const CaminhoDiretorio *caminho );

gboolean verificar_dados_da_interface( InterfacePainel *painel, const InterfaceDados *dados );

void popular_combo_box_text( GtkWidget *combo, const ItemCombo *lista, int foco, int limite, gulong handler_id );

void atualizar_dados_e_alunos_ativos( AppContext *ctx );
void inicializar_estado_do_aplicativo( AppContext *ctx );

gboolean atualizar_ano_interface( AppContext *ctx, const char *novo_ano, gboolean forcar_atualizacao );
gboolean atualizar_escola_interface( AppContext *ctx, const char *nova_escola, gboolean forcar_atualizacao );
gboolean atualizar_turma_interface( AppContext *ctx, const char *nova_turma, gboolean forcar_atualizacao );
void atualizar_disciplina_interface( AppContext *ctx, const char *nova_disciplina, gboolean forcar_atualizacao );
void atualizar_periodo_interface( AppContext *ctx, const char *novo_periodo );


#endif
