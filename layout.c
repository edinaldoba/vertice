/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include "layout.h"


// ======================================================================================
// 1. FUNÇÕES AUXILIARES DE ESTILIZAÇÃO E ALINHAMENTO
// ======================================================================================

/**
 * @brief Extrai o GtkLabel interno de um botão configurado pelo Glade e aplica o espaçamento.
 */
static void _ui_button_label_aplicar_espacamento_manual( GtkWidget *button_widget, gint quantidade_espacos ) {
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
static void _treeview_configurar_alinhamento_coluna( GtkWidget *widget,
      int coluna,
      float align_celula,
      guint pad_celula_x,
      float align_cabecalho ) {
   g_return_if_fail( GTK_IS_TREE_VIEW( widget ) );

   GtkTreeViewColumn *col_ch = gtk_tree_view_get_column( GTK_TREE_VIEW( widget ), coluna );
   if ( !col_ch ) return;

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
static void _treeview_configurar_font_features_col( GtkWidget *widget, int coluna, const char *font_features ) {
   g_return_if_fail( GTK_IS_TREE_VIEW( widget ) );
   g_return_if_fail( font_features != NULL );

   GtkTreeViewColumn *col_ch = gtk_tree_view_get_column( GTK_TREE_VIEW( widget ), coluna );
   if ( !col_ch ) return;

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
static void _destacar_cabecalho_nota( GtkWidget *treeview_avaliacoes ) {
   g_return_if_fail( treeview_avaliacoes && GTK_IS_TREE_VIEW( treeview_avaliacoes ) );

   // 1. Obtém a coluna 12 (Nota do Período) da TreeView de Avaliações
   GtkTreeViewColumn *col_nota = gtk_tree_view_get_column( GTK_TREE_VIEW( treeview_avaliacoes ), 12 );
   if ( !col_nota ) return;

   // 2. Captura o widget real do botão do cabeçalho
   GtkWidget *button = gtk_tree_view_column_get_button( col_nota );
   if ( !button ) return;

   // 3. Adiciona a classe CSS nativamente
   GtkStyleContext *context = gtk_widget_get_style_context( button );
   gtk_style_context_add_class( context, "cabecalho-destaque-nota" );
}






// ======================================================================================
// 2. FUNÇÕES DE MAPEAMENTO DE WIDGETS POR DOMÍNIO
// ======================================================================================

static void _mapear_janela_mestra( AppContext *ctx, GtkBuilder *builder, GtkApplication *app ) {
   ctx->window   = GTK_WIDGET( gtk_builder_get_object( builder, "janela_principal" ) );
   ctx->notebook = GTK_WIDGET( gtk_builder_get_object( builder, "notebook_principal" ) );

   gtk_window_set_application( GTK_WINDOW( ctx->window ), app );

   if ( detectar_ubuntu() ) {
      gtk_window_set_position( GTK_WINDOW( ctx->window ), GTK_WIN_POS_CENTER );
   } else {
      gtk_window_move( GTK_WINDOW( ctx->window ), 1019, 141 );
   }

   // Ativação da HeaderBar customizada para controle CSS
   GtkWidget *header = gtk_header_bar_new();
   gtk_header_bar_set_show_close_button( GTK_HEADER_BAR( header ), TRUE );
   gtk_header_bar_set_title( GTK_HEADER_BAR( header ), "Vértice: diários e provas" );
   gtk_window_set_titlebar( GTK_WINDOW( ctx->window ), header );
}


static void _mapear_cabecalho( AppContext *ctx, GtkBuilder *builder ) {
   InterfaceCabecalho *cab = &ctx->cabecalho;

   cab->combo_ano            = GTK_WIDGET( gtk_builder_get_object( builder, "combo_ano" ) );
   cab->combo_escola         = GTK_WIDGET( gtk_builder_get_object( builder, "combo_escola" ) );
   cab->combo_turma          = GTK_WIDGET( gtk_builder_get_object( builder, "combo_turma" ) );
   cab->combo_disciplina     = GTK_WIDGET( gtk_builder_get_object( builder, "combo_disciplina" ) );
   cab->combo_periodo        = GTK_WIDGET( gtk_builder_get_object( builder, "combo_periodos" ) );
   cab->combo_estilo         = GTK_WIDGET( gtk_builder_get_object( builder, "combo_estilo" ) );

   cab->btn_atualizar_alunos = GTK_WIDGET( gtk_builder_get_object( builder, "button_atualizar_alunos" ) );
}


static void _mapear_aba_diarios( AppContext *ctx, GtkBuilder *builder ) {
   InterfaceDiario     *diario     = &ctx->diario;
   InterfaceConteudo   *conteudo   = &ctx->diario.conteudo;
   InterfaceFrequencia *frequencia = &ctx->diario.frequencia;
   InterfaceAvaliacao  *avaliacao  = &ctx->diario.avaliacao;
   InterfaceRelatorio  *relatorio  = &ctx->diario.relatorio;

   // --- RAIZ DA ABA DIÁRIOS ---
   diario->stack_pages            = GTK_WIDGET( gtk_builder_get_object( builder, "stack_pages_diarios" ) );
   diario->check_modo_edicao      = GTK_WIDGET( gtk_builder_get_object( builder, "check_modo_edicao" ) );
   diario->check_expor_dados      = GTK_WIDGET( gtk_builder_get_object( builder, "check_expor_dados" ) );
   diario->responsavel.gestor     = GTK_WIDGET( gtk_builder_get_object( builder, "box_gestor" ) );
   diario->responsavel.professor  = GTK_WIDGET( gtk_builder_get_object( builder, "box_professor" ) );

   diario->btn_conteudo           = GTK_WIDGET( gtk_builder_get_object( builder, "button_conteudos" ) );
   diario->btn_frequencia         = GTK_WIDGET( gtk_builder_get_object( builder, "button_frequencia" ) );
   diario->btn_avaliacao          = GTK_WIDGET( gtk_builder_get_object( builder, "button_avaliacoes" ) );
   diario->btn_relatorio          = GTK_WIDGET( gtk_builder_get_object( builder, "button_relatorio_final" ) );

   // --- SUB-ABA: CONTEÚDOS ---
   conteudo->entry_data         = GTK_WIDGET( gtk_builder_get_object( builder, "entry_data" ) );
   conteudo->popover_calendario = GTK_WIDGET( gtk_builder_get_object( builder, "popover_calendario" ) );
   conteudo->calendario_data    = GTK_WIDGET( gtk_builder_get_object( builder, "calendar_data" ) );
   conteudo->stepper_menos      = GTK_WIDGET( gtk_builder_get_object( builder, "button_stepper_menos" ) );
   conteudo->label_ch           = GTK_WIDGET( gtk_builder_get_object( builder, "label_qtd_aulas" ) );
   conteudo->stepper_mais       = GTK_WIDGET( gtk_builder_get_object( builder, "button_stepper_mais" ) );
   conteudo->tipo_registro      = GTK_WIDGET( gtk_builder_get_object( builder, "combo_tipo_registro" ) );
   conteudo->entry_tema               = GTK_WIDGET( gtk_builder_get_object( builder, "entry_tema" ) );
   conteudo->entry_descricao          = GTK_WIDGET( gtk_builder_get_object( builder, "entry_descricao" ) );
   conteudo->btn_salvar         = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_conteudo" ) );
   conteudo->remover_registro   = GTK_WIDGET( gtk_builder_get_object( builder, "button_remover_registro" ) );

   conteudo->treeview = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_conteudo" ) );
   for ( int i = 0; i < 4; i++ ) {
      if ( i < 2 ) _treeview_configurar_alinhamento_coluna( conteudo->treeview, i, 0.5, 0, 0.5 );
      if ( i < 2 ) _treeview_configurar_font_features_col( conteudo->treeview, i, "tnum=1, lnum=1, zero=1" );
   }

   // --- SUB-ABA: FREQUÊNCIA ---
   frequencia->combo_data       = GTK_WIDGET( gtk_builder_get_object( builder, "combo_data" ) );
   frequencia->label_ch         = GTK_WIDGET( gtk_builder_get_object( builder, "label_ch_freq" ) );
   frequencia->check_por_aluno  = GTK_WIDGET( gtk_builder_get_object( builder, "check_chamada_por_aluno" ) );
   frequencia->combo_alunos     = GTK_WIDGET( gtk_builder_get_object( builder, "combo_alunos" ) );
   frequencia->combo_status     = GTK_WIDGET( gtk_builder_get_object( builder, "combo_status" ) );
   frequencia->btn_salvar       = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_frequencia" ) );
   frequencia->btn_presente     = GTK_WIDGET( gtk_builder_get_object( builder, "button_presente" ) );
   frequencia->btn_ausente      = GTK_WIDGET( gtk_builder_get_object( builder, "button_ausente" ) );
   frequencia->btn_justificada  = GTK_WIDGET( gtk_builder_get_object( builder, "button_justificada" ) );

   frequencia->treeview = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_frequencia" ) );
   for ( int i = 0; i < 5; i++ ) {
      if ( i != 1 ) _treeview_configurar_alinhamento_coluna( frequencia->treeview, i, 0.5, 0, 0.5 );
      if ( i == 0 || i == 2 ) _treeview_configurar_font_features_col( frequencia->treeview, i, "tnum=1, lnum=1, zero=1" );
   }

   // --- SUB-ABA: AVALIAÇÕES ---
   avaliacao->combo_avaliacao           = GTK_WIDGET( gtk_builder_get_object( builder, "combo_avaliacoes" ) );
   avaliacao->btn_editar_avaliacao      = GTK_WIDGET( gtk_builder_get_object( builder, "button_editar_avaliacao" ) );
   avaliacao->btn_nova_avaliacao        = GTK_WIDGET( gtk_builder_get_object( builder, "button_nova_avaliacao" ) );
   avaliacao->check_desativar_avaliacao = GTK_WIDGET( gtk_builder_get_object( builder, "check_desativar_avaliacao" ) );
   avaliacao->popover_nomear_avaliacao  = GTK_WIDGET( gtk_builder_get_object( builder, "popover_nomear_avaliacao" ) );
   avaliacao->label_popover_avaliacao   = GTK_WIDGET( gtk_builder_get_object( builder, "label_popover_avaliacao" ) );
   avaliacao->entry_popover_nomear      = GTK_WIDGET( gtk_builder_get_object( builder, "entry_popover_nomear" ) );
   avaliacao->btn_popover_nomear        = GTK_WIDGET( gtk_builder_get_object( builder, "button_popover_avaliacao" ) );
   avaliacao->btn_salvar                = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_avaliacao" ) );

   avaliacao->treeview = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_avaliacoes" ) );
   for ( int i = 0; i < 13; i++ ) {
      if ( i != 1 ) _treeview_configurar_alinhamento_coluna( avaliacao->treeview, i, 0.5, 0, 0.5 );
      if ( i != 1 ) _treeview_configurar_font_features_col( avaliacao->treeview, i, "tnum=1, lnum=1, zero=1" );
   }
   _destacar_cabecalho_nota( avaliacao->treeview );

   // --- SUB-ABA: RELATÓRIO ---
   relatorio->btn_consolidar   = GTK_WIDGET( gtk_builder_get_object( builder, "button_consolidar" ) );
   relatorio->check_pendencias = GTK_WIDGET( gtk_builder_get_object( builder, "check_pendencias" ) );

   relatorio->treeview = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_relatorio" ) );
   for ( int i = 0; i < 11; i++ ) {
      if ( i != 1 ) _treeview_configurar_alinhamento_coluna( relatorio->treeview, i, 0.5, 0, 0.5 );
      if ( i != 1 ) _treeview_configurar_font_features_col( relatorio->treeview, i, "tnum=1, lnum=1, zero=1" );
   }
}


static void _mapear_aba_latex( AppContext *ctx, GtkBuilder *builder ) {
   InterfaceLatex *latex = &ctx->latex;

   latex->combo_tema_espelho = GTK_WIDGET( gtk_builder_get_object( builder, "combo_tema_acervo" ) );
   latex->listbox_subtemas   = GTK_WIDGET( gtk_builder_get_object( builder, "listbox_subtemas_acervo" ) );

   latex->stack_pages        = GTK_WIDGET( gtk_builder_get_object( builder, "stack_pages_latex" ) );

   latex->btn_config         = GTK_WIDGET( gtk_builder_get_object( builder, "button_config_acervo" ) );
   GtkWidget *icone_config = gtk_image_new_from_icon_name( "emblem-system-symbolic", GTK_ICON_SIZE_BUTTON );
   gtk_button_set_image( GTK_BUTTON( latex->btn_config ), icone_config );
   gtk_widget_set_tooltip_text( latex->btn_config, "Configurar Acervo de Questões" );

   latex->combo_cor_serie    = GTK_WIDGET( gtk_builder_get_object( builder, "combo_cor_serie" ) );
   latex->combo_decoracao    = GTK_WIDGET( gtk_builder_get_object( builder, "combo_decoracao" ) );

   latex->btn_pdflatex       = GTK_WIDGET( gtk_builder_get_object( builder, "button_pdf_latex" ) );
   latex->btn_compilar       = GTK_WIDGET( gtk_builder_get_object( builder, "button_compilar" ) );
   latex->btn_diversificar   = GTK_WIDGET( gtk_builder_get_object( builder, "button_diversificar" ) );

   char id_string[64];
   for ( int i = 0; i < 2; i++ ) {
      snprintf( id_string, sizeof( id_string ), "radio_colunas_%d", i + 1 );
      latex->radio_qtd_colunas[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      snprintf( id_string, sizeof( id_string ), "radio_separadores_%d", i + 1 );
      latex->radio_separadores[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      snprintf( id_string, sizeof( id_string ), "radio_fonte_%d", i + 1 );
      latex->radio_fonte_latex[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      snprintf( id_string, sizeof( id_string ), "radio_paginas_%d", i + 1 );
      latex->radio_qtd_paginas[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      snprintf( id_string, sizeof( id_string ), "radio_cabecalho_%d", i + 1 );
      latex->radio_cabecalho_tipo[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );
   }
}


static void _mapear_aba_provas( AppContext *ctx, GtkBuilder *builder ) {
   InterfaceProva *prova = &ctx->prova;

   prova->combo_tema           = GTK_WIDGET( gtk_builder_get_object( builder, "combo_tema" ) );
   prova->listbox_subtemas     = GTK_WIDGET( gtk_builder_get_object( builder, "listbox_subtemas" ) );
   prova->flowbox_selecionados = GTK_WIDGET( gtk_builder_get_object( builder, "flowbox_selecionados" ) );
   prova->scrolled_window      = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_subtemas_selecionados" ) );

   prova->label_qtd_ativos     = GTK_WIDGET( gtk_builder_get_object( builder, "label_qtd_ativos" ) );
   prova->combo_seq_prova      = GTK_WIDGET( gtk_builder_get_object( builder, "combo_sequencia_prova" ) );
   prova->check_validar_ciclos = GTK_WIDGET( gtk_builder_get_object( builder, "check_validar_ciclos" ) );
   prova->check_nao_presencial = GTK_WIDGET( gtk_builder_get_object( builder, "check_nao_presencial" ) );

   prova->btn_importar         = GTK_WIDGET( gtk_builder_get_object( builder, "button_importar_dados" ) );
   prova->btn_gerar_prova      = GTK_WIDGET( gtk_builder_get_object( builder, "button_gerar_prova" ) );
   prova->btn_corrigir         = GTK_WIDGET( gtk_builder_get_object( builder, "button_corrigir_prova" ) );
   prova->btn_processar        = GTK_WIDGET( gtk_builder_get_object( builder, "button_processar_imagens" ) );
}


static void _mapear_painel_feedback( AppContext *ctx, GtkBuilder *builder ) {
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


static void _aplicar_espacamentos_interface( AppContext *ctx, GtkBuilder *builder ) {
   // Acesso direto aos domínios perfeitamente estruturados
   const InterfaceDiario *diario = &ctx->diario;
   InterfaceLatex  *latex  = &ctx->latex;
   InterfaceProva  *prova  = &ctx->prova;

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
         latex->label_diag_acervo = label;
      }
   }

   // Espaçamento nos Labels Internos dos Botões (Usando a nova topologia)
   GtkWidget *widgets_botoes[17] = {
      diario->relatorio.btn_consolidar,
      diario->avaliacao.btn_nova_avaliacao,
      diario->avaliacao.btn_salvar,
      diario->conteudo.btn_salvar,
      diario->frequencia.btn_salvar,
      diario->avaliacao.btn_popover_nomear,
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
      prova->btn_importar
   };

   for ( int i = 0; i < 17; i++ ) {
      if ( widgets_botoes[i] ) {
         _ui_button_label_aplicar_espacamento_manual( widgets_botoes[i], 1 );
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
   _mapear_janela_mestra( ctx, builder, app );
   _mapear_cabecalho( ctx, builder );
   _mapear_aba_diarios( ctx, builder );
   _mapear_aba_latex( ctx, builder );
   _mapear_aba_provas( ctx, builder );
   _mapear_painel_feedback( ctx, builder );

   // Aplica os estilos finais visuais via Pango/GtkLabel
   _aplicar_espacamentos_interface( ctx, builder );

   // Libera o objeto builder da memória de forma segura
   g_object_unref( builder );
}
