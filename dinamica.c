/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>

#include "callbacks.h"
#include "basicas.h"
#include "comum.h"
#include "interface.h"
#include "mensagens.h"

#include "dinamica.h"



// ==================================================================================================
// 1. HIGIENE E LIMPEZA DE CONTAINERES
// ==================================================================================================
static void destruir_widget_callback( GtkWidget *widget, gpointer data ) {
   ( void )data;
   gtk_widget_destroy( widget );
}

void limpar_container( GtkWidget *container ) {
   if ( !container ) {
      return;
   }
   gtk_container_foreach( GTK_CONTAINER( container ), destruir_widget_callback, NULL );
}

// ==================================================================================================
// 2. BUSCA DE WIDGETS INTERNOS E LISTBOX LATEX
// ==================================================================================================
static void atualizar_listbox_subtemas_latex( InterfaceLatex *latex, InterfaceProva *prova ) {
   if ( !latex || !prova ) {
      return;
   }

   limpar_container( latex->listbox_subtemas );

   for ( int i = 0; i < prova->limite_subtemas; i++ ) {
      if ( !prova->subtemas[i] ) {
         continue;
      }

      GtkWidget *linha_box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 8 );

      GtkWidget *indicador_label = gtk_label_new( "›" );
      gtk_style_context_add_class( gtk_widget_get_style_context( indicador_label ), "indicador-item-subtema" );

      GtkWidget *texto_label = gtk_label_new( prova->subtemas[i] );

      gtk_box_pack_start( GTK_BOX( linha_box ), indicador_label, FALSE, FALSE, 0 );
      gtk_box_pack_start( GTK_BOX( linha_box ), texto_label, FALSE, FALSE, 0 );

      gtk_container_add( GTK_CONTAINER( latex->listbox_subtemas ), linha_box );
   }

   gtk_widget_show_all( latex->listbox_subtemas );
}


static void comparar_nome_do_filho( GtkWidget *container_child, gpointer user_data ) {
   BuscaWidgetData *data = ( BuscaWidgetData * )user_data;

   if ( data->encontrado != NULL ) {
      return;
   }

   GtkWidget *widget_interno = gtk_bin_get_child( GTK_BIN( container_child ) );

   if ( widget_interno && g_strcmp0( gtk_widget_get_name( widget_interno ), data->nome_alvo ) == 0 ) {
      data->encontrado = widget_interno;
   }
}

static GtkWidget* buscar_widget_por_nome_interno( GtkWidget *container, const char *nome_alvo ) {
   if ( !container || !nome_alvo ) {
      return NULL;
   }

   BuscaWidgetData data = { .nome_alvo = nome_alvo, .encontrado = NULL };
   gtk_container_foreach( GTK_CONTAINER( container ), comparar_nome_do_filho, &data );

   return data.encontrado;
}

// ==================================================================================================
// 3. ATUALIZAÇÃO DA LISTA DE SUBTEMAS DA PROVA
// ==================================================================================================
void atualizar_listbox_subtemas( AppContext *ctx ) {
   if ( !ctx ) {
      return;
   }

   InterfaceDinamica *montagem = &ctx->prova.montagem;
   InterfaceProva    *prova = &ctx->prova;

   g_autofree gchar *tema_ativo = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( prova->combo_tema ) );
   if ( !tema_ativo ) {
      limpar_container( prova->listbox_subtemas );
      return;
   }

   limpar_container( prova->listbox_subtemas );

   // 1. Libera a alocação anterior com segurança
   g_clear_pointer( &montagem->handlers_check, g_free );

   // 2. Aloca o novo vetor simples para os subtemas visíveis
   if ( prova->limite_subtemas > 0 ) {
      montagem->handlers_check = g_new0( gulong, ( gsize )prova->limite_subtemas );
   }

   for ( int i = 0; i < prova->limite_subtemas; i++ ) {
      if ( !prova->subtemas[i] ) {
         continue;
      }

      GtkWidget *check = gtk_check_button_new_with_label( prova->subtemas[i] );
      gtk_widget_set_name( check, prova->subtemas[i] );
      g_object_set_data( G_OBJECT( check ), "id_widget", GINT_TO_POINTER( i ) );

      char checa_id[256];
      g_snprintf( checa_id, sizeof( checa_id ), "%s/%s", tema_ativo, prova->subtemas[i] );

      GtkWidget *box_encontrado = buscar_widget_por_nome_interno( prova->flowbox_selecionados, checa_id );
      if ( box_encontrado != NULL ) {
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check ), TRUE );
      }

      gulong handler_id = g_signal_connect( check, "toggled", G_CALLBACK( on_subtema_check_toggled ), ctx );

      if ( prova->limite_subtemas > 0 && montagem->handlers_check != NULL ) {
         montagem->handlers_check[i] = handler_id;
      }

      gtk_list_box_insert( GTK_LIST_BOX( prova->listbox_subtemas ), check, -1 );
   }

   gtk_widget_show_all( prova->listbox_subtemas );
   atualizar_listbox_subtemas_latex( &ctx->latex, &ctx->prova );
}

