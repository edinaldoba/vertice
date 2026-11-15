/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */


#include <gtk/gtk.h>
#include "config.h"
#include "comum.h"
#include "auxiliar.h"

// ====================================================================================
// 1. DESTAQUE VISUAL DO CARD DO ALUNO SELECIONADO NO FLOWBOX
// ====================================================================================
static void destacar_card_aluno_selecionado_aux( InterfaceAtipicidade *atip, int idx_aluno_ativo ) {
   g_return_if_fail( atip != NULL && GTK_IS_FLOW_BOX( atip->flowbox ) );

   GList *children = gtk_container_get_children( GTK_CONTAINER( atip->flowbox ) );

   for ( GList *l = children; l != NULL; l = l->next ) {
      GtkWidget *flow_child = GTK_WIDGET( l->data );
      GtkWidget *card_box   = gtk_bin_get_child( GTK_BIN( flow_child ) );
      if ( !card_box ) {
         continue;
      }

      int idx_card = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( flow_child ), "idx_aluno" ) ) - 1;
      GtkStyleContext *sc = gtk_widget_get_style_context( card_box );

      if ( idx_card == idx_aluno_ativo ) {
         gtk_style_context_add_class( sc, "card-atipico-destaque" );
         gtk_flow_box_select_child( GTK_FLOW_BOX( atip->flowbox ), GTK_FLOW_BOX_CHILD( flow_child ) );
      } else {
         gtk_style_context_remove_class( sc, "card-atipico-destaque" );
      }
   }

   g_list_free( children );
}

// ====================================================================================
// 2. SINCRONIZAÇÃO DOS CHECKBUTTONS E COMBO TEA PARA O ALUNO ATUAL DO COMBO
// ====================================================================================
static void sincronizar_checks_com_ficha_aux( AppContext *ctx, int idx_aluno ) {
   InterfaceAtipicidade *atip   = &ctx->config.atipicidade;
   GPtrArray            *fichas = ctx->diario.fichas;

   FlagsAtipicidade flags_aluno = ATIPICO_NENHUM;
   guint8           nivel_tea   = 0;
   gboolean         aluno_valido = ( fichas != NULL && idx_aluno >= 0 && ( guint )idx_aluno < fichas->len );

   if ( aluno_valido ) {
      const FichaAluno *ficha = ( const FichaAluno * )g_ptr_array_index( fichas, idx_aluno );
      if ( ficha != NULL ) {
         flags_aluno = ficha->atipico.flags;
         nivel_tea   = ficha->atipico.nivel_suporte_tea;
      }
   }

   // Atualiza cada CheckButton com o sinal bloqueado para evitar recursão
   for ( int i = 0; i < MAX_FLAGS_ATIPICIDADES; i++ ) {
      GtkWidget *chk = atip->checks[i];
      if ( chk == NULL ) {
         continue;
      }

      gboolean marcado = aluno_valido && ( ( ( guint32 )flags_aluno & ( 1U << ( guint )i ) ) != 0U );

      if ( atip->handlers_check[i] > 0 ) {
         g_signal_handler_block( chk, atip->handlers_check[i] );
      }
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( chk ), marcado );
      gtk_widget_set_sensitive( chk, aluno_valido );
      if ( atip->handlers_check[i] > 0 ) {
         g_signal_handler_unblock( chk, atip->handlers_check[i] );
      }
   }

   // Atualiza o Combo de Nível de Suporte TEA (0=N/A, 1=Nível 1, 2=Nível 2, 3=Nível 3)
   if ( atip->combo_suporte_tea != NULL ) {
      gboolean tea_marcado = aluno_valido && ( ( flags_aluno & ATIPICO_TEA ) != 0 );
      int      idx_combo   = ( tea_marcado && nivel_tea <= 3 ) ? ( int )nivel_tea : 0;

      if ( atip->handler_combo_suporte_tea > 0 ) {
         g_signal_handler_block( atip->combo_suporte_tea, atip->handler_combo_suporte_tea );
      }
      gtk_combo_box_set_active( GTK_COMBO_BOX( atip->combo_suporte_tea ), idx_combo );
      gtk_widget_set_sensitive( atip->combo_suporte_tea, tea_marcado );
      if ( atip->handler_combo_suporte_tea > 0 ) {
         g_signal_handler_unblock( atip->combo_suporte_tea, atip->handler_combo_suporte_tea );
      }
   }
}

// Forward declaration para a callback do botão de lixeira
static void reconstruir_flowbox_atipicidades_aux( AppContext *ctx );

