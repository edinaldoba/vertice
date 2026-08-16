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

// =========================================================================
// 2. EMISSÃO DE RELATÓRIOS E DIÁRIOS DE CLASSE
// =========================================================================
// Gerenciam a leitura de arquivos locais e geração de planilhas/dados de acompanhamento
void on_botao_relatorio_de_avalicoes_clicked( GtkWidget *widget, gpointer user_data );
void on_botao_relatorio_de_conteudos_clicked( GtkWidget *widget, gpointer user_data );
void on_botao_relatorio_de_frequencia_clicked( GtkWidget *widget, gpointer user_data );
void on_botao_relatorio_final_clicked( GtkWidget *widget, gpointer user_data );
void on_botao_abrir_arquivos_de_dados_clicked( GtkWidget *widget, gpointer user_data );
void on_botao_siaep_atualizar_alunos_clicked( GtkWidget *widget, gpointer user_data );

// =========================================================================
// 3. CURADORIA E COMPILAÇÃO DE Acervo
// =========================================================================
// Manipulam o banco de dados pedagógico de temas e tópicos específicos
void on_botao_abrir_tema_clicked( GtkWidget *widget, gpointer user_data );
void on_botao_compilar_questoes_clicked( GtkWidget *widget, gpointer user_data );
void on_botao_atualizar_questoes_clicked( GtkWidget *widget, gpointer user_data );

// =========================================================================
// 4. MOTOR DE PRODUÇÃO (GERAÇÃO, PRODUÇÃO E CORREÇÃO)
// =========================================================================
// Ações finais pesadas que acionam o compilador TeX, processam imagens e geram PDFs
void on_botao_carregar_estado_aplicativo_clicked( GtkWidget *widget, gpointer user_data );

void on_botao_gerar_prova_clicked( GtkWidget *widget, gpointer user_data );

void on_botao_processar_imagens_clicked( GtkWidget *widget, gpointer user_data );
void on_botao_corrigir_prova_clicked( GtkWidget *widget, gpointer user_data );

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
