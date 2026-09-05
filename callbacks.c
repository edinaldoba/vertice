/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <gdk/gdkkeysyms.h> // Certifique-se de incluir para ter acesso às chaves GDK_KEY_*

#include "signals.h"
#include "callbacks.h"
#include "comum.h"
#include "imagens.h"
#include "interface.h"
#include "relatorios.h"
#include "provas.h"
#include "basicas.h"
#include "acervo.h"
#include "gabaritos.h"
#include "mensagens.h"
#include "dinamica.h"
#include "assincrono.h"
#include "seduc-ma.h"





void on_entry_atualizar_ano_interface_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   ctx->cascata.foco.ano = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.ano ) );
   if ( ctx->cascata.foco.ano < 0 || ctx->listas.anos == NULL ) return;

   gchar *ano_selecionado = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.ano ) );
   if ( !ano_selecionado ) return;

   gboolean clique_real = ( widget != NULL );
   gboolean ano_mudou = atualizar_ano_interface( ctx, ano_selecionado, !clique_real );
   g_free( ano_selecionado );

   if ( !ano_mudou ) return;

   popular_combo_box_text( ctx->entry.escola, ctx->listas.escolas, 0, ctx->cascata.limite.escolas, ctx->handlers.escola );
   on_entry_atualizar_escola_interface_changed( NULL, ctx );
}




void on_entry_atualizar_escola_interface_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   ctx->cascata.foco.escola = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.escola ) );
   if ( ctx->cascata.foco.escola < 0 || ctx->listas.escolas == NULL ) return;

   gchar *escola_selecionada = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.escola ) );
   if ( !escola_selecionada ) return;

   gboolean clique_real = ( widget != NULL );
   gboolean escola_mudou = atualizar_escola_interface( ctx, escola_selecionada, !clique_real );
   g_free( escola_selecionada );

   if ( !escola_mudou ) return;

   popular_combo_box_text( ctx->entry.turma, ctx->listas.turmas, 0, ctx->cascata.limite.turmas, ctx->handlers.turma );
   on_entry_atualizar_turma_interface_changed( NULL, ctx ); // Ou o novo nome da callback de turma!
}




void on_entry_atualizar_turma_interface_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   ctx->cascata.foco.turma = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.turma ) );
   if ( ctx->cascata.foco.turma < 0 || ctx->listas.turmas == NULL ) return;

   gchar *turma_selecionada = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.turma ) );
   if ( !turma_selecionada ) return;

   // 3. Passa a responsabilidade para a função embrulhada
   gboolean clique_real = ( widget != NULL );
   gboolean turma_mudou = atualizar_turma_interface( ctx, turma_selecionada, !clique_real );
   g_free( turma_selecionada );

   if ( !turma_mudou ) return;

   popular_combo_box_text( ctx->entry.disciplina, ctx->listas.disciplinas, 0,
                           ctx->cascata.limite.disciplinas, ctx->handlers.disciplina );
   on_entry_atualizar_disciplina_interface_changed( NULL, ctx );
}




void on_entry_atualizar_disciplina_interface_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   ctx->cascata.foco.disciplina = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.disciplina ) );
   if ( ctx->cascata.foco.disciplina < 0 || ctx->listas.disciplinas == NULL ) return;

   gchar *disciplina_selecionada = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.disciplina ) );
   if ( !disciplina_selecionada ) return;

   gboolean clique_real = ( widget != NULL );
   atualizar_disciplina_interface( ctx, disciplina_selecionada, !clique_real );
   g_free( disciplina_selecionada );
}




void on_entry_periodo_interface_changed( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_COMBO_BOX( widget ) );
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;

   ctx->cascata.foco.periodo = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.periodo ) );
   if ( ctx->cascata.foco.periodo < 0 || ctx->listas.periodos == NULL ) return;

   gchar *periodo_selecionado = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.periodo ) );
   if ( !periodo_selecionado ) return;

   atualizar_periodo_interface( ctx, periodo_selecionado );
   g_free( periodo_selecionado ); // Limpa a memória
}




void on_entry_cor_destaque_interface_changed( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_COMBO_BOX( widget ) );
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   ctx->cascata.foco.cor_destaque = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.cor_destaque ) );
   if ( ctx->cascata.foco.cor_destaque < 0 || ctx->listas.cores_destaque == NULL ) return;

   gchar *cor_destaque_selecionada = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.cor_destaque ) );
   if ( !cor_destaque_selecionada ) return;

   snprintf( ctx->dados.cor_destaque, sizeof( ctx->dados.cor_destaque ), "%s", cor_destaque_selecionada );
   g_free( cor_destaque_selecionada ); // Limpa a memória

}