// ====================================================================================
// 3. CALLBACK DO BOTÃO DE REMOVER MINI-CARD (.botao-remover-ifood)
// ====================================================================================
static void on_btn_remover_mini_card_clicked_aux( GtkWidget *btn, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   g_return_if_fail( ctx != NULL && GTK_IS_BUTTON( btn ) );

   GPtrArray *fichas = ctx->diario.fichas;
   if ( !fichas ) {
      return;
   }

   int idx_aluno = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( btn ), "idx_aluno" ) ) - 1;
   int bit_idx   = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( btn ), "bit_idx" ) ) - 1;

   if ( idx_aluno < 0 || ( guint )idx_aluno >= fichas->len || bit_idx < 0 || bit_idx >= MAX_FLAGS_ATIPICIDADES ) {
      return;
   }

   FichaAluno *ficha = ( FichaAluno * )g_ptr_array_index( fichas, idx_aluno );
   if ( !ficha ) {
      return;
   }

   // Remove o bit correspondente na RAM e marca a ficha como modificada
   guint32 mascara = ~( 1U << ( guint )bit_idx );
   ficha->atipico.flags = ( FlagsAtipicidade )( ( guint32 )ficha->atipico.flags & mascara );

   if ( bit_idx == 0 ) { // ATIPICO_TEA desmarcado -> zera nível de suporte
      ficha->atipico.nivel_suporte_tea = 0;
   }

   ficha->ficha_modificada       = TRUE;
   ctx->diario.dados_modificados = TRUE;

   // Se o card clicado não era o aluno ativo no combo, muda o combo para ele;
   // caso já seja o aluno ativo, apenas ressincroniza os checks.
   InterfaceAtipicidade *atip = &ctx->config.atipicidade;
   int idx_combo_atual = gtk_combo_box_get_active( GTK_COMBO_BOX( atip->combo_alunos ) );

   if ( idx_combo_atual != idx_aluno ) {
      g_signal_handler_block( atip->combo_alunos, atip->handler_combo_alunos );
      gtk_combo_box_set_active( GTK_COMBO_BOX( atip->combo_alunos ), idx_aluno );
      g_signal_handler_unblock( atip->combo_alunos, atip->handler_combo_alunos );
   }

   sincronizar_checks_com_ficha_aux( ctx, idx_aluno );
   reconstruir_flowbox_atipicidades_aux( ctx );
}

