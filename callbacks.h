#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <gtk/gtk.h>



// =========================================================================
// 1. NAVEGAÇÃO DE DADOS ESCOLARES (SIAEP / SEDUC)
// =========================================================================
// Controlam o fluxo em cascata para carregar as pastas e preencher os buffers da tela
void on_entry_atualizar_ano_interface_changed( GtkWidget *widget, gpointer user_data );
void on_entry_atualizar_escola_interface_changed( GtkWidget *widget, gpointer user_data );
void on_entry_atualizar_turma_interface_changed( GtkWidget *widget, gpointer user_data );
void on_entry_atualizar_disciplina_interface_changed( GtkWidget *widget, gpointer user_data );
void on_entry_periodo_interface_changed( GtkWidget *widget, gpointer user_data );
void on_entry_cor_destaque_interface_changed( GtkWidget *widget, gpointer user_data );
void on_entry_decoracao_estilo_interface_changed( GtkWidget *widget, gpointer user_data );

gboolean on_entry_data_button_press( GtkWidget *widget, GdkEventButton *event, gpointer user_data );
void on_calendar_navigation( GtkWidget *widget, gpointer user_data );
void on_calendar_day_selected( GtkWidget *widget, gpointer user_data );
gboolean on_entry_validar_data_focus_out( GtkWidget *widget, GdkEventFocus *event, gpointer user_data );
void on_button_stepper_menos_num_horarios_clicked( GtkWidget *widget, gpointer user_data );
void on_button_stepper_mais_num_horarios_clicked( GtkWidget *widget, gpointer user_data );
void on_button_remover_conteudo_por_indice_clicked( GtkWidget *widget, gpointer user_data );
void on_diario_selection_changed( GtkTreeSelection *selection, gpointer user_data );
void on_button_salvar_conteudo_clicked( GtkWidget *widget, gpointer user_data );
void on_entry_salvar_conteudo_activate( GtkWidget *widget, gpointer user_data );
void on_treeview_carregar_registro_para_edicao_row_activated( GtkTreeView *treeview, GtkTreePath *path,
                                                              GtkTreeViewColumn *column, gpointer user_data );

void on_combo_data_frequencia_changed( GtkWidget *widget, gpointer user_data );
void on_button_presente_clicked( GtkWidget *widget, gpointer user_data );
void on_button_ausente_clicked( GtkWidget *widget, gpointer user_data );
void on_button_salvar_frequencia_clicked( GtkWidget *widget, gpointer user_data );

gboolean on_button_frequencia_enter_notify_event( GtkWidget *widget, GdkEventCrossing *event, gpointer user_data );
gboolean on_button_conteudos_enter_notify_event( GtkWidget *widget, GdkEventCrossing *event, gpointer user_data );
gboolean on_button_avaliacoes_enter_notify_event( GtkWidget *widget, GdkEventCrossing *event, gpointer user_data );

// =========================================================================
// 2. EMISSÃO DE RELATÓRIOS E DIÁRIOS DE CLASSE
// =========================================================================
// Gerenciam a leitura de arquivos locais e geração de planilhas/dados de acompanhamento
void on_button_relatorio_de_avalicoes_clicked( GtkWidget *widget, gpointer user_data );
void on_button_relatorio_de_conteudos_clicked( GtkWidget *widget, gpointer user_data );
void on_button_relatorio_de_frequencia_clicked( GtkWidget *widget, gpointer user_data );
void on_button_relatorio_final_clicked( GtkWidget *widget, gpointer user_data );
void on_button_abrir_arquivos_de_dados_clicked( GtkWidget *widget, gpointer user_data );
void on_button_siaep_atualizar_alunos_clicked( GtkWidget *widget, gpointer user_data );

// =========================================================================
// 3. CURADORIA E COMPILAÇÃO DE Acervo
// =========================================================================
// Manipulam o banco de dados pedagógico de temas e tópicos específicos
void on_button_abrir_tema_clicked( GtkWidget *widget, gpointer user_data );
void on_button_compilar_questoes_clicked( GtkWidget *widget, gpointer user_data );
void on_button_atualizar_questoes_clicked( GtkWidget *widget, gpointer user_data );

// =========================================================================
// 4. MOTOR DE PRODUÇÃO (GERAÇÃO, PRODUÇÃO E CORREÇÃO)
// =========================================================================
// Ações finais pesadas que acionam o compilador TeX, processam imagens e geram PDFs
void on_button_carregar_estado_aplicativo_clicked( GtkWidget *widget, gpointer user_data );

void on_button_gerar_prova_clicked( GtkWidget *widget, gpointer user_data );

void on_button_processar_imagens_clicked( GtkWidget *widget, gpointer user_data );
void on_button_corrigir_prova_clicked( GtkWidget *widget, gpointer user_data );

// =========================================================================
// 5. CONFIGURAÇÕES VISUAIS DA PROVA (RADIO BUTTONS - SELEÇÃO EXCLUSIVA)
// =========================================================================
// Embrulho (wrapper) que configura a roupagem e estrutura de saída do LaTeX
void on_radio_atualizar_generic_interface_toggled( GtkWidget *widget, gpointer user_data );
// =========================================================================
// 6. CAIXAS DE SELEÇÃO E CONTROLE DE ESTADO (CHECK BUTTONS)
// =========================================================================
// Chaves booleanas independentes de configuração da interface e do sistema
void on_check_atualizar_booleanos_interface_toggled( GtkWidget *widget, gpointer user_data );
//==========================================================================

// =========================================================================
// 7. ENTRADAS DINÂMICAS DE TEXTO (CAIXAS DE ENTRADA / ENTRIES)
// =========================================================================
void on_entry_atualizar_tema_changed( GtkWidget *widget, gpointer user_data );


// --- EMBRULHOS GTK INTERFACE DINÂMICA ---

void on_subtema_check_toggled( GtkWidget *widget, gpointer user_data );

void on_stepper_mais_clicked( GtkWidget *button, gpointer user_data );

void on_stepper_menos_clicked( GtkWidget *button, gpointer user_data );




void on_scrolled_rolar_para_o_fim_sizeallocate( GtkWidget *widget, GdkRectangle *allocation, gpointer user_data );


void on_combo_alunos_changed( GtkWidget *widget, gpointer user_data );




#endif // CALLBACKS_H
