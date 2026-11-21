/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include "layout.h"
#include "gtk/gtk.h"
#include "interface.h"


// ======================================================================================
// 1. FUNÇÕES AUXILIARES DE ESTILIZAÇÃO E ALINHAMENTO
// ======================================================================================
/**
 * @brief Extrai o GtkLabel interno de um botão configurado pelo Glade e aplica o espaçamento.
 */
static void ui_button_label_aplicar_espacamento_manual_lc( GtkWidget *button_widget, gint quantidade_espacos ) {
   g_return_if_fail( GTK_IS_BUTTON( button_widget ) );
   GtkWidget *label_interno = gtk_bin_get_child( GTK_BIN( button_widget ) );
   if ( GTK_IS_LABEL( label_interno ) ) {
      ui_label_aplicar_espacamento_manual( label_interno, quantidade_espacos );
   }
}

/**
 * @brief Alinha o conteúdo de uma coluna de um GtkTreeView com controle independente para cabeçalho e células.
 *
 * @param widget Pointer para o GtkTreeView.
 * @param coluna Índice base zero da coluna.
 * @param align_celula Alinhamento horizontal das células de dados (0.0 = esquerda, 0.5 = centro, 1.0 = direita).
 * @param pad_celula_x Espaçamento horizontal interno das células de dados (em pixels).
 * @param align_cabecalho Alinhamento horizontal do título no cabeçalho (0.0 = esquerda, 0.5 = centro, 1.0 = direita).
 */
static void treeview_configurar_alinhamento_coluna_lc( GtkWidget *widget,
      int coluna,
      double align_celula,
      guint pad_celula_x,
      float align_cabecalho ) {
   g_return_if_fail( GTK_IS_TREE_VIEW( widget ) );

   GtkTreeViewColumn *col_ch = gtk_tree_view_get_column( GTK_TREE_VIEW( widget ), coluna );
   if ( !col_ch ) {
      return;
   }

   // 1. Controle do Cabeçalho (Botão superior)
   gtk_tree_view_column_set_alignment( col_ch, align_cabecalho );

   // Força o ajuste do GtkLabel interno do cabeçalho para garantir alinhamento perfeito
   GtkWidget *widget_cabecalho = gtk_tree_view_column_get_widget( col_ch );
   if ( GTK_IS_LABEL( widget_cabecalho ) ) {
      gtk_label_set_xalign( GTK_LABEL( widget_cabecalho ), align_cabecalho );
   }

   // 2. Controle das Células de Dados (Linhas da tabela)
   GList *renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( col_ch ) );
   if ( renderers ) {
      g_object_set( G_OBJECT( renderers->data ),
                    "xalign", align_celula,
                    "xpad", pad_celula_x,
                    NULL );
      g_list_free( renderers );
   }
}



/**
 * @brief Configura atributos OpenType (ex: números tabulares/alinhados) no renderer de uma coluna do GtkTreeView.
 *
 * @param widget Ponteiro para o GtkTreeView.
 * @param coluna Índice base zero da coluna a ser modificada.
 * @param font_features String de features OpenType (ex: "tnum=1, lnum=1, zero=1").
 */
static void treeview_configurar_font_features_col_lc( GtkWidget *widget, int coluna, const char *font_features ) {
   g_return_if_fail( GTK_IS_TREE_VIEW( widget ) );
   g_return_if_fail( font_features != NULL );

   GtkTreeViewColumn *col_ch = gtk_tree_view_get_column( GTK_TREE_VIEW( widget ), coluna );
   if ( !col_ch ) {
      return;
   }

   GList *renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( col_ch ) );

   // Itera sobre todos os renderers da coluna (previne bugs se houver ícone + texto juntos)
   for ( GList *l = renderers; l != NULL; l = l->next ) {
      if ( GTK_IS_CELL_RENDERER_TEXT( l->data ) ) {
         PangoAttrList *attrs = NULL;

         // 1. Resgata a lista de atributos existente para não destruir formatações prévias (ex: cores, negrito)
         g_object_get( G_OBJECT( l->data ), "attributes", &attrs, NULL );

         if ( !attrs ) {
            attrs = pango_attr_list_new();
         }

         // 2. Cria e insere o atributo de font features
         // O 'pango_attr_list_insert' assume a propriedade do atributo, não precisamos dar free nele
         PangoAttribute *attr_features = pango_attr_font_features_new( font_features );
         pango_attr_list_insert( attrs, attr_features );

         // 3. Aplica a lista atualizada de volta ao GtkCellRendererText
         g_object_set( G_OBJECT( l->data ), "attributes", attrs, NULL );

         // 4. Libera a nossa referência (g_object_get ou pango_attr_list_new adicionam +1 ref)
         pango_attr_list_unref( attrs );
      }
   }

   // 5. Libera apenas a lista (os renderers internos pertencem à coluna e não devem sofrer unref aqui)
   g_list_free( renderers );
}





