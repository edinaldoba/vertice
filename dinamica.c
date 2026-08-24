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







//==================================================================================================
static void destruir_widget_callback( GtkWidget *widget, gpointer data ) {
   ( void )data; // Evita aviso de variável não utilizada
   gtk_widget_destroy( widget );
}
//==================================================================================================
void limpar_container( GtkWidget *container ) {
   if ( !container ) return;

   // Passamos a própria função de destruição do GTK como callback!
   gtk_container_foreach( GTK_CONTAINER( container ), destruir_widget_callback, NULL );
}
//==================================================================================================







//==================================================================================================
static void atualizar_listbox_subtemas_latex( InterfaceLatex *latex, InterfaceListas *listas, LimitesFiltro *limite ) {
   if ( !latex || !listas || !limite ) return;

   // 2. Limpa os elementos antigos de dentro do contêiner
   limpar_container( latex->listbox_subtemas );

   // 3. Varre os subtemas do tema selecionado e monta a listagem estilizada
   for ( int i = 0; i < limite->subtemas; i++ ) {

      // Cria a linha da lista (Row Container) - Este sim é um Box Horizontal
      GtkWidget *linha_box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 8 );

      // A. O INDICADOR ELEGANTE (Marcador visual estilizado)
      GtkWidget *indicador_label = gtk_label_new( "›" );
      gtk_style_context_add_class( gtk_widget_get_style_context( indicador_label ), "indicador-item-subtema" );

      // B. O RÓTULO DO SUBTEMA
      GtkWidget *texto_label = gtk_label_new( listas->subtemas[i].str );

      // Monta os elementos internos dentro da linha_box
      gtk_box_pack_start( GTK_BOX( linha_box ), indicador_label, FALSE, FALSE, 0 );
      gtk_box_pack_start( GTK_BOX( linha_box ), texto_label, FALSE, FALSE, 0 );

      // C. CORREÇÃO: Adiciona a linha customizada ao GtkListBox ou GtkContainer genérico
      gtk_container_add( GTK_CONTAINER( latex->listbox_subtemas ), linha_box );
   }

   // Força o redesenho e exibição de todos os novos widgets injetados
   gtk_widget_show_all( latex->listbox_subtemas );
}
//==================================================================================================





//==================================================================================================
static void comparar_nome_do_filho( GtkWidget *container_child, gpointer user_data ) {
   BuscaWidgetData *data = ( BuscaWidgetData * )user_data;

   // Se já encontramos o widget em outra iteração, interrompe o processamento
   if ( data->encontrado != NULL ) return;

   // gtk_bin_get_child funciona tanto para GtkFlowBoxChild quanto para GtkListBoxRow
   GtkWidget *widget_interno = gtk_bin_get_child( GTK_BIN( container_child ) );

   if ( widget_interno && g_strcmp0( gtk_widget_get_name( widget_interno ), data->nome_alvo ) == 0 ) {
      data->encontrado = widget_interno;
   }
}
//==================================================================================================
static GtkWidget* buscar_widget_por_nome_interno( GtkWidget *container, const char *nome_alvo ) {
   if ( !container || !nome_alvo ) return NULL;

   BuscaWidgetData data = { .nome_alvo = nome_alvo, .encontrado = NULL };

   gtk_container_foreach( GTK_CONTAINER( container ), comparar_nome_do_filho, &data );

   return data.encontrado;
}
//==================================================================================================