void on_entry_decoracao_estilo_interface_changed( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_COMBO_BOX( widget ) );
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   ctx->cascata.foco.decoracao_estilo = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.decoracao_estilo ) );
   if ( ctx->cascata.foco.decoracao_estilo < 0 || ctx->listas.decoracoes_estilo == NULL ) return;

   gchar *decoracao_estilo_selecionado = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.decoracao_estilo ) );
   if ( !decoracao_estilo_selecionado ) return;

   snprintf( ctx->dados.decoracao_estilo, sizeof( ctx->dados.decoracao_estilo ), "%s", decoracao_estilo_selecionado );
   g_free( decoracao_estilo_selecionado ); // Limpa a memória
}








// ============================================================================
// CALLBACKS DO GTK (Focadas exclusivamente na Interface Visual)
// ============================================================================

gboolean on_entry_data_button_press( GtkWidget *widget, GdkEventButton *event, gpointer user_data ) {
   g_return_val_if_fail( GTK_IS_ENTRY( widget ), FALSE );
   ( void )event;
   AppContext *ctx = ( AppContext * )user_data;

   g_return_val_if_fail( widget && ctx && ctx->ui_diario.popover_calendario, FALSE );

   GtkPopover *popover = GTK_POPOVER( ctx->ui_diario.popover_calendario );

   gtk_popover_set_modal( popover, FALSE );

   gtk_widget_set_can_focus( ctx->ui_diario.calendario_data, FALSE );

   // Ancora na caixa de texto e exibe
   gtk_popover_set_relative_to( popover, widget );
   gtk_widget_show_all( GTK_WIDGET( popover ) );
   gtk_popover_popup( popover );

   return FALSE;
}


void on_calendar_navigation( GtkWidget *widget, gpointer user_data ) {
   ( void )user_data;
   g_return_if_fail( GTK_IS_CALENDAR( widget ) );

   // Avisa para o calendário: "A próxima seleção de dia é falsa, ignore!"
   g_object_set_data( G_OBJECT( widget ), "ignorar_dia", GINT_TO_POINTER( 1 ) );
}

void on_calendar_day_selected( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_if_fail( GTK_IS_CALENDAR( widget ) && ctx );

   // 1. O PULO DO GATO: Tem uma flag pedindo para ignorar?
   if ( GPOINTER_TO_INT( g_object_get_data( G_OBJECT( widget ), "ignorar_dia" ) ) == 1 ) {
      // Apaga a flag (reseta para 0) e aborta silenciosamente. O popover continua aberto!
      g_object_set_data( G_OBJECT( widget ), "ignorar_dia", GINT_TO_POINTER( 0 ) );
      return;
   }

   // 2. SUCESSO! Não tinha flag, então o professor realmente clicou com o mouse no dia!
   guint ano, mes, dia;
   gtk_calendar_get_date( GTK_CALENDAR( widget ), &ano, &mes, &dia );

   g_autofree char *data_formatada = g_strdup_printf( "%02u/%02u/%04u", dia, mes + 1, ano );
   gtk_entry_set_text( GTK_ENTRY( ctx->ui_diario.entry_data ), data_formatada );

   gtk_popover_popdown( GTK_POPOVER( ctx->ui_diario.popover_calendario ) );
}


gboolean on_entry_validar_data_focus_out( GtkWidget *widget, GdkEventFocus *event, gpointer user_data ) {
   ( void )event;
   AppContext *ctx = ( AppContext * )user_data;
   g_return_val_if_fail( GTK_IS_ENTRY( widget ) && ctx, FALSE );

   // 1. A PRIMEIRA AÇÃO DA PERDA DE FOCO: Esconder o popover
   gtk_popover_popdown( GTK_POPOVER( ctx->ui_diario.popover_calendario ) );

   GtkEntry *entry = GTK_ENTRY( widget );
   const gchar *texto_digitado = gtk_entry_get_text( entry );

   // 2. Delega todo o trabalho pesado e matemático para a função modular
   g_autofree gchar *data_final = validar_data( texto_digitado );

   // 3. Atualiza a interface apenas se for estritamente necessário
   if ( g_strcmp0( texto_digitado, data_final ) != 0 ) {
      gtk_entry_set_text( entry, data_final );
   }

   return FALSE;
}


