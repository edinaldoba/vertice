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



void criar_mensagem_painel( MensagemTipo MENSAGEM, InterfacePainel *painel ) {
   g_return_if_fail( painel != NULL );

   // 1. Captura individual e limpa de cada contexto de estilo
   GtkStyleContext *ctx_titulo    = gtk_widget_get_style_context( painel->titulo );
   GtkStyleContext *ctx_subtitulo = gtk_widget_get_style_context( painel->subtitulo );
   GtkStyleContext *ctx_instrucao = gtk_widget_get_style_context( painel->instrucao );

   // 2. Limpeza Absoluta: Remove qualquer rastro de estados anteriores de cada label
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

   // 3. Aplicação Reativa das Classes Baseada no Tipo da Mensagem
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

   // 4. Injeção Definitiva do Texto Puro nas Labels
   gtk_label_set_text( GTK_LABEL( painel->titulo ), painel->format_titulo );
   gtk_label_set_text( GTK_LABEL( painel->subtitulo ), painel->format_subtitulo );
   gtk_label_set_text( GTK_LABEL( painel->instrucao ), painel->format_instrucao );

   // 5. Gerenciamento e Desalocação Segura da Memória Heap
   g_free( painel->format_titulo );
   g_free( painel->format_subtitulo );
   g_free( painel->format_instrucao );

   // Aterramento dos ponteiros para evitar acessos fantasmas
   painel->format_titulo    = NULL;
   painel->format_subtitulo = NULL;
   painel->format_instrucao = NULL;
}






gchar* meu_gerador_variadico( const char *formato, ... ) {
   va_list args;
   va_start( args, formato );
   gchar *resultado = g_markup_vprintf_escaped( formato, args );
   va_end( args );
   return resultado;
}
