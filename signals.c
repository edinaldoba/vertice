/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include "signals.h"
#include "acervo.h"
#include "comum.h"
#include "interface.h"
#include "callbacks.h"
#include "imagens.h"
#include "relatorios.h"
#include "acervo.h"





/**
 * @brief Atribui metadados GObject (categoria e valor) aos radio buttons e nomeia widgets para o CSS.
 */
void configurar_nomes_dos_widgets( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );

   // 1. Nomes específicos para seletores do CSS
   gtk_widget_set_name( ctx->entry.tema, "tema" );
   gtk_widget_set_name( ctx->check.validar_ciclos, "Números Cruzados" );
   gtk_widget_set_name( ctx->check.nao_presencial, "Não Presencial" );
   gtk_widget_set_name( ctx->check.expor_dados, "Expôr Dados" );

   g_object_set_data( G_OBJECT( ctx->check.validar_ciclos ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_NUM_CRUZADOS ) );
   g_object_set_data( G_OBJECT( ctx->check.nao_presencial ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_NAO_PRESENCIAL ) );
   g_object_set_data( G_OBJECT( ctx->check.expor_dados ),    "categoria", GINT_TO_POINTER( CATEGORIA_UI_EXPOR_DADOS ) );

   // 2. Categoria 1: Colunas (Valor 2 ou 3)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( ctx->radio.qtd_colunas[i], "Colunas" );
      g_object_set_data( G_OBJECT( ctx->radio.qtd_colunas[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_COLUNAS ) );
      g_object_set_data( G_OBJECT( ctx->radio.qtd_colunas[i] ), "valor",     GINT_TO_POINTER( i + 2 ) ); // 2 ou 3
   }

   // 3. Categoria 2: Separadores (Valor 1 = Pontilhado, Valor 2 = Contínuo)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( ctx->radio.separadores[i], "Separadores" );
      g_object_set_data( G_OBJECT( ctx->radio.separadores[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_SEPARADORES ) );
      g_object_set_data( G_OBJECT( ctx->radio.separadores[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1 ou 2
   }

   // 4. Categoria 3: Fonte TeX (Valor 1 = CMB Right, Valor 2 = CMU Serif)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( ctx->radio.fonte_latex[i], "Fonte TeX" );
      g_object_set_data( G_OBJECT( ctx->radio.fonte_latex[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_FONTE_LATEX ) );
      g_object_set_data( G_OBJECT( ctx->radio.fonte_latex[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1 ou 2
   }

   // 5. Categoria 4: Quantidade de Páginas (Valor 1 ou 2)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( ctx->radio.qtd_paginas[i], "Qtd Páginas" );
      g_object_set_data( G_OBJECT( ctx->radio.qtd_paginas[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_PAGINAS ) );
      g_object_set_data( G_OBJECT( ctx->radio.qtd_paginas[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1 ou 2
   }

   // 6. Categoria 5: Tipo de Cabeçalho (Valor 1 = Acima, Valor 2 = À esquerda)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( ctx->radio.cabecalho_tipo[i], "Cabeçalho" );
      g_object_set_data( G_OBJECT( ctx->radio.cabecalho_tipo[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_CABECALHO ) );
      g_object_set_data( G_OBJECT( ctx->radio.cabecalho_tipo[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1 ou 2
   }

   // 7. Categoria 6: Sequência Prova (Valor 1 = 1ª, Valor 2 = 2ª, Valor 3 = 3ª)
   for ( int i = 0; i < 3; i++ ) {
      gtk_widget_set_name( ctx->radio.avaliacao[i], "Seq Prova" );
      g_object_set_data( G_OBJECT( ctx->radio.avaliacao[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_PROVA ) );
      g_object_set_data( G_OBJECT( ctx->radio.avaliacao[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1, 2 ou 3
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
 * @brief Conecta todos os sinais de eventos (cliques e alternâncias)do aplicativo.
 * @param bot      Ponteiro para a estrutura persistente de botões de controle.
 * @param opt      Ponteiro para a estrutura persistente de configurações LaTeX da prova.
 * @param alt      Ponteiro para a estrutura persistente de seletores booleanos (toggles).
 * @param provider Ponteiro para o gerenciador de estilo CSS do GTK.
 */
void app_signals_connect( gpointer user_data ) {

   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   configurar_nomes_dos_widgets( ctx );

   int i;

   // =========================================================================
   // 2. VERIFICADORES DE ESTADO DA INTERFACE (BOOLEANOS)
   // =========================================================================
   // Conecta os seletores booleanos do diário/sistema de forma clara e explícita
   g_signal_connect( ctx->check.validar_ciclos, "toggled",
                     G_CALLBACK( on_check_atualizar_booleanos_interface_toggled ), ctx );
   ctx->handlers.nao_presencial = g_signal_connect( ctx->check.nao_presencial, "toggled",
                                  G_CALLBACK( on_check_atualizar_booleanos_interface_toggled ), ctx );
   ctx->handlers.expor_dados = g_signal_connect( ctx->check.expor_dados,    "toggled",
                               G_CALLBACK( on_check_atualizar_booleanos_interface_toggled ), ctx );

   // =========================================================================
   // 3. SELETORES DE CONFIGURAÇÃO DO DOCUMENTO LATEX (OPÇÕES COMPILADAS)
   // =========================================================================
   for ( i = 0; i < 2 ; i++ ) {
      g_signal_connect( ctx->radio.qtd_colunas[i],    "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
      g_signal_connect( ctx->radio.separadores[i],    "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
      g_signal_connect( ctx->radio.fonte_latex[i],    "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
      g_signal_connect( ctx->radio.qtd_paginas[i],    "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
      g_signal_connect( ctx->radio.cabecalho_tipo[i], "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
   }
   for ( i = 0; i < 3; i++ ) {
      g_signal_connect( ctx->radio.avaliacao[i],       "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
   }

   // =========================================================================
   // 4. CONEXÃO DE SINAIS: CICLO PEDAGÓGICO E PRODUÇÃO DE DOCUMENTOS
   // =========================================================================
   g_signal_connect( ctx->button.frequencia,        "clicked", G_CALLBACK( on_button_relatorio_de_frequencia_clicked ), ctx );
   g_signal_connect( ctx->button.conteudos,         "clicked", G_CALLBACK( on_button_relatorio_de_conteudos_clicked ), ctx );
   g_signal_connect( ctx->button.avaliacoes,        "clicked", G_CALLBACK( on_button_relatorio_de_avaliacoes_clicked ), ctx );
   g_signal_connect( ctx->button.relatorio_final,   "clicked", G_CALLBACK( on_button_relatorio_final_clicked ), ctx );
   g_signal_connect( ctx->button.atualizar_alunos,  "clicked", G_CALLBACK( on_button_siaep_atualizar_alunos_clicked ), ctx );

   g_signal_connect( ctx->ui_diario.calendario_data, "next-month", G_CALLBACK( on_calendar_navigation ), ctx );
   g_signal_connect( ctx->ui_diario.calendario_data, "prev-month", G_CALLBACK( on_calendar_navigation ), ctx );
   g_signal_connect( ctx->ui_diario.calendario_data, "next-year", G_CALLBACK( on_calendar_navigation ), ctx );
   g_signal_connect( ctx->ui_diario.calendario_data, "prev-year", G_CALLBACK( on_calendar_navigation ), ctx );

   g_signal_connect( ctx->ui_diario.calendario_data, "day-selected", G_CALLBACK( on_calendar_day_selected ), ctx );
   g_signal_connect( ctx->ui_diario.entry_data, "button-press-event", G_CALLBACK( on_entry_data_button_press ), ctx );
   g_signal_connect( ctx->ui_diario.entry_data, "focus-out-event", G_CALLBACK( on_entry_validar_data_focus_out ), ctx );

   g_signal_connect( ctx->ui_diario.stepper_menos, "clicked", G_CALLBACK( on_button_stepper_menos_num_horarios_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.stepper_mais,  "clicked", G_CALLBACK( on_button_stepper_mais_num_horarios_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.remover_registro, "clicked",
                     G_CALLBACK( on_button_remover_registro_diario_selecionado_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.treeview_conteudo, "key-press-event",
                     G_CALLBACK( on_treeview_remover_registro_diario_selecionado_key_press_event ), ctx );
   // Obtém o controle de seleção do TreeView
   GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( ctx->ui_diario.treeview_conteudo ) );
   g_signal_connect( selection, "changed", G_CALLBACK( on_diario_selection_changed ), ctx );

   g_signal_connect( ctx->ui_diario.descricao, "activate", G_CALLBACK( on_entry_registrar_aula_activate ), ctx );
   g_signal_connect( ctx->ui_diario.btn_salvar_conteudo, "clicked", G_CALLBACK( on_button_registrar_aula_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.treeview_conteudo, "row-activated",
                     G_CALLBACK( on_treeview_carregar_registro_para_edicao_row_activated ), ctx );


   ctx->ui_diario.handler_combo_data = g_signal_connect( ctx->ui_diario.combo_data, "changed",
                                       G_CALLBACK( on_combo_data_frequencia_changed ), ctx );
   g_signal_connect( ctx->ui_diario.check_por_aluno, "toggled", G_CALLBACK( on_check_por_aluno_toggled ), ctx );
   g_signal_connect( ctx->ui_diario.btn_presente, "clicked", G_CALLBACK( on_button_presente_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.btn_ausente, "clicked", G_CALLBACK( on_button_ausente_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.btn_justificada, "clicked", G_CALLBACK( on_button_justificada_clicked ), ctx );
   g_signal_connect( ctx->window, "key-press-event",
                     G_CALLBACK( on_key_presente_ou_ausente_key_press_event ), ctx );
   g_signal_connect( ctx->ui_diario.btn_salvar_frequencia, "clicked", G_CALLBACK( on_button_salvar_frequencia_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.treeview_frequencia, "cursor-changed",
                     G_CALLBACK( on_treeview_frequencia_cursor_changed ), ctx );


   // AVALIAÇÕES
   ctx->ui_diario.handler_combo_avaliacoes = g_signal_connect( ctx->ui_diario.combo_avaliacoes, "changed",
         G_CALLBACK( on_combo_selecionar_avaliacao_changed ), ctx );
   g_signal_connect( ctx->ui_diario.btn_nova_avaliacao, "clicked", G_CALLBACK( on_button_nova_avaliacao_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.btn_editar_avaliacao, "clicked", G_CALLBACK( on_button_editar_avaliacao_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.btn_popover_nomear, "clicked",
                     G_CALLBACK( on_button_popover_nomear_avaliacao_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.entry_popover_nomear, "activate",
                     G_CALLBACK( on_entry_popover_nomear_avaliacao_activate ), ctx );
   ctx->ui_diario.handler_check_desativar = g_signal_connect( ctx->ui_diario.check_desativar_avaliacao, "toggled",
         G_CALLBACK( on_check_desativar_avaliacao_toggled ), ctx );

   // Aba Avaliações (Colunas 2 a 11 contíguas)
   _configurar_colunas_editaveis( ctx, GTK_TREE_VIEW( ctx->ui_diario.treeview_avaliacoes ), 2, 11 );

   g_signal_connect( ctx->ui_diario.treeview_avaliacoes, "cursor-changed",
                     G_CALLBACK( on_treeview_notas_cursor_changed ), ctx );
   g_signal_connect( ctx->ui_diario.btn_salvar_avaliacoes, "clicked",
                     G_CALLBACK( on_button_salvar_avaliacoes_clicked ), ctx );

   // RELATÓRIO FINAL
   g_signal_connect( ctx->ui_diario.check_pendencias, "toggled", G_CALLBACK( on_check_pendencias_relatorio_toggled ), ctx );
   g_signal_connect( ctx->ui_diario.btn_consolidar, "clicked", G_CALLBACK( on_button_consolidar_relatorio_clicked ), ctx );

   // Aba Relatório Final (Colunas 2, 3 e 4: 1º, 2º e 3º Períodos) (Colunas 8 e 9: Rec Final e Conselho)
   _configurar_colunas_editaveis( ctx, GTK_TREE_VIEW( ctx->ui_diario.treeview_relatorio ), 2, 4 );
   _configurar_colunas_editaveis( ctx, GTK_TREE_VIEW( ctx->ui_diario.treeview_relatorio ), 8, 9 );



   g_signal_connect( ctx->button.frequencia, "enter-notify-event", G_CALLBACK( on_button_frequencia_enter_notify_event ), ctx );
   g_signal_connect( ctx->button.conteudos,  "enter-notify-event", G_CALLBACK( on_button_conteudos_enter_notify_event ), ctx );
   g_signal_connect( ctx->button.avaliacoes, "enter-notify-event", G_CALLBACK( on_button_avaliacoes_enter_notify_event ), ctx );
   g_signal_connect( ctx->button.relatorio_final, "enter-notify-event",
                     G_CALLBACK( on_button_relatorio_enter_notify_event ), ctx );

   g_signal_connect( ctx->ui_diario.check_modo_edicao, "toggled", G_CALLBACK( on_check_modo_edicao_toggled ), ctx );
   g_signal_connect( ctx->notebook, "switch-page", G_CALLBACK( on_notebook_principal_switch_page ), ctx );



   // Operação Crítica: Passa o ponteiro 'opt' para leitura e aplicação estável do cache físico (.dat)
   g_signal_connect( ctx->button.importar_dados, "clicked", G_CALLBACK( on_button_carregar_estado_aplicativo_clicked ), ctx );
   g_signal_connect( ctx->button.gerar_prova,       "clicked", G_CALLBACK( on_button_gerar_prova_clicked ), ctx );
   g_signal_connect( ctx->button.processar_imagens, "clicked", G_CALLBACK( on_button_processar_imagens_clicked ), ctx );
   g_signal_connect( ctx->button.corrigir_prova,    "clicked", G_CALLBACK( on_button_corrigir_prova_clicked ), ctx );

   g_signal_connect( ctx->button.abrir_pdf_acervo,      "clicked", G_CALLBACK( on_button_abrir_tema_clicked ), ctx );
   g_signal_connect( ctx->button.compilar_latex_acervo, "clicked", G_CALLBACK( on_button_compilar_questoes_clicked ), ctx );
   g_signal_connect( ctx->button.executar_gcc_acervo,   "clicked", G_CALLBACK( on_button_atualizar_questoes_clicked ), ctx );


   ctx->handlers.ano = g_signal_connect( ctx->entry.ano, "changed",
                                         G_CALLBACK( on_entry_atualizar_ano_interface_changed ), ctx );

   ctx->handlers.escola = g_signal_connect( ctx->entry.escola, "changed",
                          G_CALLBACK( on_entry_atualizar_escola_interface_changed ), ctx );

   ctx->handlers.turma = g_signal_connect( ctx->entry.turma, "changed",
                                           G_CALLBACK( on_entry_atualizar_turma_interface_changed ), ctx );

   ctx->handlers.disciplina = g_signal_connect( ctx->entry.disciplina, "changed",
                              G_CALLBACK( on_entry_atualizar_disciplina_interface_changed ), ctx );

   ctx->handlers.periodo = g_signal_connect( ctx->entry.periodo, "changed",
                           G_CALLBACK( on_entry_periodo_interface_changed ), ctx );

   ctx->handlers.cor_destaque = g_signal_connect( ctx->entry.cor_destaque, "changed",
                                G_CALLBACK( on_entry_cor_destaque_interface_changed ), ctx );

   ctx->handlers.decoracao_estilo = g_signal_connect( ctx->entry.decoracao_estilo, "changed",
                                    G_CALLBACK( on_entry_decoracao_estilo_interface_changed ), ctx );

   g_signal_connect( ctx->entry.estilo, "changed", G_CALLBACK( on_entry_estilo_interface_changed ), ctx );

   // =========================================================================
   // 6. RASTREAMENTO AUTOMÁTICO DINÂMICO (INPUTS DE TEMAS E QUESTÕES)
   // =========================================================================
   ctx->handlers.tema = g_signal_connect( ctx->entry.tema, "changed",
                                          G_CALLBACK( on_entry_atualizar_tema_changed ), ctx );

   ctx->handlers.tema_espelho = g_signal_connect( ctx->entry.tema_espelho, "changed",
                                G_CALLBACK( on_entry_atualizar_tema_changed ), ctx );



   ctx->provas.handler_scrolled = g_signal_connect( ctx->provas.scrolled_window, "size-allocate",
                                  G_CALLBACK( on_scrolled_rolar_para_o_fim_sizeallocate ), ctx );
   g_signal_handler_block( ctx->provas.scrolled_window, ctx->provas.handler_scrolled );
   ctx->provas.importar = false;


   ctx->ui_diario.handler_combo_alunos = g_signal_connect( G_OBJECT( ctx->ui_diario.combo_alunos ), "changed",
                                         G_CALLBACK( on_combo_alunos_changed ), ctx );


   g_signal_connect( G_OBJECT( ctx->painel.eventbox_painel ), "enter-notify-event",
                     G_CALLBACK( on_painel_feedback_enter_notify_event ), ctx );

   g_signal_connect( G_OBJECT( ctx->painel.eventbox_painel ), "leave-notify-event",
                     G_CALLBACK( on_painel_feedback_leave_notify_event ), ctx );

   g_signal_connect( G_OBJECT( ctx->painel.eventbox_painel ), "button-press-event",
                     G_CALLBACK( on_painel_feedback_button_press_event ), ctx );

   g_signal_connect( G_OBJECT( ctx->painel.eventbox_orelhinha ), "button-press-event",
                     G_CALLBACK( on_orelhinha_button_press_event ), ctx );

   g_signal_connect( ctx->painel.revealer_painel, "notify::child-revealed",
                     G_CALLBACK( on_revealer_child_revealed_notify ), ctx );


}