//==================================================================================================
void atualizar_listbox_subtemas( AppContext *ctx ) {
   if ( !ctx ) return;

   InterfaceListas   *listas = &ctx->listas;
   InterfaceEntry    *entry  = &ctx->entry;
   InterfaceDinamica *provas = &ctx->provas;
   LimitesFiltro     *limite = &ctx->cascata.limite;
   FocoCoordenadas   *foco   = &ctx->cascata.foco;

   // 1. Limpa a listagem do tema anterior na esquerda
   limpar_container( provas->listbox_subtemas );

   char *tema_ativo = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( entry->tema ) );
   if ( !tema_ativo ) return;


   int f = foco->tema;
   free( provas->handler[f] );
   provas->handler[f] = ( gulong* ) calloc( limite->subtemas, sizeof( gulong ) );

   // 2. Injeta os novos CheckButtons baseados no diretório carregado
   for ( int i = 0; i < limite->subtemas; i++ ) {
      GtkWidget *check = gtk_check_button_new_with_label( listas->subtemas[i].str );

      // O nome do widget agora fica limpo, apenas o nome do subtema
      gtk_widget_set_name( check, listas->subtemas[i].str );

      // A MÁGICA AQUI: Penduramos o índice 'i' invisivelmente no botão
      g_object_set_data( G_OBJECT( check ), "id_widget", GINT_TO_POINTER( i ) );

      char checa_id[128];
      snprintf( checa_id, sizeof( checa_id ), "%s/%s", tema_ativo, listas->subtemas[i].str );

      // Procura no FlowBox se esse cara já está selecionado
      GtkWidget *box_encontrado = buscar_widget_por_nome_interno( provas->flowbox_selecionados, checa_id );
      if ( box_encontrado != NULL ) {
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check ), TRUE );
      }

      provas->handler[f][i] = g_signal_connect( check, "toggled", G_CALLBACK( on_subtema_check_toggled ), ctx );
      gtk_list_box_insert( GTK_LIST_BOX( provas->listbox_subtemas ), check, -1 );
   }

   g_free( tema_ativo );
   gtk_widget_show_all( provas->listbox_subtemas );

   atualizar_listbox_subtemas_latex( &ctx->latex, listas, limite );

}
//==================================================================================================











// Fábrica que monta o widget do mini-card e devolve pronto para ser inserido
static GtkWidget* criar_mini_card_stepper( InterfaceDinamica *provas, const char *tag_id, int qtd_tex, int id_widget ) {

   // --- 1. CONTAINER DA TAG PRINCIPAL ---
   GtkWidget *tag_box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 12 );
   gtk_widget_set_vexpand( tag_box, FALSE );
   gtk_widget_set_valign( tag_box, GTK_ALIGN_START );
   gtk_widget_set_name( tag_box, tag_id );
   gtk_style_context_add_class( gtk_widget_get_style_context( tag_box ), "mini-card-flowbox" );

   GtkWidget *label_nome = gtk_label_new( tag_id );
   gtk_box_pack_start( GTK_BOX( tag_box ), label_nome, FALSE, FALSE, 4 );

   // --- 2. O CONTROLE COMPACTO (Estilo iFood) (BOX flexível no formato de pílula) ---
   GtkWidget *stepper_box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 6 );
   gtk_style_context_add_class( gtk_widget_get_style_context( stepper_box ), "stepper-container" );

   provas->btn_menos = gtk_button_new();
   gtk_button_set_relief( GTK_BUTTON( provas->btn_menos ), GTK_RELIEF_NONE );
   gtk_style_context_add_class( gtk_widget_get_style_context( provas->btn_menos ), "btn-stepper" );
   if ( qtd_tex == 1 ) {
      gtk_style_context_add_class( gtk_widget_get_style_context( provas->btn_menos ), "botao-remover-ifood" );
   } else {
      gtk_button_set_label( GTK_BUTTON( provas->btn_menos ), "－" );
   }



   char str_qtd[16];
   snprintf( str_qtd, sizeof( str_qtd ), "%d", qtd_tex );
   GtkWidget *label_qtd = gtk_label_new( str_qtd );
   gtk_style_context_add_class( gtk_widget_get_style_context( label_qtd ), "label-stepper" );

   provas->btn_mais = gtk_button_new_with_label( "＋" );
   gtk_button_set_relief( GTK_BUTTON( provas->btn_mais ), GTK_RELIEF_NONE );
   gtk_style_context_add_class( gtk_widget_get_style_context( provas->btn_mais ), "btn-stepper" );

   // Armazena as referências na tag_box
   g_object_set_data( G_OBJECT( tag_box ), "label_quantidade", label_qtd );
   g_object_set_data( G_OBJECT( tag_box ), "btn_menos", provas->btn_menos );
   g_object_set_data( G_OBJECT( tag_box ), "limite_max", GINT_TO_POINTER( qtd_tex ) );

   // IDs e Callbacks
   g_object_set_data( G_OBJECT( provas->btn_menos ), "tag_id_origem", g_strdup( tag_id ) );
   g_object_set_data( G_OBJECT( provas->btn_menos ), "tag_pai", tag_box );
   g_object_set_data( G_OBJECT( provas->btn_mais ), "tag_pai", tag_box );
   g_object_set_data( G_OBJECT( provas->btn_menos ), "id_widget_origem", GINT_TO_POINTER( id_widget ) );

   gtk_box_pack_start( GTK_BOX( stepper_box ), provas->btn_menos, FALSE, FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( stepper_box ), label_qtd, FALSE, FALSE, 4 );
   gtk_box_pack_start( GTK_BOX( stepper_box ), provas->btn_mais,  FALSE, FALSE, 0 );

   gtk_box_pack_end( GTK_BOX( tag_box ), stepper_box, FALSE, FALSE, 4 );

   gtk_widget_set_size_request( tag_box, -1, 44 ); // mini-card
   gtk_widget_set_size_request( provas->btn_mais, 34, 34 );
   gtk_widget_set_size_request( provas->btn_menos, 34, 34 );

   return tag_box;
}





