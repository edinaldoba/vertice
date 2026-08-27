/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <gtk/gtk.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>

#include "interface.h"
#include "layout.h"
#include "signals.h"
#include "basicas.h"

#include "mensagens.h"
#include "callbacks.h"




// Protótipo local da função de inicialização
void inicializacao_app_context( AppContext *ctx );
void limpeza_final( AppContext *ctx );



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
   // preparar_ambiente_latex_no_disco();

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

   // 1. Define tudo para o padrão brasileiro (incluindo acentos e vírgula decimal)
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

      status = g_application_run( G_APPLICATION( app ), argc, argv );
      g_object_unref( app );
   } else {
      g_printerr( "Erro crítico: Não foi possível instanciar o objeto GtkApplication.\n" );
      status = EXIT_FAILURE;
   }

   // HIGIENE DE MEMÓRIA: Libera o provider se ele terminou alocado (evita memory leak ao fechar)
   if ( ctx.provider != NULL ) {
      g_object_unref( ctx.provider );
      ctx.provider = NULL; // Evita ponteiro solto (dangling pointer)
   }

   limpeza_final( &ctx );

   return status;
}




//=====================================================================================================//
// DADOS PRIVADOS DO ARQUIVO (Invisíveis para o resto do sistema)                                      //
//=====================================================================================================//
const ItemCombo periodos[] = {
   {"1º Período"}, {"2º Período"}, {"3º Período"}, {"4º Período"}, {"Recuperação Final"}, {"Conselho de Classe"}
};

const ItemCombo cores_destaque[] = {
   {"primeiro"}, {"segundo"}, {"terceiro"}, {"sexto"}, {"setimo"}, {"oitavo"}, {"nono"}
};

const ItemCombo decoracoes_estilo[] = {
   {"Trapezios"}, {"Ondas"}, {"Quadrados"}, {"Linhas"}, {"Senoides"}, {"Circulos"}
};

const ItemCombo provas_sequencia[] = {
   {"Primeira"}, {"Segunda"}, {"Terceira"}
};

// Macros locais (só funcionam aqui dentro para alimentar a inicialização)
#define LOCAL_4   (sizeof(periodos) / sizeof(periodos[0]))
#define LOCAL_QTD_CORES      (sizeof(cores_destaque) / sizeof(cores_destaque[0]))
#define LOCAL_QTD_DECORACOES (sizeof(decoracoes_estilo) / sizeof(decoracoes_estilo[0]))




//=====================================================================================================//
// FUNÇÃO DE INICIALIZAÇÃO                                                                              //
//=====================================================================================================//
void inicializacao_app_context( AppContext *ctx ) {
   if ( !ctx ) return;

   *ctx = ( AppContext ) {
      .ficha = NULL,
      .caminho = {{""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
         .recursos_prefix = NULL
      },
      .provider = NULL,

      // 2. Inicialização do Estado Nativo da RAM (InterfaceDados)
      .dados = {
         .ano               = "",
         .disciplina        = "",
         .escola            = "",
         .turma             = "",
         .periodo           = "",
         .gestor            = "",
         .professor         = "",
         .tema    = "",
         .decoracao_estilo = "",
         .cor_destaque     = "",
         .prova_sequencia  = "Primeira",

         .serie             = 1,
         .iprova            = 1,
         .qtd_alunos_ativos = 0,
         .qtd_alunos_total  = 0,

         .qtd_paginas       = 1,
         .qtd_colunas       = 2,
         .separadores       = 1,
         .cabecalho_tipo    = 1,
         .fonte_latex       = 1,
         .interface_style   = 0,

         .qtd_questoes      = {0},
         .total_questoes    = 10,
         .temas_prova_sequencia = {
            {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}
         },

         .expor            = false,
         .cruz             = false,
         .naopresencial    = false
      },

      .cascata = {
         .limite = {
            .periodos          = LOCAL_4,
            .cores_destaque    = LOCAL_QTD_CORES,
            .decoracoes_estilo = LOCAL_QTD_DECORACOES
         }
      },

      // 3. Inicialização das Listas Dinâmicas e Constantes (InterfaceListas)
      .listas = {
         .qtd_subtemas     = NULL,
         .subtemas         = NULL,
         .temas = NULL,
         .anos             = NULL,
         .escolas          = NULL,
         .turmas           = NULL,
         .disciplinas      = NULL,

         .periodos          = periodos,
         .cores_destaque    = cores_destaque,
         .decoracoes_estilo = decoracoes_estilo,
         .provas_sequencia  = provas_sequencia,
      },

      .painel = {
         .format_titulo    = NULL,
         .format_subtitulo = NULL,
         .format_instrucao = NULL
      },

      .handlers = {
         .ano              = 0,
         .escola           = 0,
         .turma            = 0,
         .disciplina       = 0,
         .periodo          = 0,
         .alunos           = 0,
         .decoracao_estilo = 0,
         .cor_destaque     = 0,
         .tema   = 0
      },

      .provas = {
         .listbox_subtemas     = NULL,
         .flowbox_selecionados = NULL,
         .scrolled_window      = NULL,
         .btn_menos            = NULL,
         .btn_mais             = NULL,
         .handler              = NULL,
         .handler_scrolled     = 0
      },

      .cabecalho = {
         .gestor    = NULL,
         .professor = NULL
      }
   };
}



void limpeza_final( AppContext *ctx ) {
   // =========================================================================
   // 🛡️ HIGIENE DE MEMÓRIA FINAL (O que o unref do app não limpa sozinho)
   // =========================================================================
   g_print( "\n[Higiene] g_application_run finalizado. Limpando estruturas de dados...\n" );

   // A. Libera o Diário de Alunos (Heap)
   if ( ctx->ficha != NULL ) {
      free( ctx->ficha );
      ctx->ficha = NULL;
   }

   // B. Libera a matriz bidimensional de ponteiros do Acervo (Heap)
   if ( ctx->provas.handler != NULL ) {
      for ( int i = 0; i < ctx->cascata.limite.temas; i++ ) {
         if ( ctx->provas.handler[i] != NULL ) {
            free( ctx->provas.handler[i] );
         }
      }
      free( ctx->provas.handler );
      ctx->provas.handler = NULL;
   }

   // C. Libera as listas de strings dinâmicas (InterfaceListas)
   free( ctx->listas.subtemas );
   free( ctx->listas.temas );
   free( ctx->listas.qtd_subtemas );
   free( ctx->listas.anos );
   free( ctx->listas.escolas );
   free( ctx->listas.turmas );
   free( ctx->listas.disciplinas );

   // D. Libera as máscaras de formatação de texto gchar* (Painel)
   if ( ctx->painel.format_titulo != NULL )    g_free( ctx->painel.format_titulo );
   if ( ctx->painel.format_subtitulo != NULL ) g_free( ctx->painel.format_subtitulo );
   if ( ctx->painel.format_instrucao != NULL ) g_free( ctx->painel.format_instrucao );

   // E. Libera o Provedor de Estilo CSS de forma segura
   if ( ctx->provider != NULL ) {
      g_object_unref( ctx->provider );
      ctx->provider = NULL;
   }
   // Quando o Vértice for fechado pelo usuário:
   if ( ctx->ui_diario.popover_calendario ) {
      g_object_unref( ctx->ui_diario.popover_calendario );
   }

   g_print( "✔ [Sucesso] Toda a memória Heap foi devolvida ao sistema operacional.\n\n" );
}


