/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdint.h>

#include "assincrono.h"
#include "comum.h"
#include "glib/gstdio.h"
#include "mensagens.h"
#include "provas.h"
#include "basicas.h"
#include "imagens.h"
#include "glib_gio.h"
#include "imgcore.h"
#include "gabaritos.h"



// ====================================================================================
// ESTRUTURA DE TRANSPORTE E ISOLAMENTO DA THREAD
// ====================================================================================
typedef struct {
   AppContext        *ctx;         // Usado EXCLUSIVAMENTE nas callbacks da Main Thread
   GtkWidget         *botao_gerar; // Ponteiro para o botão que disparou a ação
   MotorProvaSnapshot snap;        // O pacote de dados 100% isolado na Heap para a Thread 2
   gboolean           sucesso;
} ProvaThreadArgs;

// ====================================================================================
// 1. CLONAGEM PROFUNDA (DEEP COPY) E DESTRUIÇÃO SIMÉTRICA DO SNAPSHOT
// ====================================================================================

/**
 * @brief Clona todos os ponteiros dinâmicos de CaminhoDiretorio (incluindo raiz, temp e base).
 */
static void clonar_caminho_diretorio_lc( const CaminhoDiretorio *orig, CaminhoDiretorio *dest ) {
   dest->raiz             = g_strdup( orig->raiz );
   dest->dados            = g_strdup( orig->dados );
   dest->gabaritos        = g_strdup( orig->gabaritos );
   dest->relatorios       = g_strdup( orig->relatorios );
   dest->externo          = g_strdup( orig->externo );
   dest->relatorios_final = g_strdup( orig->relatorios_final );
   dest->externo_final    = g_strdup( orig->externo_final );
   dest->externo_escola   = g_strdup( orig->externo_escola );
   dest->banco_questoes   = g_strdup( orig->banco_questoes );
   dest->base             = g_strdup( orig->base );
   dest->questoes_tex     = g_strdup( orig->questoes_tex );
   dest->temp             = g_strdup( orig->temp );

   // String literal constante (.rodata), cópia direta segura
   dest->recursos_prefix  = orig->recursos_prefix;
}

/**
 * @brief Libera todos os ponteiros clonados de CaminhoDiretorio no Snapshot.
 */
static void liberar_caminho_diretorio_snapshot_lc( CaminhoDiretorio *c ) {
   g_clear_pointer( &c->raiz,             g_free );
   g_clear_pointer( &c->dados,            g_free );
   g_clear_pointer( &c->gabaritos,        g_free );
   g_clear_pointer( &c->relatorios,       g_free );
   g_clear_pointer( &c->externo,          g_free );
   g_clear_pointer( &c->relatorios_final, g_free );
   g_clear_pointer( &c->externo_final,    g_free );
   g_clear_pointer( &c->externo_escola,   g_free );
   g_clear_pointer( &c->banco_questoes,   g_free );
   g_clear_pointer( &c->base,             g_free );
   g_clear_pointer( &c->questoes_tex,     g_free );
   g_clear_pointer( &c->temp,             g_free );
}

/**
 * @brief Clona profundamente InterfaceDados (strings, vetores de questões e metadados).
 */
static void clonar_interface_dados_lc( const InterfaceDados *orig,
                                       const CaminhoDiretorio *caminho_clone,
                                       InterfaceDados *dest ) {
   // 1. Copia todos os valores escalares (ints e gbooleans)
   *dest = *orig;

   // 2. Clona todas as strings dinâmicas do cabeçalho e estilo visual
   dest->ano             = g_strdup( orig->ano );
   dest->disciplina      = g_strdup( orig->disciplina );
   dest->escola          = g_strdup( orig->escola );
   dest->turma           = g_strdup( orig->turma );
   dest->periodo         = g_strdup( orig->periodo );
   dest->gestor          = g_strdup( orig->gestor );
   dest->professor       = g_strdup( orig->professor );
   dest->tema            = g_strdup( orig->tema );
   dest->prova_sequencia = g_strdup( orig->prova_sequencia );

   // 3. Vincula os ponteiros const gchar* (raiz e temp) às cópias já blindadas do snapshot
   dest->raiz = caminho_clone->raiz;
   dest->temp = caminho_clone->temp;

   // 4. Clona os arrays dinâmicos usando o tamanho real alocado (total_quest_prova)
   dest->qtd_quest_subtema   = NULL;
   dest->selecao_quest_prova = NULL;

   if ( orig->total_quest_prova > 0 ) {
      gsize qtd = ( gsize )orig->total_quest_prova;

      if ( orig->qtd_quest_subtema != NULL ) {
         dest->qtd_quest_subtema = ( gint * )g_memdup2(
                                      orig->qtd_quest_subtema,
                                      qtd * sizeof( gint )
                                   );
      }

      if ( orig->selecao_quest_prova != NULL ) {
         dest->selecao_quest_prova = g_new0( gchar *, qtd + 1U );
         for ( gsize i = 0; i < qtd; i++ ) {
            if ( orig->selecao_quest_prova[i] != NULL ) {
               dest->selecao_quest_prova[i] = g_strdup( orig->selecao_quest_prova[i] );
            }
         }
      }
   }
}

/**
 * @brief Libera todos os recursos dinâmicos clonados em InterfaceDados.
 */
static void liberar_interface_dados_snapshot_lc( InterfaceDados *d ) {
   g_clear_pointer( &d->ano,               g_free );
   g_clear_pointer( &d->disciplina,        g_free );
   g_clear_pointer( &d->escola,            g_free );
   g_clear_pointer( &d->turma,             g_free );
   g_clear_pointer( &d->periodo,           g_free );
   g_clear_pointer( &d->gestor,            g_free );
   g_clear_pointer( &d->professor,         g_free );
   g_clear_pointer( &d->tema,              g_free );
   g_clear_pointer( &d->prova_sequencia,   g_free );
   g_clear_pointer( &d->qtd_quest_subtema, g_free );

   if ( d->selecao_quest_prova != NULL ) {
      for ( int i = 0; i < d->total_quest_prova; i++ ) {
         g_free( d->selecao_quest_prova[i] );
      }
      g_clear_pointer( ( gpointer* )&d->selecao_quest_prova, g_free );
   }

   d->raiz = NULL;
   d->temp = NULL;
}

/**
 * @brief Clona profundamente o diário de alunos na Heap (FichaAluno + FAM HistoricoDisciplina).
 */