// ==================================================================================================
// 4. CONSTRUÇÃO DO MINI-CARD (STEPPER iFOOD)
// ==================================================================================================
static GtkWidget* criar_mini_card_stepper( InterfaceDinamica *montagem, const char *tag_id, int qtd_tex, int id_widget ) {
   GtkWidget *tag_box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 12 );
   gtk_widget_set_vexpand( tag_box, FALSE );
   gtk_widget_set_valign( tag_box, GTK_ALIGN_START );
   gtk_widget_set_name( tag_box, tag_id );
   gtk_style_context_add_class( gtk_widget_get_style_context( tag_box ), "mini-card-flowbox" );

   GtkWidget *label_nome = gtk_label_new( tag_id );
   gtk_box_pack_start( GTK_BOX( tag_box ), label_nome, FALSE, FALSE, 4 );

   GtkWidget *stepper_box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 6 );
   gtk_style_context_add_class( gtk_widget_get_style_context( stepper_box ), "stepper-container" );

   montagem->btn_menos = gtk_button_new();
   gtk_button_set_relief( GTK_BUTTON( montagem->btn_menos ), GTK_RELIEF_NONE );
   gtk_style_context_add_class( gtk_widget_get_style_context( montagem->btn_menos ), "btn-stepper" );

   if ( qtd_tex == 1 ) {
      gtk_style_context_add_class( gtk_widget_get_style_context( montagem->btn_menos ), "botao-remover-ifood" );
   } else {
      gtk_button_set_label( GTK_BUTTON( montagem->btn_menos ), "－" );
   }

   char str_qtd[16];
   g_snprintf( str_qtd, sizeof( str_qtd ), "%d", qtd_tex );
   GtkWidget *label_qtd = gtk_label_new( str_qtd );
   gtk_style_context_add_class( gtk_widget_get_style_context( label_qtd ), "label-stepper" );

   montagem->btn_mais = gtk_button_new_with_label( "＋" );
   gtk_button_set_relief( GTK_BUTTON( montagem->btn_mais ), GTK_RELIEF_NONE );
   gtk_style_context_add_class( gtk_widget_get_style_context( montagem->btn_mais ), "btn-stepper" );

   // Armazena as referências seguras na tag_box (GTK gerencia a árvore)
   g_object_set_data( G_OBJECT( tag_box ), "label_quantidade", label_qtd );
   g_object_set_data( G_OBJECT( tag_box ), "btn_menos", montagem->btn_menos );
   g_object_set_data( G_OBJECT( tag_box ), "limite_max", GINT_TO_POINTER( qtd_tex ) );

   g_object_set_data( G_OBJECT( montagem->btn_menos ), "tag_pai", tag_box );
   g_object_set_data( G_OBJECT( montagem->btn_mais ), "tag_pai", tag_box );
   g_object_set_data( G_OBJECT( montagem->btn_menos ), "id_widget_origem", GINT_TO_POINTER( id_widget ) );

   gtk_box_pack_start( GTK_BOX( stepper_box ), montagem->btn_menos, FALSE, FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( stepper_box ), label_qtd, FALSE, FALSE, 4 );
   gtk_box_pack_start( GTK_BOX( stepper_box ), montagem->btn_mais, FALSE, FALSE, 0 );

   gtk_box_pack_end( GTK_BOX( tag_box ), stepper_box, FALSE, FALSE, 4 );

   gtk_widget_set_size_request( tag_box, -1, 44 );
   gtk_widget_set_size_request( montagem->btn_mais, 34, 34 );
   gtk_widget_set_size_request( montagem->btn_menos, 34, 34 );

   return tag_box;
}