//===================================================================================================
// FUNÇÃO VISUAL: Aplica a classe CSS no botão de cabeçalho da coluna NOTA
// Chamar apenas UMA VEZ durante a inicialização (ex: layout.c)
//===================================================================================================
static void destacar_cabecalho_nota_lc( GtkWidget *treeview_avaliacoes ) {
   g_return_if_fail( treeview_avaliacoes != NULL && GTK_IS_TREE_VIEW( treeview_avaliacoes ) );

   // 1. Obtém a coluna 12 (Nota do Período) da TreeView de Avaliações
   GtkTreeViewColumn *col_nota = gtk_tree_view_get_column( GTK_TREE_VIEW( treeview_avaliacoes ), 12 );
   if ( !col_nota ) {
      return;
   }

   // 2. Captura o widget real do botão do cabeçalho
   GtkWidget *button = gtk_tree_view_column_get_button( col_nota );
   if ( !button ) {
      return;
   }

   // 3. Adiciona a classe CSS nativamente
   GtkStyleContext *context = gtk_widget_get_style_context( button );
   gtk_style_context_add_class( context, "cabecalho-destaque-nota" );
}






// ======================================================================================
// 2. FUNÇÕES DE MAPEAMENTO DE WIDGETS POR DOMÍNIO
// ======================================================================================

static void mapear_janela_mestra_lc( AppContext *ctx, GtkBuilder *builder, GtkApplication *app ) {
   ctx->window   = GTK_WIDGET( gtk_builder_get_object( builder, "janela_principal" ) );
   ctx->notebook = GTK_WIDGET( gtk_builder_get_object( builder, "notebook_principal" ) );

   gtk_window_set_application( GTK_WINDOW( ctx->window ), app );

   gtk_window_set_position( GTK_WINDOW( ctx->window ), GTK_WIN_POS_CENTER );

   // Ativação da HeaderBar customizada para controle CSS
   GtkWidget *header = gtk_header_bar_new();
   gtk_header_bar_set_show_close_button( GTK_HEADER_BAR( header ), TRUE );
   gtk_header_bar_set_title( GTK_HEADER_BAR( header ), "Vértice: diários e provas" );
   gtk_window_set_titlebar( GTK_WINDOW( ctx->window ), header );
}


static void mapear_cabecalho_lc( AppContext *ctx, GtkBuilder *builder ) {
   InterfaceCabecalho *cab = &ctx->cabecalho;

   cab->cmb_ano            = GTK_WIDGET( gtk_builder_get_object( builder, "combo_ano" ) );
   cab->cmb_escola         = GTK_WIDGET( gtk_builder_get_object( builder, "combo_escola" ) );
   cab->cmb_turma          = GTK_WIDGET( gtk_builder_get_object( builder, "combo_turma" ) );
   cab->cmb_disciplina     = GTK_WIDGET( gtk_builder_get_object( builder, "combo_disciplina" ) );
   cab->cmb_periodo        = GTK_WIDGET( gtk_builder_get_object( builder, "combo_periodos" ) );
   cab->cmb_estilo         = GTK_WIDGET( gtk_builder_get_object( builder, "combo_estilo" ) );

   cab->btn_atualizar_alunos = GTK_WIDGET( gtk_builder_get_object( builder, "button_atualizar_alunos" ) );
}


