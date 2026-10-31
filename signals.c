/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include "signals.h"
#include "comum.h"
#include "interface.h"
#include "callbacks.h"
#include "rgba.h"




/**
 * @brief Atribui metadados GObject (categoria e valor) aos radio buttons e nomeia widgets para o CSS.
 */
static void _configurar_nomes_dos_widgets( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );
   InterfaceLatex  *latex  = &ctx->ui_latex;
   InterfaceProva  *prova  = &ctx->ui_prova;
   InterfaceDiario *diario = &ctx->ui_diario;

   // 1. Nomes específicos para seletores do CSS
   gtk_widget_set_name( prova->combo_tema, "tema" );
   gtk_widget_set_name( prova->check_validar_ciclos, "Números Cruzados" );
   gtk_widget_set_name( prova->check_nao_presencial, "Não Presencial" );
   gtk_widget_set_name( diario->check_expor_dados,   "Expôr Dados" );

   g_object_set_data( G_OBJECT( prova->check_validar_ciclos ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_VALIDAR_CICLOS ) );
   g_object_set_data( G_OBJECT( prova->check_nao_presencial ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_NAO_PRESENCIAL ) );
   g_object_set_data( G_OBJECT( diario->check_expor_dados ),   "categoria", GINT_TO_POINTER( CATEGORIA_UI_EXPOR_DADOS ) );

   // 2. Categoria 1: Colunas (Valor 2 ou 3)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( latex->radio_qtd_colunas[i], "Colunas" );
      g_object_set_data( G_OBJECT( latex->radio_qtd_colunas[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_COLUNAS ) );
      g_object_set_data( G_OBJECT( latex->radio_qtd_colunas[i] ), "valor",     GINT_TO_POINTER( i + 2 ) ); // 2 ou 3
   }

   // 3. Categoria 2: Separadores (Valor 1 = Pontilhado, Valor 2 = Contínuo)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( latex->radio_separadores[i], "Separadores" );
      g_object_set_data( G_OBJECT( latex->radio_separadores[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_SEPARADORES ) );
      g_object_set_data( G_OBJECT( latex->radio_separadores[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1 ou 2
   }

   // 4. Categoria 3: Fonte TeX (Valor 1 = CMB Right, Valor 2 = CMU Serif)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( latex->radio_fonte_latex[i], "Fonte TeX" );
      g_object_set_data( G_OBJECT( latex->radio_fonte_latex[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_FONTE_LATEX ) );
      g_object_set_data( G_OBJECT( latex->radio_fonte_latex[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1 ou 2
   }

   // 5. Categoria 4: Quantidade de Páginas (Valor 1 ou 2)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( latex->radio_qtd_paginas[i], "Qtd Páginas" );
      g_object_set_data( G_OBJECT( latex->radio_qtd_paginas[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_PAGINAS ) );
      g_object_set_data( G_OBJECT( latex->radio_qtd_paginas[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1 ou 2
   }

   // 6. Categoria 5: Tipo de Cabeçalho (Valor 1 = Acima, Valor 2 = À esquerda)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( latex->radio_cabecalho_tipo[i], "Cabeçalho" );
      g_object_set_data( G_OBJECT( latex->radio_cabecalho_tipo[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_CABECALHO ) );
      g_object_set_data( G_OBJECT( latex->radio_cabecalho_tipo[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1 ou 2
   }
}




/**
 * @brief Função auxiliar universal para configurar renderizadores de texto editáveis em um intervalo de colunas.
 *
 * @param ctx Contexto global da aplicação.
 * @param treeview Ponteiro para o GtkTreeView alvo.
 * @param col_inicio Índice numérico da primeira coluna editável.
 * @param col_fim Índice numérico da última coluna editável.
 */
static void _configurar_colunas_editaveis( AppContext *ctx, GtkTreeView *treeview, int col_inicio, int col_fim ) {
   g_return_if_fail( GTK_IS_TREE_VIEW( treeview ) );

   for ( int col_idx = col_inicio; col_idx <= col_fim; col_idx++ ) {
      GtkTreeViewColumn *col = gtk_tree_view_get_column( treeview, col_idx );
      if ( !col ) continue;

      GList *renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( col ) );
      if ( renderers && GTK_IS_CELL_RENDERER_TEXT( renderers->data ) ) {
         GtkCellRendererText *r = GTK_CELL_RENDERER_TEXT( renderers->data );

         // 1. Garante que o renderizador permite edição
         g_object_set( G_OBJECT( r ), "editable", TRUE, NULL );

         // 2. Salva os metadados (índice da coluna e ponteiro do TreeView)
         g_object_set_data( G_OBJECT( r ), "col_model_idx", GINT_TO_POINTER( col_idx ) );
         g_object_set_data( G_OBJECT( r ), "ptr_treeview", treeview );

         // 3. Conecta os callbacks UNIVERSAIS
         g_signal_connect( r, "editing-started", G_CALLBACK( on_cell_nota_editing_started ), ctx );
         g_signal_connect( r, "edited",          G_CALLBACK( on_cell_nota_edited ),          ctx );

         g_list_free( renderers );
      }
   }
}





/**
 * Verifica se a linha atual está clicada/selecionada pelo usuário.
 * Retorna TRUE se a cor de seleção foi aplicada.
 */
// static gboolean _cor_fundo_linha_selecionada_modelo( GtkTreeView *treeview, GtkTreeModel *model,
//                                                        GtkTreeIter *iter, int tema_ativo, GdkRGBA *cor_out ) {
//    GtkTreePath *path = gtk_tree_model_get_path( model, iter );
//    if ( !path ) return FALSE;
//
//    GtkTreeSelection *selecao = gtk_tree_view_get_selection( treeview );
//    gboolean selecionada = gtk_tree_selection_path_is_selected( selecao, path );
//    gtk_tree_path_free( path );
//
//    if ( selecionada ) {
//       return cor_fundo_linha_selecionada( tema_ativo, cor_out );
//    }
//    return FALSE;
// }


//===================================================================================================
// 1. FUNÇÕES UTILITÁRIAS GENÉRICAS (MÓDULO BASE)
//===================================================================================================

/**
 * Aplica o padrão zebrado descobrindo automaticamente a linha (row) atual.
 * Retorna TRUE se a cor foi aplicada.
 */
static gboolean _cor_fundo_linha_zebrada_modelo( GtkTreeModel *model, GtkTreeIter *iter, int tema_ativo, GdkRGBA *cor_out ) {
   GtkTreePath *path = gtk_tree_model_get_path( model, iter );
   if ( !path ) return FALSE;

   gint row_idx = gtk_tree_path_get_indices( path )[0];
   gtk_tree_path_free( path );

   return cor_fundo_linha_zebrada( row_idx, tema_ativo, cor_out );
}

/**
 * Utilitário para injetar o índice da coluna em todos os renderizadores de um intervalo
 * e conectar a função de CellDataFunc. Elimina a repetição dos 4 loops.
 */
static void _vincular_renderizadores_em_massa( GtkTreeView *tree_view, int col_inicio, int col_fim,
      GtkTreeCellDataFunc callback, AppContext *ctx ) {
   if ( !tree_view ) return;

   for ( int i = col_inicio; i <= col_fim; i++ ) {
      GtkTreeViewColumn *col = gtk_tree_view_get_column( tree_view, i );
      if ( !col ) continue;

      GList *renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( col ) );
      if ( renderers ) {
         GtkCellRenderer *renderer = GTK_CELL_RENDERER( renderers->data );

         // Injeta a identidade da coluna no próprio renderer
         g_object_set_data( G_OBJECT( renderer ), "col_model_idx", GINT_TO_POINTER( i ) );
         gtk_tree_view_column_set_cell_data_func( col, renderer, callback, ctx, NULL );

         g_list_free( renderers );
      }
   }
}

/**
 * Utilitário para aplicar ou remover a cor de fundo nativa no renderer.
 */
static inline void _aplicar_cor_fundo( GtkCellRenderer *cell, gboolean usar, const GdkRGBA *cor ) {
   if ( usar ) g_object_set( cell, "cell-background-rgba", cor, "cell-background-set", TRUE, NULL );
   else        g_object_set( cell, "cell-background-set", FALSE, NULL );
}

/**
 * Utilitário para aplicar ou remover a cor de fonte nativa no renderer.
 */
static inline void _aplicar_cor_texto( GtkCellRenderer *cell, gboolean usar, const GdkRGBA *cor ) {
   if ( usar ) g_object_set( cell, "foreground-rgba", cor, "foreground-set", TRUE, NULL );
   else        g_object_set( cell, "foreground-set", FALSE, NULL );
}


//===================================================================================================
// 2. MÓDULO: DIÁRIO DE CONTEÚDO
//===================================================================================================
static void _on_renderizar_cores_conteudo( GtkTreeViewColumn *col, GtkCellRenderer *cell,
      GtkTreeModel *model, GtkTreeIter *iter, gpointer data ) {
   ( void )col;
   AppContext *ctx = ( AppContext * )data;
   int tema_ativo = ctx->cabecalho.foco_estilo;

   // 1. Fundo da linha (Zebrado)
   GdkRGBA cor;
   gboolean zebrado = _cor_fundo_linha_zebrada_modelo( model, iter, tema_ativo, &cor );
   _aplicar_cor_fundo( cell, zebrado, &cor );

   // 2. Identificação da coluna e aplicação condicional da fonte
   gint col_idx = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( cell ), "col_model_idx" ) );

   if ( col_idx == 2 || col_idx == 3 ) {
      int tipo_registro = 0;
      gtk_tree_model_get( model, iter, 4, &tipo_registro, -1 );

      gboolean tem_cor = cor_texto_tipo_registro( tipo_registro, tema_ativo, &cor );
      _aplicar_cor_texto( cell, tem_cor, &cor );
   } else {
      _aplicar_cor_texto( cell, FALSE, NULL );
   }
}

static void _inicializar_renderizadores_conteudo( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );
   _vincular_renderizadores_em_massa( GTK_TREE_VIEW( ctx->ui_diario.conteudo.treeview ),
                                      0, 3, _on_renderizar_cores_conteudo, ctx );
}


//===================================================================================================
// 3. MÓDULO: DIÁRIO DE FREQUÊNCIA
//===================================================================================================
static void _on_renderizar_cores_frequencia( GtkTreeViewColumn *col, GtkCellRenderer *cell,
      GtkTreeModel *model, GtkTreeIter *iter, gpointer data ) {
   ( void )col;
   AppContext *ctx = ( AppContext * )data;
   int tema_ativo = ctx->cabecalho.foco_estilo;

   // 1. Fundo da linha (Zebrado)
   GdkRGBA cor;
   gboolean zebrado = _cor_fundo_linha_zebrada_modelo( model, iter, tema_ativo, &cor );
   _aplicar_cor_fundo( cell, zebrado, &cor );

   gint col_idx = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( cell ), "col_model_idx" ) );

   // 2. Fonte (Protegendo a Coluna 0 de numeração)
   if ( col_idx != 0 ) {
      int status = 0;
      gtk_tree_model_get( model, iter, 6, &status, -1 );

      gboolean tem_cor = cor_texto_linha_frequencia( status, tema_ativo, &cor );
      _aplicar_cor_texto( cell, tem_cor, &cor );
   } else {
      _aplicar_cor_texto( cell, FALSE, NULL );
   }
}

static void _inicializar_renderizadores_frequencia( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );
   _vincular_renderizadores_em_massa( GTK_TREE_VIEW( ctx->ui_diario.frequencia.treeview ),
                                      0, 4, _on_renderizar_cores_frequencia, ctx );
}


//===================================================================================================
// 4. MÓDULO: AVALIAÇÕES / NOTAS
//===================================================================================================
static void _aplicar_cor_condicional_nota( GtkCellRenderer *cell, gboolean eh_inativo,
      const gchar *texto_nota, int tema_ativo ) {

   double nota_val = 10.0;

   if ( !eh_inativo && texto_nota && texto_nota[0] != '\0' && g_strcmp0( texto_nota, "-" ) != 0 ) {
      const gchar *ponteiro_leitura = g_str_has_prefix( texto_nota, "\u2007" ) ? texto_nota + 3 : texto_nota;

      char buffer_float[16];
      g_strlcpy( buffer_float, ponteiro_leitura, sizeof( buffer_float ) );
      g_strdelimit( buffer_float, ",", '.' );

      nota_val = g_ascii_strtod( buffer_float, NULL );
   }

   GdkRGBA cor;
   gboolean usar_cor = cor_texto_aluno_inativo( eh_inativo, tema_ativo, &cor ) ||
                       cor_texto_nota_vermelha( nota_val, tema_ativo, &cor );

   _aplicar_cor_texto( cell, usar_cor, &cor );
}

static void _on_renderizar_cores_notas( GtkTreeViewColumn *col, GtkCellRenderer *cell, GtkTreeModel *model,
                                        GtkTreeIter *iter, gpointer data ) {
   ( void )col;
   AppContext *ctx = ( AppContext * )data;
   int tema_ativo = ctx->cabecalho.foco_estilo;

   GdkRGBA cor;
   gboolean zebrado = _cor_fundo_linha_zebrada_modelo( model, iter, tema_ativo, &cor );
   _aplicar_cor_fundo( cell, zebrado, &cor );

   gboolean eh_inativo = FALSE;
   gtk_tree_model_get( model, iter, 13, &eh_inativo, -1 );

   gint col_idx = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( cell ), "col_model_idx" ) );

   // Fonte e Editabilidade
   if ( col_idx == 0 ) {
      _aplicar_cor_texto( cell, FALSE, NULL );
      g_object_set( cell, "editable", FALSE, NULL );
   } else if ( col_idx == 1 ) {
      _aplicar_cor_condicional_nota( cell, eh_inativo, "", tema_ativo );
      g_object_set( cell, "editable", FALSE, NULL );
   } else {
      gchar *texto = NULL;
      g_object_get( cell, "text", &texto, NULL );
      _aplicar_cor_condicional_nota( cell, eh_inativo, texto, tema_ativo );
      g_free( texto );

      // Média final (12) é negrito e não editável. Notas normais (2 a 11) são editáveis se ativo.
      g_object_set( cell, "editable", ( !eh_inativo && col_idx < 12 ), NULL );
   }

   // Negrito para coluna 12 (Média)
   g_object_set( cell, "weight", PANGO_WEIGHT_BOLD, "weight-set", ( col_idx == 12 ), NULL );
}

static void _inicializar_renderizadores_avaliacoes( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );
   _vincular_renderizadores_em_massa( GTK_TREE_VIEW( ctx->ui_diario.avaliacao.treeview ),
                                      0, 12, _on_renderizar_cores_notas, ctx );
}