static EstadoStepper extrair_estado_stepper( GtkWidget *button ) {
   EstadoStepper st = {0};

   st.tag_box = GTK_WIDGET( g_object_get_data( G_OBJECT( button ), "tag_pai" ) );
   if ( !st.tag_box ) {
      return st;
   }

   st.label_qtd  = GTK_WIDGET( g_object_get_data( G_OBJECT( st.tag_box ), "label_quantidade" ) );
   st.btn_menos  = GTK_WIDGET( g_object_get_data( G_OBJECT( st.tag_box ), "btn_menos" ) );
   st.limite_max = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( st.tag_box ), "limite_max" ) );

   if ( st.label_qtd ) {
      st.valor_atual = atoi( gtk_label_get_text( GTK_LABEL( st.label_qtd ) ) );
   }

   return st;
}

// ==================================================================================================
// 5. SINCRONIZAÇÃO DA ESTRUTURA COM O FLOWBOX
// ==================================================================================================
static void sincronizar_vetor_com_flowbox( InterfaceDados *dados, const InterfaceProva *prova ) {
   g_return_if_fail( dados != NULL );
   g_return_if_fail( prova != NULL );
   g_return_if_fail( GTK_IS_CONTAINER( prova->flowbox_selecionados ) );

   for ( int i = 0; i < dados->total_quest_prova; i++ ) {
      g_clear_pointer( &dados->selecao_quest_prova[i], g_free );
      dados->qtd_quest_subtema[i] = 0;
   }

   g_autoptr( GList ) children = gtk_container_get_children( GTK_CONTAINER( prova->flowbox_selecionados ) );
   int idx = 0;

   for ( GList *iter = children; iter != NULL; iter = g_list_next( iter ) ) {
      GtkWidget *flow_child = GTK_WIDGET( iter->data );
      GtkWidget *tag_box = gtk_bin_get_child( GTK_BIN( flow_child ) );

      if ( tag_box ) {
         const char *tag_id = gtk_widget_get_name( tag_box );
         GtkWidget *label_qtd = g_object_get_data( G_OBJECT( tag_box ), "label_quantidade" );

         if ( tag_id && label_qtd ) {
            dados->selecao_quest_prova[idx] = g_strdup( tag_id );
            dados->qtd_quest_subtema[idx]   = atoi( gtk_label_get_text( GTK_LABEL( label_qtd ) ) );
            idx++;
         }
      }

      if ( idx == dados->total_quest_prova ) {
         break;
      }
   }
}

// ==================================================================================================
// 6. LÓGICA DOS BOTÕES STEPPER (MAIS E MENOS - CORRIGIDA)
// ==================================================================================================
void logica_stepper_mais( GtkWidget *button, AppContext *ctx ) {
   if ( !ctx ) {
      return;
   }

   EstadoStepper st = extrair_estado_stepper( button );
   if ( !st.tag_box ) {
      return;
   }

   if ( st.valor_atual < st.limite_max ) {
      st.valor_atual++;
      char str[16];
      g_snprintf( str, sizeof( str ), "%d", st.valor_atual );
      gtk_label_set_text( GTK_LABEL( st.label_qtd ), str );

      if ( st.valor_atual == 2 ) {
         gtk_button_set_label( GTK_BUTTON( st.btn_menos ), "－" );
         gtk_style_context_remove_class( gtk_widget_get_style_context( st.btn_menos ), "botao-remover-ifood" );
      }

      sincronizar_vetor_com_flowbox( &ctx->dados, &ctx->prova );
   }
}

