/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <gtk/gtk.h>
#include <string.h>
#include "config.h"
#include "comum.h"
#include "auxiliar.h"
#include "diarios.h"
#include "interface.h"
#include "tex.h"
#include "glib_gio.h"
#include "mensagens.h"

// ========================================================================
// ROLAGEM AUTOMÁTICA (SCROLL-TO-VIEW) PARA O CARD DESTACADO
// ========================================================================
static void rolagem_automatica_scrolled_window_atipicidades_lc( GtkWidget *flowbox, GtkWidget *card_para_rolar ) {
   if ( card_para_rolar == NULL ) {
      return;
   }

   // 1. Busca a janela de rolagem ancestral que envolve o FlowBox
   GtkWidget *scrolled_window = gtk_widget_get_ancestor( flowbox, GTK_TYPE_SCROLLED_WINDOW );

   if ( scrolled_window ) {
      GtkAdjustment *v_adj = gtk_scrolled_window_get_vadjustment( GTK_SCROLLED_WINDOW( scrolled_window ) );
      gint y_pos;

      // 2. Traduz a coordenada do card para a coordenada relativa ao FlowBox
      if ( gtk_widget_translate_coordinates( card_para_rolar, flowbox, 0, 0, NULL, &y_pos ) ) {
         GtkAllocation alloc;
         gtk_widget_get_allocation( card_para_rolar, &alloc );

         // 3. Ajusta a barra vertical para garantir que o topo (y_pos)
         // e a base (y_pos + altura) do card estejam perfeitamente visíveis
         gtk_adjustment_clamp_page( v_adj, y_pos, y_pos + alloc.height );
      }
   }
}


// ====================================================================================
// 1. DESTAQUE VISUAL DO CARD DO ALUNO SELECIONADO NO FLOWBOX
// ====================================================================================
static void destacar_card_aluno_selecionado_lc( InterfaceAtipicidade *atip, int idx_aluno_ativo ) {
   g_return_if_fail( atip != NULL && GTK_IS_FLOW_BOX( atip->flowbox ) );

   GList *children = gtk_container_get_children( GTK_CONTAINER( atip->flowbox ) );
   GtkWidget *card_para_rolar = NULL;

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
         card_para_rolar = flow_child; // Guarda a referência do card que precisa ficar visível
      } else {
         gtk_style_context_remove_class( sc, "card-atipico-destaque" );
      }
   }
   g_list_free( children );

   rolagem_automatica_scrolled_window_atipicidades_lc( atip->flowbox, card_para_rolar );
}

// ====================================================================================
// 2. SINCRONIZAÇÃO DOS CHECKBUTTONS E COMBO TEA PARA O ALUNO ATUAL DO COMBO
// ====================================================================================
static void sincronizar_checks_com_ficha_lc( AppContext *ctx, int idx_aluno ) {
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
static void reconstruir_flowbox_atipicidades_lc( AppContext *ctx );

// ====================================================================================
// 3. CALLBACK DO BOTÃO DE REMOVER MINI-CARD (.botao-remover-ifood)
// ====================================================================================
static void on_btn_remover_mini_card_clicked_lc( GtkWidget *btn, gpointer user_data ) {
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

   sincronizar_checks_com_ficha_lc( ctx, idx_aluno );
   reconstruir_flowbox_atipicidades_lc( ctx );
}

// ====================================================================================
// 4. CONSTRUÇÃO DOS CARDS E MINI-CARDS NO FLOWBOX
// ====================================================================================
static void reconstruir_flowbox_atipicidades_lc( AppContext *ctx ) {
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
      GtkWidget *card_aluno = gtk_box_new( GTK_ORIENTATION_VERTICAL, 9 );

      // FIX: Impede que o card seja esticado verticalmente pelo GTK
      gtk_widget_set_vexpand( card_aluno, FALSE );
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
         gtk_style_context_add_class( gtk_widget_get_style_context( btn_remover ), "btn-stepper" );
         gtk_widget_set_size_request( btn_remover, 34, 34 );

         // Guarda idx_aluno (+1) e bit (+1) no GObject sem alocar memória dinâmica
         g_object_set_data( G_OBJECT( btn_remover ), "idx_aluno", GINT_TO_POINTER( ( int )i + 1 ) );
         g_object_set_data( G_OBJECT( btn_remover ), "bit_idx",   GINT_TO_POINTER( bit + 1 ) );
         g_signal_connect( btn_remover, "clicked", G_CALLBACK( on_btn_remover_mini_card_clicked_lc ), ctx );

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
void mapear_sub_aba_atipicidades( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );
   InterfaceAtipicidade *atip = &ctx->config.atipicidade;

   // Não esquecer que ( ctx->diario.fichas->len == ctx->dados.qtd_alunos_total ) = TRUE
   sincronizar_checks_com_ficha_lc( ctx, ( ctx->diario.fichas->len > 0U ) ? 0 : -1 );
   reconstruir_flowbox_atipicidades_lc( ctx );

   GtkWidget *scrolled_window = gtk_widget_get_ancestor( atip->flowbox, GTK_TYPE_SCROLLED_WINDOW );
   GtkAdjustment *vadjustment = gtk_scrolled_window_get_vadjustment( GTK_SCROLLED_WINDOW( scrolled_window ) );

   if ( vadjustment ) {
      // Define o valor da rolagem para o topo absoluto (0.0 ou o lower do adjustment)
      gtk_adjustment_set_value( vadjustment, 0.0 );
   }
}

// Executada quando o usuário seleciona outro aluno no combo_alunos_atipicos
void logica_alunos_atipicos( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );

   InterfaceAtipicidade *atip = &ctx->config.atipicidade;
   int idx_aluno = gtk_combo_box_get_active( GTK_COMBO_BOX( atip->combo_alunos ) );

   sincronizar_checks_com_ficha_lc( ctx, idx_aluno );
   destacar_card_aluno_selecionado_lc( atip, idx_aluno );
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

   reconstruir_flowbox_atipicidades_lc( ctx );
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

   reconstruir_flowbox_atipicidades_lc( ctx );
}


