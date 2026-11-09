/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <gtk/gtk.h>
#include <glib-unix.h>
#include <locale.h>
#include <stdlib.h>
#include <glib.h>

#include "interface.h"
#include "layout.h"
#include "signals.h"
#include "basicas.h"

#include "mensagens.h"
#include "diario.h"




// Protótipo local da função de inicialização
void inicializacao_app_context( AppContext *ctx );
void limpeza_final( AppContext *ctx );
static gboolean tratar_sinais_unix( gpointer user_data );



//=====================================================================================================//
//                               FUNÇÃO DE ATIVAÇÃO DA INTERFACE                                       //
//=====================================================================================================//
static void activate( GtkApplication *app, gpointer user_data ) {

   // CORREÇÃO: Força APENAS a formatação de números a voltar para o ponto decimal (.)
   setlocale( LC_NUMERIC, "C" );

   //--- Resgata e valida o ponteiro ctx
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) {
      g_printerr( "Erro crítico: Contexto do aplicativo inválido (NULL) na ativação.\n" );
      return;
   }

   // =========================================================================
   // 🛡️ GUARDA DE INSTÂNCIA ÚNICA: Se a janela já existe, traz para o foco e sai
   // =========================================================================
   if ( ctx->window != NULL ) {
      gtk_window_present( GTK_WINDOW( ctx->window ) );
      return;
   }

   //--- Caminho recursos.xml -----------------------------
   ctx->caminho.recursos_prefix = "/br/com/alencar/edinaldo/vertice";

   //--- Constrói a interface -----------------------------
   construir_interface( app, ctx );

   //--- Verifica se professor.sty está instalado --------------
   preparar_ambiente_latex( ctx->caminho.recursos_prefix );

   //--- Inicializa o estado inicial do aplicativo --------
   inicializar_estado_do_aplicativo( ctx );

   //--- Ativa o modo style_dark_green na interface -------
   interface_style( ctx );

   //--- Conecta os sinais da interface --------------------
   app_signals_connect( ctx );

   //--- Exibe a janela da aplicação -----------------------
   gtk_widget_show_all( ctx->window );

   //--- Exibe uma mensagem de boas vindas -----------------
   atualizar_boas_vindas( &ctx->painel, &( ctx->dados ) );
}





//=====================================================================================================//
//                                                                                                     //
//                   ███╗   ███╗  █████╗  ██╗ ███╗   ██╗                                               //
//                   ████╗ ████║ ██╔══██╗ ██║ ████╗  ██║                                               //
//                   ██╔████╔██║ ███████║ ██║ ██╔██╗ ██║                                               //
//                   ██║╚██╔╝██║ ██╔══██║ ██║ ██║╚██╗██║                                               //
//                   ██║ ╚═╝ ██║ ██║  ██║ ██║ ██║ ╚████║                                               //
//                   ╚═╝     ╚═╝ ╚═╝  ╚═╝ ╚═╝ ╚═╝  ╚═══╝                                               //
//                                                                                                     //
//=====================================================================================================//
int main( int argc, char *argv[] ) {

   // Define tudo para o padrão brasileiro (incluindo acentos e vírgula decimal)
   if ( !setlocale( LC_ALL, "pt_BR.UTF-8" ) ) {
      g_warning( "Aviso: O locale 'pt_BR.UTF-8' não está disponível no sistema. Usando padrão." );
   }

   gtk_init( &argc, &argv );

   // Inicialização estática e segura do contexto do aplicativo
   AppContext ctx;
   inicializacao_app_context( &ctx );

   // Inicialização estocástica nativa e de alta entropia da GLib
   guint32 sementes[4];
   gerar_sementes( sementes );
   g_autoptr( GRand ) rand_context = g_rand_new_with_seed_array( sementes, G_N_ELEMENTS( sementes ) );
   ctx.rand = g_steal_pointer( &rand_context );

   GtkApplication *app;
   int status;

   app = gtk_application_new( "br.com.alencar.edinaldo.vertice", G_APPLICATION_DEFAULT_FLAGS );

   if ( app != NULL ) {
      // Passa o ENDEREÇO (&ctx) do contexto para a função activate
      g_signal_connect( app, "activate", G_CALLBACK( activate ), &ctx );

      // ADICIONA A CAPTURA SEGURA DOS SINAIS DE SISTEMA (Linux/Debian)
      // SIGINT = Ctrl+C no terminal
      // SIGTERM = Pedido de fechamento do sistema (Gerenciador de Tarefas)
      g_unix_signal_add( SIGINT, tratar_sinais_unix, app );
      g_unix_signal_add( SIGTERM, tratar_sinais_unix, app );

      // O programa fica rodando aqui até a janela ser fechada OU g_application_quit ser chamado
      status = g_application_run( G_APPLICATION( app ), argc, argv );

      g_object_unref( app );
   } else {
      g_printerr( "Erro crítico: Não foi possível instanciar o objeto GtkApplication.\n" );
      status = EXIT_FAILURE;
   }

   // HIGIENE DE MEMÓRIA
   // Removi o bloco do ctx.provider daqui, pois ele já está dentro da sua limpeza_final()!
   limpeza_final( &ctx );

   return status;
}