// Função que desembrulha as propriedades "escondidas" no botão
static EstadoStepper extrair_estado_stepper( GtkWidget *button ) {
   EstadoStepper st = {0};

   st.tag_box = GTK_WIDGET( g_object_get_data( G_OBJECT( button ), "tag_pai" ) );
   if ( !st.tag_box ) return st; // Proteção

   st.label_qtd  = GTK_WIDGET( g_object_get_data( G_OBJECT( st.tag_box ), "label_quantidade" ) );
   st.btn_menos  = GTK_WIDGET( g_object_get_data( G_OBJECT( st.tag_box ), "btn_menos" ) );
   st.limite_max = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( st.tag_box ), "limite_max" ) );

   if ( st.label_qtd ) {
      st.valor_atual = atoi( gtk_label_get_text( GTK_LABEL( st.label_qtd ) ) );
   }

   return st;
}






static void sincronizar_vetor_com_flowbox( InterfaceDados *dados, const InterfaceDinamica *provas ) {
   if ( !dados || !provas ) return;

   // 1. Limpeza inicial completa das NTI (10) posições do vetor de dados
   for ( int i = 0; i < NTI; i++ ) {
      dados->temas_prova_sequencia[i].str[0] = '\0';
      dados->qtd_questoes[i] = 0;
   }

   // 2. Varre as tags visuais do FlowBox e preenche o vetor linearmente de 0 a N
   GList *children = gtk_container_get_children( GTK_CONTAINER( provas->flowbox_selecionados ) );
   int idx = 0;

   for ( GList *iter = children; iter != NULL; iter = g_list_next( iter ) ) {

      GtkWidget *flow_child = GTK_WIDGET( iter->data );
      GtkWidget *tag_box = gtk_bin_get_child( GTK_BIN( flow_child ) );

      if ( tag_box ) {
         const char *tag_id = gtk_widget_get_name( tag_box );

         GtkWidget *label_qtd = g_object_get_data( G_OBJECT( tag_box ), "label_quantidade" );

         if ( tag_id && label_qtd ) {

            size_t tam = sizeof( dados->temas_prova_sequencia[idx].str );
            snprintf( dados->temas_prova_sequencia[idx].str, tam, "%s", tag_id );

            dados->qtd_questoes[idx] = atoi( gtk_label_get_text( GTK_LABEL( label_qtd ) ) );

            idx++;
         }
      }

      if ( idx == NTI ) break;
   }

   g_list_free( children );

   // puts( "\n" );
   // for ( int i = 0; i < NTI; i++ ) {
   //    printf( "%s -> %d\n", dados->temas_prova_sequencia[i].str, dados->qtd_questoes[i] );
   // }
}