// ====================================================================================
// FUNÇÃO AUXILIAR DE DIÁLOGO DE ERRO NATIVO (GTK3)
// ====================================================================================
static void exibir_erro_relatorio_aux( GtkWindow   *parent,
                                       const gchar *titulo,
                                       const gchar *detalhe ) {
   g_warning( "Vértice [%s]: %s", titulo, detalhe );

   GtkWidget *dialog = gtk_message_dialog_new(
                          parent,
                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                          GTK_MESSAGE_ERROR,
                          GTK_BUTTONS_OK,
                          "%s", titulo
                       );

   if ( detalhe != NULL && *detalhe != '\0' ) {
      gtk_message_dialog_format_secondary_text( GTK_MESSAGE_DIALOG( dialog ), "%s", detalhe );
   }

   gtk_window_set_title( GTK_WINDOW( dialog ), "Vértice — Erro na Geração de Documento" );
   gtk_dialog_run( GTK_DIALOG( dialog ) );
   gtk_widget_destroy( dialog );
}



// =====================================================================
// 1. VALIDAÇÃO DAS REGRAS DE ENTRADA
// =====================================================================
static gboolean validar_termo_busca( InterfacePainel *painel, const gchar *texto_busca,
                                     gsize *out_len, gboolean *out_tudo_alfabetico ) {
   *out_len = 0U;
   *out_tudo_alfabetico = TRUE;

   if ( !texto_busca || *texto_busca == '\0' ) {
      return FALSE;
   }

   gsize len = strlen( texto_busca );
   *out_len = len;

   if ( len < 2U ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Termo Muito Curto" );
      painel->format_subtitulo = meu_gerador_variadico( "A pesquisa exige pelo menos 2 caracteres para ser realizada." );
      painel->format_instrucao = meu_gerador_variadico( "Digite uma sigla válida (ex: TEA, TDAH) ou o código CID (ex: F84) e tente novamente." );
      criar_mensagem_painel( AVISO, painel );
      return FALSE;
   }

   gboolean apenas_numeros = TRUE;
   gboolean tem_digito = FALSE;
   gboolean tudo_alfabetico = TRUE;
   gboolean tem_especial = FALSE; // Nova flag para detectar caracteres especiais

   for ( gsize i = 0U; i < len; i++ ) {
      char c = texto_busca[i];

      if ( g_ascii_isdigit( c ) ) {
         tem_digito = TRUE;
      } else {
         apenas_numeros = FALSE;
      }

      if ( !g_ascii_isalpha( c ) ) {
         tudo_alfabetico = FALSE;
      }

      // Se não for letra, não for número e não for espaço, é considerado caractere especial
      if ( !g_ascii_isalnum( c ) && ( c != '.' ) ) {
         tem_especial = TRUE;
      }
   }

   *out_tudo_alfabetico = tudo_alfabetico;

   // Bloqueia a busca imediatamente se houver caractere especial
   if ( tem_especial ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Termo Inválido" );
      painel->format_subtitulo = meu_gerador_variadico( "A busca não pode conter caracteres especiais ou espaços." );
      painel->format_instrucao = meu_gerador_variadico( "Remova símbolos, acentos ou pontuações e tente novamente." );
      criar_mensagem_painel( AVISO, painel );
      return FALSE;
   }

   if ( apenas_numeros ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Termo Inválido" );
      painel->format_subtitulo = meu_gerador_variadico( "A busca não pode conter apenas números." );
      painel->format_instrucao = meu_gerador_variadico( "Digite termos descritivos ou siglas alfabéticas válidas." );
      criar_mensagem_painel( AVISO, painel );
      return FALSE;
   }

   if ( len == 2U && tem_digito ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Termo Inválido" );
      painel->format_subtitulo = meu_gerador_variadico( "Buscas de 2 caracteres não podem conter números." );
      painel->format_instrucao = meu_gerador_variadico( "Insira uma sigla alfabética válida (ex: TEA)." );
      criar_mensagem_painel( AVISO, painel );
      return FALSE;
   }

   return TRUE;
}