static void remover_tag_no_flowbox( GtkWidget *flowbox, const char *tag_id ) {
   if ( !flowbox || !tag_id ) {
      return;
   }

   GtkWidget *box = buscar_widget_por_nome_interno( flowbox, tag_id );
   if ( box ) {
      gtk_widget_destroy( gtk_widget_get_parent( box ) );
   }
}

void logica_stepper_menos( GtkWidget *button, AppContext *ctx ) {
   if ( !ctx || !button ) {
      return;
   }

   EstadoStepper st = extrair_estado_stepper( button );
   if ( !st.tag_box ) {
      return;
   }

   InterfaceDados    *dados    = &ctx->dados;
   InterfaceProva    *prova = &ctx->prova;
   InterfaceDinamica *montagem = &ctx->prova.montagem;

   if ( st.valor_atual > 1 ) {
      st.valor_atual--;
      char str[16];
      g_snprintf( str, sizeof( str ), "%d", st.valor_atual );
      gtk_label_set_text( GTK_LABEL( st.label_qtd ), str );

      if ( st.valor_atual == 1 ) {
         gtk_button_set_label( GTK_BUTTON( button ), "" );
         gtk_style_context_add_class( gtk_widget_get_style_context( button ), "botao-remover-ifood" );
      }

      sincronizar_vetor_com_flowbox( dados, prova );

   } else {
      // 🎯 CORREÇÃO DO BOTÃO IFOOD:
      // Lê o tag_id direto do nome da tag_box (mantido em memória pelo GTK)
      const char *label_tema = gtk_widget_get_name( st.tag_box );
      if ( !label_tema ) {
         return;
      }

      g_auto( GStrv ) pedacos = g_strsplit( label_tema, "/", 2 );
      if ( !pedacos || !pedacos[0] || !pedacos[1] ) {
         return;
      }

      const char *tema_alvo    = pedacos[0];
      const char *subtema_alvo = pedacos[1];

      // 1. Remove a tag do FlowBox e atualiza a estrutura de dados
      remover_tag_no_flowbox( prova->flowbox_selecionados, label_tema );
      sincronizar_vetor_com_flowbox( dados, prova );

      if ( montagem->cont_add > 0 ) {
         montagem->cont_add--;
      }

      // 2. Se o tema da tag removida for o tema ativo visível na tela, desmarca o CheckBox
      g_autofree gchar *tema_ativo = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( prova->combo_tema ) );
      if ( tema_ativo && g_strcmp0( tema_ativo, tema_alvo ) == 0 ) {
         GtkWidget *check_box = buscar_widget_por_nome_interno( prova->listbox_subtemas, subtema_alvo );

         if ( check_box && GTK_IS_TOGGLE_BUTTON( check_box ) ) {
            int id_widget = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( check_box ), "id_widget" ) );

            if ( montagem->handlers_check != NULL ) {
               g_signal_handler_block( check_box, montagem->handlers_check[id_widget] );
               gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check_box ), FALSE );
               g_signal_handler_unblock( check_box, montagem->handlers_check[id_widget] );
            } else {
               gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check_box ), FALSE );
            }
         }
      }
   }
}

void rolar_para_o_fim( GtkScrolledWindow *scrolled_window ) {
   if ( !scrolled_window ) {
      return;
   }

   GtkAdjustment *v_adj = gtk_scrolled_window_get_vadjustment( scrolled_window );
   double max_v = gtk_adjustment_get_upper( v_adj ) - gtk_adjustment_get_page_size( v_adj );
   if ( max_v > 0 ) {
      gtk_adjustment_set_value( v_adj, max_v );
   }

   GtkAdjustment *h_adj = gtk_scrolled_window_get_hadjustment( scrolled_window );
   double max_h = gtk_adjustment_get_upper( h_adj ) - gtk_adjustment_get_page_size( h_adj );
   if ( max_h > 0 ) {
      gtk_adjustment_set_value( h_adj, max_h );
   }
}