// --- LÓGICA CORE: STEPPER MAIS ---
void logica_stepper_mais( GtkWidget *button, AppContext *ctx ) {
   if ( !ctx ) return;

   EstadoStepper st = extrair_estado_stepper( button );

   if ( !st.tag_box ) return;

   if ( st.valor_atual < st.limite_max ) {
      st.valor_atual++;
      char str[16];
      snprintf( str, sizeof( str ), "%d", st.valor_atual );
      gtk_label_set_text( GTK_LABEL( st.label_qtd ), str );

      if ( st.valor_atual == 2 ) {
         gtk_button_set_label( GTK_BUTTON( st.btn_menos ), "－" );
         gtk_style_context_remove_class( gtk_widget_get_style_context( st.btn_menos ), "botao-remover-ifood" );
         // gtk_style_context_add_class( gtk_widget_get_style_context( st.btn_menos ), "btn-stepper" );
      }

      sincronizar_vetor_com_flowbox( &ctx->dados, &ctx->provas );
   }
}




void logica_stepper_menos( GtkWidget *button, AppContext *ctx ) {
   if ( !ctx || !button ) return;

   EstadoStepper st = extrair_estado_stepper( button );

   if ( !st.tag_box ) return;

   InterfaceDados    *dados  = &ctx->dados;
   InterfaceDinamica *provas = &ctx->provas;
   InterfaceEntry    *entry  = &ctx->entry;
   InterfaceListas   *listas = &ctx->listas;
   LimitesFiltro     *limite = &ctx->cascata.limite;
   FocoCoordenadas   *foco   = &ctx->cascata.foco;

   if ( st.valor_atual > 1 ) {
      st.valor_atual--;
      char str[16];
      snprintf( str, sizeof( str ), "%d", st.valor_atual );
      gtk_label_set_text( GTK_LABEL( st.label_qtd ), str );

      if ( st.valor_atual == 1 ) {
         gtk_button_set_label( GTK_BUTTON( button ), "" );
         // gtk_style_context_remove_class( gtk_widget_get_style_context( button ), "btn-stepper" );
         gtk_style_context_add_class( gtk_widget_get_style_context( button ), "botao-remover-ifood" );
      }

      sincronizar_vetor_com_flowbox( dados, provas );

   } else {
      // --- ROTINA DE DESTRUIÇÃO COM FOCO AUTOMÁTICO ---
      const char *label_tema = g_object_get_data( G_OBJECT( button ), "tag_id_origem" );
      // 1. Quebra a tag "Tema/Subtema" para descobrir quem é o Tema Principal alvo
      char **pedacos = g_strsplit( label_tema, "/", 2 );
      if ( !pedacos || !pedacos[0] || !pedacos[1] ) {
         if ( pedacos ) g_strfreev( pedacos );
         return;
      }
      const char *tema_alvo = pedacos[0];
      const char *subtema_alvo = pedacos[1];

      ItemCombo chave;
      snprintf( chave.str, sizeof( chave.str ), "%s", tema_alvo );
      int t = buscar_indice_bsearch( &chave, listas->temas, limite->temas, sizeof( chave ), comparar_item_combo );

      // 3. Executa a mudança visual e aciona o gatilho de remoção do CheckBox
      if ( t != -1 ) {
         // Isso dispara a 'atualizar_listbox_subtemas' que recriará os botões.
         // Como a tag AINDA está no FlowBox, o botão nascerá como TRUE.

         int f = foco->tema;
         gtk_combo_box_set_active( GTK_COMBO_BOX( entry->tema ), t );

         GtkWidget *check_box = buscar_widget_por_nome_interno( provas->listbox_subtemas, subtema_alvo );

         if ( check_box ) {
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check_box ), FALSE );
         }

         gtk_combo_box_set_active( GTK_COMBO_BOX( entry->tema ), f );
      }

      g_strfreev( pedacos );
   }
}






