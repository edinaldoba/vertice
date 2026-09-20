/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdarg.h>

#include "mensagens.h"
#include "comum.h"
#include "interface.h"
#include "basicas.h"





/* =================================================================================================================
   2. VERIFICAR ESTADO DE ARQUIVO - Redução de Linhas
   ================================================================================================================= */
bool verificar_estado_de_arquivo( const char *path, InterfacePainel *painel, const InterfaceDados *dados ) {
   if ( !dados ) return false;
   const char *nome_arquivo = strrchr( path, '/' );
   nome_arquivo = ( nome_arquivo == NULL ) ? path : nome_arquivo + 1;

   char artigo = ( dados->periodo[0] == 'R' ) ? 'a' : 'o';
   bool retorno = false;

   switch ( verificar_arquivo( path ) ) {
   case ARQUIVO_INEXISTENTE:
      painel->format_titulo    = meu_gerador_variadico( "✘ Erro Crítico:" );
      painel->format_subtitulo = meu_gerador_variadico( "O arquivo '%s' não foi encontrado.", nome_arquivo );
      painel->format_instrucao = meu_gerador_variadico( "Verifique o arquivo n%c %s", artigo, dados->periodo );
      criar_mensagem_painel( ERRO, painel );
      break;

   case ARQUIVO_VAZIO:
      painel->format_titulo    = meu_gerador_variadico( "⚠ Aviso:" );
      painel->format_subtitulo = meu_gerador_variadico( "O arquivo '%s' está vazio.", nome_arquivo );
      painel->format_instrucao = meu_gerador_variadico( "Nada para processar n%c %s", artigo, dados->periodo );
      criar_mensagem_painel( AVISO, painel );
      break;

   case ARQUIVO_PRONTO:
      painel->format_titulo    = meu_gerador_variadico( "✔ Sucesso:" );
      painel->format_subtitulo = meu_gerador_variadico( "Arquivo validado. Iniciando leitura..." );
      painel->format_instrucao = meu_gerador_variadico( "Processando dados d%c %s", artigo, dados->periodo );
      criar_mensagem_painel( SUCESSO, painel );
      retorno = true;
      break;
   }

   return retorno;
}
//=================================================================================================================





//=================================================================================================================

//=================================================================================================================





void atualizar_boas_vindas( InterfacePainel *painel, const InterfaceDados *dados ) {
   char artigo = ( dados->periodo[0] == 'R' ) ? 'a' : 'o';

   painel->format_titulo    = meu_gerador_variadico( "Bem-vindo ao Vértice!" );
   painel->format_subtitulo = meu_gerador_variadico( "Sistema de Gestão Educacional - %s", dados->ano );
   painel->format_instrucao = meu_gerador_variadico( "Pronto para organizar %c %s?", artigo, dados->periodo );

   criar_mensagem_painel( SUCESSO, painel );

}


//-------------------------------------------------------------------------------
gboolean ocultar_painel_feedback_cb( gpointer user_data ) {
   InterfacePainel *painel = ( InterfacePainel * )user_data;

   if ( painel && painel->revealer_painel ) {
      // Oculta o revealer de forma suave com a animação configurada no Glade
      gtk_revealer_set_reveal_child( GTK_REVEALER( painel->revealer_painel ), FALSE );
   }

   painel->timeout_id = 0;
   return G_SOURCE_REMOVE;
}

void reexibir_ultima_mensagem( InterfacePainel *painel ) {
   g_return_if_fail( painel != NULL );
   g_return_if_fail( painel->revealer_painel != NULL );

   // 1. Zera o estado de hover para reavaliar a posição atual do ponteiro
   // painel->mouse_hover = FALSE;

   // 2. Se já houver um temporizador rodando, cancela para evitar atropelos
   if ( painel->timeout_id > 0 ) {
      g_source_remove( painel->timeout_id );
      painel->timeout_id = 0;
   }

   // 3. Garante que os widgets filhos estejam visíveis dentro da caixa do revealer
   GtkWidget *child = gtk_bin_get_child( GTK_BIN( painel->revealer_painel ) );
   if ( child ) {
      gtk_widget_show_all( child );
   }

   // 4. Faz o Toast deslizar suavemente para cima sobre o overlay
   gtk_revealer_set_reveal_child( GTK_REVEALER( painel->revealer_painel ), TRUE );

   // 5. Agenda o recolhimento automático para daqui a 5 segundos (5000 ms)
   painel->timeout_id = g_timeout_add( 5000, ocultar_painel_feedback_cb, painel );
}