static void mapear_aba_diarios_lc( AppContext *ctx, GtkBuilder *builder ) {
   InterfaceDiario     *diario     = &ctx->diario;
   InterfaceConteudo   *cont   = &ctx->diario.conteudo;
   InterfaceFrequencia *freq = &ctx->diario.frequencia;
   InterfaceAvaliacao  *aval  = &ctx->diario.avaliacao;
   InterfaceRelatorio  *rel  = &ctx->diario.relatorio;

   // --- RAIZ DA ABA DIÁRIOS ---
   diario->stack_pages            = GTK_WIDGET( gtk_builder_get_object( builder, "stack_pages_diarios" ) );
   diario->chk_modo_edicao      = GTK_WIDGET( gtk_builder_get_object( builder, "check_modo_edicao" ) );
   diario->chk_expor_dados      = GTK_WIDGET( gtk_builder_get_object( builder, "check_expor_dados" ) );
   diario->responsavel.gestor     = GTK_WIDGET( gtk_builder_get_object( builder, "box_gestor" ) );
   diario->responsavel.professor  = GTK_WIDGET( gtk_builder_get_object( builder, "box_professor" ) );

   diario->btn_conteudo           = GTK_WIDGET( gtk_builder_get_object( builder, "button_conteudos" ) );
   diario->btn_frequencia         = GTK_WIDGET( gtk_builder_get_object( builder, "button_frequencia" ) );
   diario->btn_avaliacao          = GTK_WIDGET( gtk_builder_get_object( builder, "button_avaliacoes" ) );
   diario->btn_relatorio          = GTK_WIDGET( gtk_builder_get_object( builder, "button_relatorio_final" ) );

   // --- SUB-ABA: CONTEÚDOS ---
   cont->ent_data         = GTK_WIDGET( gtk_builder_get_object( builder, "entry_data" ) );
   cont->pop_calendario = GTK_WIDGET( gtk_builder_get_object( builder, "popover_calendario" ) );
   cont->calendario_data    = GTK_WIDGET( gtk_builder_get_object( builder, "calendar_data" ) );
   cont->btn_menos      = GTK_WIDGET( gtk_builder_get_object( builder, "button_stepper_menos" ) );
   cont->lbl_ch           = GTK_WIDGET( gtk_builder_get_object( builder, "label_qtd_aulas" ) );
   cont->btn_mais       = GTK_WIDGET( gtk_builder_get_object( builder, "button_stepper_mais" ) );
   cont->tipo_registro      = GTK_WIDGET( gtk_builder_get_object( builder, "combo_tipo_registro" ) );
   cont->ent_tema               = GTK_WIDGET( gtk_builder_get_object( builder, "entry_tema" ) );
   cont->ent_descricao          = GTK_WIDGET( gtk_builder_get_object( builder, "entry_descricao" ) );
   cont->btn_salvar         = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_conteudo" ) );
   cont->btn_rm_registro   = GTK_WIDGET( gtk_builder_get_object( builder, "button_remover_registro" ) );
   gtk_button_set_relief( GTK_BUTTON( cont->btn_rm_registro ), GTK_RELIEF_NONE );
   gtk_widget_set_sensitive( cont->btn_rm_registro, FALSE );

   cont->treeview = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_conteudo" ) );
   for ( int i = 0; i < 4; i++ ) {
      if ( i < 2 ) {
         treeview_configurar_alinhamento_coluna_lc( cont->treeview, i, 0.5, 0, 0.5f );
      }
      if ( i < 2 ) {
         treeview_configurar_font_features_col_lc( cont->treeview, i, "tnum=1, lnum=1, zero=1" );
      }
   }

   // --- SUB-ABA: FREQUÊNCIA ---
   freq->cmb_data       = GTK_WIDGET( gtk_builder_get_object( builder, "combo_data" ) );
   freq->lbl_ch         = GTK_WIDGET( gtk_builder_get_object( builder, "label_ch_freq" ) );
   freq->chk_por_aluno  = GTK_WIDGET( gtk_builder_get_object( builder, "check_chamada_por_aluno" ) );
   freq->cmb_alunos     = GTK_WIDGET( gtk_builder_get_object( builder, "combo_alunos" ) );
   freq->cmb_status     = GTK_WIDGET( gtk_builder_get_object( builder, "combo_status" ) );
   freq->btn_salvar       = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_frequencia" ) );
   freq->btn_presente     = GTK_WIDGET( gtk_builder_get_object( builder, "button_presente" ) );
   freq->btn_ausente      = GTK_WIDGET( gtk_builder_get_object( builder, "button_ausente" ) );
   freq->btn_justificada  = GTK_WIDGET( gtk_builder_get_object( builder, "button_justificada" ) );

   freq->treeview = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_frequencia" ) );
   for ( int i = 0; i < 5; i++ ) {
      if ( i != 1 ) {
         treeview_configurar_alinhamento_coluna_lc( freq->treeview, i, 0.5, 0, 0.5f );
      }
      if ( i == 0 || i == 2 ) {
         treeview_configurar_font_features_col_lc( freq->treeview, i, "tnum=1, lnum=1, zero=1" );
      }
   }

   // --- SUB-ABA: AVALIAÇÕES ---
   aval->cmb_aval           = GTK_WIDGET( gtk_builder_get_object( builder, "combo_avaliacoes" ) );
   aval->btn_editar_aval      = GTK_WIDGET( gtk_builder_get_object( builder, "button_editar_avaliacao" ) );
   aval->btn_nova_aval        = GTK_WIDGET( gtk_builder_get_object( builder, "button_nova_avaliacao" ) );
   aval->chk_desativar_aval = GTK_WIDGET( gtk_builder_get_object( builder, "check_desativar_avaliacao" ) );
   aval->pop_nomear_aval  = GTK_WIDGET( gtk_builder_get_object( builder, "popover_nomear_avaliacao" ) );
   aval->lbl_pop_aval   = GTK_WIDGET( gtk_builder_get_object( builder, "label_popover_avaliacao" ) );
   aval->ent_pop_nomear      = GTK_WIDGET( gtk_builder_get_object( builder, "entry_popover_nomear" ) );
   aval->btn_pop_nomear        = GTK_WIDGET( gtk_builder_get_object( builder, "button_popover_avaliacao" ) );
   aval->btn_salvar                = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_avaliacao" ) );

   aval->treeview = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_avaliacoes" ) );
   for ( int i = 0; i < 13; i++ ) {
      if ( i != 1 ) {
         treeview_configurar_alinhamento_coluna_lc( aval->treeview, i, 0.5, 0, 0.5f );
      }
      if ( i != 1 ) {
         treeview_configurar_font_features_col_lc( aval->treeview, i, "tnum=1, lnum=1, zero=1" );
      }
   }
   destacar_cabecalho_nota_lc( aval->treeview );

   // --- SUB-ABA: RELATÓRIO ---
   rel->btn_consolidar   = GTK_WIDGET( gtk_builder_get_object( builder, "button_consolidar" ) );
   rel->chk_pendencias = GTK_WIDGET( gtk_builder_get_object( builder, "check_pendencias" ) );

   rel->treeview = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_relatorio" ) );
   for ( int i = 0; i < 11; i++ ) {
      if ( i != 1 ) {
         treeview_configurar_alinhamento_coluna_lc( rel->treeview, i, 0.5, 0, 0.5f );
      }
      if ( i != 1 ) {
         treeview_configurar_font_features_col_lc( rel->treeview, i, "tnum=1, lnum=1, zero=1" );
      }
   }
}