static GPtrArray *clonar_diario_alunos_lc( GPtrArray *fichas_originais, guint8 lim_disc ) {
   if ( !fichas_originais || fichas_originais->len == 0 ) {
      return NULL;
   }

   GPtrArray *fichas_clone = g_ptr_array_new_full( fichas_originais->len, g_free );

   gsize tamanho_real_ficha = sizeof( FichaAluno ) +
                              ( ( gsize )lim_disc * sizeof( HistoricoDisciplina ) );

   for ( guint i = 0; i < fichas_originais->len; i++ ) {
      const FichaAluno *orig = ( const FichaAluno * )g_ptr_array_index( fichas_originais, i );
      if ( !orig ) {
         continue;
      }

      FichaAluno *clone = ( FichaAluno * )g_memdup2( orig, tamanho_real_ficha );
      g_ptr_array_add( fichas_clone, clone );
   }

   return fichas_clone;
}

/**
 * @brief Destrutor unificado para ProvaThreadArgs (Compatível com GDestroyNotify).
 */
static void liberar_prova_thread_args( gpointer data ) {
   ProvaThreadArgs *args = ( ProvaThreadArgs * )data;
   if ( !args ) {
      return;
   }

   if ( args->snap.fichas != NULL ) {
      g_ptr_array_unref( args->snap.fichas );
      args->snap.fichas = NULL;
   }

   liberar_interface_dados_snapshot_lc( &args->snap.dados );
   liberar_caminho_diretorio_snapshot_lc( &args->snap.caminho );

   g_free( args );
}

// ====================================================================================
// 2. CONTROLE DE SENSIBILIDADE DA INTERFACE
// ====================================================================================
static void definir_estado_widgets_geracao_lc( AppContext *ctx, GtkWidget *botao_gerar, gboolean ativo ) {
   if ( !ctx ) {
      return;
   }

   if ( botao_gerar ) {
      gtk_widget_set_sensitive( botao_gerar, ativo );
   }
   if ( ctx->prova.btn_corrigir ) {
      gtk_widget_set_sensitive( ctx->prova.btn_corrigir, ativo );
   }
   if ( ctx->prova.btn_importar ) {
      gtk_widget_set_sensitive( ctx->prova.btn_importar, ativo );
   }

   if ( ctx->notebook ) {
      GtkWidget *aba_latex = gtk_notebook_get_nth_page( GTK_NOTEBOOK( ctx->notebook ), 1 );
      if ( aba_latex ) {
         gtk_widget_set_sensitive( aba_latex, ativo );
      }
   }
}

// ====================================================================================
// 3. ENTRADA DO MOTOR ASSÍNCRONO
// ====================================================================================
void disparar_geracao_prova_assincrona( GtkWidget *widget, AppContext *ctx, GThreadFunc funcao_background ) {
   g_return_if_fail( widget != NULL && ctx != NULL && funcao_background != NULL );

   ProvaThreadArgs *args = g_new0( ProvaThreadArgs, 1 );


   // 1. Clona CaminhoDiretorio primeiro (pois InterfaceDados->raiz e ->temp apontam para ele)
   clonar_caminho_diretorio_lc( &ctx->caminho, &args->snap.caminho );

   // 2. Clona InterfaceDados profundamente (todas as strings e vetores de questões)
   clonar_interface_dados_lc( &ctx->dados, &args->snap.caminho, &args->snap.dados );

   // 3. Copia metadados escalares de cabeçalho, data e clona o diário de fichas
   args->snap.cabecalho = ctx->cabecalho;
   args->snap.hoje      = ctx->hoje;
   args->snap.fichas    = clonar_diario_alunos_lc(
                             ctx->diario.fichas,
                             ( guint8 )ctx->cabecalho.lim_disciplinas
                          );

   // 4. Referências Mestre para retorno na Main Thread
   args->ctx         = ctx;
   args->botao_gerar = widget;

   // 5. Congelamento Parcial da Interface (Aba LaTeX e botões de prova)
   definir_estado_widgets_geracao_lc( ctx, widget, FALSE );

   // 6. Disparo da Thread gerenciada pela GLib
   GError  *erro_thread = NULL;
   GThread *worker      = g_thread_try_new( "vertice-tex-worker", funcao_background, args, &erro_thread );

   if ( !worker ) {
      definir_estado_widgets_geracao_lc( ctx, widget, TRUE );

      InterfacePainel *painel  = &ctx->painel;
      painel->format_titulo    = meu_gerador_variadico( "✘ Falha de Concorrência:" );
      painel->format_subtitulo = meu_gerador_variadico( "Não foi possível iniciar a thread de compilação em segundo plano." );
      painel->format_instrucao = meu_gerador_variadico( "Detalhe: %s", erro_thread ? erro_thread->message : "Erro interno." );

      criar_mensagem_painel( ERRO, painel );
      g_clear_error( &erro_thread );
      liberar_prova_thread_args( args );
      return;
   }

   g_thread_unref( worker );
}

// ====================================================================================
// 4. CALLBACKS DE RETORNO E TOAST (MAIN THREAD)
// ====================================================================================
static gboolean ocultar_painel_toast_cb( gpointer user_data ) {
   ProvaThreadArgs *args = ( ProvaThreadArgs * )user_data;
   if ( !args || !args->ctx ) {
      return G_SOURCE_REMOVE;
   }

   InterfacePainel *painel = &args->ctx->painel;

   if ( painel->mouse_hover ) {
      return G_SOURCE_CONTINUE;
   }

   if ( painel->revealer_painel ) {
      gtk_revealer_set_reveal_child( GTK_REVEALER( painel->revealer_painel ), FALSE );
   }
   painel->timeout_id = 0;

   return G_SOURCE_REMOVE;
}

static gboolean reativar_botao_gerar_prova( gpointer user_data ) {
   ProvaThreadArgs *args = ( ProvaThreadArgs * )user_data;
   if ( !args || !args->ctx ) {
      return G_SOURCE_REMOVE;
   }

   AppContext *ctx = args->ctx;

   definir_estado_widgets_geracao_lc( ctx, args->botao_gerar, TRUE );

   InterfacePainel *painel = &ctx->painel;

   if ( args->sucesso ) {
      painel->format_titulo    = meu_gerador_variadico( "✔ Prova Gerada com Sucesso!" );
      painel->format_subtitulo = meu_gerador_variadico( "O arquivo PDF foi compilado e estruturado corretamente via TeX." );
      painel->format_instrucao = meu_gerador_variadico( "Pronto para impressão: %s Prova.pdf (em tela)",
                                 args->snap.dados.prova_sequencia ? args->snap.dados.prova_sequencia : "" );

      criar_mensagem_painel( SUCESSO, painel );
   } else {
      painel->format_titulo    = meu_gerador_variadico( "✘ Erro no Motor TeX" );
      painel->format_subtitulo = meu_gerador_variadico( "Falha crítica na estruturação ou compilação assíncrona dos gabaritos." );
      painel->format_instrucao = meu_gerador_variadico( "Verifique o arquivo lista.dat do \"%s\" ou os logs do compilador.",
                                 args->snap.dados.periodo ? args->snap.dados.periodo : "período atual" );

      criar_mensagem_painel( ERRO, painel );
   }

   if ( painel->timeout_id > 0 ) {
      g_source_remove( painel->timeout_id );
      painel->timeout_id = 0;
   }

   guint tempo_exibicao = args->sucesso ? 4000U : 6000U;
   painel->timeout_id   = g_timeout_add_full(
                             G_PRIORITY_DEFAULT,
                             tempo_exibicao,
                             ocultar_painel_toast_cb,
                             args,
                             liberar_prova_thread_args
                          );

   return G_SOURCE_REMOVE;
}