// ==================================================================================================
// 7. LÓGICA DE SELEÇÃO DO SUBTEMA (TOGGLED)
// ==================================================================================================
void logica_subtema_toggled( GtkWidget *widget, AppContext *ctx ) {
   if ( !ctx || !widget ) {
      return;
   }

   CaminhoDiretorio  *caminho  = &ctx->caminho;
   InterfaceProva    *prova = &ctx->prova;
   InterfaceDinamica *montagem = &ctx->prova.montagem;
   InterfaceDados    *dados    = &ctx->dados;
   InterfacePainel   *painel   = &ctx->painel;

   gboolean estado = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) );
   const char *subtema = gtk_button_get_label( GTK_BUTTON( widget ) );
   int id_widget = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( widget ), "id_widget" ) );

   if ( estado && montagem->cont_add == dados->total_quest_prova ) {
      if ( montagem->handlers_check != NULL ) {
         g_signal_handler_block( widget, montagem->handlers_check[id_widget] );
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), FALSE );
         g_signal_handler_unblock( widget, montagem->handlers_check[id_widget] );
      } else {
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), FALSE );
      }

      painel->format_titulo    = meu_gerador_variadico( "⚠ Aviso:" );
      painel->format_subtitulo = meu_gerador_variadico( "O limite de %d subtemas adicionados foi atingido.", dados->total_quest_prova );
      painel->format_instrucao = meu_gerador_variadico( "%s", "Verifique os temas selecionados e defina as quantidades de questões." );

      criar_mensagem_painel( AVISO, painel );
      fprintf( stderr, "[AVISO] O limite de %d subtemas adicionados foi atingido.\n", dados->total_quest_prova );
      return;
   }

   g_autofree gchar *tema_ativo = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->prova.combo_tema ) );
   if ( !tema_ativo ) {
      return;
   }

   char tag_id[256];
   g_snprintf( tag_id, sizeof( tag_id ), "%s/%s", tema_ativo, subtema );

   if ( estado ) {
      GtkWidget *box_encontrado = buscar_widget_por_nome_interno( prova->flowbox_selecionados, tag_id );

      if ( box_encontrado == NULL ) {
         g_clear_pointer( &caminho->questoes_tex, g_free );
         caminho->questoes_tex = g_build_filename( caminho->banco_questoes, tag_id, NULL );
         int qtd_tex = quantidade_arquivos_por_extensao( caminho->questoes_tex, ".tex" );

         if ( qtd_tex == 0 ) {
            if ( montagem->handlers_check != NULL ) {
               g_signal_handler_block( widget, montagem->handlers_check[id_widget] );
               gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), FALSE );
               g_signal_handler_unblock( widget, montagem->handlers_check[id_widget] );
            } else {
               gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), FALSE );
            }

            fprintf( stderr, "[AVISO] Subtema VAZIO! Escolha outro ou adicione novas questões.\n" );
            return;
         }

         GtkWidget *novo_card = criar_mini_card_stepper( montagem, tag_id, qtd_tex, id_widget );
         if ( novo_card != NULL ) {
            g_signal_connect( montagem->btn_menos, "clicked", G_CALLBACK( on_stepper_menos_clicked ), ctx );
            g_signal_connect( montagem->btn_mais, "clicked", G_CALLBACK( on_stepper_mais_clicked ), ctx );
         }

         gtk_flow_box_insert( GTK_FLOW_BOX( prova->flowbox_selecionados ), novo_card, -1 );

         if ( montagem->handler_scrolled > 0 && montagem->importar == FALSE ) {
            g_signal_handler_unblock( prova->scrolled_window, montagem->handler_scrolled );
         }

         gtk_widget_show_all( prova->flowbox_selecionados );
         sincronizar_vetor_com_flowbox( dados, prova );
         montagem->cont_add++;
      }
   } else {
      remover_tag_no_flowbox( prova->flowbox_selecionados, tag_id );
      sincronizar_vetor_com_flowbox( dados, prova );

      if ( montagem->cont_add > 0 ) {
         montagem->cont_add--;
      }
   }
}

