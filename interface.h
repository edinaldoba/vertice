#ifndef INTERFACE_H
#define INTERFACE_H

#include <gtk/gtk.h>

#include "comum.h"



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
   GtkWidget *cmb_ano;
   GtkWidget *cmb_disciplina;
   GtkWidget *cmb_escola;
   GtkWidget *cmb_turma;
   GtkWidget *cmb_periodo;
   GtkWidget *cmb_estilo;

   gulong hdl_ano;
   gulong hdl_escola;
   gulong hdl_turma;
   gulong hdl_disciplina;

   gchar **anos;
   gchar **escolas;
   gchar **turmas;
   gchar **disciplinas;

   int foc_ano;
   int foc_escola;
   int foc_turma;
   int foc_disciplina;
   int foc_periodo;
   int foc_periodo_vigente;
   int foc_estilo;

   int lim_anos;
   int lim_escolas;
   int lim_turmas;
   int lim_disciplinas;

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
   GtkWidget *ent_data;
   GtkWidget *pop_calendario;
   GtkWidget *calendario_data;

   GtkWidget *btn_menos;
   GtkWidget *btn_mais;
   GtkWidget *lbl_ch;

   GtkWidget *treeview;          // Tabela principal de conteúdos

   GtkWidget *ent_tema;              // Entry do Tema
   GtkWidget *ent_descricao;         // Entry da Descrição
   GtkWidget *tipo_registro;     // Combo (Normal, Pedagógico, Feriado e Extra)
   GtkWidget *btn_salvar;
   GtkWidget *btn_rm_registro;

   GtkTreeIter iter_em_edicao;
   gboolean editando;            // FALSE = Novo Registro, TRUE = Editando Existente
} InterfaceConteudo;

// Sub-aba: Chamada e Assiduidade
typedef struct {
   GtkWidget *cmb_data;        // Filtra por data da aula
   GtkWidget *lbl_ch;
   GtkWidget *chk_por_aluno;
   GtkWidget *cmb_status;      // StatusAssiduidade (Presente, Falta Justificada, etc.)

   GtkWidget *btn_salvar;        // Aplica o status selecionado
   GtkWidget *btn_presente;      // Atalho para PRESENTE
   GtkWidget *btn_ausente;       // Atalho para AUSENTE
   GtkWidget *btn_justificada;   // Atalho para FALTA JUSTIFICADA

   GtkWidget *cmb_alunos;
   GtkWidget *treeview;          // Tabela de assiduidade

   gulong hdl_cmb_alunos;
   gulong hdl_cmb_data;

   int lim_cmb_alunos;
   int foc_cmb_alunos;
} InterfaceFrequencia;

// Sub-aba: Lançamento de Notas e Recuperações
typedef struct {
   GtkWidget *cmb_aval;   // Seleciona a coluna ativa (0=Av1, 1=Rec1... 9=Rec5)
   GtkWidget *btn_nova_aval;
   GtkWidget *btn_editar_aval;
   gboolean  *validas; // Adicionei dia 4 de novembro de 2026
   int qtd_validas;    // Adicionei dia 4 de novembro de 2026

   // Popover de nomenclatura da avaliação
   GtkWidget *pop_nomear_aval;
   GtkWidget *lbl_pop_aval;
   GtkWidget *ent_pop_nomear;
   GtkWidget *btn_pop_nomear;

   GtkWidget *chk_desativar_aval;
   gulong hdl_chk_desativar;

   GtkWidget *btn_salvar;
   GtkWidget *treeview;          // Tabela de notas

   gulong hdl_cmb_aval;
   int foc_aval;           // Guardião do índice da avaliação ativa (0 a 9)
} InterfaceAvaliacao;

// Sub-aba: Fechamento de Bimestre e Situação Final
typedef struct {
   GtkWidget *treeview;          // Tabela resumo
   GtkWidget *btn_consolidar;    // Executa cálculos finais
   GtkWidget *chk_pendencias;  // Filtra alunos em recuperação/dependência
} InterfaceRelatorio;