// ====================================================================================
// 5. WORKER: EXECUÇÃO EM SEGUNDO PLANO (100% ISOLADA DA UI)
// ====================================================================================
void *thread_gerar_prova_background( void *data ) {
   g_return_val_if_fail( data != NULL, NULL );

   g_autoptr( GTimer ) cronometro = g_timer_new();
   ProvaThreadArgs    *args       = ( ProvaThreadArgs * )data;
   MotorProvaSnapshot *snap       = &args->snap;

   args->sucesso = FALSE;

   if ( snap->dados.qtd_alunos_ativos <= 0 ) {
      g_idle_add( reativar_botao_gerar_prova, args );
      return NULL;
   }

   g_autofree gchar *destino = g_build_filename(
                                  snap->caminho.raiz, "dados", "gabaritos", snap->dados.ano, snap->dados.escola, "gabaritos", NULL
                               );

   g_autofree gchar *nome = nome_base_gabaritos_bin(
                               snap->cabecalho.foc_turma,
                               snap->cabecalho.foc_disciplina,
                               snap->cabecalho.foc_periodo,
                               snap->dados.seq_prova
                            );

   g_autofree gchar *arquivo = g_build_filename( destino, nome, NULL );

   gsize num_alunos      = ( gsize )snap->dados.qtd_alunos_ativos;
   gsize max_len         = ( gsize )MAX_QUEST_PROVA + 1U;
   gsize bytes_esperados = num_alunos * max_len;

   g_autofree gchar *G           = NULL;
   gsize             lidos_bytes = 0;

   if ( g_file_get_contents( arquivo, &G, &lidos_bytes, NULL ) && G != NULL ) {
      gsize lidos_alunos = lidos_bytes / max_len;

      if ( lidos_bytes >= max_len && lidos_alunos > 0 ) {
         if ( lidos_bytes < bytes_esperados ) {
            gchar *bloco_completo = g_malloc0( bytes_esperados );
            memcpy( bloco_completo, G, lidos_bytes );
            g_free( G );
            G = bloco_completo;
         }

         args->sucesso = TRUE;

         prova( snap, G );

         salvar_estado_aplicativo( &snap->dados, &snap->caminho );
      }
   }

   g_clear_pointer( &G, g_free );

   g_idle_add( reativar_botao_gerar_prova, args );
   display_tempo( "Geração de prova", cronometro );

   return NULL;
}












// ====================================================================================
// ESTRUTURA DE TRANSPORTE E ISOLAMENTO DA THREAD DE VISÃO COMPUTACIONAL
// ====================================================================================
typedef struct {
   AppContext        *ctx;             // Usado EXCLUSIVAMENTE nas callbacks da Main Thread
   InterfacePainel   *painel;          // Ponteiro vivo para a Main Thread (lê mouse_hover)
   InterfaceDados     dados;           // Deep Copy (Isolamento 100% blindado na Heap)
   InterfaceCabecalho cabecalho;       // Cópia escalar dos índices de foco
   CaminhoDiretorio   caminho_min;     // Guarda cópias próprias de raiz e temp para InterfaceDados
   GtkWidget         *botao_processar; // Referência de UI para reativação na Main Thread
   GtkWidget         *botao_corrigir;  // Referência de UI para reativação na Main Thread
   int                n_rejeitadas;    // Resultado do lote (< 0 indica erro operacional)
} ProcessarThreadArgs;

/* Destrutor compatível com GDestroyNotify para limpeza garantida pelo loop da GLib */
static void liberar_processar_thread_args( gpointer data ) {
   ProcessarThreadArgs *args = ( ProcessarThreadArgs * )data;
   if ( !args ) {
      return;
   }

   // Libera todas as strings e vetores clonados em InterfaceDados e CaminhoDiretorio
   liberar_interface_dados_snapshot_lc( &args->dados );
   liberar_caminho_diretorio_snapshot_lc( &args->caminho_min );

   g_free( args );
}

// ====================================================================================
// DISPARADOR ASSÍNCRONO DE PROCESSAMENTO DE IMAGENS
// ====================================================================================
void disparar_processamento_imagens_assincrono( GtkWidget *widget, AppContext *ctx, GThreadFunc funcao_background ) {
   g_return_if_fail( widget != NULL && ctx != NULL && funcao_background != NULL );

   ProcessarThreadArgs *args = g_new0( ProcessarThreadArgs, 1 );

   // 1. Snapshot Profundo (Deep Copy): Isola 100% os ponteiros de InterfaceDados da Main Thread
   clonar_caminho_diretorio_lc( &ctx->caminho, &args->caminho_min );
   clonar_interface_dados_lc( &ctx->dados, &args->caminho_min, &args->dados );
   args->cabecalho = ctx->cabecalho;

   // 2. Referências Vivas (Acessadas exclusivamente na Main Thread)
   args->ctx             = ctx;
   args->painel          = &ctx->painel;
   args->botao_processar = widget;
   args->botao_corrigir  = ctx->prova.btn_corrigir;
   args->n_rejeitadas    = -1;

   // 3. Bloqueio preventivo dos controles conflitantes na UI
   gtk_widget_set_sensitive( widget, FALSE );
   if ( ctx->prova.btn_corrigir ) {
      gtk_widget_set_sensitive( ctx->prova.btn_corrigir, FALSE );
   }

   // 4. Criação da Thread gerenciada pela GLib
   GError  *erro_thread = NULL;
   GThread *worker      = g_thread_try_new( "vertice-omr-worker", funcao_background, args, &erro_thread );

   if ( !worker ) {
      gtk_widget_set_sensitive( widget, TRUE );
      if ( ctx->prova.btn_corrigir ) {
         gtk_widget_set_sensitive( ctx->prova.btn_corrigir, TRUE );
      }

      InterfacePainel *painel  = &ctx->painel;
      painel->format_titulo    = meu_gerador_variadico( "✘ Falha de Concorrência:" );
      painel->format_subtitulo = meu_gerador_variadico( "Não foi possível iniciar a thread de visão computacional." );
      painel->format_instrucao = meu_gerador_variadico( "Detalhe: %s", erro_thread ? erro_thread->message : "Erro interno." );

      criar_mensagem_painel( ERRO, painel );
      g_clear_error( &erro_thread );
      liberar_processar_thread_args( args );
      return;
   }

   g_thread_unref( worker );
}