// ==================================================================================================
// 8. RESTAURAÇÃO DE RASCUNHO TEMPORÁRIO
// ==================================================================================================
static gboolean bloquear_sinal_com_atraso( gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) {
      return G_SOURCE_REMOVE;
   }

   if ( ctx->prova.montagem.handler_scrolled > 0 ) {
      g_signal_handler_block( ctx->prova.scrolled_window, ctx->prova.montagem.handler_scrolled );
      ctx->prova.montagem.importar = FALSE;
   }

   return G_SOURCE_REMOVE;
}

void alimentar_interface_temporaria( AppContext *ctx, const DadosRascunho *rascunho ) {
   if ( !ctx || !rascunho ) {
      return;
   }

   InterfaceDados *dados    = &ctx->dados;
   InterfaceProva *prova = &ctx->prova;

   if ( prova->montagem.handler_scrolled > 0 ) {
      g_signal_handler_unblock( prova->scrolled_window, prova->montagem.handler_scrolled );
      prova->montagem.importar = TRUE;
   }

   dados->total_quest_prova = rascunho->total_quest_prova;

   limpar_container( prova->listbox_subtemas );
   limpar_container( prova->flowbox_selecionados );
   prova->montagem.cont_add = 0;

   atualizar_listbox_subtemas( ctx );

   for ( int i = 0; i < dados->total_quest_prova; i++ ) {
      if ( rascunho->questoes[i].str[0] == '\0' ) {
         continue;
      }

      g_auto( GStrv ) pedacos = g_strsplit( rascunho->questoes[i].str, "/", -1 );
      char *tema_salvo    = pedacos[0];
      char *subtema_salvo = pedacos[1];

      if ( !tema_salvo || !subtema_salvo ) {
         continue;
      }

      // Busca rápida sem alocações extras no Heap
      int t = buscar_indice_bsearch( &tema_salvo, prova->temas, ( guint )prova->limite_temas,
                                     sizeof( tema_salvo ), comparar_item_combo );
      if ( t >= 0 ) {
         gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->prova.combo_tema ), t );
      }

      GtkWidget *check_box = buscar_widget_por_nome_interno( prova->listbox_subtemas, subtema_salvo );

      if ( check_box && GTK_IS_TOGGLE_BUTTON( check_box ) ) {
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check_box ), TRUE );

         GtkWidget *tag_box = buscar_widget_por_nome_interno( prova->flowbox_selecionados, rascunho->questoes[i].str );
         if ( tag_box ) {
            GtkWidget *label_qtd = g_object_get_data( G_OBJECT( tag_box ), "label_quantidade" );
            GtkWidget *btn_menos = g_object_get_data( G_OBJECT( tag_box ), "btn_menos" );

            if ( label_qtd ) {
               char str_qtd[16];
               g_snprintf( str_qtd, sizeof( str_qtd ), "%d", rascunho->questoes[i].qtd );
               gtk_label_set_text( GTK_LABEL( label_qtd ), str_qtd );

               if ( rascunho->questoes[i].qtd == 1 && btn_menos ) {
                  gtk_button_set_label( GTK_BUTTON( btn_menos ), "" );
                  gtk_style_context_add_class( gtk_widget_get_style_context( btn_menos ), "botao-remover-ifood" );
               }
            }
         }
      }
   }

   sincronizar_vetor_com_flowbox( dados, prova );

   gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->latex.combo_cor_serie ), rascunho->foco_cor_serie );
   gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->latex.combo_decoracao ), rascunho->foco_decoracao );

   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->diario.check_expor_dados ), rascunho->expor_dados );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->prova.check_nao_presencial ), rascunho->nao_presencial );

   int idx_valido = CLAMP( rascunho->separadores - 1, 0, 1 );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->latex.radio_separadores[ idx_valido ] ), TRUE );

   idx_valido = CLAMP( rascunho->fonte_latex - 1, 0, 1 );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->latex.radio_fonte_latex[ idx_valido ] ), TRUE );

   idx_valido = CLAMP( rascunho->qtd_paginas - 1, 0, 1 );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->latex.radio_qtd_paginas[ idx_valido ] ), TRUE );

   idx_valido = CLAMP( rascunho->qtd_colunas - 2, 0, 1 );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->latex.radio_qtd_colunas[ idx_valido ] ), TRUE );

   idx_valido = CLAMP( rascunho->cabecalho_tipo - 1, 0, 1 );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->latex.radio_cabecalho_tipo[ idx_valido ] ), TRUE );

   g_timeout_add( 30, bloquear_sinal_com_atraso, ctx );
}