// =====================================================================
// 2. AVALIAÇÃO DE COINCIDÊNCIA (Com strcasecmp para busca curta)
// =====================================================================
static gboolean verificar_match_atipicidade( GtkWidget *chk, const gchar *busca_lower, gboolean busca_curta_alfabetica ) {
   if ( chk == NULL || !GTK_IS_CHECK_BUTTON( chk ) ) {
      return FALSE;
   }

   const gchar *label = gtk_button_get_label( GTK_BUTTON( chk ) );
   g_autofree gchar *tooltip = gtk_widget_get_tooltip_text( chk );

   if ( busca_curta_alfabetica ) {
      // Verifica correspondência exata (case-insensitive) no prefixo do label
      if ( label != NULL ) {
         const char *hyphen = strchr( label, '-' );
         g_autofree gchar *prefixo_label = NULL;
         if ( hyphen != NULL ) {
            prefixo_label = g_strndup( label, ( gsize )( hyphen - label ) );
         } else {
            prefixo_label = g_strndup( label, 5U );
         }
         g_strstrip( prefixo_label );

         // Se o prefixo contiver '/', faz correspondência cruzada dividindo as partes
         if ( strchr( prefixo_label, '/' ) != NULL ) {
            g_auto( GStrv ) partes = g_strsplit_set( prefixo_label, "/", -1 );
            for ( int p = 0; partes[p] != NULL; p++ ) {
               g_autofree gchar *parte_limpa = g_strdup( partes[p] );
               g_strstrip( parte_limpa );
               if ( g_ascii_strcasecmp( parte_limpa, busca_lower ) == 0 ) {
                  puts( parte_limpa );
                  return TRUE;
               }
            }
         } else {
            if ( g_ascii_strcasecmp( prefixo_label, busca_lower ) == 0 ) {
               return TRUE;
            }
         }
      }

      // Verifica correspondência no prefixo do tooltip (com suporte a '/' e múltiplos códigos)
      if ( tooltip != NULL ) {
         const char *paren = strchr( tooltip, '(' );
         g_autofree gchar *prefixo_tooltip = NULL;
         if ( paren != NULL ) {
            prefixo_tooltip = g_strndup( tooltip, ( gsize )( paren - tooltip ) );
         } else {
            prefixo_tooltip = g_strndup( tooltip, 5U );
         }
         g_strstrip( prefixo_tooltip );

         if ( strchr( prefixo_tooltip, '/' ) != NULL ) {
            g_auto( GStrv ) partes = g_strsplit_set( prefixo_tooltip, "/", -1 );
            for ( int p = 0; partes[p] != NULL; p++ ) {
               g_autofree gchar *parte_limpa = g_strdup( partes[p] );
               g_strstrip( parte_limpa );
               if ( g_ascii_strcasecmp( parte_limpa, busca_lower ) == 0 ) {
                  return TRUE;
               }
            }
         } else {
            if ( g_ascii_strcasecmp( prefixo_tooltip, busca_lower ) == 0 ) {
               return TRUE;
            }
         }
      }

   } else {
      // Varredura padrão global (substring search) para termos maiores ou alfanuméricos
      if ( label != NULL ) {
         g_autofree gchar *label_lower = g_ascii_strdown( label, -1 );
         if ( strstr( label_lower, busca_lower ) != NULL ) {
            return TRUE;
         }
      }

      if ( tooltip != NULL ) {
         g_autofree gchar *tooltip_lower = g_ascii_strdown( tooltip, -1 );
         if ( strstr( tooltip_lower, busca_lower ) != NULL ) {
            return TRUE;
         }
      }
   }

   return FALSE;
}