//===================================================================================================
// 5. MÓDULO: RELATÓRIOS
//===================================================================================================
static void _on_renderizar_cores_relatorio( GtkTreeViewColumn *col, GtkCellRenderer *cell,
      GtkTreeModel *model, GtkTreeIter *iter, gpointer data ) {
   ( void )col;
   AppContext *ctx = ( AppContext * )data;
   int tema_ativo = ctx->cabecalho.foco_estilo;

   gboolean eh_inativo = FALSE;
   int mascara_pendencias = 0, mascara_observacao = 0;

   gtk_tree_model_get( model, iter, 11, &eh_inativo, 12, &mascara_pendencias, 13, &mascara_observacao, -1 );
   gint col_idx = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( cell ), "col_model_idx" ) );

   gboolean modo_pendencias = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( ctx->ui_diario.relatorio.check_pendencias ) );

   gboolean editavel = FALSE;
   gboolean usar_fundo = FALSE;
   GdkRGBA cor_fundo;

   // Lógica de Pendências
   if ( modo_pendencias && !eh_inativo ) {
      gboolean permissao_ativa = FALSE;
      switch ( col_idx ) {
      case 2:
         permissao_ativa = ( mascara_pendencias & MASK_PENDENCIA_1_PERIODO );
         break;
      case 3:
         permissao_ativa = ( mascara_pendencias & MASK_PENDENCIA_2_PERIODO );
         break;
      case 4:
         permissao_ativa = ( mascara_pendencias & MASK_PENDENCIA_3_PERIODO );
         break;
      case 5:
         permissao_ativa = ( mascara_pendencias & MASK_PENDENCIA_4_PERIODO );
         break;
      case 8:
         permissao_ativa = ( mascara_pendencias & MASK_DESTRAVA_REC_FINAL );
         break;
      case 9:
         permissao_ativa = ( mascara_pendencias & MASK_DESTRAVA_CONSELHO );
         break;
      }

      if ( permissao_ativa ) {
         editavel = TRUE;
         usar_fundo = cor_fundo_celula_pendente( tema_ativo, &cor_fundo );
      }
   }

   // Zebrado como fallback do fundo
   if ( !usar_fundo ) {
      usar_fundo = _cor_fundo_linha_zebrada_modelo( model, iter, tema_ativo, &cor_fundo );
   }

   _aplicar_cor_fundo( cell, usar_fundo, &cor_fundo );
   g_object_set( cell, "editable", editavel, NULL );

   // Lógica de Fonte
   if ( col_idx == 10 ) {
      GdkRGBA cor_texto;
      if ( cor_texto_observacao( eh_inativo, tema_ativo, mascara_observacao, &cor_texto ) ) {
         g_object_set( cell, "foreground-rgba", &cor_texto, "foreground-set", TRUE,
                       "weight", PANGO_WEIGHT_BOLD, "weight-set", TRUE, NULL );
      } else {
         g_object_set( cell, "foreground-set", FALSE, "weight-set", FALSE, NULL );
      }
   } else if ( col_idx >= 1 && col_idx <= 9 ) {
      gchar *texto = NULL;
      if ( ( col_idx >= 2 && col_idx <= 5 ) || ( col_idx >= 7 && col_idx <= 9 ) ) {
         g_object_get( cell, "text", &texto, NULL );
      }
      _aplicar_cor_condicional_nota( cell, eh_inativo, texto, tema_ativo );
      g_object_set( cell, "weight-set", FALSE, NULL );
      g_free( texto );
   }
}