// ====================================================================================
// 4. CONSTRUÇÃO DOS CARDS E MINI-CARDS NO FLOWBOX
// ====================================================================================
static void reconstruir_flowbox_atipicidades_aux( AppContext *ctx ) {
   InterfaceAtipicidade *atip = &ctx->config.atipicidade;
   g_return_if_fail( GTK_IS_FLOW_BOX( atip->flowbox ) );

   // Limpa todos os cards anteriores do FlowBox
   GList *children = gtk_container_get_children( GTK_CONTAINER( atip->flowbox ) );
   for ( GList *l = children; l != NULL; l = l->next ) {
      gtk_widget_destroy( GTK_WIDGET( l->data ) );
   }
   g_list_free( children );

   GPtrArray *fichas = ctx->diario.fichas;
   if ( !fichas || fichas->len == 0 ) {
      return;
   }

   int idx_aluno_selecionado = gtk_combo_box_get_active( GTK_COMBO_BOX( atip->combo_alunos ) );

   for ( guint i = 0; i < fichas->len; i++ ) {
      const FichaAluno *ficha = ( const FichaAluno * )g_ptr_array_index( fichas, i );
      if ( !ficha || !ficha->ativo || ficha->atipico.flags == ATIPICO_NENHUM ) {
         continue;
      }

      // 1. Card Maior do Aluno (GtkBox Vertical)
      GtkWidget *card_aluno = gtk_box_new( GTK_ORIENTATION_VERTICAL, 6 );

      // FIX: Impede que o card seja esticado verticalmente pelo GTK
      gtk_widget_set_valign( card_aluno, GTK_ALIGN_START );

      GtkStyleContext *sc_card = gtk_widget_get_style_context( card_aluno );
      gtk_style_context_add_class( sc_card, "card-aluno-atipico" );

      if ( ( int )i == idx_aluno_selecionado ) {
         gtk_style_context_add_class( sc_card, "card-atipico-destaque" );
      }

      // 2. Cabeçalho do Card com o Nome do Aluno
      g_autofree gchar *titulo_aluno = g_strdup_printf( "%.2u - %s", i + 1U, ficha->aluno );
      GtkWidget *lbl_nome = gtk_label_new( titulo_aluno );
      gtk_label_set_xalign( GTK_LABEL( lbl_nome ), 0.0F );
      gtk_label_set_ellipsize( GTK_LABEL( lbl_nome ), PANGO_ELLIPSIZE_END );
      gtk_style_context_add_class( gtk_widget_get_style_context( lbl_nome ), "titulo-secao" );
      gtk_box_pack_start( GTK_BOX( card_aluno ), lbl_nome, FALSE, FALSE, 2 );

      // 3. Um Mini-Card Horizontal (.mini-card-flowbox) para cada Atipicidade ativa
      for ( int bit = 0; bit < MAX_FLAGS_ATIPICIDADES; bit++ ) {
         if ( ( ( guint32 )ficha->atipico.flags & ( 1U << ( guint )bit ) ) == 0U ) {
            continue;
         }

         GtkWidget *chk_ref = atip->checks[bit];
         const gchar *texto_check = ( chk_ref != NULL )
                                    ? gtk_button_get_label( GTK_BUTTON( chk_ref ) )
                                    : "Atipicidade";

         g_autofree gchar *texto_mini = NULL;
         if ( bit == 0 && ficha->atipico.nivel_suporte_tea >= 1 && ficha->atipico.nivel_suporte_tea <= 3 ) {
            texto_mini = g_strdup_printf( "%s (Nível %u)", texto_check, ( guint )ficha->atipico.nivel_suporte_tea );
         } else {
            texto_mini = g_strdup( texto_check );
         }

         GtkWidget *mini_card = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 8 );
         gtk_style_context_add_class( gtk_widget_get_style_context( mini_card ), "mini-card-flowbox" );

         GtkWidget *lbl_atip = gtk_label_new( texto_mini );
         gtk_label_set_xalign( GTK_LABEL( lbl_atip ), 0.0F );
         gtk_label_set_ellipsize( GTK_LABEL( lbl_atip ), PANGO_ELLIPSIZE_END );
         gtk_box_pack_start( GTK_BOX( mini_card ), lbl_atip, TRUE, TRUE, 0 );

         // Botão minimalista alinhado à direita (.botao-remover-ifood)
         GtkWidget *btn_remover = gtk_button_new();
         gtk_button_set_relief( GTK_BUTTON( btn_remover ), GTK_RELIEF_NONE );
         gtk_widget_set_valign( btn_remover, GTK_ALIGN_CENTER );
         gtk_style_context_add_class( gtk_widget_get_style_context( btn_remover ), "botao-remover-ifood" );

         // Guarda idx_aluno (+1) e bit (+1) no GObject sem alocar memória dinâmica
         g_object_set_data( G_OBJECT( btn_remover ), "idx_aluno", GINT_TO_POINTER( ( int )i + 1 ) );
         g_object_set_data( G_OBJECT( btn_remover ), "bit_idx",   GINT_TO_POINTER( bit + 1 ) );
         g_signal_connect( btn_remover, "clicked", G_CALLBACK( on_btn_remover_mini_card_clicked_aux ), ctx );

         gtk_box_pack_end( GTK_BOX( mini_card ), btn_remover, FALSE, FALSE, 0 );
         gtk_box_pack_start( GTK_BOX( card_aluno ), mini_card, FALSE, FALSE, 0 );
      }

      // 4. Insere no FlowBox e vincula o índice do aluno ao GtkFlowBoxChild
      GtkWidget *flow_child = gtk_flow_box_child_new();
      g_object_set_data( G_OBJECT( flow_child ), "idx_aluno", GINT_TO_POINTER( ( int )i + 1 ) );
      gtk_container_add( GTK_CONTAINER( flow_child ), card_aluno );
      gtk_container_add( GTK_CONTAINER( atip->flowbox ), flow_child );

      if ( ( int )i == idx_aluno_selecionado ) {
         gtk_flow_box_select_child( GTK_FLOW_BOX( atip->flowbox ), GTK_FLOW_BOX_CHILD( flow_child ) );
      }
   }

   gtk_widget_show_all( atip->flowbox );
}

// ====================================================================================
// 5. FUNÇÕES PÚBLICAS CHAMADAS PELAS CALLBACKS
// ====================================================================================

// Chamada ao abrir a sub-aba ATIPICIDADES ou ao trocar de turma no cabeçalho
void inicializar_aba_atipicidades( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );

   // InterfaceAtipicidade *atip   = &ctx->config.atipicidade;
   GPtrArray            *fichas = ctx->diario.fichas;
   int                   limite = ( fichas != NULL ) ? ( int )fichas->len : 0;
   /*
      popular_combo_box_generico( atip->combo_alunos, fichas, limite, 0,
                                  atip->handler_combo_alunos, mapear_alunos );*/

   sincronizar_checks_com_ficha_aux( ctx, ( limite > 0 ) ? 0 : -1 );
   reconstruir_flowbox_atipicidades_aux( ctx );
}

// Executada quando o usuário seleciona outro aluno no combo_alunos_atipicos
void logica_alunos_atipicos( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );

   InterfaceAtipicidade *atip = &ctx->config.atipicidade;
   int idx_aluno = gtk_combo_box_get_active( GTK_COMBO_BOX( atip->combo_alunos ) );

   sincronizar_checks_com_ficha_aux( ctx, idx_aluno );
   destacar_card_aluno_selecionado_aux( atip, idx_aluno );
}