void on_button_stepper_menos_num_horarios_clicked( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_if_fail( GTK_IS_BUTTON( widget ) && ctx );

   GtkLabel *label_qtd = GTK_LABEL( ctx->ui_diario.qtd_aulas );
   const char *str_qtd_aulas = gtk_label_get_text( label_qtd );

   int qtd_aulas = 1;
   if ( str_qtd_aulas ) sscanf( str_qtd_aulas, "%d", &qtd_aulas );

   if ( qtd_aulas <= 1 ) return;

   g_autofree char *label_novo = g_strdup_printf( "%d h", --qtd_aulas );
   gtk_label_set_text( label_qtd, label_novo );
}


void on_button_stepper_mais_num_horarios_clicked( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_if_fail( GTK_IS_BUTTON( widget ) && ctx );

   GtkLabel *label_qtd = GTK_LABEL( ctx->ui_diario.qtd_aulas );
   const char *str_qtd_aulas = gtk_label_get_text( label_qtd );

   int qtd_aulas = 1;
   if ( str_qtd_aulas ) sscanf( str_qtd_aulas, "%d", &qtd_aulas );

   // Carga horária igual a 3h é o máximo de aulas admitidas por registro
   if ( qtd_aulas >= 3 ) {
      ctx->painel.format_titulo    = meu_gerador_variadico( "⚠ Carga Horária Excedida" );
      ctx->painel.format_subtitulo = meu_gerador_variadico( "O limite máximo permitido por registro é de %d horas / aula.", 3 );
      ctx->painel.format_instrucao = meu_gerador_variadico( "%s", "Para cargas horárias diárias maiores, divida o conteúdo e faça múltiplos registros." );
      criar_mensagem_painel( AVISO, &ctx->painel );

      return; // Trava o incremento após emitir o aviso
   }

   g_autofree char *label_novo = g_strdup_printf( "%d h", ++qtd_aulas );
   gtk_label_set_text( label_qtd, label_novo );
}




static RegistroDiario *_indexar_array_diario( AppContext *ctx, int foco ) {
   g_return_val_if_fail( ctx, NULL );

   RegistroDiario *diario = NULL;

   if ( ctx->diarios && foco >= 0 && ( guint )foco < ctx->diarios->len ) {
      diario = &g_array_index( ctx->diarios, RegistroDiario, foco );
   }

   return diario;
}



static gboolean _treeview_remover_registro_diario_selecionado( AppContext *ctx, GtkTreeSelection *selection ) {
   GtkTreeModel *model;
   GtkTreeIter iter;

   // Se houver uma linha selecionada, o GTK preenche 'model' e 'iter' aqui mesmo
   if ( gtk_tree_selection_get_selected( selection, &model, &iter ) ) {
      g_autoptr( GtkTreePath ) path_remocao = gtk_tree_model_get_path( model, &iter );

      if ( path_remocao ) {
         int indice_remocao = gtk_tree_path_get_indices( path_remocao )[0];

         // Delega a execução atômica para a função embrulhada
         remover_registro_diario_selecionado( ctx, indice_remocao, model, &iter );

         // SINCRONIZAÇÃO: Repopula o combo da frequência (agora com um item a menos)
         popular_datas( ctx );

         int foco = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_data ) );
         ctx->diario = _indexar_array_diario( ctx, foco );

         renderizar_frequencia_por_data( ctx );

         // Retorna TRUE para indicar que o evento foi tratado/consumido
         return TRUE;
      }
   }
   return FALSE;
}

void on_button_remover_registro_diario_selecionado_clicked( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_if_fail( GTK_IS_BUTTON( widget ) && ctx && ctx->diarios );

   GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( ctx->ui_diario.treeview_conteudo ) );

   // Chamada limpa e direta
   _treeview_remover_registro_diario_selecionado( ctx, selection );
}

gboolean on_treeview_remover_registro_diario_selecionado_key_press_event( GtkWidget *widget, GdkEventKey *event,
                                                                          gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_val_if_fail( ctx && ctx->diarios, FALSE );

   // Verifica se a tecla pressionada foi o DELETE
   if ( event->keyval == GDK_KEY_Delete ) {
      GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( widget ) );

      // O retorno TRUE ou FALSE agora flui naturalmente
      return _treeview_remover_registro_diario_selecionado( ctx, selection );
   }

   // Retorna FALSE para permitir que outras teclas (como setas para cima/baixo) funcionem normalmente
   return FALSE;
}