// Contêiner Mestre: Aba DIÁRIOS (Notebook Page 1)
typedef struct {
   // --- ARMAZENAMENTO EM MEMÓRIA RAM (Gerido pela GLib) ---
   GArray *aulas;    // Registros cronológicos de aula
   GPtrArray *fichas;     // Perfil e configurações dos alunos
   GArray *metas_avs;  // Lista de avaliações por período

   // --- CONTROLE DE AUTOSAVE E DISCO ---
   gboolean dados_modificados; // Flag "dirty" (true = precisa salvar)
   guint autosave_timer_id;    // Timer loop em background

   // Caminhos estáticos mantidos em RAM para evitar perda de referência
   // durante a troca de contextos (ex: usuário muda de turma sem salvar)
   gchar *path_save_diarios;
   gchar *path_save_fichas;
   gchar *path_save_metas_avs;

   GtkWidget *stack_pages;       // Gerencia a navegação entre as sub-abas

   gboolean botoes_travados;     // Controle de bloqueio de UI durante operações
   GtkWidget *btn_ativo;       // Ponteiro para o botão do stack em evidência

   InterfaceResponsavel responsavel;
   GtkWidget *chk_expor_dados; // Exporta pdfs de relatório para a pasta da escola
   GtkWidget *chk_modo_edicao;

   InterfaceConteudo   conteudo;
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

// Contêiner Mestre: Aba LATEX (Notebook Page 2)
typedef struct {
   GtkWidget *cmb_tema_espelho;
   gulong hdl_cmb_tema_espelho;
   GtkWidget *listbox_subtemas;

   GtkWidget *stack_pages;       // Gerencia a navegação entre as sub-abas
   GtkWidget *lbl_diag_acervo;
   GtkWidget *btn_config;

   // Parâmetros do gerador de documento (Radio buttons agrupados em vetor)
   GtkWidget *rad_fonte_latex[2];
   GtkWidget *rad_qtd_paginas[2];
   GtkWidget *rad_separadores[2];
   GtkWidget *rad_qtd_colunas[2];
   GtkWidget *rad_cabecalho_tipo[2];

   GtkWidget *cmb_cor_serie;
   GtkWidget *cmb_decoracao;

   int foc_cor_serie;
   int foc_decoracao;

   GtkWidget *btn_pdflatex;      // Visualiza o catálogo de questões selecionadas em PDF
   GtkWidget *btn_compilar;      // Renderiza o script LaTeX final
   GtkWidget *btn_diversificar;  // Compila e executa as rotinas geradoras em C
} InterfaceLatex;


/*** 4. MÓDULO PROVAS (Avaliações Customizadas e OMR) ***/

// Gerenciamento de itens de UI alocados em tempo de execução
typedef struct {
   GtkWidget *btn_menos;         // Criado dinâmicamente fora do Glade
   GtkWidget *btn_mais;          // Criado dinâmicamente fora do Glade
   gulong *hdls_chks;
   gulong hdl_scrolled;
   gboolean importar;
   int cont_add;
} InterfaceDinamica;

// Contêiner Mestre: Aba PROVAS (Notebook Page 3)
typedef struct {
   gchar **temas;
   int foc_tema;
   int lim_temas;

   GtkWidget *cmb_tema;
   gulong hdl_cmb_tema;

   gchar **subtemas;
   int lim_subtemas;

   GtkWidget *listbox_subtemas;

   GtkWidget *lbl_qtd_ativos;

   GtkWidget *cmb_seq_prova;
   int foc_seq_prova;

   GtkWidget *chk_nao_presencial; // Regras flexíveis para alunos remotos
   GtkWidget *chk_validar_ciclos; // Detecção de fraudes cíclicas nos gabaritos

   GtkWidget *scrolled_window;      // Área de rolagem da montagem
   GtkWidget *flowbox_selecionados; // Grid reativo das questões selecionadas

   InterfaceDinamica montagem;      // Estrutura de widgets instanciados via código

   GtkWidget *btn_gerar_prova;      // Motor Estocástico e LaTeX
   GtkWidget *btn_processar;        // OCR / Visão Computacional de cartões
   GtkWidget *btn_corrigir;         // Lógica de cálculo de nota sobre a matriz extraída
   GtkWidget *btn_importar;         // Recupera último setup de prova gerada
} InterfaceProva;


/*** 5. MÓDULO CONFIG (Parâmetros Globais, Escolas e Acervo) ***/

// Sub-aba: Configurações Gerais e Estrutura Escolar (vertice.ini)
typedef struct {
   GtkWidget *btn_salvar;
   // Adicione aqui os widgets específicos de escolas, turmas e disciplinas...
} InterfaceGeral;

// Sub-aba: Atribuição de Atipicidades e CIDs dos Alunos
typedef struct {
   GtkWidget *entry;
   GtkWidget *cmb_alunos;          // GtkComboBox associado ao GtkListStore da função mapear_alunos()
   gulong     hdl_cmb_alunos;

   GtkWidget *cmb_sup_tea;     // GtkComboBoxText (0=N/A, 1=Nível 1, 2=Nível 2, 3=Nível 3)
   gulong     hdl_cmb_sup_tea;

   GtkWidget *btn_relatorio;
   GtkWidget *flowbox;               // GtkFlowBox dos cards de alunos atípicos

   GtkWidget *chks[MAX_FLAGS_ATIPICIDADES];
   gulong     hdls_chks[MAX_FLAGS_ATIPICIDADES];
} InterfaceAtipicidades;

// Sub-aba: Gerenciamento do Acervo de Questões (TeX)
typedef struct {
   GtkWidget *btn_salvar;
   // Adicione aqui os widgets de manipulação do banco de questões...
} InterfaceAcervo;

// Contêiner Mestre: Aba CONFIG (Notebook Page 4)
typedef struct {
   GtkWidget *stack_pages;       // Sub-páginas: Geral, Atipicidades, Acervo

   GtkWidget *btn_sub_aba_geral;
   GtkWidget *btn_sub_aba_atip;
   GtkWidget *btn_sub_aba_acervo;

   InterfaceGeral       geral;
   InterfaceAtipicidades atipicidades;
   InterfaceAcervo      acervo;
} InterfaceConfig;


/*** 6. COMPONENTES DE FEEDBACK E ESTADO GLOBAL ***/

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
   // --- ÁRVORE DE INTERFACES (VIEW) ---
   InterfaceCabecalho cabecalho;
   InterfaceDiario    diario;
   InterfaceLatex     latex;
   InterfaceProva     prova;
   InterfaceConfig    config;
   InterfacePainel    painel;

   // --- ESTRUTURAS DE APOIO E CSS ---
   CalendarioData hoje;
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
                                 gulong hdl_id, ComboMapperFunc mapper );