//==================================================================================================
// Função auxiliar interna para verificar se o arquivo oculto é lixo do sistema
static int eh_metadado_valido( const char *nome ) {
   // Ignora o diretório atual (.) e pai (..)
   if ( g_strcmp0( nome, "." ) == 0 || g_strcmp0( nome, ".." ) == 0 ) {
      return 0;
   }

   // Ignora pastas ou arquivos comuns de sistemas/ferramentas
   if ( g_strcmp0( nome, ".git" ) == 0 ||
         g_strcmp0( nome, ".vscode" ) == 0 ||
         g_strcmp0( nome, ".DS_Store" ) == 0 ) {
      return 0;
   }

   return 1;
}
//==================================================================================================
static gchar *preencher_painel_responsavel( const char *diretorio, GtkWidget *box, const char *prefixo ) {
   // 1. Defesas rigorosas com macros GLib
   g_return_val_if_fail( diretorio != NULL, NULL );
   g_return_val_if_fail( GTK_IS_BOX( box ), NULL );
   g_return_val_if_fail( prefixo != NULL, NULL );

   // 2. Abertura do diretório no padrão GLib
   GDir *dir = g_dir_open( diretorio, 0, NULL );
   if ( !dir ) {
      g_printerr( "ERRO: Não foi possível abrir o diretório do responsável: %s\n", diretorio );
      return NULL;
   }

   const gchar *nome_arquivo;
   gchar *responsavel_extraido = NULL;

   // 3. Iteração segura
   while ( ( nome_arquivo = g_dir_read_name( dir ) ) != NULL ) {
      // Identifica o arquivo de metadados oculto (iniciado por '.')
      if ( nome_arquivo[0] == '.' && eh_metadado_valido( nome_arquivo ) ) {

         // Aloca a string ignorando o caractere '.' inicial (índice 1 em diante)
         responsavel_extraido = g_strdup( nome_arquivo + 1 );

         // Limita/limpa o tamanho com a sua função customizada
         int len = calcular_len_limpo( responsavel_extraido, 20 );
         responsavel_extraido[len] = '\0';

         break;
      }
   }
   g_dir_close( dir );

   // 4. Fallback: Decide qual string exibir
   gchar *nome_exibicao = g_strdup( responsavel_extraido ? responsavel_extraido : "Não Identificado" );

   // 5. Atualização da Interface
   limpar_container( box );

   GtkWidget *label_prefixo = gtk_label_new( prefixo );
   GtkWidget *label_reponsavel = gtk_label_new( nome_exibicao );

   gtk_style_context_add_class( gtk_widget_get_style_context( label_prefixo ), "cabecalho" );

   gtk_box_pack_start( GTK_BOX( box ), label_prefixo, FALSE, FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( box ), label_reponsavel, FALSE, FALSE, 0 );

   gtk_widget_show_all( box );

   // 6. Higiene de memória: o GTK já copiou a string internamente para o label
   g_free( responsavel_extraido );

   return nome_exibicao;
}
//======================================================================================================================//
/**
 * @brief Lê o metadado do "professor" no diretório e atualiza o box visual correspondente.
 */
gchar *professor_da_disciplina( const char *diretorio, GtkWidget *box_professor ) {
   return preencher_painel_responsavel( diretorio, box_professor, "Prof. " );
}
//======================================================================================================================//
/**
 * @brief Lê o metadado do "gestor" no diretório e atualiza o box visual correspondente.
 */
gchar *gestor_da_escola( const char *diretorio, GtkWidget *box_gestor ) {
   return preencher_painel_responsavel( diretorio, box_gestor, "Gest. " );
}
//======================================================================================================================//