void on_diario_selection_changed( GtkTreeSelection *selection, gpointer user_data ) {
   AppContext *ctx = (AppContext *)user_data;

   GtkTreeModel *model;
   GtkTreeIter iter;

   // 1. Verifica se existe linha selecionada
   gboolean tem_selecao = gtk_tree_selection_get_selected( selection, &model, &iter );

   // 2. Garante a classe CSS do botão
   GtkStyleContext *context = gtk_widget_get_style_context( GTK_WIDGET( ctx->ui_diario.remover_registro ) );
   if ( tem_selecao ) {
      if ( !gtk_style_context_has_class( context, "botao-remover-ifood" ) ) {
         gtk_style_context_add_class( context, "botao-remover-ifood" );
      }
   } else {
      gtk_style_context_remove_class( context, "botao-remover-ifood" );
   }

}


static void _treeview_adicionar_ou_modificar_registro_diario( AppContext *ctx ) {
   if ( ctx->ui_diario.editando ) {
      modificar_registro_aula( ctx );
   } else {
      registrar_aula( ctx );
   }
   popular_datas( ctx );

   int foco = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_data ) );
   ctx->diario = _indexar_array_diario( ctx, foco );

   renderizar_frequencia_por_data( ctx );
}

void on_button_registrar_aula_clicked( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_if_fail( GTK_IS_BUTTON( widget ) && ctx );

   _treeview_adicionar_ou_modificar_registro_diario( ctx );
}

void on_entry_registrar_aula_activate( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_if_fail( GTK_IS_ENTRY( widget ) && ctx );

   _treeview_adicionar_ou_modificar_registro_diario( ctx );
}


void on_treeview_carregar_registro_para_edicao_row_activated( GtkTreeView *treeview, GtkTreePath *path,
                                                              GtkTreeViewColumn *column, gpointer user_data ) {
   ( void )column;
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   GtkTreeModel *model = gtk_tree_view_get_model( treeview );
   GtkTreeIter iter;

   if ( gtk_tree_model_get_iter( model, &iter, path ) ) {
      // Passa o contexto completo para termos acesso a ctx->chamada e ctx->caminho
      carregar_registro_para_edicao( ctx, &iter );

      // Apenas gerencia a usabilidade visual
      gtk_widget_grab_focus( ctx->ui_diario.descricao );
   }
}





void on_combo_data_frequencia_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   GtkTreeIter iter;

   GtkComboBox *combo = GTK_COMBO_BOX( widget );

   if ( gtk_combo_box_get_active_iter( combo, &iter ) ) {
      GtkTreeModel *model = gtk_combo_box_get_model( combo );
      guint qtd_aulas = 0;

      // Lê o valor da coluna 1 (Configurada como guint no Glade)
      gtk_tree_model_get( model, &iter, 1, &qtd_aulas, -1 );

      // Formata a string conforme o design e insere no Label
      g_autofree gchar *str_qtd_aulas = meu_gerador_variadico( "<b>%u h</b>", qtd_aulas );
      gtk_label_set_markup( GTK_LABEL( ctx->ui_diario.label_ch ), str_qtd_aulas );

      int foco = gtk_combo_box_get_active( combo );
      ctx->diario = _indexar_array_diario( ctx, foco );

      renderizar_frequencia_por_data( ctx );
   }
}