// ====================================================================================
// CALLBACK: Oculta o painel Toast (Executa na Main Thread)
// ====================================================================================
static gboolean ocultar_painel_cv_cb( gpointer user_data ) {
   ProcessarThreadArgs *args = ( ProcessarThreadArgs * )user_data;
   if ( !args || !args->painel ) {
      return G_SOURCE_REMOVE;
   }

   InterfacePainel *painel = args->painel;

   // 1. Se o cursor estiver sobre o Toast, mantém visível na tela
   if ( painel->mouse_hover ) {
      return G_SOURCE_CONTINUE;
   }

   // 2. Oculta o Revealer de forma suave
   if ( painel->revealer_painel ) {
      gtk_revealer_set_reveal_child( GTK_REVEALER( painel->revealer_painel ), FALSE );
   }
   painel->timeout_id = 0;

   // Ao retornar G_SOURCE_REMOVE, a GLib invoca liberar_processar_thread_args(args) automaticamente
   return G_SOURCE_REMOVE;
}

// ====================================================================================
// CALLBACK: Retorno da Thread (Executa na Main Thread via g_idle_add)
// ====================================================================================
static gboolean reativar_botao_processar_imagens( gpointer user_data ) {
   ProcessarThreadArgs *args = ( ProcessarThreadArgs * )user_data;
   if ( !args ) {
      return G_SOURCE_REMOVE;
   }

   if ( args->botao_processar ) {
      gtk_widget_set_sensitive( args->botao_processar, TRUE );
   }
   if ( args->botao_corrigir ) {
      gtk_widget_set_sensitive( args->botao_corrigir, TRUE );
   }

   InterfacePainel *painel = args->painel;
   if ( !painel ) {
      liberar_processar_thread_args( args );
      return G_SOURCE_REMOVE;
   }

   // Textos conforme o código de retorno do motor OMR
   if ( args->n_rejeitadas == 0 ) {
      painel->format_titulo    = meu_gerador_variadico( "✔ Processamento Concluído!" );
      painel->format_subtitulo = meu_gerador_variadico( "Todas as provas foram lidas, alinhadas e validadas sem falhas." );
      painel->format_instrucao = meu_gerador_variadico( "O lote está pronto. Você já pode prosseguir para a etapa de Correção." );
      criar_mensagem_painel( SUCESSO, painel );

   } else if ( args->n_rejeitadas > 0 ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Processamento com Alertas" );
      painel->format_subtitulo = meu_gerador_variadico( "Não foi possível ler os marcadores ou identificar %d imagem(ns).", args->n_rejeitadas );
      painel->format_instrucao = meu_gerador_variadico( "Estes arquivos foram isolados na pasta 'rejeitadas' para sua verificação manual." );
      criar_mensagem_painel( AVISO, painel );

   } else if ( args->n_rejeitadas == -1 ) {
      painel->format_titulo    = meu_gerador_variadico( "✘ Dados Não Encontrados" );
      painel->format_subtitulo = meu_gerador_variadico( "O sistema não localizou gabaritos estruturais de nenhuma avaliação." );
      painel->format_instrucao = meu_gerador_variadico( "Certifique-se de gerar os cadernos de prova antes de tentar processar as imagens." );
      criar_mensagem_painel( ERRO, painel );

   } else {
      painel->format_titulo    = meu_gerador_variadico( "✘ Nenhuma Imagem Localizada" );
      painel->format_subtitulo = meu_gerador_variadico( "O sistema não encontrou fotografias ou digitalizações de respostas na pasta de entrada." );
      painel->format_instrucao = meu_gerador_variadico( "Transfira os ficheiros de imagem das provas para a pasta 'Downloads/imagens' antes de iniciar o processamento." );
      criar_mensagem_painel( ERRO, painel );
   }

   // Cancela qualquer timer anterior (acionando seu respectivo GDestroyNotify sem vazar memória)
   if ( painel->timeout_id > 0 ) {
      g_source_remove( painel->timeout_id );
      painel->timeout_id = 0;
   }

   guint tempo_exibicao = ( args->n_rejeitadas == 0 ) ? 4000U : 6000U;
   painel->timeout_id   = g_timeout_add_full(
                             G_PRIORITY_DEFAULT,
                             tempo_exibicao,
                             ocultar_painel_cv_cb,
                             args,
                             liberar_processar_thread_args
                          );

   return G_SOURCE_REMOVE;
}

// ====================================================================================
// WORKER: Executa o Pipeline OMR em Background (100% Isolado da UI)
// ====================================================================================
void *thread_processar_imagens_background( void *data ) {
   g_return_val_if_fail( data != NULL, NULL );

   g_autoptr( GTimer ) cronometro = g_timer_new();
   ProcessarThreadArgs *args      = ( ProcessarThreadArgs * )data;

   args->n_rejeitadas = cs_omr_processar_imagens( &args->dados, &args->cabecalho );

   // Agenda a execução do retorno gráfico na Thread Principal do GTK
   g_idle_add( reativar_botao_processar_imagens, args );

   display_tempo( "Processamento", cronometro );
   return NULL;
}










// ====================================================================================
// ESTRUTURA DE TRANSPORTE E ISOLAMENTO DO COMPILADOR DE ACERVO
// ====================================================================================
typedef struct {
   gchar           *tema;                   // Cópia profunda de ctx->dados.tema
   int              qtd_subtemas;           // Cópia de ctx->prova.lim_subtemas
   GtkWidget       *btn_compilar;           // Referência de UI (ctx->latex.btn_compilar)
   GtkWidget       *btn_pdflatex;           // Referência de UI (ctx->latex.btn_pdflatex)
   GtkWidget       *widget;                 // Botão exato que originou o disparo
   GStrv            subtemas;               // Deep copy NULL-terminated das strings de subtemas
   InterfacePainel *painel;                 // Painel de notificações Toast (Main Thread)
   gchar           *dir_compile;            // Cópia do diretório temporário de compilação
   gchar           *caminho_banco_questoes; // Cópia de ctx->caminho.banco_questoes
} DadosCompilacaoAsync;

