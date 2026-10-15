/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include "layout.h"
#include "comum.h"
#include "basicas.h"
#include "interface.h"
#include <stdio.h>
#include <pango/pango.h>


// ======================================================================================
// 1. FUNÇÕES AUXILIARES DE ESTILIZAÇÃO E ALINHAMENTO
// ======================================================================================

/**
 * @brief Aplica espaçamento uniforme intercalando Hair Spaces (\u200A)
 *        diretamente no texto de um GtkLabel.
 */
static void _ui_label_aplicar_espacamento_manual( GtkWidget *label_widget, gint quantidade_espacos ) {
   g_return_if_fail( GTK_IS_LABEL( label_widget ) );

   const gchar *texto_original = gtk_label_get_text( GTK_LABEL( label_widget ) );
   if ( !texto_original || *texto_original == '\0' ) return;

   GString *resultado = g_string_new( "" );
   const gchar *p = texto_original;

   while ( *p != '\0' ) {
      gunichar c = g_utf8_get_char( p );
      const gchar *proximo = g_utf8_next_char( p );

      if ( c == 0x200A ) {
         p = proximo;
         continue;
      }

      g_string_append_unichar( resultado, c );
      gunichar proximo_c = g_utf8_get_char( proximo );

      if ( *proximo != '\0' && proximo_c != 0x200A ) {
         for ( gint i = 0; i < quantidade_espacos; i++ ) {
            g_string_append( resultado, "\u200A" );
         }
      }
      p = proximo;
   }

   gtk_label_set_text( GTK_LABEL( label_widget ), resultado->str );
   g_string_free( resultado, TRUE );
}

/**
 * @brief Extrai o GtkLabel interno de um botão configurado pelo Glade e aplica o espaçamento.
 */