void rolar_para_o_fim( GtkScrolledWindow *scrolled_window ) {
   if ( !scrolled_window ) return;

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







//==================================================================================================
static void remover_tag_no_flowbox( GtkWidget *flowbox, const char *tag_id ) {
   if ( !flowbox || !tag_id ) return;

   // 1. Localiza o box interno usando a nossa nova função genérica
   GtkWidget *box = buscar_widget_por_nome_interno( flowbox, tag_id );

   if ( box ) {
      // 2. Destrói diretamente o "pai" (o GtkFlowBoxChild), removendo-o do container
      gtk_widget_destroy( gtk_widget_get_parent( box ) );
   }
}
//==================================================================================================
void logica_subtema_toggled( GtkWidget *widget, AppContext *ctx ) {
   if ( !ctx || !widget ) return;

   CaminhoDiretorio  *caminho = &ctx->caminho;
   InterfaceEntry    *entry   = &ctx->entry;
   InterfaceDinamica *provas  = &ctx->provas;
   FocoCoordenadas   *foco    = &ctx->cascata.foco;
   InterfaceDados    *dados   = &ctx->dados;
   InterfacePainel   *painel  = &ctx->painel;

   int f = foco->tema;

   bool estado = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) );

   const char *subtema = gtk_button_get_label( GTK_BUTTON( widget ) );

   int id_widget = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( widget ), "id_widget" ) );

   if ( estado && provas->cont_add == NTI ) {

      g_signal_handler_block( widget, provas->handler[f][id_widget] );
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), FALSE );
      g_signal_handler_unblock( widget, provas->handler[f][id_widget] );

      // Alimenta os buffers variádicos e delega o estilo para o motor central (AVISO)
      painel->format_titulo    = meu_gerador_variadico( "⚠ Aviso:" );
      painel->format_subtitulo = meu_gerador_variadico( "O limite de %d subtemas adicionados foi atingido.", NTI );
      painel->format_instrucao = meu_gerador_variadico( "%s", "Verifique os temas selecionados e defina as quantidades de questões." );

      criar_mensagem_painel( AVISO, painel );

      fprintf( stderr, "[AVISO] O limite de %d subtemas adicionados foi atingido.\n", NTI );
      return;
   }

   // 1. Captura o Tema Principal ativo no momento do clique
   char *tema_ativo = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( entry->tema ) );
   if ( !tema_ativo ) {
      return;
   }

   char tag_id[128];
   snprintf( tag_id, sizeof( tag_id ), "%s/%s", tema_ativo, subtema );

   if ( estado ) {
      // --- VERIFICAÇÃO DE DUPLICIDADE NO FLOWBOX ---
      GtkWidget *box_encontrado = buscar_widget_por_nome_interno( provas->flowbox_selecionados, tag_id );

      if ( box_encontrado == NULL ) {

         snprintf( caminho->questoes_tex, sizeof( caminho->questoes_tex ), "%s/%s", caminho->banco_questoes, tag_id );
         int qtd_tex = quantidade_arquivos_por_extensao( caminho->questoes_tex, ".tex" );

         if ( qtd_tex == 0 ) {

            g_signal_handler_block( widget, provas->handler[f][id_widget] );
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), FALSE );
            g_signal_handler_unblock( widget, provas->handler[f][id_widget] );

            fprintf( stderr, "[AVISO] Subtema VAZIO! Escolha outro ou adicione novas questões.\n" );
            g_free( tema_ativo );
            return;
         }

         GtkWidget *novo_card = criar_mini_card_stepper( provas, tag_id, qtd_tex, id_widget );
         if ( novo_card != NULL ) {
            g_signal_connect( provas->btn_menos, "clicked", G_CALLBACK( on_stepper_menos_clicked ), ctx );
            g_signal_connect( provas->btn_mais, "clicked", G_CALLBACK( on_stepper_mais_clicked ), ctx );
         }

         gtk_flow_box_insert( GTK_FLOW_BOX( provas->flowbox_selecionados ), novo_card, -1 );

         // Proteção de concorrência com o Main Loop
         if ( provas->handler_scrolled > 0 && provas->importar == false ) {
            g_signal_handler_unblock( provas->scrolled_window, provas->handler_scrolled );
         }

         gtk_widget_show_all( provas->flowbox_selecionados );

         sincronizar_vetor_com_flowbox( dados, provas );

         provas->cont_add ++;
      }
   } else {
      // --- REMOVER TAG VISUAL ---
      remover_tag_no_flowbox( provas->flowbox_selecionados, tag_id );

      sincronizar_vetor_com_flowbox( dados, provas );

      provas->cont_add --;
   }

   g_free( tema_ativo );

}
//==================================================================================================