void interface_style( AppContext *ctx );

// Funções de carregamento e salvamento
void atualizar_booleanos_interface( const gboolean estado, const int categoria, AppContext *ctx );

void atualizar_generic_interface( AppContext *ctx, const int categoria, const int valor );

void ui_label_aplicar_espacamento_manual( GtkWidget *lbl_widget, gint quantidade_espacos );

gboolean carregar_estado_aplicativo( AppContext *ctx );

void atualizar_tema( AppContext *ctx, char *tema );

gboolean mostrar_popup_confirmacao( GtkWindow *parent, const char *titulo, const char *mensagem );

void salvar_estado_aplicativo( const InterfaceDados *dados, const CaminhoDiretorio *caminho );

gboolean verificar_dados_da_interface( InterfacePainel *painel, const InterfaceDados *dados );

void popular_combo_box_text( GtkWidget *combo, gchar **lista, int foco, int limite, gulong hdl_id );
gchar *combo_box_text_get_text_at( GtkComboBox *combo, gint index );

void atualizar_dados_e_alunos_ativos( AppContext *ctx );
void inicializar_estado_do_aplicativo( AppContext *ctx );

gboolean atualizar_ano_interface( AppContext *ctx, gchar *novo_ano, gboolean forcar_atualizacao );
gboolean atualizar_escola_interface( AppContext *ctx, gchar *nova_escola, gboolean forcar_atualizacao );
gboolean atualizar_turma_interface( AppContext *ctx, gchar *nova_turma, gboolean forcar_atualizacao );
void atualizar_disciplina_interface( AppContext *ctx, gchar *nova_disciplina, gboolean forcar_atualizacao );
void atualizar_periodo_interface( AppContext *ctx, gchar *novo_periodo );


#endif