static void _inicializar_renderizadores_relatorio( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );
   _vincular_renderizadores_em_massa( GTK_TREE_VIEW( ctx->ui_diario.relatorio.treeview ),
                                      0, 10, _on_renderizar_cores_relatorio, ctx );
}
//============================================================================================================








// ======================================================================================
// CONEXÃO DE SINAIS MODULARIZADA POR DOMÍNIO
// ======================================================================================

static void _conectar_sinais_cabecalho( AppContext *ctx ) {
   InterfaceCabecalho *cab = &ctx->cabecalho;

   cab->handler_ano        = g_signal_connect( cab->combo_ano, "changed",
                             G_CALLBACK( on_entry_atualizar_ano_interface_changed ), ctx );
   cab->handler_escola     = g_signal_connect( cab->combo_escola, "changed",
                             G_CALLBACK( on_entry_atualizar_escola_interface_changed ), ctx );
   cab->handler_turma      = g_signal_connect( cab->combo_turma, "changed",
                             G_CALLBACK( on_entry_atualizar_turma_interface_changed ), ctx );
   cab->handler_disciplina = g_signal_connect( cab->combo_disciplina, "changed",
                             G_CALLBACK( on_entry_atualizar_disciplina_interface_changed ), ctx );

   g_signal_connect( cab->combo_periodo, "changed", G_CALLBACK( on_entry_periodo_interface_changed ), ctx );
   g_signal_connect( cab->combo_estilo, "changed", G_CALLBACK( on_entry_estilo_interface_changed ), ctx );
   g_signal_connect( cab->btn_atualizar_alunos, "clicked", G_CALLBACK( on_button_siaep_atualizar_alunos_clicked ), ctx );
}