static void mapear_aba_latex_lc( AppContext *ctx, GtkBuilder *builder ) {
   InterfaceLatex *latex = &ctx->latex;

   latex->cmb_tema_espelho = GTK_WIDGET( gtk_builder_get_object( builder, "combo_tema_acervo" ) );
   latex->listbox_subtemas   = GTK_WIDGET( gtk_builder_get_object( builder, "listbox_subtemas_acervo" ) );

   latex->stack_pages        = GTK_WIDGET( gtk_builder_get_object( builder, "stack_pages_latex" ) );

   latex->btn_config         = GTK_WIDGET( gtk_builder_get_object( builder, "button_config_acervo" ) );
   GtkWidget *icone_config = gtk_image_new_from_icon_name( "emblem-system-symbolic", GTK_ICON_SIZE_BUTTON );
   gtk_button_set_image( GTK_BUTTON( latex->btn_config ), icone_config );
   gtk_widget_set_tooltip_text( latex->btn_config, "Configurar Acervo de Questões" );

   latex->cmb_cor_serie    = GTK_WIDGET( gtk_builder_get_object( builder, "combo_cor_serie" ) );
   latex->cmb_decoracao    = GTK_WIDGET( gtk_builder_get_object( builder, "combo_decoracao" ) );

   latex->btn_pdflatex       = GTK_WIDGET( gtk_builder_get_object( builder, "button_pdf_latex" ) );
   latex->btn_compilar       = GTK_WIDGET( gtk_builder_get_object( builder, "button_compilar" ) );
   latex->btn_diversificar   = GTK_WIDGET( gtk_builder_get_object( builder, "button_diversificar" ) );

   char id_string[64];
   for ( int i = 0; i < 2; i++ ) {
      g_snprintf( id_string, sizeof( id_string ), "radio_colunas_%d", i + 1 );
      latex->rad_qtd_colunas[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      g_snprintf( id_string, sizeof( id_string ), "radio_separadores_%d", i + 1 );
      latex->rad_separadores[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      g_snprintf( id_string, sizeof( id_string ), "radio_fonte_%d", i + 1 );
      latex->rad_fonte_latex[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      g_snprintf( id_string, sizeof( id_string ), "radio_paginas_%d", i + 1 );
      latex->rad_qtd_paginas[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      g_snprintf( id_string, sizeof( id_string ), "radio_cabecalho_%d", i + 1 );
      latex->rad_cabecalho_tipo[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );
   }
}


static void mapear_aba_provas_lc( AppContext *ctx, GtkBuilder *builder ) {
   InterfaceProva *prova = &ctx->prova;

   prova->cmb_tema           = GTK_WIDGET( gtk_builder_get_object( builder, "combo_tema" ) );
   prova->listbox_subtemas     = GTK_WIDGET( gtk_builder_get_object( builder, "listbox_subtemas" ) );
   prova->flowbox_selecionados = GTK_WIDGET( gtk_builder_get_object( builder, "flowbox_selecionados" ) );
   prova->scrolled_window      = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_subtemas_selecionados" ) );

   prova->lbl_qtd_ativos     = GTK_WIDGET( gtk_builder_get_object( builder, "label_qtd_ativos" ) );
   prova->cmb_seq_prova      = GTK_WIDGET( gtk_builder_get_object( builder, "combo_sequencia_prova" ) );
   prova->chk_validar_ciclos = GTK_WIDGET( gtk_builder_get_object( builder, "check_validar_ciclos" ) );
   prova->chk_nao_presencial = GTK_WIDGET( gtk_builder_get_object( builder, "check_nao_presencial" ) );

   prova->btn_importar         = GTK_WIDGET( gtk_builder_get_object( builder, "button_importar_dados" ) );
   prova->btn_gerar_prova      = GTK_WIDGET( gtk_builder_get_object( builder, "button_gerar_prova" ) );
   prova->btn_corrigir         = GTK_WIDGET( gtk_builder_get_object( builder, "button_corrigir_prova" ) );
   prova->btn_processar        = GTK_WIDGET( gtk_builder_get_object( builder, "button_processar_imagens" ) );
}


typedef struct {
   int         bit_idx;
   FlagsAtipicidade flag;
   const char  *glade_id;
   const char  *label;
   const char  *tooltip;
} MapaCheckAtipico;

static const MapaCheckAtipico MAPA_CHECKS_ATIPICOS[] = {
   // 1. Neurodesenvolvimento e Aprendizagem
   {
      0, ATIPICO_TEA, "check_atipico_1",
      "TEA - Transtorno do Espectro Autista",
      "TEA (CID-10: F84, F84.0 / CID-11: 6A02, 6A02.0 — Níveis 1, 2 e 3)"
   },
   {
      1, ATIPICO_TDAH, "check_atipico_2",
      "TDAH - Transtorno do Déficit de Atenção com Hiperatividade",
      "TDAH (CID-10: F90, F90.0 / CID-11: 6A05 — DDA / TDAH)"
   },
   {
      2, ATIPICO_DEF_INTELECTUAL, "check_atipico_3",
      "DI - Deficiência Intelectual",
      "DI (CID-10: F70, F70.0, F70.1, F71, F71.0, F71.8, F72–F79 / CID-11: 6A00, 6E60 — DI Leve/Moderada)"
   },
   {
      3, ATIPICO_TRANSTORNO_APRENDIZ, "check_atipico_4",
      "TA - Transtorno Específico de Aprendizagem",
      "TA (CID-10: F81, F81.0–F81.9 / CID-11: 6A03 — Dislexia, Discalculia, Disgrafia)"
   },
   {
      4, ATIPICO_TPAC, "check_atipico_5",
      "TPAC - Transtorno do Processamento Auditivo Central",
      "TPAC (CID-10: H93.25 / CID-11: AB5Y — Dificuldade de Processamento Auditivo)"
   },
   {
      5, ATIPICO_SUPERDOTACAO, "check_atipico_6",
      "AH/SD - Altas Habilidades / Superdotação",
      "AH/SD (CID-10: Z55.8 / Classificação MEC — Censo Escolar / AEE)"
   },

   // 2. Transtornos Mentais, Emocionais e Comportamentais
   {
      6, ATIPICO_TRANSTORNO_MENTAL, "check_atipico_7",
      "TM - Transtorno Mental",
      "TM (CID-10: F31, F31.6, F20 / CID-11: 6A60, 6A20 — Transtorno Bipolar, Esquizofrenia)"
   },
   {
      7, ATIPICO_TOD, "check_atipico_8",
      "TOD - Transtorno Opositivo-Desafiador",
      "TOD (CID-10: F91.3 / CID-11: 6C90 — Comportamento Opositor/Desafiador)"
   },
   {
      8, ATIPICO_ANSIEDADE_DEPRESSAO, "check_atipico_9",
      "TE - Transtornos Emocionais: Ansiedade / Depressão",
      "TE (CID-10: F32, F41, F41.0 / CID-11: 6A70, 6B00, 6B01 — Depressão, Ansiedade, Pânico)"
   },
   {
      9, ATIPICO_MUTISMO_SELETIVO, "check_atipico_10",
      "MS - Mutismo Seletivo",
      "MS (CID-10: F94.0 / CID-11: 6B06 — Mutismo Seletivo)"
   },
   {
      20, ATIPICO_TRANSTORNO_CONDUTA, "check_atipico_21",
      "TC - Transtorno de Conduta",
      "TC (CID-10: F91.0, F91.1, F91.2 / CID-11: 6C91 — Transtorno de Conduta Socializado/Regras)"
   },

   // 3. Deficiências Físicas, Sensoriais e Múltiplas
   {
      10, ATIPICO_SINDROME_DOWN, "check_atipico_11",
      "DOWN - Síndrome de Down / Trissomia 21",
      "DOWN (CID-10: Q90, Q90.0–Q90.9 / CID-11: LD40.0 — Trissomia 21)"
   },
   {
      11, ATIPICO_PARALISIA_CEREBRAL, "check_atipico_12",
      "PC - Paralisia Cerebral",
      "PC (CID-10: G80, G80.0–G80.9, G80.8 / CID-11: 8D20 — Monoplégica / Adaptação Motora)"
   },
   {
      12, ATIPICO_DEF_FISICA, "check_atipico_13",
      "DF - Deficiência Física / Motora",
      "DF (CID-10: G81–G83, M00–M99 / CID-11: 8B40–8D8Z — ex.: Cadeirante / Mobilidade Reduzida)"
   },
   {
      13, ATIPICO_DEF_AUDITIVA, "check_atipico_14",
      "DA - Deficiência Auditiva / Surdez",
      "DA (CID-10: H90, H91 / CID-11: AB50, AB51 — LIBRAS / Leitura Labial)"
   },
   {
      14, ATIPICO_DEF_VISUAL, "check_atipico_15",
      "DV - Deficiência Visual / Baixa Visão / Cegueira",
      "DV (CID-10: H54, H54.0–H54.9 / CID-11: 9D90 — Braille / Fonte Ampliada)"
   },
   {
      15, ATIPICO_SURDOCEGUEIRA, "check_atipico_16",
      "SC - Surdocegueira",
      "SC (CID-10: H90 + H54 / CID-11: AB51 + 9D90 — Categoria AEE Específica)"
   },
   {
      16, ATIPICO_DEF_MULTIPLA, "check_atipico_17",
      "DM - Deficiência Múltipla",
      "DM (CID-10: F84/F70 + G80/H90/H54 — Associação de duas ou mais deficiências primárias / Censo MEC)"
   },

   // 4. Condições Médicas e Contexto Escolar
   {
      17, ATIPICO_EPILEPSIA, "check_atipico_18",
      "EP - Epilepsia",
      "EP (CID-10: G40, G40.3 / CID-11: 8A60 — Requer protocolo de crise em sala)"
   },
   {
      18, ATIPICO_DOENCA_CRONICA, "check_atipico_19",
      "DC - Doença Crônica",
      "DC (CID-10: E10–E14, I00–I99, J45, T78.4 / CID-11: 5A10, CA23 — Diabetes, Cardiopatias, Asma, Alergias severas)"
   },
   {
      19, ATIPICO_VULNERABILIDADE, "check_atipico_20",
      "VS - Vulnerabilidade Social",
      "VS (CID-10: Z59–Z60 / CID-11: QD50–QD8Z — Acompanhamento Conselho Tutelar / CRAS)"
   },

   // 5. Metadados e Validação Pedagógica
   {
      28, ATIPICO_LAUDADO, "check_atipico_29",
      "LAUDO - Laudado",
      "LAUDO (Possui laudo médico formal anexado na pasta do estudante)"
   },
   {
      29, ATIPICO_PROVA_ADAPTADA, "check_atipico_30",
      "ADAPT - Prova Adaptada",
      "ADAPT (Requer banco de questões paralelo / avaliação adaptada)"
   },
   {
      30, ATIPICO_OBSERVACAO, "check_atipico_31",
      "OBS - Observação Pedagógica",
      "OBS (Possui anotação específica da coordenação / gestão)"
   }
};

static void mapear_aba_config_lc( AppContext *ctx, GtkBuilder *builder ) {
   InterfaceConfig *config = &ctx->config;
   InterfaceAtipicidades *atip = &config->atipicidades;

   /*** ABA NOTEBOOK CONFIG ***/
   config->stack_pages                = GTK_WIDGET( gtk_builder_get_object( builder, "stack_config" ) );
   config->btn_sub_aba_geral          = GTK_WIDGET( gtk_builder_get_object( builder, "btn_config_geral" ) );
   config->btn_sub_aba_atip = GTK_WIDGET( gtk_builder_get_object( builder, "btn_config_atipicidades" ) );
   config->btn_sub_aba_acervo        = GTK_WIDGET( gtk_builder_get_object( builder, "btn_config_acervo" ) );

   gtk_widget_set_sensitive( config->btn_sub_aba_geral, FALSE );

   // Widgets internos da sub-aba Atipicidades
   atip->entry             = GTK_WIDGET( gtk_builder_get_object( builder, "entry_atipicidades" ) );
   atip->btn_relatorio     = GTK_WIDGET( gtk_builder_get_object( builder, "button_relatorio_atipicidades" ) );
   atip->flowbox           = GTK_WIDGET( gtk_builder_get_object( builder, "flowbox_atipicidades" ) );
   atip->cmb_alunos      = GTK_WIDGET( gtk_builder_get_object( builder, "combo_alunos_atipicos" ) );
   atip->cmb_sup_tea = GTK_WIDGET( gtk_builder_get_object( builder, "combo_suporte_tea" ) );

   // Define o tooltip do combo box de suporte TEA programaticamente também
   if ( atip->cmb_sup_tea != NULL ) {
      gtk_widget_set_tooltip_text( atip->cmb_sup_tea, "Nível de Suporte TEA (1, 2 ou 3)" );
   }

   // Tooltip detalhado ao pairar o mouse
   if ( atip->entry != NULL ) {
      gtk_widget_set_tooltip_text(
         atip->entry,
         "Digite apenas o código da CID-10/CID-11 (ex: F84, F90.0, 6A02) ou a sigla/nome da condição e pressione Enter."
      );
   }

   // Inicializa todos os ponteiros com NULL (garante segurança nos bits vagos)
   for ( int i = 0; i < MAX_FLAGS_ATIPICIDADES; i++ ) {
      atip->chks[i] = NULL;
   }

   // Injeta dinamicamente labels, tooltips e IDs de forma centralizada
   for ( gsize k = 0; k < G_N_ELEMENTS( MAPA_CHECKS_ATIPICOS ); k++ ) {
      int       bit_idx = MAPA_CHECKS_ATIPICOS[k].bit_idx;
      GtkWidget *chk     = GTK_WIDGET( gtk_builder_get_object( builder, MAPA_CHECKS_ATIPICOS[k].glade_id ) );

      if ( chk != NULL && GTK_IS_CHECK_BUTTON( chk ) ) {
         atip->chks[bit_idx] = chk;

         // Atribui o rótulo e a dica de ferramenta diretamente via código
         gtk_button_set_label( GTK_BUTTON( chk ), MAPA_CHECKS_ATIPICOS[k].label );
         gtk_widget_set_tooltip_text( chk, MAPA_CHECKS_ATIPICOS[k].tooltip );

         // Embute o índice do bit diretamente no GObject para acesso O(1) na callback
         g_object_set_data( G_OBJECT( chk ), "bit_idx", GINT_TO_POINTER( bit_idx + 1 ) );
      }
   }
}


static void mapear_painel_feedback_lc( AppContext *ctx, GtkBuilder *builder ) {
   ctx->painel.eventbox_painel = GTK_WIDGET( gtk_builder_get_object( builder, "eventbox_painel" ) );
   if ( ctx->painel.eventbox_painel ) {
      gtk_widget_add_events( ctx->painel.eventbox_painel, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK );
   }

   ctx->painel.container       = GTK_WIDGET( gtk_builder_get_object( builder, "painel_feedback" ) );
   ctx->painel.revealer_painel = GTK_WIDGET( gtk_builder_get_object( builder, "revealer_painel_feedback" ) );
   ctx->painel.cabecalho       = GTK_WIDGET( gtk_builder_get_object( builder, "label_cabecalho" ) );
   ctx->painel.titulo          = GTK_WIDGET( gtk_builder_get_object( builder, "label_titulo" ) );
   ctx->painel.subtitulo       = GTK_WIDGET( gtk_builder_get_object( builder, "label_subtitulo" ) );
   ctx->painel.instrucao       = GTK_WIDGET( gtk_builder_get_object( builder, "label_instrucao" ) );

   gtk_label_set_line_wrap( GTK_LABEL( ctx->painel.titulo ),    TRUE );
   gtk_label_set_line_wrap( GTK_LABEL( ctx->painel.subtitulo ), TRUE );
   gtk_label_set_line_wrap( GTK_LABEL( ctx->painel.instrucao ), TRUE );

   ctx->painel.eventbox_orelhinha = GTK_WIDGET( gtk_builder_get_object( builder, "eventbox_orelhinha" ) );
   if ( ctx->painel.eventbox_orelhinha ) {
      gtk_widget_add_events( ctx->painel.eventbox_orelhinha, GDK_BUTTON_PRESS_MASK );
   }
}


static void aplicar_espacamentos_interface_lc( AppContext *ctx, GtkBuilder *builder ) {
   // Acesso direto aos domínios perfeitamente estruturados
   const InterfaceDiario *diario = &ctx->diario;
   InterfaceLatex  *latex  = &ctx->latex;
   InterfaceProva  *prova  = &ctx->prova;
   InterfaceConfig *config = &ctx->config;

   // Espaçamento nos Labels Soltos
   const gchar *labels[8] = {
      "label_conteudo", "label_frequencia", "label_avaliacoes", "label_relatorio",
      "label_tema_latex", "label_config_diagramacao", "label_tema_provas", "label_montagem_da_prova"
   };

   for ( int i = 0; i < 8; i++ ) {
      GtkWidget *label = GTK_WIDGET( gtk_builder_get_object( builder, labels[i] ) );
      if ( label ) {
         ui_label_aplicar_espacamento_manual( label, 2 );
      }

      if ( g_strcmp0( labels[i], "label_config_diagramacao" ) == 0 ) {
         latex->lbl_diag_acervo = label;
      }
   }

   // Espaçamento nos Labels Internos dos Botões (Usando a nova topologia)
   GtkWidget *widgets_botoes[23] = {
      diario->relatorio.btn_consolidar,
      diario->avaliacao.btn_nova_aval,
      diario->avaliacao.btn_salvar,
      diario->conteudo.btn_salvar,
      diario->frequencia.btn_salvar,
      diario->avaliacao.btn_pop_nomear,
      latex->btn_pdflatex,
      latex->btn_compilar,
      latex->btn_diversificar,
      diario->btn_frequencia,
      diario->btn_conteudo,
      diario->btn_avaliacao,
      diario->btn_relatorio,
      prova->btn_gerar_prova,
      prova->btn_corrigir,
      prova->btn_processar,
      prova->btn_importar,
      config->btn_sub_aba_acervo,
      config->acervo.btn_salvar,
      config->btn_sub_aba_atip,
      config->btn_sub_aba_geral,
      config->geral.btn_salvar,
      config->atipicidades.btn_relatorio
   };

   for ( int i = 0; i < 23; i++ ) {
      if ( widgets_botoes[i] ) {
         ui_button_label_aplicar_espacamento_manual_lc( widgets_botoes[i], 1 );
      }
   }
}

// ======================================================================================
// 3. ORQUESTRADOR MESTRE
// ======================================================================================

/**
 * @brief Orquestrador mestre da interface gráfica.
 * Aloca a janela principal, carrega o arquivo Glade e despacha a construção modular.
 *
 * @param app Ponteiro para a GtkApplication ativa.
 * @param ctx Ponteiro para o contexto global da aplicação (AppContext).
 */
void construir_interface( GtkApplication *app, AppContext *ctx ) {
   GtkBuilder *builder = gtk_builder_new();
   GError *error = NULL;

   g_autofree gchar *path_ui = g_build_filename( ctx->caminho.recursos_prefix, "interface.glade", NULL );

   if ( !gtk_builder_add_from_resource( builder, path_ui, &error ) ) {
      g_printerr( "❌ Erro ao carregar interface do GResource: %s\n", error->message );
      g_clear_error( &error );
      return;
   }

   // Despacho Modular Restrito por Domínio Visual
   mapear_janela_mestra_lc( ctx, builder, app );
   mapear_cabecalho_lc( ctx, builder );
   mapear_aba_diarios_lc( ctx, builder );
   mapear_aba_latex_lc( ctx, builder );
   mapear_aba_provas_lc( ctx, builder );
   mapear_aba_config_lc( ctx, builder );
   mapear_painel_feedback_lc( ctx, builder );

   // Aplica os estilos finais visuais via Pango/GtkLabel
   aplicar_espacamentos_interface_lc( ctx, builder );

   // Libera o objeto builder da memória de forma segura
   g_object_unref( builder );
}