void criar_mensagem_painel( MensagemTipo MENSAGEM, InterfacePainel *painel ) {
   g_return_if_fail( painel != NULL );
   g_return_if_fail( painel->revealer_painel != NULL );

   // 1. Obtém os contextos de estilo CSS das labels
   GtkStyleContext *ctx_titulo    = gtk_widget_get_style_context( painel->titulo );
   GtkStyleContext *ctx_subtitulo = gtk_widget_get_style_context( painel->subtitulo );
   GtkStyleContext *ctx_instrucao = gtk_widget_get_style_context( painel->instrucao );

   // 2. Limpeza de classes anteriores
   gtk_style_context_remove_class( ctx_titulo, "sucesso-titulo" );
   gtk_style_context_remove_class( ctx_titulo, "aviso-titulo" );
   gtk_style_context_remove_class( ctx_titulo, "erro-titulo" );
   gtk_style_context_remove_class( ctx_titulo, "info-titulo" );

   gtk_style_context_remove_class( ctx_subtitulo, "sucesso-subtitulo" );
   gtk_style_context_remove_class( ctx_subtitulo, "aviso-subtitulo" );
   gtk_style_context_remove_class( ctx_subtitulo, "erro-subtitulo" );
   gtk_style_context_remove_class( ctx_subtitulo, "info-subtitulo" );

   gtk_style_context_remove_class( ctx_instrucao, "sucesso-instrucao" );
   gtk_style_context_remove_class( ctx_instrucao, "aviso-instrucao" );
   gtk_style_context_remove_class( ctx_instrucao, "erro-instrucao" );
   gtk_style_context_remove_class( ctx_instrucao, "info-instrucao" );

   // 3. Aplicação das novas classes conforme o tipo da mensagem
   switch ( MENSAGEM ) {
   case AVISO:
      gtk_style_context_add_class( ctx_titulo,    "aviso-titulo" );
      gtk_style_context_add_class( ctx_subtitulo, "aviso-subtitulo" );
      gtk_style_context_add_class( ctx_instrucao, "aviso-instrucao" );
      break;

   case ERRO:
      gtk_style_context_add_class( ctx_titulo,    "erro-titulo" );
      gtk_style_context_add_class( ctx_subtitulo, "erro-subtitulo" );
      gtk_style_context_add_class( ctx_instrucao, "erro-instrucao" );
      break;

   case SUCESSO:
      gtk_style_context_add_class( ctx_titulo,    "sucesso-titulo" );
      gtk_style_context_add_class( ctx_subtitulo, "sucesso-subtitulo" );
      gtk_style_context_add_class( ctx_instrucao, "sucesso-instrucao" );
      break;

   case INFO:
   default:
      gtk_style_context_add_class( ctx_titulo,    "info-titulo" );
      gtk_style_context_add_class( ctx_subtitulo, "info-subtitulo" );
      gtk_style_context_add_class( ctx_instrucao, "info-instrucao" );
      break;
   }

   // 4. Injeção dos textos formatados
   gtk_label_set_text( GTK_LABEL( painel->titulo ), painel->format_titulo );
   gtk_label_set_text( GTK_LABEL( painel->subtitulo ), painel->format_subtitulo );
   gtk_label_set_text( GTK_LABEL( painel->instrucao ), painel->format_instrucao );

   // 5. Liberação de memória das strings temporárias
   g_free( painel->format_titulo );
   g_free( painel->format_subtitulo );
   g_free( painel->format_instrucao );

   painel->format_titulo    = NULL;
   painel->format_subtitulo = NULL;
   painel->format_instrucao = NULL;

   // 6. Exibição e temporização
   // Garante que o conteúdo interno do revealer esteja visível
   gtk_widget_show_all( gtk_bin_get_child( GTK_BIN( painel->revealer_painel ) ) );

   // Inicia a animação de deslizar para cima sobre o overlay
   gtk_revealer_set_reveal_child( GTK_REVEALER( painel->revealer_painel ), TRUE );

   // Reinicia o timer caso uma nova mensagem seja disparada em sequência
   if ( painel->timeout_id > 0 ) {
      g_source_remove( painel->timeout_id );
   }

   // Mensagens de erro permanecem mais tempo na tela para leitura
   guint tempo_exibicao = ( MENSAGEM == ERRO ) ? 6000 : 4000;
   painel->timeout_id = g_timeout_add( tempo_exibicao, ocultar_painel_feedback_cb, painel );
}
//-------------------------------------------------------------------------------





gchar* meu_gerador_variadico( const char *formato, ... ) {
   va_list args;
   va_start( args, formato );
   gchar *resultado = g_markup_vprintf_escaped( formato, args );
   va_end( args );
   return resultado;
}