// Executada quando qualquer CheckButton de atipicidade é marcado/desmarcado
void logica_atipicidades( GtkWidget *widget, AppContext *ctx ) {
   g_return_if_fail( ctx != NULL && GTK_IS_TOGGLE_BUTTON( widget ) );

   InterfaceAtipicidade *atip   = &ctx->config.atipicidade;
   GPtrArray            *fichas = ctx->diario.fichas;
   if ( !fichas ) {
      return;
   }

   int idx_aluno = gtk_combo_box_get_active( GTK_COMBO_BOX( atip->combo_alunos ) );
   int bit_idx   = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( widget ), "bit_idx" ) ) - 1;

   if ( idx_aluno < 0 || ( guint )idx_aluno >= fichas->len || bit_idx < 0 || bit_idx >= MAX_FLAGS_ATIPICIDADES ) {
      return;
   }

   FichaAluno *ficha = ( FichaAluno * )g_ptr_array_index( fichas, idx_aluno );
   if ( !ficha ) {
      return;
   }

   gboolean ativo  = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) );
   guint32  bit    = ( 1U << ( guint )bit_idx );
   guint32  atuais = ( guint32 )ficha->atipico.flags;

   if ( ativo ) {
      ficha->atipico.flags = ( FlagsAtipicidade )( atuais | bit );
   } else {
      ficha->atipico.flags = ( FlagsAtipicidade )( atuais & ~bit );
   }

   // Sincroniza o combo de Nível de Suporte TEA caso o check alterado seja o ATIPICO_TEA (bit 0)
   if ( bit_idx == 0 && atip->combo_suporte_tea != NULL ) {
      if ( ativo ) {
         if ( ficha->atipico.nivel_suporte_tea == 0 ) {
            ficha->atipico.nivel_suporte_tea = 1; // Padrão inicial ao marcar TEA: Nível 1
         }
      } else {
         ficha->atipico.nivel_suporte_tea = 0;
      }

      g_signal_handler_block( atip->combo_suporte_tea, atip->handler_combo_suporte_tea );
      gtk_combo_box_set_active( GTK_COMBO_BOX( atip->combo_suporte_tea ), ( int )ficha->atipico.nivel_suporte_tea );
      gtk_widget_set_sensitive( atip->combo_suporte_tea, ativo );
      g_signal_handler_unblock( atip->combo_suporte_tea, atip->handler_combo_suporte_tea );
   }

   ficha->ficha_modificada       = TRUE;
   ctx->diario.dados_modificados = TRUE;

   reconstruir_flowbox_atipicidades_aux( ctx );
}

// Executada quando o usuário altera o Nível de Suporte TEA no combo_suporte_tea
void logica_suporte_tea( GtkWidget *widget, AppContext *ctx ) {
   g_return_if_fail( ctx != NULL && GTK_IS_COMBO_BOX( widget ) );

   InterfaceAtipicidade *atip   = &ctx->config.atipicidade;
   GPtrArray            *fichas = ctx->diario.fichas;
   if ( !fichas ) {
      return;
   }

   int idx_aluno = gtk_combo_box_get_active( GTK_COMBO_BOX( atip->combo_alunos ) );
   if ( idx_aluno < 0 || ( guint )idx_aluno >= fichas->len ) {
      return;
   }

   FichaAluno *ficha = ( FichaAluno * )g_ptr_array_index( fichas, idx_aluno );
   if ( !ficha ) {
      return;
   }

   int idx_nivel = gtk_combo_box_get_active( GTK_COMBO_BOX( widget ) );
   if ( idx_nivel < 0 || idx_nivel > 3 ) {
      idx_nivel = 0;
   }

   ficha->atipico.nivel_suporte_tea = ( guint8 )idx_nivel;

   // Se escolheu "N/A" (0), desmarca o check TEA; se escolheu Nível 1..3, garante TEA marcado
   if ( idx_nivel == 0 ) {
      ficha->atipico.flags = ( FlagsAtipicidade )( ( guint32 )ficha->atipico.flags & ~( ( guint32 )ATIPICO_TEA ) );
   } else {
      ficha->atipico.flags = ( FlagsAtipicidade )( ( guint32 )ficha->atipico.flags | ( guint32 )ATIPICO_TEA );
   }

   if ( atip->checks[0] != NULL ) {
      g_signal_handler_block( atip->checks[0], atip->handlers_check[0] );
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( atip->checks[0] ), idx_nivel > 0 );
      g_signal_handler_unblock( atip->checks[0], atip->handlers_check[0] );
   }

   ficha->ficha_modificada       = TRUE;
   ctx->diario.dados_modificados = TRUE;

   reconstruir_flowbox_atipicidades_aux( ctx );
}