/* Destrutor compatível com GDestroyNotify para higienização total */
static void liberar_dados_compilacao_async( gpointer data ) {
   DadosCompilacaoAsync *async_data = ( DadosCompilacaoAsync * )data;
   if ( !async_data ) {
      return;
   }

   g_clear_pointer( &async_data->tema,                   g_free );
   g_clear_pointer( &async_data->dir_compile,            g_free );
   g_clear_pointer( &async_data->caminho_banco_questoes, g_free );
   g_clear_pointer( &async_data->subtemas,               g_strfreev );

   g_free( async_data );
}

// ====================================================================================
// CALLBACK: Oculta o painel Toast do Acervo (Executa na Main Thread)
// ====================================================================================
static gboolean ocultar_painel_acervo_cb( gpointer user_data ) {
   InterfacePainel *painel = ( InterfacePainel * )user_data;
   if ( !painel ) {
      return G_SOURCE_REMOVE;
   }

   if ( painel->mouse_hover ) {
      return G_SOURCE_CONTINUE;
   }

   if ( painel->revealer_painel ) {
      gtk_revealer_set_reveal_child( GTK_REVEALER( painel->revealer_painel ), FALSE );
   }
   painel->timeout_id = 0;

   return G_SOURCE_REMOVE;
}

static void agendar_ocultacao_toast_acervo_lc( InterfacePainel *painel, guint tempo_ms ) {
   if ( !painel ) {
      return;
   }

   if ( painel->timeout_id > 0 ) {
      g_source_remove( painel->timeout_id );
      painel->timeout_id = 0;
   }

   painel->timeout_id = g_timeout_add( tempo_ms, ocultar_painel_acervo_cb, painel );
}

// ====================================================================================
// DIAGNÓSTICO DE INTEGRIDADE DOS PDFS DO ACERVO
// ====================================================================================
static gboolean verificar_pdfs_latex_acervo_questoes( InterfacePainel *painel, const gchar *tema,
      int qtd_subtemas, int falhas ) {
   g_return_val_if_fail( painel != NULL && tema != NULL, FALSE );

   if ( falhas == 0 && qtd_subtemas > 0 ) {
      // --- ESTADO 1: SUCESSO TOTAL ---
      painel->format_titulo    = meu_gerador_variadico( "✔ Sucesso Total!" );
      painel->format_subtitulo = meu_gerador_variadico( "Todos os %d PDFs foram gerados corretamente.", qtd_subtemas );
      painel->format_instrucao = meu_gerador_variadico( "Concatenação concluída para %s.pdf.", tema );
      criar_mensagem_painel( SUCESSO, painel );
      agendar_ocultacao_toast_acervo_lc( painel, 4000U );
      return TRUE;

   } else if ( falhas < qtd_subtemas ) {
      // --- ESTADO 2: ALERTA (SUCESSO PARCIAL) ---
      int sucessos             = qtd_subtemas - falhas;
      painel->format_titulo    = meu_gerador_variadico( "⚠ Atenção (Sucesso Parcial):" );
      painel->format_subtitulo = meu_gerador_variadico( "Processados %d de %d arquivos com sucesso.", sucessos, qtd_subtemas );
      painel->format_instrucao = meu_gerador_variadico( "Houve falha em %d item(ns). PDFs disponíveis unidos em %s.pdf.", falhas, tema );
      criar_mensagem_painel( AVISO, painel );
      agendar_ocultacao_toast_acervo_lc( painel, 6000U );
      return TRUE;

   } else {
      // --- ESTADO 3: ERRO CRÍTICO (NADA FOI GERADO) ---
      painel->format_titulo    = meu_gerador_variadico( "✘ Erro de Processamento:" );
      painel->format_subtitulo = meu_gerador_variadico( "Nenhum arquivo PDF foi gerado pelo motor LaTeX." );
      painel->format_instrucao = meu_gerador_variadico( "Verifique a sintaxe TeX das questões no tema \"%s\".", tema );
      criar_mensagem_painel( ERRO, painel );
      agendar_ocultacao_toast_acervo_lc( painel, 6000U );
      return FALSE;
   }
}

// ====================================================================================
// CALLBACK DE RETORNO DO SUBPROCESSO (Executa na Main Thread via GChildWatch)
// ====================================================================================
static void ao_terminar_compilacao_banco( GPid pid, gint wait_status, gpointer user_data ) {
   DadosCompilacaoAsync *async_data = ( DadosCompilacaoAsync * )user_data;
   g_spawn_close_pid( pid );

   if ( !async_data ) {
      return;
   }

#if GLIB_CHECK_VERSION( 2, 70, 0 )
   gboolean proc_ok = g_spawn_check_wait_status( wait_status, NULL );
#else
   gboolean proc_ok = g_spawn_check_exit_status( wait_status, NULL );
#endif

   const gchar *tema_seguro = async_data->tema ? async_data->tema : "Acervo";

   if ( proc_ok && async_data->qtd_subtemas > 0 && async_data->subtemas != NULL ) {
      // 1. Prepara o array dinâmico com os nomes exatos dos PDFs efetivamente gerados
      gsize   cap_pdfs           = ( gsize )async_data->qtd_subtemas + 1U;
      gchar **arquivos_pdf       = g_new0( gchar *, cap_pdfs );
      int     qtd_sucessos_reais = 0;

      for ( int i = 0; i < async_data->qtd_subtemas; i++ ) {
         const gchar *subtema_atual = async_data->subtemas[i];
         if ( !subtema_atual || subtema_atual[0] == '\0' ) {
            continue;
         }

         gchar           *nome_pdf   = g_strdup_printf( "%s.pdf", subtema_atual );
         g_autofree gchar *path_teste = g_build_filename( async_data->dir_compile, nome_pdf, NULL );

         if ( g_file_test( path_teste, G_FILE_TEST_IS_REGULAR ) ) {
            arquivos_pdf[qtd_sucessos_reais++] = nome_pdf; // Transfere a posse diretamente
         } else {
            g_free( nome_pdf );
         }
      }

      int falhas = async_data->qtd_subtemas - qtd_sucessos_reais;

      if ( verificar_pdfs_latex_acervo_questoes( async_data->painel, tema_seguro,
            async_data->qtd_subtemas, falhas ) ) {
         // 2. Caminho absoluto do arquivo final unificado do tema
         g_autofree gchar *nome_arquivo  = g_strdup_printf( "%s.pdf", tema_seguro );
         g_autofree gchar *arquivo_saida = g_build_filename( async_data->caminho_banco_questoes, nome_arquivo, NULL );

         // 3. Unifica os PDFs disponíveis
         g_remove( arquivo_saida );
         if ( qtd_sucessos_reais > 0 ) {
            g_pdfunite( async_data->dir_compile, arquivos_pdf, qtd_sucessos_reais, arquivo_saida );
         }

         // 4. Apaga artefatos temporários (.aux, .log, .synctex.gz) da compilação LaTeX
         apagar_temporarios_latex_paralelo( async_data->dir_compile, async_data->subtemas, -1 );

         if ( async_data->widget == async_data->btn_pdflatex &&
               g_file_test( arquivo_saida, G_FILE_TEST_IS_REGULAR ) ) {
            g_xdg_open( arquivo_saida, NULL );
         }
      }

      g_strfreev( arquivos_pdf );

   } else if ( !proc_ok && async_data->painel ) {
      async_data->painel->format_titulo    = meu_gerador_variadico( "✘ Falha na Compilação:" );
      async_data->painel->format_subtitulo = meu_gerador_variadico( "O processo paralelo retornou erro ao compilar \"%s\".", tema_seguro );
      async_data->painel->format_instrucao = meu_gerador_variadico( "Código de término do subprocesso: %d.", wait_status );
      criar_mensagem_painel( ERRO, async_data->painel );
      agendar_ocultacao_toast_acervo_lc( async_data->painel, 6000U );
   }

   // Reabilita os botões na interface
   if ( async_data->btn_pdflatex ) {
      gtk_widget_set_sensitive( async_data->btn_pdflatex, TRUE );
   }
   if ( async_data->btn_compilar ) {
      gtk_widget_set_sensitive( async_data->btn_compilar, TRUE );
   }

   liberar_dados_compilacao_async( async_data );
}