gboolean on_key_presente_ou_ausente_key_press_event( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;

   // 1. Validação inicial reforçada
   g_return_val_if_fail( GTK_IS_WIDGET( widget ) && event && ctx, FALSE );

   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;
   g_return_val_if_fail( ui_diario->stack_pages, FALSE );

   // 2. TRAVA DE NAVEGAÇÃO CORRIGIDA: Exige estar estritamente no Aba de Relatórios (0) E na Frequência
   gint pagina_atual = gtk_notebook_get_current_page( GTK_NOTEBOOK( ctx->notebook ) );
   const gchar *aba_ativa = gtk_stack_get_visible_child_name( GTK_STACK( ui_diario->stack_pages ) );

   // Se NÃO estiver na aba 0 OU NÃO estiver no stack frequencia, aborta.
   if ( pagina_atual != 0 || g_strcmp0( aba_ativa, "page_frequencia" ) != 0 ) {
      return FALSE;
   }

   // 3. CAPTURA E SIMULAÇÃO DOS CLIQUES VIA TECLADO
   switch ( gdk_keyval_to_lower( event->keyval ) ) {
      case GDK_KEY_p:
         gtk_button_clicked( GTK_BUTTON( ui_diario->btn_presente ) );
         return TRUE;

      case GDK_KEY_f:
         gtk_button_clicked( GTK_BUTTON( ui_diario->btn_ausente ) );
         return TRUE;

      // Deixei o esqueleto pronto para a Justificada caso queira usar a tecla "J" no futuro
      case GDK_KEY_j:
         gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_status ), 3 );
         gtk_button_clicked( GTK_BUTTON( ui_diario->btn_salvar_frequencia ) );
         return TRUE;

      default:
         return FALSE; // Permite que a digitação de outras teclas flua normalmente para o GTK
   }
}

void on_button_presente_clicked( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_if_fail( GTK_IS_BUTTON( widget ) && ctx );

   // Passando o endereço do painel que está dentro do AppContext
   registrar_status_assiduidade_frequencia( &ctx->painel, ctx, PRESENTE );
}

void on_button_ausente_clicked( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_if_fail( GTK_IS_BUTTON( widget ) && ctx );

   registrar_status_assiduidade_frequencia( &ctx->painel, ctx, AUSENTE );
}

void on_button_salvar_frequencia_clicked( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_if_fail( GTK_IS_BUTTON( widget ) && ctx );

   StatusAssiduidade status = (StatusAssiduidade) gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_status ) );
   registrar_status_assiduidade_frequencia( &ctx->painel, ctx, status );
}





void on_check_por_aluno_toggled( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;

   GtkToggleButton *togglebutton = GTK_TOGGLE_BUTTON( widget );
   gboolean modo_por_aluno = gtk_toggle_button_get_active( togglebutton );

   // A coluna 2 é a "NASCIMENTO" / "DATA AULA"
   GtkTreeViewColumn *coluna = gtk_tree_view_get_column( GTK_TREE_VIEW( ctx->ui_diario.treeview_frequencia ), 2 );

   if ( modo_por_aluno ) {
      gtk_tree_view_column_set_title( coluna, "DATA  AULA" );
      renderizar_frequencia_por_aluno( ctx );
   } else {
      gtk_tree_view_column_set_title( coluna, "NASCIMENTO" );
      renderizar_frequencia_por_data( ctx );
   }
}




void on_treeview_frequencia_cursor_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;

   // Validações de segurança iniciais
   g_return_if_fail( ctx != NULL && ctx->diarios != NULL );
   g_return_if_fail( GTK_IS_TREE_VIEW( widget ) );

   GtkTreeView *treeview = GTK_TREE_VIEW( widget );

   // 1. Obtém o caminho da linha atualmente focada
   g_autoptr( GtkTreePath ) path = NULL;
   gtk_tree_view_get_cursor( treeview, &path, NULL );
   if ( !path ) return;

   // 2. Extrai o índice numérico da linha visível na tela
   int *indices = gtk_tree_path_get_indices( path );
   if ( !indices ) return;

   int indice_linha = indices[0];
   gboolean modo_por_aluno = FALSE;

   if ( ctx->ui_diario.check_por_aluno ) {
      modo_por_aluno = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( ctx->ui_diario.check_por_aluno ) );
   }

   // 3. Roteamento de navegação baseado no modo ativo
   if ( modo_por_aluno ) {
      // MODO POR ALUNO: Mapeia a linha visível filtrada para o índice real do GArray
      treeview_frequencia_navegar_modo_por_aluno( ctx, indice_linha );

   } else {
      // MODO NORMAL: A linha na TreeView representa um Aluno diretamente
      treeview_frequencia_navegar_modo_normal( ctx, treeview, indice_linha );
   }
}