// =====================================================================
// 3. FUNÇÃO ORQUESTRADORA PRINCIPAL
// =====================================================================
void localizar_cid_atipicidade( InterfacePainel *painel, const InterfaceAtipicidade *atip ) {
   g_return_if_fail( painel != NULL );
   g_return_if_fail( atip != NULL );
   g_return_if_fail( atip->entry != NULL && GTK_IS_ENTRY( atip->entry ) );

   const gchar *texto_busca = gtk_entry_get_text( GTK_ENTRY( atip->entry ) );
   gsize len = 0U;
   gboolean tudo_alfabetico = TRUE;

   // Executa as validações estruturadas e extrai os metadados em passada única
   if ( !validar_termo_busca( painel, texto_busca, &len, &tudo_alfabetico ) ) {
      return;
   }

   gboolean busca_curta_alfabetica = ( len >= 2U && len <= 4U && tudo_alfabetico );

   // Prepara a string de busca em minúsculas e remove espaços
   g_autofree gchar *busca_lower = g_ascii_strdown( texto_busca, -1 );
   g_strstrip( busca_lower );

   if ( *busca_lower == '\0' ) {
      return;
   }

   gboolean encontrado = FALSE;
   const gchar *label_encontrado = NULL;

   // Varre os check buttons registrados
   for ( int i = 0; i < MAX_FLAGS_ATIPICIDADES; i++ ) {
      GtkWidget *chk = atip->checks[i];
      if ( verificar_match_atipicidade( chk, busca_lower, busca_curta_alfabetica ) ) {
         label_encontrado = gtk_button_get_label( GTK_BUTTON( chk ) );
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( chk ), TRUE );
         encontrado = TRUE;
         break;
      }
   }

   // Feedback visual explícito no painel de mensagens
   if ( encontrado ) {
      painel->format_titulo    = meu_gerador_variadico( "✔ Atipicidade Vinculada" );
      painel->format_subtitulo = meu_gerador_variadico( "%s", label_encontrado ? label_encontrado : "Condição Atípica" );
      painel->format_instrucao = meu_gerador_variadico( "A ficha do estudante foi atualizada com sucesso." );

      criar_mensagem_painel( SUCESSO, painel );
      gtk_entry_set_text( GTK_ENTRY( atip->entry ), "" );
   } else {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Termo Não Encontrado" );
      painel->format_subtitulo = meu_gerador_variadico( "Nenhuma condição mapeada corresponde à busca." );
      painel->format_instrucao = meu_gerador_variadico( "Verifique a digitação da CID/termo '%s' e tente novamente.", texto_busca );

      criar_mensagem_painel( AVISO, painel );
   }
}


// ====================================================================================
// PIPELINE COMPLETO DO RELATÓRIO DE ATIPICIDADES COM TRATAMENTO DE FALHAS
// ====================================================================================
void relatorio_atipicidades( const AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );

   const CaminhoDiretorio *caminho = &ctx->caminho;
   GtkWindow *janela_pai = GTK_IS_WINDOW( ctx->window )
                           ? GTK_WINDOW( ctx->window )
                           : NULL;

   const gchar *nome_base = "cards_atipicidades";
   g_autofree gchar *nome_base_tex = g_strconcat( nome_base, ".tex", NULL );
   g_autofree gchar *arq_tex       = g_build_filename( caminho->temp, nome_base_tex, NULL );

   // 1. Geração do código-fonte LaTeX/TikZ
   if ( !gerar_latex_cards_atipicidades( ctx, arq_tex ) ) {
      exibir_erro_relatorio_aux(
         janela_pai,
         "Falha ao gerar o arquivo LaTeX dos cartões de atipicidades.",
         "Verifique se há espaço em disco e permissão de escrita no diretório temporário do Vértice."
      );
      return;
   }

   // 2. Compilação serial via pdflatex
   g_pdflatex_serial( caminho->temp, arq_tex );

   g_autofree gchar *nome_base_pdf = g_strconcat( nome_base, ".pdf", NULL );
   g_autofree gchar *orig_arq_pdf = g_build_filename( caminho->temp, nome_base_pdf, NULL );

   // Valida se o PDF realmente foi produzido pelo pdflatex antes de prosseguir
   if ( !g_file_test( orig_arq_pdf, G_FILE_TEST_IS_REGULAR ) ) {
      exibir_erro_relatorio_aux(
         janela_pai,
         "Falha na compilação do documento PDF.",
         "O pdflatex não gerou o arquivo final. Verifique o log de compilação ou a instalação do pacote professor.sty."
      );
      apagar_temporarios_latex_serial( caminho->temp, nome_base, -1 );
      return;
   }

   // 3. Cópia para o diretório definitivo de relatórios e limpeza
   g_autofree gchar *dest_arq_pdf = g_build_filename( caminho->relatorios, nome_base_pdf, NULL );

   if ( !gio_copiar_arquivo( orig_arq_pdf, dest_arq_pdf ) ) {
      exibir_erro_relatorio_aux(
         janela_pai,
         "Erro ao salvar o relatório na pasta de destino.",
         "Não foi possível copiar o PDF compilado para o diretório de relatórios."
      );
      apagar_temporarios_latex_serial( caminho->temp, nome_base, -1 );
      return;
   }

   apagar_temporarios_latex_serial( caminho->temp, nome_base, -1 );

   // 4. Abertura no visualizador de PDF padrão do sistema
   g_xdg_open( dest_arq_pdf );
}