// ====================================================================================
// DISPARADOR ASSÍNCRONO DE COMPILAÇÃO DO BANCO DE QUESTÕES
// ====================================================================================
void g_pdflatex_parallel_async( GtkWidget *widget, const char *dir_compile,
                                InterfacePainel *painel, const AppContext *ctx ) {
   g_return_if_fail( dir_compile != NULL && painel != NULL && ctx != NULL );

   if ( ctx->prova.lim_subtemas <= 0 || ctx->prova.subtemas == NULL ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Acervo Vazio:" );
      painel->format_subtitulo = meu_gerador_variadico( "Nenhum subtema disponível para compilação no tema atual." );
      painel->format_instrucao = meu_gerador_variadico( "Cadastre questões no diretório antes de compilar." );
      criar_mensagem_painel( AVISO, painel );
      agendar_ocultacao_toast_acervo_lc( painel, 4000U );
      return;
   }

   if ( ctx->latex.btn_pdflatex ) {
      gtk_widget_set_sensitive( ctx->latex.btn_pdflatex, FALSE );
   }
   if ( ctx->latex.btn_compilar ) {
      gtk_widget_set_sensitive( ctx->latex.btn_compilar, FALSE );
   }

   DadosCompilacaoAsync *async_data   = g_new0( DadosCompilacaoAsync, 1 );
   async_data->tema                   = g_strdup( ctx->dados.tema );
   async_data->qtd_subtemas           = ctx->prova.lim_subtemas;
   async_data->btn_compilar           = ctx->latex.btn_compilar;
   async_data->btn_pdflatex           = ctx->latex.btn_pdflatex;
   async_data->widget                 = widget;
   async_data->painel                 = painel;
   async_data->dir_compile            = g_strdup( dir_compile );
   async_data->caminho_banco_questoes = g_strdup( ctx->caminho.banco_questoes );

   // Deep Copy real do vetor de strings (+1 para o terminador NULL exigido por g_strfreev)
   async_data->subtemas = g_new0( gchar *, ( gsize )async_data->qtd_subtemas + 1U );
   for ( int i = 0; i < async_data->qtd_subtemas; i++ ) {
      async_data->subtemas[i] = g_strdup( ctx->prova.subtemas[i] );
   }

   guint             num_cores = g_get_num_processors();
   g_autoptr( GError ) erro    = NULL;
   GPid              pid       = 0;

   g_autofree gchar *comando_interno = g_strdup_printf(
                                          "parallel -j %u nice -n 5 pdflatex -synctex=1 -interaction=nonstopmode ::: *.tex || true",
                                          num_cores
                                       );

   g_auto( GStrv ) argv = g_new0( gchar*, 4U );
   argv[0] = g_strdup( "sh" );
   argv[1] = g_strdup( "-c" );
   argv[2] = g_strdup( comando_interno );

   if ( !g_spawn_async( dir_compile, argv, NULL,
                        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                        NULL, NULL, &pid, &erro ) ) {
      if ( async_data->btn_pdflatex ) {
         gtk_widget_set_sensitive( async_data->btn_pdflatex, TRUE );
      }
      if ( async_data->btn_compilar ) {
         gtk_widget_set_sensitive( async_data->btn_compilar, TRUE );
      }

      painel->format_titulo    = meu_gerador_variadico( "✘ Erro de Processamento:" );
      painel->format_subtitulo = meu_gerador_variadico( "Não foi possível iniciar o compilador paralelo." );
      painel->format_instrucao = meu_gerador_variadico( "Detalhe: %s", erro ? erro->message : "Verifique o GNU Parallel/TeX." );
      criar_mensagem_painel( ERRO, painel );
      agendar_ocultacao_toast_acervo_lc( painel, 6000U );

      liberar_dados_compilacao_async( async_data );
   } else {
      g_child_watch_add( pid, ao_terminar_compilacao_banco, async_data );
   }
}







// ====================================================================================
// ESTRUTURA DE TRANSPORTE E GESTÃO DO SUBPROCESSO DE CORREÇÃO
// ====================================================================================
typedef struct {
   GArray            *map_array;   // Posse transferida de corrigir_prova()
   AppContext        *ctx;         // Referência da Main Thread apenas para reabilitar botões
   InterfacePainel   *painel;      // Painel de notificações Toast (Main Thread)
   InterfaceDados     dados;       // Deep Copy do estado da turma no momento do disparo
   CaminhoDiretorio   caminho;     // Deep Copy dos diretórios da turma no momento do disparo
   InterfaceCabecalho cabecalho;   // Cópia escalar do foco (período/turma/disciplina)
   GPtrArray         *fichas;      // Deep Copy das fichas dos alunos para exportação PNG
   gchar             *dir_compile; // Diretório temporário de compilação
   GTimer            *cronometro;  // Medidor de performance do lote
} DadosCorrecaoAsync;