static void _ui_button_label_aplicar_espacamento_manual( GtkWidget *button_widget, gint quantidade_espacos ) {
   g_return_if_fail( GTK_IS_BUTTON( button_widget ) );
   GtkWidget *label_interno = gtk_bin_get_child( GTK_BIN( button_widget ) );
   if ( GTK_IS_LABEL( label_interno ) ) {
      _ui_label_aplicar_espacamento_manual( label_interno, quantidade_espacos );
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
static void treeview_configurar_alinhamento_coluna( GtkWidget *widget,
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
static void treeview_configurar_font_features_col( GtkWidget *widget, int coluna, const char *font_features ) {
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


// ======================================================================================
// 2. FUNÇÕES DE MAPEAMENTO DE WIDGETS POR DOMÍNIO
// ======================================================================================

static void _mapear_janela_e_cabecalho( AppContext *ctx, GtkBuilder *builder, GtkApplication *app ) {
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

static void _mapear_filtros_e_configuracoes( AppContext *ctx, GtkBuilder *builder ) {
   ctx->entry.ano            = GTK_WIDGET( gtk_builder_get_object( builder, "combo_ano" ) );
   ctx->entry.escola         = GTK_WIDGET( gtk_builder_get_object( builder, "combo_escola" ) );
   ctx->entry.turma          = GTK_WIDGET( gtk_builder_get_object( builder, "combo_turma" ) );
   ctx->entry.disciplina     = GTK_WIDGET( gtk_builder_get_object( builder, "combo_disciplina" ) );
   ctx->entry.periodo        = GTK_WIDGET( gtk_builder_get_object( builder, "combo_momento" ) );
   ctx->entry.estilo         = GTK_WIDGET( gtk_builder_get_object( builder, "combo_estilo" ) );
   ctx->entry.cor_destaque   = GTK_WIDGET( gtk_builder_get_object( builder, "combo_cor_serie" ) );
   ctx->entry.decoracao_estilo = GTK_WIDGET( gtk_builder_get_object( builder, "combo_decoracao" ) );

   ctx->cabecalho.gestor     = GTK_WIDGET( gtk_builder_get_object( builder, "box_gestor" ) );
   ctx->cabecalho.professor  = GTK_WIDGET( gtk_builder_get_object( builder, "box_professor" ) );

   ctx->check.validar_ciclos = GTK_WIDGET( gtk_builder_get_object( builder, "check_cruz" ) );
   ctx->check.nao_presencial = GTK_WIDGET( gtk_builder_get_object( builder, "check_naopresencial" ) );
   ctx->check.expor_dados    = GTK_WIDGET( gtk_builder_get_object( builder, "check_expor" ) );
}

static void _mapear_aba_diario( AppContext *ctx, GtkBuilder *builder ) {
   ctx->stack_pages = GTK_WIDGET( gtk_builder_get_object( builder, "stack_pages" ) );
   ctx->ui_diario.check_modo_edicao = GTK_WIDGET( gtk_builder_get_object( builder, "check_modo_edicao" ) );

   // --- SUB-ABA: CONTEÚDOS ---
   ctx->ui_diario.entry_data         = GTK_WIDGET( gtk_builder_get_object( builder, "entry_data" ) );
   ctx->ui_diario.popover_calendario = GTK_WIDGET( gtk_builder_get_object( builder, "popover_calendario" ) );
   ctx->ui_diario.calendario_data    = GTK_WIDGET( gtk_builder_get_object( builder, "calendar_data" ) );

   ctx->ui_diario.stepper_menos      = GTK_WIDGET( gtk_builder_get_object( builder, "button_stepper_menos" ) );
   ctx->ui_diario.qtd_aulas          = GTK_WIDGET( gtk_builder_get_object( builder, "label_qtd_aulas" ) );
   ctx->ui_diario.stepper_mais       = GTK_WIDGET( gtk_builder_get_object( builder, "button_stepper_mais" ) );
   ctx->ui_diario.tipo_registro      = GTK_WIDGET( gtk_builder_get_object( builder, "combo_tipo_registro" ) );
   ctx->ui_diario.tema               = GTK_WIDGET( gtk_builder_get_object( builder, "entry_tema" ) );
   ctx->ui_diario.descricao          = GTK_WIDGET( gtk_builder_get_object( builder, "entry_descricao" ) );
   ctx->ui_diario.btn_salvar_conteudo = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_conteudo" ) );
   ctx->ui_diario.remover_registro   = GTK_WIDGET( gtk_builder_get_object( builder, "button_remover_registro" ) );

   ctx->ui_diario.scrolled_window_conteudo = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_window_conteudo" ) );
   ctx->ui_diario.treeview_conteudo  = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_conteudo" ) );
   for ( int i = 0; i < 4; i++ ) {
      if( i < 2 ) treeview_configurar_alinhamento_coluna( ctx->ui_diario.treeview_conteudo, i, 0.5, 0, 0.5 );
      if( i < 2 ) treeview_configurar_font_features_col( ctx->ui_diario.treeview_conteudo, i, "tnum=1, lnum=1, zero=1" );
   }

   // --- SUB-ABA: FREQUÊNCIA ---
   ctx->ui_diario.scrolled_window_frequencia = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_window_frequencia" ) );
   ctx->ui_diario.treeview_frequencia        = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_frequencia" ) );
   for ( int i = 0; i < 4; i++ ) {
      if ( i != 1 ) treeview_configurar_alinhamento_coluna( ctx->ui_diario.treeview_frequencia, i, 0.5, 0, 0.5 );
      if ( i == 0 ) treeview_configurar_font_features_col( ctx->ui_diario.treeview_frequencia, i, "tnum=1, lnum=1, zero=1" );
   }

   ctx->ui_diario.combo_data            = GTK_WIDGET( gtk_builder_get_object( builder, "combo_data" ) );
   ctx->ui_diario.label_ch              = GTK_WIDGET( gtk_builder_get_object( builder, "label_ch_freq" ) );
   ctx->ui_diario.check_por_aluno       = GTK_WIDGET( gtk_builder_get_object( builder, "check_chamada_por_aluno" ) );
   ctx->ui_diario.combo_alunos          = GTK_WIDGET( gtk_builder_get_object( builder, "combo_alunos" ) );
   ctx->ui_diario.btn_salvar_frequencia = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_frequencia" ) );
   ctx->ui_diario.btn_presente          = GTK_WIDGET( gtk_builder_get_object( builder, "button_presente" ) );
   ctx->ui_diario.btn_ausente           = GTK_WIDGET( gtk_builder_get_object( builder, "button_ausente" ) );
   ctx->ui_diario.btn_justificada       = GTK_WIDGET( gtk_builder_get_object( builder, "button_justificada" ) );
   ctx->ui_diario.combo_status          = GTK_WIDGET( gtk_builder_get_object( builder, "combo_status" ) );

   // --- SUB-ABA: AVALIAÇÕES ---
   ctx->ui_diario.scrolled_window_avaliacoes = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_window_avaliacoes" ) );
   ctx->ui_diario.treeview_avaliacoes        = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_avaliacoes" ) );
   for ( int i = 0; i < 12; i++ ) {
      if ( i != 1 ) treeview_configurar_alinhamento_coluna( ctx->ui_diario.treeview_avaliacoes, i, 0.5, 0, 0.5 );
      if ( i != 1 ) treeview_configurar_font_features_col( ctx->ui_diario.treeview_avaliacoes, i, "tnum=1, lnum=1, zero=1" );
   }

   ctx->ui_diario.combo_avaliacoes          = GTK_WIDGET( gtk_builder_get_object( builder, "combo_avaliacoes" ) );
   ctx->ui_diario.btn_editar_avaliacao      = GTK_WIDGET( gtk_builder_get_object( builder, "button_editar_avaliacao" ) );
   ctx->ui_diario.btn_nova_avaliacao        = GTK_WIDGET( gtk_builder_get_object( builder, "button_nova_avaliacao" ) );
   ctx->ui_diario.check_desativar_avaliacao = GTK_WIDGET( gtk_builder_get_object( builder, "check_desativar_avaliacao" ) );
   ctx->ui_diario.popover_nomear_avaliacao  = GTK_WIDGET( gtk_builder_get_object( builder, "popover_nomear_avaliacao" ) );
   ctx->ui_diario.label_popover_avaliacao   = GTK_WIDGET( gtk_builder_get_object( builder, "label_popover_avaliacao" ) );
   ctx->ui_diario.entry_popover_nomear      = GTK_WIDGET( gtk_builder_get_object( builder, "entry_popover_nomear" ) );
   ctx->ui_diario.btn_popover_nomear        = GTK_WIDGET( gtk_builder_get_object( builder, "button_popover_avaliacao" ) );
   ctx->ui_diario.btn_salvar_avaliacoes     = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_avaliacao" ) );

   // --- SUB-ABA: RELATÓRIO ---
   ctx->ui_diario.scrolled_window_relatorio = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_window_relatorio" ) );
   ctx->ui_diario.btn_consolidar            = GTK_WIDGET( gtk_builder_get_object( builder, "button_consolidar" ) );
   ctx->ui_diario.treeview_relatorio        = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_relatorio" ) );
   for ( int i = 0; i < 11; i++ ) {
      if ( i != 1 ) treeview_configurar_alinhamento_coluna( ctx->ui_diario.treeview_relatorio, i, 0.5, 0, 0.5 );
      if ( i != 1 ) treeview_configurar_font_features_col( ctx->ui_diario.treeview_relatorio, i, "tnum=1, lnum=1, zero=1" );
      // if ( i != 1 ) treeview_configurar_font_features_col( ctx->ui_diario.treeview_relatorio, i, "tnum=1, lnum=1, zero=1, cv05=1" );
   }
}

static void _mapear_aba_provas_e_acervo( AppContext *ctx, GtkBuilder *builder ) {
   ctx->entry.tema               = GTK_WIDGET( gtk_builder_get_object( builder, "combo_tema" ) );
   ctx->entry.tema_espelho       = GTK_WIDGET( gtk_builder_get_object( builder, "combo_tema_acervo" ) );

   ctx->latex.listbox_subtemas   = GTK_WIDGET( gtk_builder_get_object( builder, "listbox_subtemas_acervo" ) );
   ctx->provas.listbox_subtemas  = GTK_WIDGET( gtk_builder_get_object( builder, "listbox_subtemas" ) );
   ctx->provas.flowbox_selecionados = GTK_WIDGET( gtk_builder_get_object( builder, "flowbox_selecionados" ) );
   ctx->provas.scrolled_window   = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_subtemas_selecionados" ) );

   char id_string[64];
   for ( int i = 0; i < 2; i++ ) {
      snprintf( id_string, sizeof( id_string ), "radio_colunas_%d", i + 1 );
      ctx->radio.qtd_colunas[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      snprintf( id_string, sizeof( id_string ), "radio_separadores_%d", i + 1 );
      ctx->radio.separadores[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      snprintf( id_string, sizeof( id_string ), "radio_fonte_%d", i + 1 );
      ctx->radio.fonte_latex[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      snprintf( id_string, sizeof( id_string ), "radio_paginas_%d", i + 1 );
      ctx->radio.qtd_paginas[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      snprintf( id_string, sizeof( id_string ), "radio_cabecalho_%d", i + 1 );
      ctx->radio.cabecalho_tipo[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );
   }

   for ( int i = 0; i < 3; i++ ) {
      snprintf( id_string, sizeof( id_string ), "radio_prova_%d", i + 1 );
      ctx->radio.avaliacao[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );
   }
}

static void _mapear_motores_de_acao( AppContext *ctx, GtkBuilder *builder ) {
   ctx->button.abrir_pdf_acervo      = GTK_WIDGET( gtk_builder_get_object( builder, "button_pdf_latex" ) );
   ctx->button.compilar_latex_acervo = GTK_WIDGET( gtk_builder_get_object( builder, "button_compilar" ) );
   ctx->button.executar_gcc_acervo   = GTK_WIDGET( gtk_builder_get_object( builder, "button_gcc" ) );

   ctx->button.importar_dados        = GTK_WIDGET( gtk_builder_get_object( builder, "button_importar_dados" ) );
   ctx->button.frequencia            = GTK_WIDGET( gtk_builder_get_object( builder, "button_frequencia" ) );
   ctx->button.conteudos             = GTK_WIDGET( gtk_builder_get_object( builder, "button_conteudos" ) );
   ctx->button.avaliacoes            = GTK_WIDGET( gtk_builder_get_object( builder, "button_avaliacoes" ) );
   ctx->button.relatorio_final       = GTK_WIDGET( gtk_builder_get_object( builder, "button_relatorio_final" ) );
   ctx->button.atualizar_alunos      = GTK_WIDGET( gtk_builder_get_object( builder, "button_atualizar_alunos" ) );

   ctx->button.gerar_prova           = GTK_WIDGET( gtk_builder_get_object( builder, "button_gerar_prova" ) );
   ctx->button.corrigir_prova        = GTK_WIDGET( gtk_builder_get_object( builder, "button_corrigir_prova" ) );
   ctx->button.processar_imagens     = GTK_WIDGET( gtk_builder_get_object( builder, "button_processar_imagens" ) );
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
   // Espaçamento nos Labels Soltos
   const gchar *labels[7] = {
      "label_conteudo", "label_frequencia", "label_avaliacoes", "label_relatorio",
      "label_tema_acervo", "label_tema_prova", "label_montagem_da_prova"
   };

   for ( int i = 0; i < 7; i++ ) {
      GtkWidget *label = GTK_WIDGET( gtk_builder_get_object( builder, labels[i] ) );
      if ( label ) {
         _ui_label_aplicar_espacamento_manual( label, 2 );
      }
   }

   // Espaçamento nos Labels Internos dos Botões
   InterfaceRegistroDiario *d = &ctx->ui_diario;
   InterfaceButton *b = &ctx->button;
   GtkWidget *widgets_botoes[17] = {
      d->btn_consolidar, d->btn_nova_avaliacao, d->btn_salvar_avaliacoes,
      d->btn_salvar_conteudo, d->btn_salvar_frequencia, d->btn_popover_nomear,
      b->abrir_pdf_acervo, b->compilar_latex_acervo, b->executar_gcc_acervo, b->importar_dados, b->frequencia,
      b->conteudos, b->avaliacoes, b->relatorio_final, b->gerar_prova, b->corrigir_prova,
      b->processar_imagens
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

   // Em produção, retorne o uso do g_build_filename com recursos_prefix
   gtk_builder_add_from_file( builder, "./recursos/interface.glade", &error );

   if ( error != NULL ) {
      g_printerr( "🚨 Erro ao carregar a interface embutida do Glade: %s\n", error->message );
      g_clear_error( &error );
      g_object_unref( builder );
      return;
   }

   // Despacha a montagem chamando os módulos focados
   _mapear_janela_e_cabecalho( ctx, builder, app );
   _mapear_filtros_e_configuracoes( ctx, builder );
   _mapear_aba_diario( ctx, builder );
   _mapear_aba_provas_e_acervo( ctx, builder );
   _mapear_motores_de_acao( ctx, builder );
   _mapear_painel_feedback( ctx, builder );

   // Aplica os estilos finais visuais via Pango/GtkLabel
   _aplicar_espacamentos_interface( ctx, builder );

   // Libera o objeto builder da memória de forma segura
   g_object_unref( builder );
}