static void _conectar_sinais_aba_diarios( AppContext *ctx ) {
   InterfaceDiario     *diario     = &ctx->ui_diario;
   InterfaceConteudo   *conteudo   = &diario->conteudo;
   InterfaceFrequencia *frequencia = &diario->frequencia;
   InterfaceAvaliacao  *avaliacao  = &diario->avaliacao;
   InterfaceRelatorio  *relatorio  = &diario->relatorio;

   // --- ROOT: CONTROLES DO DIÁRIO ---
   g_signal_connect( diario->check_modo_edicao, "toggled", G_CALLBACK( on_check_modo_edicao_toggled ), ctx );
   g_signal_connect( diario->check_expor_dados, "toggled", G_CALLBACK( on_check_atualizar_booleanos_interface_toggled ), ctx );

   // --- NAVEGAÇÃO LATERAL (Hover e Click) ---
   g_signal_connect( diario->btn_conteudo, "enter-notify-event", G_CALLBACK( on_button_conteudos_enter_notify_event ), ctx );
   g_signal_connect( diario->btn_frequencia, "enter-notify-event",
                     G_CALLBACK( on_button_frequencia_enter_notify_event ), ctx );
   g_signal_connect( diario->btn_avaliacao, "enter-notify-event", G_CALLBACK( on_button_avaliacoes_enter_notify_event ), ctx );
   g_signal_connect( diario->btn_relatorio, "enter-notify-event", G_CALLBACK( on_button_relatorio_enter_notify_event ), ctx );

   g_signal_connect( diario->btn_conteudo, "clicked", G_CALLBACK( on_button_relatorio_de_conteudos_clicked ), ctx );
   g_signal_connect( diario->btn_frequencia, "clicked", G_CALLBACK( on_button_relatorio_de_frequencia_clicked ), ctx );
   g_signal_connect( diario->btn_avaliacao, "clicked", G_CALLBACK( on_button_relatorio_de_avaliacoes_clicked ), ctx );
   g_signal_connect( diario->btn_relatorio, "clicked", G_CALLBACK( on_button_relatorio_final_clicked ), ctx );

   // --- SUB-ABA: CONTEÚDO ---
   g_signal_connect( conteudo->calendario_data, "next-month", G_CALLBACK( on_calendar_navigation ), ctx );
   g_signal_connect( conteudo->calendario_data, "prev-month", G_CALLBACK( on_calendar_navigation ), ctx );
   g_signal_connect( conteudo->calendario_data, "next-year", G_CALLBACK( on_calendar_navigation ), ctx );
   g_signal_connect( conteudo->calendario_data, "prev-year", G_CALLBACK( on_calendar_navigation ), ctx );
   g_signal_connect( conteudo->calendario_data, "day-selected", G_CALLBACK( on_calendar_day_selected ), ctx );

   g_signal_connect( conteudo->entry_data, "button-press-event", G_CALLBACK( on_entry_data_button_press ), ctx );
   g_signal_connect( conteudo->entry_data, "focus-out-event", G_CALLBACK( on_entry_validar_data_focus_out_event ), ctx );

   g_signal_connect( conteudo->stepper_menos, "clicked", G_CALLBACK( on_button_stepper_menos_num_horarios_clicked ), ctx );
   g_signal_connect( conteudo->stepper_mais,  "clicked", G_CALLBACK( on_button_stepper_mais_num_horarios_clicked ), ctx );

   g_signal_connect( conteudo->entry_tema, "focus-out-event", G_CALLBACK( on_entry_tema_focus_out_event ), ctx );
   g_signal_connect( conteudo->entry_tema, "activate", G_CALLBACK( on_entry_tema_activate ), ctx );
   g_signal_connect( conteudo->entry_descricao, "activate", G_CALLBACK( on_entry_registrar_aula_activate ), ctx );
   g_signal_connect( conteudo->btn_salvar, "clicked", G_CALLBACK( on_button_registrar_aula_clicked ), ctx );
   g_signal_connect( conteudo->remover_registro, "clicked",
                     G_CALLBACK( on_button_remover_registro_diario_selecionado_clicked ), ctx );

   g_signal_connect( conteudo->treeview, "key-press-event", G_CALLBACK( on_treeview_remover_registro_diario_selecionado_key_press_event ), ctx );
   g_signal_connect( conteudo->treeview, "row-activated",
                     G_CALLBACK( on_treeview_carregar_registro_para_edicao_row_activated ), ctx );

   GtkTreeSelection *sel_conteudo = gtk_tree_view_get_selection( GTK_TREE_VIEW( conteudo->treeview ) );
   g_signal_connect( sel_conteudo, "changed",
                     G_CALLBACK( on_diario_selection_changed ), ctx );

   _inicializar_renderizadores_conteudo( ctx );

   // --- SUB-ABA: FREQUÊNCIA ---
   frequencia->handler_combo_data = g_signal_connect( frequencia->combo_data, "changed",
                                    G_CALLBACK( on_combo_data_frequencia_changed ), ctx );
   frequencia->handler_combo_alunos = g_signal_connect( frequencia->combo_alunos, "changed",
                                      G_CALLBACK( on_combo_alunos_changed ), ctx );

   g_signal_connect( frequencia->check_por_aluno, "toggled", G_CALLBACK( on_check_por_aluno_toggled ), ctx );
   g_signal_connect( frequencia->btn_presente, "clicked", G_CALLBACK( on_button_presente_clicked ), ctx );
   g_signal_connect( frequencia->btn_ausente, "clicked", G_CALLBACK( on_button_ausente_clicked ), ctx );
   g_signal_connect( frequencia->btn_justificada, "clicked", G_CALLBACK( on_button_justificada_clicked ), ctx );
   g_signal_connect( frequencia->btn_salvar, "clicked", G_CALLBACK( on_button_salvar_frequencia_clicked ), ctx );

   // Atalho de teclado global focado na janela principal
   g_signal_connect( ctx->window, "key-press-event", G_CALLBACK( on_key_presente_ou_ausente_key_press_event ), ctx );
   g_signal_connect( frequencia->treeview, "cursor-changed", G_CALLBACK( on_treeview_frequencia_cursor_changed ), ctx );
   _inicializar_renderizadores_frequencia( ctx );

   // --- SUB-ABA: AVALIAÇÕES ---
   avaliacao->handler_combo_avaliacao = g_signal_connect( avaliacao->combo_avaliacao, "changed", G_CALLBACK( on_combo_selecionar_avaliacao_changed ), ctx );
   avaliacao->handler_check_desativar = g_signal_connect( avaliacao->check_desativar_avaliacao, "toggled", G_CALLBACK( on_check_desativar_avaliacao_toggled ), ctx );

   g_signal_connect( avaliacao->btn_nova_avaliacao, "clicked", G_CALLBACK( on_button_nova_avaliacao_clicked ), ctx );
   g_signal_connect( avaliacao->btn_editar_avaliacao, "clicked", G_CALLBACK( on_button_editar_avaliacao_clicked ), ctx );
   g_signal_connect( avaliacao->btn_popover_nomear, "clicked", G_CALLBACK( on_button_popover_nomear_avaliacao_clicked ), ctx );
   g_signal_connect( avaliacao->entry_popover_nomear, "activate",
                     G_CALLBACK( on_entry_popover_nomear_avaliacao_activate ), ctx );
   g_signal_connect( avaliacao->btn_salvar, "clicked", G_CALLBACK( on_button_salvar_avaliacoes_clicked ), ctx );

   _configurar_colunas_editaveis( ctx, GTK_TREE_VIEW( avaliacao->treeview ), 2, 11 );
   g_signal_connect( avaliacao->treeview, "cursor-changed", G_CALLBACK( on_treeview_avaliacao_notas_cursor_changed ), ctx );
   _inicializar_renderizadores_avaliacoes( ctx );

   // --- SUB-ABA: RELATÓRIO ---
   g_signal_connect( relatorio->check_pendencias, "toggled",
                     G_CALLBACK( on_check_pendencias_excecoes_relatorio_toggled ), ctx );
   g_signal_connect( relatorio->btn_consolidar, "clicked", G_CALLBACK( on_button_consolidar_relatorio_clicked ), ctx );

   _configurar_colunas_editaveis( ctx, GTK_TREE_VIEW( relatorio->treeview ), 2, 4 );
   _configurar_colunas_editaveis( ctx, GTK_TREE_VIEW( relatorio->treeview ), 8, 9 );
   _inicializar_renderizadores_relatorio( ctx );
}