/* Destrutor centralizado para evitar vazamento de campos em qualquer fluxo de saída */
static void liberar_dados_correcao_async( gpointer data ) {
   DadosCorrecaoAsync *async = ( DadosCorrecaoAsync * )data;
   if ( !async ) {
      return;
   }

   if ( async->cronometro != NULL ) {
      g_timer_destroy( async->cronometro );
      async->cronometro = NULL;
   }
   if ( async->map_array != NULL ) {
      g_array_free( async->map_array, TRUE );
      async->map_array = NULL;
   }
   if ( async->fichas != NULL ) {
      g_ptr_array_unref( async->fichas );
      async->fichas = NULL;
   }

   liberar_interface_dados_snapshot_lc( &async->dados );
   liberar_caminho_diretorio_snapshot_lc( &async->caminho );
   g_clear_pointer( &async->dir_compile, g_free );

   g_free( async );
}

// ====================================================================================
// EXPORTAÇÃO INSTITUCIONAL E CONVERSÃO NÃO PRESENCIAL (SOBRE O SNAPSHOT ISOLADO)
// ====================================================================================
static void copiar_arquivos_correcao_externamente( const InterfaceDados *dados,
      const CaminhoDiretorio *caminho,
      const gchar *arquivo_saida ) {
   g_return_if_fail( dados != NULL && caminho != NULL && arquivo_saida != NULL );

   if ( !dados->periodo || !caminho->externo_escola ) {
      return;
   }

   g_autofree gchar *nome_arquivo_escola = NULL;
   if ( dados->periodo[0] == 'R' ) {
      nome_arquivo_escola = g_strdup_printf( "Correção Recuperação Final - %s - %s - %s.pdf",
                                             dados->ano, dados->turma, dados->disciplina );
   } else {
      gchar inicial_per   = ( gchar )( ( dados->periodo[0] != '\0' ) ? dados->periodo[0] : '1' );
      nome_arquivo_escola = g_strdup_printf( "Correção %s Prova - %s_%c - %s - %s.pdf",
                                             dados->prova_sequencia, dados->ano, inicial_per,
                                             dados->turma, dados->disciplina );
   }

   g_autofree gchar *pasta_provas_escola = g_build_filename( caminho->externo_escola, "Correções", NULL );
   g_autofree gchar *destino_escola      = g_build_filename( pasta_provas_escola, nome_arquivo_escola, NULL );

   if ( g_mkdir_with_parents( pasta_provas_escola, 0755 ) != 0 ) {
      return;
   }

   gio_copiar_arquivo( arquivo_saida, destino_escola );
}

//------------------------------------------------------------------------------------------------------
static void copiar_arquivos_correcao_nao_presencial( const MapeamentoGabarito *map,
      guint qtd_linhas,
      const DadosCorrecaoAsync *async ) {
   g_return_if_fail( map != NULL && async != NULL && async->fichas != NULL );

   const InterfaceDados   *dados   = &async->dados;
   const CaminhoDiretorio *caminho = &async->caminho;

   if ( !caminho->externo || !caminho->raiz ) {
      return;
   }

   g_autofree gchar *diretorio_imagens = NULL;

   if ( async->cabecalho.foc_periodo == RECUPERACAO_FINAL ) {
      diretorio_imagens = g_build_filename( caminho->externo, "Notas", "Recuperação Final Imagens Corrigidas", NULL );
   } else {
      g_autofree gchar *pasta_imagens = g_strdup_printf( "%s Prova Imagens Corrigidas",
                                        dados->prova_sequencia ? dados->prova_sequencia : "" );
      diretorio_imagens               = g_build_filename( caminho->externo, "Notas", pasta_imagens, NULL );
   }

   if ( g_mkdir_with_parents( diretorio_imagens, 0755 ) != 0 ) {
      return;
   }

   guint total_fichas = async->fichas->len;

   #pragma omp parallel for schedule(static)
   for ( guint i = 0; i < qtd_linhas; i++ ) {
      if ( map[i].status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
         int num_aluno = map[i].num;

         if ( num_aluno > 0 && ( guint )num_aluno <= total_fichas ) {
            const FichaAluno *ficha = ( const FichaAluno * )g_ptr_array_index( async->fichas, ( guint )( num_aluno - 1 ) );
            if ( !ficha ) {
               continue;
            }

            g_autofree gchar *nome_pdf           = g_strdup_printf( "%.2d.pdf", num_aluno );
            g_autofree gchar *thread_caminho_pdf = g_build_filename( caminho->raiz, "dados", "temporarios", nome_pdf, NULL );
            g_autofree gchar *nome_arquivo_png   = g_strdup_printf( "%.2d - %s.png", num_aluno, ficha->aluno );
            g_autofree gchar *thread_caminho_png = g_build_filename( diretorio_imagens, nome_arquivo_png, NULL );

            pdf2png( thread_caminho_pdf, thread_caminho_png, 1.5 );
         }
      }
   }
}