//==================================================================================================
static gboolean bloquear_sinal_com_atraso( gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return G_SOURCE_REMOVE;

   if ( ctx->provas.handler_scrolled > 0 ) {
      g_signal_handler_block( ctx->provas.scrolled_window, ctx->provas.handler_scrolled );
      ctx->provas.importar = false;
   }

   // O retorno G_SOURCE_REMOVE (ou FALSE) é VITAL.
   // Ele avisa o GTK para destruir este cronômetro e não repeti-lo.
   return G_SOURCE_REMOVE;
}
//==================================================================================================
void alimentar_interface_temporaria( AppContext *ctx, const DadosRascunho *rascunho, const FocoCoordenadas *foco_cache ) {
   if ( !ctx || !rascunho || !foco_cache ) return;

   InterfaceListas   *listas = &ctx->listas;
   InterfaceEntry    *entry  = &ctx->entry;
   LimitesFiltro     *limite = &ctx->cascata.limite;
   InterfaceDados    *dados  = &ctx->dados;
   InterfaceDinamica *provas = &ctx->provas;

   if ( provas->handler_scrolled > 0 ) {
      g_signal_handler_unblock( provas->scrolled_window, provas->handler_scrolled );
      provas->importar = true;
   }

   // =========================================================================
   // 1. LIMPEZA TOTAL
   // =========================================================================
   limpar_container( provas->listbox_subtemas );
   limpar_container( provas->flowbox_selecionados );
   provas->cont_add = 0;

   /* O sinal changed não vai disparar se o primeiro tema principal setado
    * for o mesmo tema principal selecionado na interface (proteção nativa do combo box).
    * Por isso, é obrigatório atualizar o listbox após a limpeza
    */
   atualizar_listbox_subtemas( ctx );

   // =========================================================================
   // 2. RECONSTRUÇÃO AUTOMÁTICA DISPARANDO OS SINAIS DO RASCUNHO
   // =========================================================================
   for ( int i = 0; i < NTI; i++ ) {
      if ( rascunho->temas_prova_sequencia[i].str[0] == '\0' ) continue;

      char **pedacos = g_strsplit( rascunho->temas_prova_sequencia[i].str, "/", -1 );
      char *tema_salvo = pedacos[0];
      char *subtema_salvo = pedacos[1];

      if ( !tema_salvo || !subtema_salvo ) {
         g_strfreev( pedacos );
         continue;
      }

      // 1. Acha o índice do tema_salvo no Combo Box e ativa-o
      ItemCombo chave;
      snprintf( chave.str, sizeof( chave.str ), "%s", tema_salvo );
      int t = buscar_indice_bsearch( &chave, listas->temas, limite->temas, sizeof( chave ), comparar_item_combo );

      gtk_combo_box_set_active( GTK_COMBO_BOX( entry->tema ), t );

      // 2. Localiza o CheckBox oculto na ListBox e o ativa (Gatilho para o Card)
      GtkWidget *check_box = buscar_widget_por_nome_interno( provas->listbox_subtemas, subtema_salvo );

      if ( check_box && GTK_IS_TOGGLE_BUTTON( check_box ) ) {
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check_box ), TRUE );

         // 3. Localiza o Card recém-criado no FlowBox
         GtkWidget *tag_box = buscar_widget_por_nome_interno( provas->flowbox_selecionados,
                              rascunho->temas_prova_sequencia[i].str );
         if ( tag_box ) {
            GtkWidget *label_qtd = g_object_get_data( G_OBJECT( tag_box ), "label_quantidade" );
            GtkWidget *btn_menos = g_object_get_data( G_OBJECT( tag_box ), "btn_menos" );

            if ( label_qtd ) {
               char str_qtd[16];
               snprintf( str_qtd, sizeof( str_qtd ), "%d", rascunho->qtd_questoes[i] );
               gtk_label_set_text( GTK_LABEL( label_qtd ), str_qtd );

               // Ajusta o ícone do botão se a quantidade recuperada for 1
               if ( rascunho->qtd_questoes[i] == 1 && btn_menos ) {
                  gtk_button_set_label( GTK_BUTTON( btn_menos ), "" );
                  gtk_style_context_add_class( gtk_widget_get_style_context( btn_menos ), "botao-remover-ifood" );
               }
            }
         }
      }
      g_strfreev( pedacos );
   }

   // Executa a sincronização final
   sincronizar_vetor_com_flowbox( dados, provas );

   // =========================================================================
   // 3. RESTAURAÇÃO DOS DEMAIS PARÂMETROS ESTÁTICOS DO LAYOUT
   // =========================================================================
   gtk_combo_box_set_active( GTK_COMBO_BOX( entry->cor_destaque ), foco_cache->cor_destaque );
   gtk_combo_box_set_active( GTK_COMBO_BOX( entry->decoracao_estilo ), foco_cache->decoracao_estilo );

   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->check.nao_presencial ), rascunho->naopresencial );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->check.expor_dados ),    rascunho->expor );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->radio.qtd_colunas[      rascunho->qtd_colunas - 2 ] ), TRUE );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->radio.separadores[      rascunho->separadores - 1 ] ), TRUE );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->radio.fonte_latex[      rascunho->fonte_latex - 1 ] ), TRUE );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->radio.qtd_paginas[      rascunho->qtd_paginas - 1 ] ), TRUE );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->radio.cabecalho_tipo[   rascunho->cabecalho_tipo - 1 ] ), TRUE );

   g_timeout_add( 30, bloquear_sinal_com_atraso, ctx );
}
//==================================================================================================