//=====================================================================================================//
// FUNÇÃO DE INICIALIZAÇÃO O(1) VIA COMPOUND LITERAL C99
//=====================================================================================================//
void inicializacao_app_context( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );

   *ctx = ( AppContext ) {
      .cabecalho = {
         .anos               = NULL,
         .escolas            = NULL,
         .turmas             = NULL,
         .disciplinas        = NULL,

         .handler_ano        = 0,
         .handler_escola     = 0,
         .handler_turma      = 0,
         .handler_disciplina = 0,
         .foco_estilo        = 0
      },

      // 1. O Provedor CSS Global
      .provider = NULL,

      // 2. Inicialização do Estado Nativo da RAM (InterfaceDados)
      .dados = {
         .raiz               = NULL,
         .ano                = NULL,
         .disciplina         = NULL,
         .escola             = NULL,
         .turma              = NULL,
         .periodo            = NULL,
         .gestor             = NULL,
         .professor          = NULL,
         .tema               = NULL,
         .prova_sequencia    = NULL,

         .seq_prova          = 1,
         .qtd_alunos_ativos  = 0,
         .qtd_alunos_total   = 0,

         .qtd_paginas        = 1,
         .qtd_colunas        = 2,
         .separadores        = 1,
         .cabecalho_tipo     = 1,
         .fonte_latex        = 1,

         .qtd_quest_subtema       = NULL,

         // Inicializa a matriz de strings de forma limpa e compatível com C11
         .selecao_quest_prova = NULL,

         .expor_dados        = FALSE,
         .validar_ciclos     = FALSE,
         .nao_presencial     = FALSE
      },

      .painel = {
         .format_titulo      = NULL,
         .format_subtitulo   = NULL,
         .format_instrucao   = NULL,
         .tem_proxima_mensagem = FALSE
      },

      .diario = {
         // 3. Ponteiros Heap Principais
         .aulas               = NULL,
         .metas_avs            = NULL,
         .fichas                = NULL,

         .path_save_metas_avs   = NULL,
         .path_save_diarios      = NULL,
         .path_save_fichas      = NULL,

         .responsavel = {
            .gestor          = NULL,
            .professor       = NULL
         },
         .frequencia = {
            .handler_combo_data   = 0,
            .handler_combo_alunos = 0
         },
         .avaliacao = {
            .handler_check_desativar = 0,
            .validas = NULL
         },
         .botoes_travados    = FALSE
      },

      .latex = {
         .handler_combo_tema_espelho = 0
      },

      .prova = {
         .subtemas           = NULL,
         .temas              = NULL,
         .handler_combo_tema = 0,

         .listbox_subtemas   = NULL,
         .flowbox_selecionados = NULL,
         .scrolled_window    = NULL,

         .montagem = {
            .btn_menos       = NULL,
            .btn_mais        = NULL,
            .handlers_check  = NULL,
            .handler_scrolled = 0
         }
      },

      // 4. Estrutura de Caminhos Dinâmicos (gchar*)
      .caminho = {
         .raiz               = NULL,
         .dados              = NULL,
         .banco_questoes     = NULL,
         .questoes_tex       = NULL,
         .base               = NULL,
         .externo            = NULL,
         .externo_escola     = NULL,
         .externo_final      = NULL,
         .relatorios_final   = NULL,
         .gabaritos          = NULL,
         .relatorios         = NULL,
         .recursos_prefix    = NULL
      }
   };
}