// ====================================================================================
// CALLBACK DE RETORNO DO PROCESSO FILHO (Executa na Main Thread via GChildWatch)
// ====================================================================================
static void ao_terminar_correcao_prova( GPid pid, gint wait_status, gpointer user_data ) {
   DadosCorrecaoAsync *async = ( DadosCorrecaoAsync * )user_data;
   g_spawn_close_pid( pid );

   if ( !async || !async->ctx ) {
      liberar_dados_correcao_async( async );
      return;
   }

   AppContext             *ctx       = async->ctx;
   const InterfaceDados   *dados     = &async->dados;
   const CaminhoDiretorio *caminho   = &async->caminho;
   InterfacePainel        *painel    = async->painel;
   GArray                 *map_array = async->map_array;

#if GLIB_CHECK_VERSION( 2, 70, 0 )
   gboolean proc_ok = g_spawn_check_wait_status( wait_status, NULL );
#else
   gboolean proc_ok = g_spawn_check_exit_status( wait_status, NULL );
#endif

   if ( proc_ok && map_array != NULL ) {
      // 1. Exportação de imagens corrigidas para modalidade não presencial (usando snapshot isolado)
      if ( dados->nao_presencial && map_array->len > 0 ) {
         copiar_arquivos_correcao_nao_presencial(
            ( const MapeamentoGabarito * )( void * )map_array->data,
            map_array->len,
            async
         );
      }

      // 2. Unificação dos PDFs gerados e higienização paralela
      guint   total_provas = map_array->len;
      gchar **arquivos_pdf = g_new0( gchar *, ( gsize )total_provas + 1U );
      int     qtd_sucessos = 0;

      for ( guint i = 0; i < total_provas; i++ ) {
         const MapeamentoGabarito *map = &g_array_index( map_array, MapeamentoGabarito, i );
         if ( map->status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
            arquivos_pdf[qtd_sucessos++] = g_strdup_printf( "%.2d.pdf", map->num );
         }
      }

      g_autofree gchar *nome_arquivo  = g_strdup_printf( "Correção_%d.pdf", dados->seq_prova );
      g_autofree gchar *arquivo_saida = g_build_filename( caminho->relatorios, nome_arquivo, NULL );

      g_remove( arquivo_saida );

      if ( qtd_sucessos > 0 ) {
         g_pdfunite( async->dir_compile, arquivos_pdf, qtd_sucessos, arquivo_saida );
      }

      // Limpeza paralela dos arquivos temporários (.tex, .aux, .log, .synctex.gz, .pdf)
      apagar_temporarios_latex_paralelo( async->dir_compile, arquivos_pdf, -1 );
      g_strfreev( arquivos_pdf );

      // 3. Exibição automática e notificação na UI
      if ( qtd_sucessos > 0 && g_file_test( arquivo_saida, G_FILE_TEST_IS_REGULAR ) ) {
         if ( dados->expor_dados ) {
            copiar_arquivos_correcao_externamente( dados, caminho, arquivo_saida );
         }

         g_xdg_open( arquivo_saida, NULL );

         if ( painel ) {
            painel->format_titulo    = meu_gerador_variadico( "✔ Correção Finalizada!" );
            painel->format_subtitulo = meu_gerador_variadico( "%d prova(s) unificada(s) com sucesso.", qtd_sucessos );
            painel->format_instrucao = meu_gerador_variadico( "O relatório de correção está pronto no diretório da turma." );
            criar_mensagem_painel( SUCESSO, painel );
            agendar_ocultacao_toast_acervo_lc( painel, 4000U );
         }
      } else if ( painel ) {
         painel->format_titulo    = meu_gerador_variadico( "✘ Falha na Geração do PDF:" );
         painel->format_subtitulo = meu_gerador_variadico( "Ocorreu um erro ao unificar os espelhos de correção." );
         painel->format_instrucao = meu_gerador_variadico( "Verifique se há provas válidas no lote processado." );
         criar_mensagem_painel( ERRO, painel );
         agendar_ocultacao_toast_acervo_lc( painel, 6000U );
      }

   } else if ( painel ) {
      painel->format_titulo    = meu_gerador_variadico( "✘ Falha na Compilação:" );
      painel->format_subtitulo = meu_gerador_variadico( "O motor LaTeX abortou a geração dos espelhos de correção." );
      painel->format_instrucao = meu_gerador_variadico( "Código de término do subprocesso: %d.", wait_status );
      criar_mensagem_painel( ERRO, painel );
      agendar_ocultacao_toast_acervo_lc( painel, 6000U );
   }

   // Reabilita os controles de correção e processamento na interface
   if ( ctx->prova.btn_corrigir ) {
      gtk_widget_set_sensitive( ctx->prova.btn_corrigir, TRUE );
   }
   if ( ctx->prova.btn_processar ) {
      gtk_widget_set_sensitive( ctx->prova.btn_processar, TRUE );
   }

   if ( async->cronometro ) {
      display_tempo( "PDF de Correção", async->cronometro );
   }

   liberar_dados_correcao_async( async );
}

// ====================================================================================
// DISPARADOR ASSÍNCRONO DE COMPILAÇÃO PARALELA (GNU PARALLEL + PDFLATEX)
// ====================================================================================
void g_pdflatex_parallel_async_corrigir_prova( InterfacePainel *painel, GArray *map_array, AppContext *ctx ) {
   g_return_if_fail( painel != NULL && map_array != NULL && ctx != NULL );

   g_autoptr( GError ) erro = NULL;

   DadosCorrecaoAsync *async_data = g_new0( DadosCorrecaoAsync, 1 );
   async_data->map_array          = map_array; // Assume a posse do GArray
   async_data->ctx                = ctx;
   async_data->painel             = painel;
   async_data->cabecalho          = ctx->cabecalho;

   // Snapshot profundo: garante que os relatórios e PNGs usem a turma exata do início da correção
   clonar_caminho_diretorio_lc( &ctx->caminho, &async_data->caminho );
   clonar_interface_dados_lc( &ctx->dados, &async_data->caminho, &async_data->dados );
   async_data->fichas = clonar_diario_alunos_lc(
                           ctx->diario.fichas,
                           ( guint8 )ctx->cabecalho.lim_disciplinas
                        );

   async_data->dir_compile = g_build_filename( async_data->caminho.raiz, "dados", "temporarios", NULL );
   async_data->cronometro  = g_timer_new();

   guint num_cores = g_get_num_processors();
   GPid  pid       = 0;

   g_autofree gchar *comando_interno = g_strdup_printf(
                                          "parallel -j %u nice -n 5 pdflatex -synctex=1 -interaction=nonstopmode ::: *.tex || true",
                                          num_cores
                                       );

   g_auto( GStrv ) argv = g_new0( gchar*, 4U );
   argv[0] = g_strdup( "sh" );
   argv[1] = g_strdup( "-c" );
   argv[2] = g_strdup( comando_interno );

   if ( !g_spawn_async( async_data->dir_compile, argv, NULL,
                        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                        NULL, NULL, &pid, &erro ) ) {
      // Restaura ambos os botões bloqueados em corrigir_prova()
      if ( ctx->prova.btn_corrigir ) {
         gtk_widget_set_sensitive( ctx->prova.btn_corrigir, TRUE );
      }
      if ( ctx->prova.btn_processar ) {
         gtk_widget_set_sensitive( ctx->prova.btn_processar, TRUE );
      }

      painel->format_titulo    = meu_gerador_variadico( "✘ Erro de Processamento:" );
      painel->format_subtitulo = meu_gerador_variadico( "Não foi possível iniciar o compilador paralelo para a correção." );
      painel->format_instrucao = meu_gerador_variadico( "Detalhe: %s", erro ? erro->message : "Verifique o GNU Parallel/TeX." );
      criar_mensagem_painel( ERRO, painel );
      agendar_ocultacao_toast_acervo_lc( painel, 6000U );

      liberar_dados_correcao_async( async_data );
   } else {
      g_child_watch_add( pid, ao_terminar_correcao_prova, async_data );
   }
}