//==================================================================================================
// Função auxiliar interna para verificar se o arquivo oculto é lixo do sistema
static int eh_metadado_valido( const char *nome ) {
   // Ignora o diretório atual (.) e pai (..)
   if ( strcmp( nome, "." ) == 0 || strcmp( nome, ".." ) == 0 ) return 0;

   // Ignora pastas ou arquivos comuns de sistemas/ferramentas
   if ( strcmp( nome, ".git" ) == 0 ||
         strcmp( nome, ".vscode" ) == 0 ||
         strcmp( nome, ".DS_Store" ) == 0 ) return 0;

   return 1;
}
//==================================================================================================
static void preencher_painel_responsavel( const char *diretorio, GtkWidget *box, const char *prefixo, char *responsavel ) {
   // 1. Defesa de ponteiros nulos
   if ( !diretorio || !box || !responsavel ) return;

   DIR *dir = opendir( diretorio );
   if ( !dir ) {
      fprintf( stderr, "ERRO: Não foi possível abrir o diretório do professor: %s\n", diretorio );
      return;
   }

   struct dirent *dp;
   gboolean encontrado = FALSE;

   while ( ( dp = readdir( dir ) ) != NULL ) {
      if ( dp->d_name[0] == '.' && eh_metadado_valido( dp->d_name ) ) {

         int len = calcular_len_limpo( &dp->d_name[1], 20 );
         snprintf( responsavel, 64, "%.*s", len, &dp->d_name[1] );

         encontrado = TRUE;
         break;
      }
   }
   closedir( dir );

   // Fallback caso o arquivo de metadados não exista ou suma
   if ( !encontrado ) {
      g_strlcpy( responsavel, "Não Identificado", 32 );
   }

   limpar_container( box );

   GtkWidget *label_prefixo = gtk_label_new( prefixo );
   GtkWidget *label_reponsavel = gtk_label_new( responsavel );

   gtk_style_context_add_class( gtk_widget_get_style_context( label_prefixo ), "cabecalho" );

   gtk_box_pack_start( GTK_BOX( box ), label_prefixo, FALSE, FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( box ), label_reponsavel, FALSE, FALSE, 0 );

   gtk_widget_show_all( box );

}
//======================================================================================================================//
/**
 * @brief Lê o metadado do "professor" no diretório e atualiza o box visual correspondente.
 */
void professor_da_disciplina( const char *diretorio, GtkWidget *box_professor, char *professor ) {
   preencher_painel_responsavel( diretorio, box_professor, "Prof. ", professor );
}
//======================================================================================================================//
/**
 * @brief Lê o metadado do "gestor" no diretório e atualiza o box visual correspondente.
 */
void gestor_da_escola( const char *diretorio, GtkWidget *box_gestor, char *gestor ) {
   preencher_painel_responsavel( diretorio, box_gestor, "Gest. ", gestor );
}
//======================================================================================================================//