// =========================================================================
// HIGIENE DE MEMÓRIA FINAL
// =========================================================================
void limpeza_final( AppContext *ctx ) {
   if ( !ctx ) return;

   g_print( "\n[Higiene] g_application_run finalizado. Limpando estruturas de dados...\n" );

   parar_autosave_diario( ctx );

   g_rand_free( ctx->rand );

   // 1. Textos da InterfaceDados
   g_clear_pointer( &ctx->dados.ano, g_free );
   g_clear_pointer( &ctx->dados.disciplina, g_free );
   g_clear_pointer( &ctx->dados.escola, g_free );
   g_clear_pointer( &ctx->dados.turma, g_free );
   g_clear_pointer( &ctx->dados.periodo, g_free );
   g_clear_pointer( &ctx->dados.gestor, g_free );
   g_clear_pointer( &ctx->dados.professor, g_free );
   g_clear_pointer( &ctx->dados.tema, g_free );
   g_clear_pointer( &ctx->dados.prova_sequencia, g_free );

   g_clear_pointer( &ctx->dados.qtd_quest_subtema, g_free );
   g_clear_pointer( &ctx->dados.selecao_quest_prova, g_strfreev );

   // 2. Liberação dos Caminhos de Diretórios
   g_clear_pointer( &ctx->caminho.raiz, g_free );
   g_clear_pointer( &ctx->caminho.dados, g_free );
   g_clear_pointer( &ctx->caminho.gabaritos, g_free );
   g_clear_pointer( &ctx->caminho.relatorios, g_free );
   g_clear_pointer( &ctx->caminho.externo, g_free );
   g_clear_pointer( &ctx->caminho.relatorios_final, g_free );
   g_clear_pointer( &ctx->caminho.externo_final, g_free );
   g_clear_pointer( &ctx->caminho.externo_escola, g_free );
   g_clear_pointer( &ctx->caminho.banco_questoes, g_free );
   g_clear_pointer( &ctx->caminho.base, g_free );
   g_clear_pointer( &ctx->caminho.questoes_tex, g_free );

   // 3. Salvamento e Liberação dos GArrays de Dados
   if ( ctx->diario.aulas ) {
      salvar_diario( ctx, TRUE );
      g_clear_pointer( ( gpointer * ) &ctx->diario.aulas, g_array_unref );
   }
   g_clear_pointer( &ctx->diario.path_save_diarios, g_free );

   if ( ctx->diario.metas_avs ) {
      salvar_avaliacoes( ctx, TRUE );
      g_clear_pointer( ( gpointer * ) &ctx->diario.metas_avs, g_array_unref );
   }
   g_clear_pointer( &ctx->diario.path_save_metas_avs, g_free );

   if ( ctx->diario.fichas ) {
      salvar_fichas( ctx, TRUE );
      g_clear_pointer( ( gpointer * ) &ctx->diario.fichas, g_ptr_array_unref );
   }
   g_clear_pointer( &ctx->diario.path_save_fichas, g_free );

   // 4. Matriz do Acervo (handlers_check)
   g_clear_pointer( &ctx->prova.montagem.handlers_check, g_free );

   // 5. Arrays Dinâmicos da Interface (Cabeçalho e Provas)
   g_clear_pointer( &ctx->prova.subtemas, g_strfreev );
   g_clear_pointer( &ctx->prova.temas, g_strfreev );

   g_clear_pointer( &ctx->cabecalho.anos, g_strfreev );
   g_clear_pointer( &ctx->cabecalho.escolas, g_strfreev );
   g_clear_pointer( &ctx->cabecalho.turmas, g_strfreev );
   g_clear_pointer( &ctx->cabecalho.disciplinas, g_strfreev );

   // 6. Máscaras de formatação de texto (Painel)
   g_clear_pointer( &ctx->painel.format_titulo, g_free );
   g_clear_pointer( &ctx->painel.format_subtitulo, g_free );
   g_clear_pointer( &ctx->painel.format_instrucao, g_free );

   // 7. Limpar gboolean
   g_clear_pointer( &ctx->diario.avaliacao.validas, g_free );

   // 8. Provedor de Estilo CSS
   g_clear_object( ( gpointer * ) &ctx->provider );

   g_print( "✔ [Sucesso] Toda a memória Heap foi devolvida ao sistema operacional.\n\n" );
}



// 1. Função callback segura na esteira do GLib para interceptar sinais
static gboolean tratar_sinais_unix( gpointer user_data ) {
   GApplication *app = G_APPLICATION( user_data );
   g_print( "\n[Sinal] Interrupção (Ctrl+C ou fechamento forçado) detectada! Encerrando graciosamente...\n" );

   // Faz o quit no tipo base correto esperado pela GLib
   g_application_quit( app );

   return G_SOURCE_REMOVE;
}