//------------------------------------------------------------------------------------------------------------------
static gboolean _ui_diario_mudar_aba( GtkWidget *widget, const char *nome_da_pagina ) {
   g_return_val_if_fail( GTK_IS_STACK( widget ) && nome_da_pagina, FALSE );
   const gchar *pagina_atual = gtk_stack_get_visible_child_name( GTK_STACK( widget ) );
   if ( g_strcmp0( pagina_atual, nome_da_pagina ) == 0 ) {
      return FALSE;
   } else {
      gtk_stack_set_visible_child_name( GTK_STACK( widget ), nome_da_pagina );
      return TRUE;
   }
}
//------------------------------------------------------------------------------------------------------------------
gboolean on_button_frequencia_enter_notify_event( GtkWidget *widget, GdkEventCrossing *event, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_val_if_fail( widget && event && ctx, FALSE );

   // 1. Bloqueia a navegação se houver uma edição de conteúdo aberta
   if ( ctx->ui_diario.editando ) {
      ctx->painel.format_titulo    = meu_gerador_variadico( "⚠ Edição em Andamento" );
      ctx->painel.format_subtitulo = meu_gerador_variadico( "Registro de aula não salvo" );
      ctx->painel.format_instrucao = meu_gerador_variadico( "Conclua as modificações e salve o conteúdo atual antes de alternar para a frequência." );
      criar_mensagem_painel( AVISO, &ctx->painel );

      return FALSE;
   }

   // 2. Permite a transição de aba se o estado estiver limpo
   if ( _ui_diario_mudar_aba( ctx->ui_diario.stack_pages, "page_frequencia" ) ) {
      // No futuro alguma coisa deverá ser posta aqui
   }

   return FALSE;
}
//------------------------------------------------------------------------------------------------------------------
gboolean on_button_conteudos_enter_notify_event( GtkWidget *widget, GdkEventCrossing *event, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_val_if_fail( widget && event &&  ctx, FALSE );

   if ( _ui_diario_mudar_aba( ctx->ui_diario.stack_pages, "page_conteudo" ) ) {
      gtk_widget_grab_focus( ctx->ui_diario.tipo_registro );
   }

   return FALSE;
}
//------------------------------------------------------------------------------------------------------------------
gboolean on_button_avaliacoes_enter_notify_event( GtkWidget *widget, GdkEventCrossing *event, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_val_if_fail( widget && event &&  ctx, FALSE );

   if ( _ui_diario_mudar_aba( ctx->ui_diario.stack_pages, "page_avaliacoes" ) ) {
      // No futuro alguma coisa deverá ser posta aqui
   }

   return FALSE;
}
//------------------------------------------------------------------------------------------------------------------





void on_button_relatorio_de_avalicoes_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   relatorio_de_avaliacoes( &ctx->painel, ctx );
}

void on_button_relatorio_de_conteudos_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   relatorio_de_conteudos( &ctx->painel, ctx );
}

void on_button_relatorio_de_frequencia_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   relatorio_de_frequencia( &ctx->painel, ctx );
}

void on_button_relatorio_final_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   relatorio_final( &ctx->painel, ctx );
}

void on_button_abrir_arquivos_de_dados_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext* ) user_data;
   if ( !ctx ) return;
   abrir_arquivos_de_dados( &ctx->painel, ctx );
}

void on_button_siaep_atualizar_alunos_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext* ) user_data;
   if ( !ctx ) return;

   siaep_atualizar_alunos( &ctx->painel, ctx );

   atualizar_dados_e_alunos_ativos( ctx ); // Adicionei esta linhas em 3 de setembro de 2026
}






void on_button_abrir_tema_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   abrir_tema( widget, &ctx->painel, ctx );
}


void on_button_compilar_questoes_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   compilar_questoes( widget, &ctx->painel, ctx );
}


void on_button_atualizar_questoes_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   atualizar_questoes( &ctx->painel, ctx );
}






void on_button_carregar_estado_aplicativo_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );

   // Recupera o ponteiro da estrutura AppContext enviada pelo sinal
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) {
      fprintf( stderr, "ERRO: Ponteiro AppContext inválido no callback de cache.\n" );
      return;
   }

   // Carrega o arquivo de forma indireta, alimentando a tela que alimenta o ctx->dados automaticamente
   if ( carregar_estado_aplicativo( ctx ) ) {
      g_print( "Restauração magnética: Interface alimentada e sincronizada via sinais nativos!\n" );
   } else {
      g_print( "Nenhum cache encontrado. Pronto para nova entrada.\n" );
   }
}






