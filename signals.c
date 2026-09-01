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
 * @brief Define os IDs (nomes) dos widgets de opções para uso no CSS ou nos callbacks.
 */
void configurar_nomes_dos_widgets( AppContext *ctx ) {

   gtk_widget_set_name( ctx->entry.tema, "tema" );

   gtk_widget_set_name( ctx->check.validar_ciclos, "check_validar_ciclos_1" );
   gtk_widget_set_name( ctx->check.nao_presencial, "nao_presencial_2" );
   gtk_widget_set_name( ctx->check.expor_dados, "expor_dados_3" );

   char id_string[64];

   for ( int i = 0; i < 2; i++ ) {
      // Mapeamento de Colunas (Categoria 1: ID 12 = 2 colunas, ID 13 = 3 colunas)
      snprintf( id_string, sizeof( id_string ), "qtd_colunas_%.2d", i + 12 );
      gtk_widget_set_name( ctx->radio.qtd_colunas[i], id_string );

      // Mapeamento de Separadores de Linha (Categoria 2: ID 21 = Ativo, ID 22 = Inativo)
      snprintf( id_string, sizeof( id_string ), "separadores_%.2d", i + 21 );
      gtk_widget_set_name( ctx->radio.separadores[i], id_string );

      // Mapeamento de Fontes TeX (Categoria 3: ID 31 = Padrão, ID 32 = Alternativa)
      snprintf( id_string, sizeof( id_string ), "fonte_latex_%.2d", i + 31 );
      gtk_widget_set_name( ctx->radio.fonte_latex[i], id_string );

      // Mapeamento do Limite de Páginas (Categoria 4: ID 41 = 1 página, ID 42 = 2 páginas)
      snprintf( id_string, sizeof( id_string ), "qtd_paginas_%.2d", i + 41 );
      gtk_widget_set_name( ctx->radio.qtd_paginas[i], id_string );

      // Mapeamento de Estilo do Cabeçalho TikZ (Categoria 5: ID 51 = Padrão, ID 52 = Reduzido)
      snprintf( id_string, sizeof( id_string ), "cabecalho_tipo_%.2d", i + 51 );
      gtk_widget_set_name( ctx->radio.cabecalho_tipo[i], id_string );
   }

   for ( int i = 0; i < 3; i++ ) {
      // Mapeamento do Bloco de Avaliações (Categoria 6: IDs 61, 62 e 63 para Provas 1, 2 e 3)
      snprintf( id_string, sizeof( id_string ), "sequencia_prova_%.2d", i + 61 );
      gtk_widget_set_name( ctx->radio.avaliacao[i], id_string );
      snprintf( id_string, sizeof( id_string ), "interface_style_%.2d", i + 70 );
      gtk_widget_set_name( ctx->radio.interface_style[i], id_string );
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
      g_signal_connect( ctx->radio.interface_style[i], "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
   }

   // =========================================================================
   // 4. CONEXÃO DE SINAIS: CICLO PEDAGÓGICO E PRODUÇÃO DE DOCUMENTOS
   // =========================================================================
   g_signal_connect( ctx->button.frequencia,        "clicked", G_CALLBACK( on_button_relatorio_de_frequencia_clicked ), ctx );
   g_signal_connect( ctx->button.conteudos,         "clicked", G_CALLBACK( on_button_relatorio_de_conteudos_clicked ), ctx );
   g_signal_connect( ctx->button.avaliacoes,        "clicked", G_CALLBACK( on_button_relatorio_de_avalicoes_clicked ), ctx );
   g_signal_connect( ctx->button.abrir,             "clicked", G_CALLBACK( on_button_abrir_arquivos_de_dados_clicked ), ctx );
   g_signal_connect( ctx->button.relatorio_final,   "clicked", G_CALLBACK( on_button_relatorio_final_clicked ), ctx );
   g_signal_connect( ctx->button.atualizar_alunos,  "clicked", G_CALLBACK( on_button_siaep_atualizar_alunos_clicked ), ctx );

   g_signal_connect( ctx->ui_diario.calendario_data, "next-month", G_CALLBACK( on_calendar_navigation ), ctx );
   g_signal_connect( ctx->ui_diario.calendario_data, "prev-month", G_CALLBACK( on_calendar_navigation ), ctx );
   g_signal_connect( ctx->ui_diario.calendario_data, "next-year", G_CALLBACK( on_calendar_navigation ), ctx );
   g_signal_connect( ctx->ui_diario.calendario_data, "prev-year", G_CALLBACK( on_calendar_navigation ), ctx );

   g_signal_connect( ctx->ui_diario.calendario_data, "day-selected", G_CALLBACK( on_calendar_day_selected ), ctx );
   g_signal_connect( ctx->ui_diario.entry_data, "button-press-event", G_CALLBACK( on_entry_data_button_press ), ctx );
   g_signal_connect( ctx->ui_diario.entry_data, "focus-out-event", G_CALLBACK( on_entry_validar_data_focus_out ), ctx );

   g_signal_connect( ctx->ui_diario.stepper_menos, "clicked", G_CALLBACK(on_button_stepper_menos_num_horarios_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.stepper_mais,  "clicked", G_CALLBACK(on_button_stepper_mais_num_horarios_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.remover_registro, "clicked", G_CALLBACK(on_button_remover_conteudo_por_indice_clicked ), ctx );
   // Obtém o controle de seleção do TreeView
   GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( ctx->ui_diario.treeview_conteudo ) );
   g_signal_connect( selection, "changed", G_CALLBACK( on_diario_selection_changed ), ctx );

   g_signal_connect( ctx->ui_diario.descricao, "activate", G_CALLBACK( on_entry_salvar_conteudo_activate ), ctx );
   g_signal_connect( ctx->ui_diario.salvar_conteudo, "clicked", G_CALLBACK( on_button_salvar_conteudo_clicked ), ctx );



   ctx->ui_diario.handler_combo_data = g_signal_connect( ctx->ui_diario.combo_data, "changed",
                                                         G_CALLBACK( on_combo_data_frequencia_changed ), ctx );
   g_signal_connect( ctx->ui_diario.presente, "clicked", G_CALLBACK( on_button_presente_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.ausente, "clicked", G_CALLBACK( on_button_ausente_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.salvar_frequencia, "clicked", G_CALLBACK( on_button_salvar_frequencia_clicked ), ctx );



   g_signal_connect( ctx->button.frequencia, "enter-notify-event", G_CALLBACK(on_button_frequencia_enter_notify_event), ctx );
   g_signal_connect( ctx->button.conteudos,  "enter-notify-event", G_CALLBACK( on_button_conteudos_enter_notify_event ), ctx );
   g_signal_connect( ctx->button.avaliacoes, "enter-notify-event", G_CALLBACK(on_button_avaliacoes_enter_notify_event ), ctx );

   g_signal_connect( ctx->ui_diario.treeview_conteudo, "row-activated",
                     G_CALLBACK( on_treeview_carregar_registro_para_edicao_row_activated ), ctx );



   // Operação Crítica: Passa o ponteiro 'opt' para leitura e aplicação estável do cache físico (.dat)
   g_signal_connect( ctx->button.carregar_dados,    "clicked", G_CALLBACK( on_button_carregar_estado_aplicativo_clicked ), ctx );
   g_signal_connect( ctx->button.gerar_prova,       "clicked", G_CALLBACK( on_button_gerar_prova_clicked ), ctx );
   g_signal_connect( ctx->button.processamento_img, "clicked", G_CALLBACK( on_button_processar_imagens_clicked ), ctx );
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



   ctx->handlers.alunos = g_signal_connect( G_OBJECT( ctx->entry.alunos ), "changed",
                          G_CALLBACK( on_combo_alunos_changed ), ctx );


}