static void _conectar_sinais_aba_latex( AppContext *ctx ) {
   InterfaceLatex *latex = &ctx->ui_latex;

   for ( int i = 0; i < 2; i++ ) {
      g_signal_connect( latex->radio_qtd_colunas[i], "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
      g_signal_connect( latex->radio_separadores[i], "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
      g_signal_connect( latex->radio_fonte_latex[i], "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
      g_signal_connect( latex->radio_qtd_paginas[i], "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
      g_signal_connect( latex->radio_cabecalho_tipo[i], "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
   }

   g_signal_connect( latex->combo_cor_serie, "changed", G_CALLBACK( on_entry_cor_serie_interface_changed ), ctx );
   g_signal_connect( latex->combo_decoracao, "changed", G_CALLBACK( on_entry_decoracao_estilo_interface_changed ), ctx );

   latex->handler_combo_tema_espelho = g_signal_connect( latex->combo_tema_espelho, "changed",
                                       G_CALLBACK( on_entry_atualizar_tema_changed ), ctx );

   g_signal_connect( latex->btn_pdflatex, "clicked", G_CALLBACK( on_button_abrir_tema_clicked ), ctx );
   g_signal_connect( latex->btn_compilar, "clicked", G_CALLBACK( on_button_compilar_questoes_clicked ), ctx );
   g_signal_connect( latex->btn_diversificar, "clicked", G_CALLBACK( on_button_atualizar_questoes_clicked ), ctx );
}


static void _conectar_sinais_aba_provas( AppContext *ctx ) {
   InterfaceProva *prova = &ctx->ui_prova;

   g_signal_connect( prova->check_validar_ciclos, "toggled",
                     G_CALLBACK( on_check_atualizar_booleanos_interface_toggled ), ctx );
   g_signal_connect( prova->check_nao_presencial, "toggled",
                     G_CALLBACK( on_check_atualizar_booleanos_interface_toggled ), ctx );

   prova->handler_combo_tema = g_signal_connect( prova->combo_tema, "changed",
                               G_CALLBACK( on_entry_atualizar_tema_changed ), ctx );
   g_signal_connect( prova->combo_seq_prova, "changed", G_CALLBACK( on_combo_sequencia_prova_changed ), ctx );

   prova->montagem.handler_scrolled = g_signal_connect( prova->scrolled_window, "size-allocate",
                                      G_CALLBACK( on_scrolled_rolar_para_o_fim_sizeallocate ), ctx );
   g_signal_handler_block( prova->scrolled_window, prova->montagem.handler_scrolled );
   prova->montagem.importar = FALSE;

   g_signal_connect( prova->btn_gerar_prova, "clicked", G_CALLBACK( on_button_gerar_prova_clicked ), ctx );
   g_signal_connect( prova->btn_processar, "clicked", G_CALLBACK( on_button_processar_imagens_clicked ), ctx );
   g_signal_connect( prova->btn_corrigir, "clicked", G_CALLBACK( on_button_corrigir_prova_clicked ), ctx );
   g_signal_connect( prova->btn_importar, "clicked", G_CALLBACK( on_button_carregar_estado_aplicativo_clicked ), ctx );
}


static void _conectar_sinais_painel_feedback( AppContext *ctx ) {
   InterfacePainel *painel = &ctx->painel;

   g_signal_connect( painel->eventbox_painel, "enter-notify-event", G_CALLBACK( on_painel_feedback_enter_notify_event ), ctx );
   g_signal_connect( painel->eventbox_painel, "leave-notify-event", G_CALLBACK( on_painel_feedback_leave_notify_event ), ctx );
   g_signal_connect( painel->eventbox_painel, "button-press-event", G_CALLBACK( on_painel_feedback_button_press_event ), ctx );

   g_signal_connect( painel->eventbox_orelhinha, "button-press-event", G_CALLBACK( on_orelhinha_button_press_event ), ctx );
   g_signal_connect( painel->revealer_painel, "notify::child-revealed", G_CALLBACK( on_revealer_child_revealed_notify ), ctx );
}


// ======================================================================================
// ORQUESTRADOR DE SINAIS MESTRE
// ======================================================================================

/**
 * @brief Orquestrador que conecta todos os sinais (cliques, toggles, keys) do Vértice.
 * @param user_data Ponteiro para a estrutura AppContext global.
 */
void app_signals_connect( gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_if_fail( ctx != NULL );

   // 1. Inicializa metadados (GObject set_data) para uso genérico nos callbacks
   _configurar_nomes_dos_widgets( ctx );

   // 2. Conexões de navegação Global
   g_signal_connect( ctx->notebook, "switch-page", G_CALLBACK( on_notebook_principal_switch_page ), ctx );

   // 3. Despacho modular
   _conectar_sinais_cabecalho( ctx );
   _conectar_sinais_aba_diarios( ctx );
   _conectar_sinais_aba_latex( ctx );
   _conectar_sinais_aba_provas( ctx );
   _conectar_sinais_painel_feedback( ctx );
}