void on_button_gerar_prova_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx || !widget ) return;

   // 1. 🛡️ VALIDAÇÃO DE ENTRADA (Alunos vazios ou Meta de 10 questões)
   if ( verificar_dados_da_interface( &ctx->painel, &ctx->dados ) ) {
      return;
   }

   // 2. 🛡️ GERENCIAMENTO HISTÓRICO DOS GABARITOS (Encapsulado!)
   // Passamos o widget para que a função interna consiga descobrir a janela pai se precisar de pop-up
   gerenciar_fluxo_gabaritos( widget, &ctx->painel, ctx );

   // 3. 🚀 DISPARO ASSÍNCRONO
   gtk_widget_set_sensitive( widget, FALSE );
   disparar_geracao_prova_assincrona( widget, ctx, thread_gerar_prova_background );
}





void on_button_processar_imagens_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx || !widget ) return;

   // GtkWindow *janela_principal = GTK_WINDOW( gtk_widget_get_toplevel( widget ) );
   //
   // // Refatoração focada na clareza e prevenção de erros humanos
   // g_autofree gchar *mensagem = meu_gerador_variadico(
   //    "Você está prestes a iniciar a leitura óptica das provas.\n\n"
   //    "<b>ESCOLA:</b> %s\n"
   //    "<b>ANO LETIVO:</b> %s\n\n"
   //    "⚠️ <b>Atenção:</b> Como o cartão-resposta não identifica a escola automaticamente, "
   //    "todos os acertos deste lote de imagens serão vinculados ao destino acima.\n\n"
   //    "Confirma que as imagens na pasta pertencem a esta escola e ano?",
   //    ctx->dados.escola, ctx->dados.ano );
   //
   // // Sugestão: Mudar o título do pop-up para algo que exija mais atenção
   // gboolean continuar = mostrar_popup_confirmacao( janela_principal, "Confirmação de Vínculo das Provas", mensagem );

   // if ( continuar ) {
   disparar_processamento_imagens_assincrono( widget, ctx, thread_processar_imagens_background );
   // }
}




void on_button_corrigir_prova_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   corrigir_prova( &ctx->painel, ctx );
}




/* * NOTA DE ESTUDO ACERCA DE ABORDAGENS ALTERNATIVAS:
 * Atualmente, esta callback utiliza um "ID Composto" numérico (ex: "radio_opcao_42"),
 * onde a dezena (4) representa a categoria e a unidade (2) representa o valor.
 * * POSSIBILIDADE DE ABORDAGEM COMPLEMENTAR (strcmp + ID simples):
 * Uma alternativa para aumentar a legibilidade textual do código seria nomear os widgets
 * com base no seu grupo semântico seguido de um ID simples (ex: "radio_fonte_1", "radio_fonte_2").
 * O fluxo de captura seria dividido em duas etapas:
 * 1. Identificação do Grupo via String:
 * if (strstr(nome_do_radio_toggle, "fonte")) categoria = CAT_FONTE;
 * else if (strstr(nome_do_radio_toggle, "colunas")) categoria = CAT_COLUNAS;
 * 2. Extração do Valor Numérico:
 * int valor = extrair_id_widget(nome_do_radio_toggle);
 * * Opção atual mantida por centralizar toda a lógica em aritmética pura de alta performance.
 */
void on_radio_atualizar_generic_interface_toggled( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );

   if ( !gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) ) return;

   AppContext *ctx = ( AppContext * ) user_data;
   if ( !ctx || !widget ) return;

   const char *nome_do_radio_toggle = gtk_widget_get_name( widget );
   if ( !nome_do_radio_toggle ) return;

   int id_composto = extrair_id_widget( nome_do_radio_toggle );
   if ( id_composto == -1 ) return;

   int categoria = id_composto / 10;  // Pega a dezena
   int valor = id_composto % 10;      // Pega a unidade

   // Passa o endereço direto da sub-struct dados
   atualizar_generic_interface( ctx, categoria, valor );

   // Opcional: feedback no terminal para depuração
   g_print( "Estado alterado com segurança: Widget '%s' (Cat %d) -> (Valor %d)\n", nome_do_radio_toggle, categoria, valor );

}







void on_check_atualizar_booleanos_interface_toggled( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx || !widget ) return;

   // 1. Descobre quem foi o "culpado" pelo disparo do evento
   const char *check_nome = gtk_widget_get_name( widget );
   if ( !check_nome ) return;

   // 3. Agora sim, conversão 100% segura
   int categoria = extrair_id_widget( check_nome );
   if ( categoria == -1 ) return;

   bool estado = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) );

   // Dentro de callbacks.c (Linha 381):
   atualizar_booleanos_interface( estado, categoria, ctx );

   // Feedback limpo no terminal do seu Debian
   g_print( "Estado alterado com segurança: Widget '%s' (Cat %d) -> %s\n",
            check_nome, categoria, estado ? "ATIVADO" : "DESATIVADO" );
}





void on_entry_atualizar_tema_changed( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_COMBO_BOX( widget ) );
   AppContext *ctx = ( AppContext * )user_data;

   if ( !widget || !ctx ) return;

   ctx->cascata.foco.tema = gtk_combo_box_get_active( GTK_COMBO_BOX( widget ) );

   if ( widget == ctx->entry.tema ) {

      g_signal_handler_block( ctx->entry.tema_espelho, ctx->handlers.tema_espelho );

      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->entry.tema_espelho ), ctx->cascata.foco.tema );

      g_signal_handler_unblock( ctx->entry.tema_espelho, ctx->handlers.tema_espelho );


   } else if ( widget == ctx->entry.tema_espelho ) {

      g_signal_handler_block( ctx->entry.tema, ctx->handlers.tema );

      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->entry.tema ), ctx->cascata.foco.tema );

      g_signal_handler_unblock( ctx->entry.tema, ctx->handlers.tema );
   }

   gchar *tema = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( widget ) );

   if ( tema != NULL ) {
      atualizar_tema( ctx, tema );
      g_free( tema ); // Limpa a memória com segurança
   }
}






// --- EMBRULHOS GTK ---

void on_subtema_check_toggled( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx || !widget ) return;

   logica_subtema_toggled( widget, ctx );
}



void on_stepper_mais_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx || !widget ) return;

   logica_stepper_mais( widget, ctx );
}



void on_stepper_menos_clicked( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_BUTTON( widget ) );
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx || !widget ) return;

   logica_stepper_menos( widget, ctx );
}



void on_scrolled_rolar_para_o_fim_sizeallocate( GtkWidget *widget, GdkRectangle *allocation, gpointer user_data ) {
   g_return_if_fail( GTK_IS_SCROLLED_WINDOW( widget ) );
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;
   ( void )allocation;

   rolar_para_o_fim( GTK_SCROLLED_WINDOW( widget ) );

   if ( ctx->provas.handler_scrolled > 0  && ctx->provas.importar == false ) {
      g_signal_handler_block( widget, ctx->provas.handler_scrolled );
   }

}




void on_combo_alunos_changed( GtkWidget *widget, gpointer user_data ) {
   g_return_if_fail( GTK_IS_COMBO_BOX( widget ) );
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   int ativo = gtk_combo_box_get_active( GTK_COMBO_BOX( widget ) );
   if ( ativo < 0 ) return;

   gboolean modo_por_aluno = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( ctx->ui_diario.check_por_aluno ) );

   // =================================================================
   // MODO 1: POR ALUNO (Auditoria Livre)
   // =================================================================
   if ( modo_por_aluno ) {
      ctx->ui_diario.foco_combo_alunos = ativo;
      renderizar_frequencia_por_aluno( ctx );
      return; // 🛑 Encerra aqui, não aplica nenhuma trava!
   }

   // =================================================================
   // MODO 2: CHAMADA NORMAL (Navegação Restrita / Sequencial)
   // =================================================================
   if ( ativo >= ctx->ui_diario.limite_combo_alunos ) {

      // Bloqueia temporariamente este callback para não gerar loop infinito
      if ( ctx->ui_diario.handler_combo_alunos > 0 ) {
         g_signal_handler_block( widget, ctx->ui_diario.handler_combo_alunos );
      }

      // Força o retorno visual para o índice máximo permitido
      ctx->ui_diario.foco_combo_alunos = ctx->ui_diario.limite_combo_alunos - 1;
      gtk_combo_box_set_active( GTK_COMBO_BOX( widget ), ctx->ui_diario.foco_combo_alunos );

      // Desbloqueia o callback para voltar a ouvir cliques futuros
      if ( ctx->ui_diario.handler_combo_alunos > 0 ) {
         g_signal_handler_unblock( widget, ctx->ui_diario.handler_combo_alunos );
      }

      return;
   }

   // Se a navegação foi válida (dentro do limite da chamada), atualiza o foco normalmente
   ctx->ui_diario.foco_combo_alunos = ativo;
}






