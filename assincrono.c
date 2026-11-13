/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "assincrono.h"
#include "comum.h"
#include "glib/gstdio.h"
#include "mensagens.h"
#include "provas.h"
#include "basicas.h"
#include "imagens.h"
#include "glib_gio.h"
#include "imgcore.h"



// ====================================================================================
// ESTRUTURA DE TRANSPORTE E ISOLAMENTO DA THREAD
// ====================================================================================
typedef struct {
   AppContext        *ctx;         // Usado EXCLUSIVAMENTE nas callbacks da Main Thread
   GtkWidget         *botao_gerar; // Ponteiro para o botão que disparou a ação
   MotorProvaSnapshot snap;        // O pacote de dados blindado para a Thread e para prova()
   gboolean           sucesso;
} ProvaThreadArgs;

/* Destrutor unificado para ProvaThreadArgs (Compatível com GDestroyNotify) */
static void liberar_prova_thread_args( gpointer data ) {
   ProvaThreadArgs *args = ( ProvaThreadArgs * )data;
   if ( !args ) {
      return;
   }

   if ( args->snap.fichas != NULL ) {
      g_ptr_array_unref( args->snap.fichas );
      args->snap.fichas = NULL;
   }
   g_free( args );
}

/**
 * Clona profundamente o diário de alunos na Heap para isolamento de threads (Deep Copy).
 * Retorna o ponteiro do novo vetor alocado ou NULL se houver falha.
 */
static GPtrArray* clonar_diario_alunos( GPtrArray *fichas_originais, guint8 lim_disc ) {
   if ( !fichas_originais || fichas_originais->len == 0 ) {
      return NULL;
   }

   GPtrArray *fichas_clone = g_ptr_array_new_full( fichas_originais->len, g_free );

   // Calcula o tamanho dinâmico real (Base da Ficha + Array Flexível de Histórico)
   gsize tamanho_real_ficha = sizeof( FichaAluno ) +
                              ( ( gsize )lim_disc * sizeof( HistoricoDisciplina ) );

   for ( guint i = 0; i < fichas_originais->len; i++ ) {
      const FichaAluno *orig = ( const FichaAluno * )g_ptr_array_index( fichas_originais, i );
      if ( !orig ) {
         continue;
      }

      // g_memdup2 aloca e copia a estrutura + histórico flexível em uma única operação
      FichaAluno *clone = ( FichaAluno * )g_memdup2( orig, tamanho_real_ficha );
      g_ptr_array_add( fichas_clone, clone );
   }

   return fichas_clone;
}

// ====================================================================================
// ENTRADA DO MOTOR ASSÍNCRONO
// ====================================================================================
void disparar_geracao_prova_assincrona( GtkWidget *widget, AppContext *ctx, void *( *funcao_background )( void * ) ) {
   g_return_if_fail( widget != NULL && ctx != NULL && funcao_background != NULL );

   ProvaThreadArgs *args = g_new0( ProvaThreadArgs, 1 );

   // 1. Snapshot Seguro (Deep Copy obrigatório para isolar a thread da UI)
   args->snap.dados     = ctx->dados;
   args->snap.cabecalho = ctx->cabecalho;
   args->snap.caminho   = ctx->caminho;
   args->snap.hoje      = ctx->hoje;
   args->snap.fichas    = clonar_diario_alunos( ctx->diario.fichas, ( guint8 )ctx->cabecalho.limite_disciplinas );

   // 2. Referências Mestre para a Main Thread
   args->ctx         = ctx;
   args->botao_gerar = widget;

   // 3. Congelamento Parcial da Interface
   GtkWidget *aba_latex = gtk_notebook_get_nth_page( GTK_NOTEBOOK( ctx->notebook ), 1 );

   gtk_widget_set_sensitive( widget, FALSE );
   if ( ctx->prova.btn_corrigir ) {
      gtk_widget_set_sensitive( ctx->prova.btn_corrigir, FALSE );
   }
   if ( ctx->prova.btn_importar ) {
      gtk_widget_set_sensitive( ctx->prova.btn_importar, FALSE );
   }
   if ( aba_latex ) {
      gtk_widget_set_sensitive( aba_latex, FALSE );
   }

   // 4. Criação da Thread Detached
   pthread_t thread_id;
   pthread_attr_t attr;
   pthread_attr_init( &attr );
   pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

   if ( pthread_create( &thread_id, &attr, funcao_background, args ) != 0 ) {
      // Rollback imediato em caso de erro na alocação da thread do SO
      gtk_widget_set_sensitive( widget, TRUE );
      if ( ctx->prova.btn_corrigir ) {
         gtk_widget_set_sensitive( ctx->prova.btn_corrigir, TRUE );
      }
      if ( ctx->prova.btn_importar ) {
         gtk_widget_set_sensitive( ctx->prova.btn_importar, TRUE );
      }
      if ( aba_latex ) {
         gtk_widget_set_sensitive( aba_latex, TRUE );
      }

      g_printerr( "[ERRO] Falha ao criar a thread de geração de prova.\n" );
      liberar_prova_thread_args( args );
   }

   pthread_attr_destroy( &attr );
}

// ====================================================================================
// CALLBACK: Oculta o painel Toast (Executa na Main Thread)
// ====================================================================================
static gboolean ocultar_painel_toast_cb( gpointer user_data ) {
   ProvaThreadArgs *args = ( ProvaThreadArgs * )user_data;
   if ( !args || !args->ctx ) {
      return G_SOURCE_REMOVE;
   }

   InterfacePainel *painel = &args->ctx->painel;

   if ( painel->mouse_hover ) {
      return G_SOURCE_CONTINUE; // Mantém o Toast vivo enquanto o cursor estiver sobre ele
   }

   if ( painel->revealer_painel ) {
      gtk_revealer_set_reveal_child( GTK_REVEALER( painel->revealer_painel ), FALSE );
   }
   painel->timeout_id = 0;

   // Ao retornar G_SOURCE_REMOVE, a própria GLib invoca liberar_prova_thread_args(args)
   g_print( "[GLib Timer] Toast ocultado. Limpeza de ProvaThreadArgs concluída.\n" );
   return G_SOURCE_REMOVE;
}

// ====================================================================================
// CALLBACK: Retorno da Thread (Executa na Main Thread via g_idle_add)
// ====================================================================================
static gboolean reativar_botao_gerar_prova( gpointer user_data ) {
   ProvaThreadArgs *args = ( ProvaThreadArgs * )user_data;
   if ( !args || !args->ctx ) {
      return G_SOURCE_REMOVE;
   }

   AppContext *ctx = args->ctx;
   GtkWidget *aba_latex = gtk_notebook_get_nth_page( GTK_NOTEBOOK( ctx->notebook ), 1 );

   // Reativa a UI acessando os ponteiros mestre armazenados no AppContext
   if ( args->botao_gerar ) {
      gtk_widget_set_sensitive( args->botao_gerar, TRUE );
   }
   if ( ctx->prova.btn_corrigir ) {
      gtk_widget_set_sensitive( ctx->prova.btn_corrigir, TRUE );
   }
   if ( ctx->prova.btn_importar ) {
      gtk_widget_set_sensitive( ctx->prova.btn_importar, TRUE );
   }
   if ( aba_latex ) {
      gtk_widget_set_sensitive( aba_latex, TRUE );
   }

   char instrucao[256] = { 0 };
   InterfacePainel *painel = &ctx->painel;

   if ( args->sucesso ) {
      g_snprintf( instrucao, sizeof( instrucao ), "Pronto para impressão: %s Prova.pdf (em tela)",
                  args->snap.dados.prova_sequencia );

      painel->format_titulo    = meu_gerador_variadico( "✔ Prova Gerada com Sucesso!" );
      painel->format_subtitulo = meu_gerador_variadico( "O arquivo PDF foi compilado e estruturado corretamente via TeX." );
      painel->format_instrucao = meu_gerador_variadico( "%s", instrucao );

      criar_mensagem_painel( SUCESSO, painel );
   } else {
      g_snprintf( instrucao, sizeof( instrucao ), "Verifique o arquivo lista.dat do \"%s\" ou os logs do compilador.",
                  args->snap.dados.periodo );

      painel->format_titulo    = meu_gerador_variadico( "✘ Erro no Motor TeX" );
      painel->format_subtitulo = meu_gerador_variadico( "Falha crítica na estruturação ou compilação assíncrona dos gabaritos." );
      painel->format_instrucao = meu_gerador_variadico( "%s", instrucao );

      criar_mensagem_painel( ERRO, painel );
   }

   // Se havia um Toast anterior ativo, g_source_remove aciona automaticamente o GDestroyNotify dele!
   if ( painel->timeout_id > 0 ) {
      g_source_remove( painel->timeout_id );
      painel->timeout_id = 0;
   }

   guint tempo_exibicao = ( args->sucesso ) ? 4000U : 6000U;
   painel->timeout_id = g_timeout_add_full(
                           G_PRIORITY_DEFAULT, tempo_exibicao, ocultar_painel_toast_cb, args, liberar_prova_thread_args
                        );

   return G_SOURCE_REMOVE;
}

// ====================================================================================
// WORKER: Executa em Background I/O Intensivo e Compilação LaTeX
// ====================================================================================
void* thread_gerar_prova_background( void *data ) {
   g_return_val_if_fail( data != NULL, NULL );

   g_autoptr( GTimer ) cronometro = g_timer_new();
   ProvaThreadArgs *args = ( ProvaThreadArgs * )data;
   MotorProvaSnapshot *snap = &args->snap;

   args->sucesso = FALSE;
   g_print( "[Thread] Iniciando compilação com contexto clonado...\n" );

   // Validação defensiva das dimensões antes de calcular o bloco contíguo
   if ( snap->dados.qtd_alunos_ativos <= 0 || snap->dados.max_quest_prova <= 0 ) {
      g_printerr( "[ERRO] Parâmetros de alunos (%d) ou máximo de questões (%d) inválidos.\n",
                  snap->dados.qtd_alunos_ativos, snap->dados.max_quest_prova );
      g_idle_add( reativar_botao_gerar_prova, args );
      return NULL;
   }

   g_autofree char *destino = g_build_filename(
                                 snap->caminho.raiz, "dados", "gabaritos", snap->dados.ano, snap->dados.escola, "gabaritos", NULL
                              );

   char nome[64] = { 0 };
   nome_base_gabaritos_bin( nome, sizeof( nome ), snap->cabecalho.foco_turma, snap->cabecalho.foco_disciplina,
                            snap->cabecalho.foco_periodo, snap->dados.seq_prova );

   g_autofree char *arquivo = g_build_filename( destino, nome, NULL );

   FILE *p = g_fopen( arquivo, "rb" );
   if ( !p ) {
      g_printerr( "[ERRO] Falha ao abrir o arquivo binário de gabaritos: %s\n", arquivo );
      g_idle_add( reativar_botao_gerar_prova, args );
      return NULL;
   }

   // =========================================================================
   // LEITURA EM BLOCO 1D CONTÍGUO (Sem ItemTextoCurto)
   // =========================================================================
   gsize num_alunos = ( gsize )snap->dados.qtd_alunos_ativos;
   gsize max_len    = ( gsize )snap->dados.max_quest_prova + 1; // +1 para o '\0'

   g_autofree char *G = g_malloc0( num_alunos * max_len );
   gsize lidos = fread( G, max_len, num_alunos, p );
   fclose( p );

   if ( lidos != num_alunos ) {
      g_printerr( "[AVISO] Esperava %" G_GSIZE_FORMAT " gabaritos, mas foram lidos %" G_GSIZE_FORMAT " em %s.\n",
                  num_alunos, lidos, arquivo );
   }

   if ( lidos > 0 ) {
      args->sucesso = TRUE;

      // Passa o bloco 1D plano 'G' para prova( MotorProvaSnapshot *snap, const char *G )
      // Dentro de prova(), o gabarito do aluno 'i' é acessado via: const char *gab_i = G + (i * max_len);
      prova( snap, G );

      salvar_estado_aplicativo( &snap->dados, &snap->caminho );
   }

   // 'G' é desalocado automaticamente pelo g_autofree ao sair do escopo.
   g_idle_add( reativar_botao_gerar_prova, args );
   display_tempo( "Geração de prova", cronometro );

   return NULL;
}












// ====================================================================================
// ESTRUTURA DE TRANSPORTE E ISOLAMENTO DA THREAD DE VISÃO COMPUTACIONAL
// ====================================================================================
typedef struct {
   InterfacePainel   *painel;          // Ponteiro vivo para a Main Thread (lê mouse_hover)
   InterfaceDados     dados;           // Cópia estática por valor (Isolamento da Thread)
   InterfaceCabecalho cabecalho;       // Cópia estática por valor
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
   g_free( args );
}

// ====================================================================================
// DISPARADOR ASSÍNCRONO DE PROCESSAMENTO DE IMAGENS
// ====================================================================================
void disparar_processamento_imagens_assincrono( GtkWidget *widget, AppContext *ctx, void *( *funcao_background )( void * ) ) {
   g_return_if_fail( widget != NULL && ctx != NULL && funcao_background != NULL );

   ProcessarThreadArgs *args = g_new0( ProcessarThreadArgs, 1 );

   // 1. Snapshot por Valor: Isolando os dados da Thread Principal GTK
   args->dados     = ctx->dados;
   args->cabecalho = ctx->cabecalho;

   // 2. Referências Vivas (Acessadas exclusivamente na Main Thread)
   args->painel          = &ctx->painel;
   args->botao_processar = widget;
   args->botao_corrigir  = ctx->prova.btn_corrigir;
   args->n_rejeitadas    = -1;

   // 3. Bloqueio preventivo dos controles conflitantes na UI
   gtk_widget_set_sensitive( widget, FALSE );
   if ( ctx->prova.btn_corrigir ) {
      gtk_widget_set_sensitive( ctx->prova.btn_corrigir, FALSE );
   }

   // 4. Criação da Thread Detached
   pthread_t thread_id;
   pthread_attr_t attr;
   pthread_attr_init( &attr );
   pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

   if ( pthread_create( &thread_id, &attr, funcao_background, args ) != 0 ) {
      g_printerr( "[ERRO] Falha ao criar a thread de processamento de imagens.\n" );

      gtk_widget_set_sensitive( widget, TRUE );
      if ( ctx->prova.btn_corrigir ) {
         gtk_widget_set_sensitive( ctx->prova.btn_corrigir, TRUE );
      }
      liberar_processar_thread_args( args );
   }

   pthread_attr_destroy( &attr );
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
   g_print( "[GLib Timer] Toast CV ocultado. Limpeza de ProcessarThreadArgs concluída.\n" );
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
   painel->timeout_id = g_timeout_add_full(
                           G_PRIORITY_DEFAULT, tempo_exibicao, ocultar_painel_cv_cb, args, liberar_processar_thread_args
                        );

   g_print( "[Thread CV] Mensagem emitida. Aguardando término do Toast para desalocar Heap...\n" );
   return G_SOURCE_REMOVE;
}

// ====================================================================================
// WORKER: Executa o Pipeline OMR em Background
// ====================================================================================
void* thread_processar_imagens_background( void *data ) {
   g_return_val_if_fail( data != NULL, NULL );

   g_autoptr( GTimer ) cronometro = g_timer_new();
   ProcessarThreadArgs *args = ( ProcessarThreadArgs * )data;

   g_print( "[Thread CV] Iniciando o processamento assíncrono das imagens...\n" );

   // args->n_rejeitadas = gas_processar_imagens( &args->dados, &args->cabecalho );
   // args->n_rejeitadas = omr_processar_imagens( &args->dados, &args->cabecalho );
   args->n_rejeitadas = cs_omr_processar_imagens( &args->dados, &args->cabecalho );

   g_print( "[Thread CV] Processamento concluído. Retornando o controle para a UI.\n" );

   // Agenda a execução do retorno gráfico na Thread Principal do GTK
   g_idle_add( reativar_botao_processar_imagens, args );

   display_tempo( "Processamento", cronometro );
   return NULL;
}










// ====================================================================================
// ESTRUTURA DE TRANSPORTE E ISOLAMENTO DO COMPILADOR DE ACERVO
// ====================================================================================
typedef struct {
   char            *tema;                   // Cópia profunda de ctx->dados.tema
   int              qtd_subtemas;           // Cópia de ctx->prova.limite_subtemas
   GtkWidget       *btn_compilar;           // Referência de UI (ctx->latex.btn_compilar)
   GtkWidget       *btn_pdflatex;           // Referência de UI (ctx->latex.btn_pdflatex)
   GtkWidget       *widget;                 // Botão exato que originou o disparo
   GStrv            subtemas;               // Deep copy NULL-terminated das strings de subtemas
   InterfacePainel *painel;                 // Painel de notificações Toast
   char            *dir_compile;            // Cópia do diretório temporário de compilação
   char            *caminho_banco_questoes; // Cópia de ctx->caminho.banco_questoes
} DadosCompilacaoAsync;

/* Destrutor centralizado para garantir higienização total (inclusive da GStrv de subtemas) */
static void liberar_dados_compilacao_async( DadosCompilacaoAsync *async_data ) {
   if ( !async_data ) {
      return;
   }
   g_free( async_data->tema );
   g_free( async_data->dir_compile );
   g_free( async_data->caminho_banco_questoes );
   g_strfreev( async_data->subtemas ); // Libera cada string clonada e o vetor de ponteiros
   g_free( async_data );
}

// ====================================================================================
// DIAGNÓSTICO DE INTEGRIDADE DOS PDFS DO ACERVO
// ====================================================================================
static gboolean verificar_pdfs_latex_acervo_questoes( InterfacePainel *painel, const char *tema,
      int qtd_subtemas, int falhas ) {
   g_return_val_if_fail( painel != NULL && tema != NULL, FALSE );

   if ( falhas == 0 && qtd_subtemas > 0 ) {
      // --- ESTADO 1: SUCESSO TOTAL ---
      painel->format_titulo    = meu_gerador_variadico( "✔ Sucesso Total!" );
      painel->format_subtitulo = meu_gerador_variadico( "Todos os %d PDFs foram gerados corretamente.", qtd_subtemas );
      painel->format_instrucao = meu_gerador_variadico( "Iniciando concatenação dos PDFs para %s.pdf...", tema );
      criar_mensagem_painel( SUCESSO, painel );
      return TRUE;

   } else if ( falhas < qtd_subtemas ) {
      // --- ESTADO 2: ALERTA (SUCESSO PARCIAL) ---
      int sucessos = qtd_subtemas - falhas;
      painel->format_titulo    = meu_gerador_variadico( "⚠ Atenção (Sucesso Parcial):" );
      painel->format_subtitulo = meu_gerador_variadico( "Processados %d de %d arquivos com sucesso.", sucessos, qtd_subtemas );
      painel->format_instrucao = meu_gerador_variadico( "Houve falha em %d item(ns). Unindo os PDFs disponíveis...", falhas );
      criar_mensagem_painel( AVISO, painel );
      return TRUE;

   } else {
      // --- ESTADO 3: ERRO CRÍTICO (NADA FOI GERADO) ---
      painel->format_titulo    = meu_gerador_variadico( "✘ Erro de Processamento:" );
      painel->format_subtitulo = meu_gerador_variadico( "Nenhum arquivo PDF foi gerado pelo LaTeX." );
      painel->format_instrucao = meu_gerador_variadico( "Verifique a pasta %s.", tema );
      criar_mensagem_painel( ERRO, painel );
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

   if ( proc_ok && async_data->qtd_subtemas > 0 && async_data->subtemas != NULL ) {
      g_print( "[SUCESSO] O tema '%s' foi compilado perfeitamente!\n", async_data->tema );

      // 1. Prepara o array dinâmico com os nomes exatos dos PDFs efetivamente gerados
      gsize cap_pdfs     = ( gsize )async_data->qtd_subtemas + 1U;
      char **arquivos_pdf = g_new0( char *, cap_pdfs );
      int qtd_sucessos_reais = 0;

      for ( int i = 0; i < async_data->qtd_subtemas; i++ ) {
         const char *subtema_atual = async_data->subtemas[i];
         if ( !subtema_atual || subtema_atual[0] == '\0' ) {
            continue;
         }

         char *nome_pdf             = g_strdup_printf( "%s.pdf", subtema_atual );
         g_autofree char *path_teste = g_build_filename( async_data->dir_compile, nome_pdf, NULL );

         if ( g_file_test( path_teste, G_FILE_TEST_EXISTS ) ) {
            arquivos_pdf[qtd_sucessos_reais++] = nome_pdf; // Transfere a posse diretamente (sem segundo g_strdup!)
         } else {
            g_free( nome_pdf );
         }
      }

      int falhas = async_data->qtd_subtemas - qtd_sucessos_reais;

      if ( verificar_pdfs_latex_acervo_questoes( async_data->painel, async_data->tema,
            async_data->qtd_subtemas, falhas ) ) {

         // 2. Caminho absoluto do arquivo final unificado do tema
         g_autofree char *nome_arquivo  = g_strdup_printf( "%s.pdf", async_data->tema );
         g_autofree char *arquivo_saida = g_build_filename( async_data->caminho_banco_questoes, nome_arquivo, NULL );

         // 3. Unifica os PDFs disponíveis
         g_remove( arquivo_saida );
         if ( qtd_sucessos_reais > 0 ) {
            g_pdfunite( async_data->dir_compile, arquivos_pdf, qtd_sucessos_reais, arquivo_saida );
         }

         // 4. Limpeza dos arquivos temporários de cada subtema
         for ( int i = 0; i < async_data->qtd_subtemas; i++ ) {
            if ( async_data->subtemas[i] != NULL ) {
               apagar_arquivos_temporarios_latex_nativamente( async_data->dir_compile, async_data->subtemas[i], 5 );
            }
         }

         if ( async_data->widget == async_data->btn_pdflatex &&
               g_file_test( arquivo_saida, G_FILE_TEST_EXISTS ) ) {
            g_xdg_open( arquivo_saida );
         }
      }

      // Liberação explícita do vetor NULL-terminated (compreendida nativamente pelo clang-analyzer)
      g_strfreev( arquivos_pdf );

   } else if ( !proc_ok ) {
      g_printerr( "[AVISO] Compilação do tema '%s' interrompida ou com erros (status %d).\n",
                  async_data->tema, wait_status );

      if ( async_data->painel ) {
         async_data->painel->format_titulo    = meu_gerador_variadico( "✘ Falha na Compilação" );
         async_data->painel->format_subtitulo = meu_gerador_variadico( "O processo paralelo retornou erro ao compilar '%s'.", async_data->tema );
         async_data->painel->format_instrucao = meu_gerador_variadico( "Verifique os logs do terminal." );
         criar_mensagem_painel( ERRO, async_data->painel );
      }
   }

   // Reabilita os botões na interface com verificação de ponteiro
   if ( async_data->btn_pdflatex ) {
      gtk_widget_set_sensitive( async_data->btn_pdflatex, TRUE );
   }
   if ( async_data->btn_compilar ) {
      gtk_widget_set_sensitive( async_data->btn_compilar, TRUE );
   }

   // Desalocação profunda e unificada
   liberar_dados_compilacao_async( async_data );
}

// ====================================================================================
// DISPARADOR ASSÍNCRONO DE COMPILAÇÃO DO BANCO DE QUESTÕES
// ====================================================================================
void g_pdflatex_parallel_async( GtkWidget *widget, const char *dir_compile,
                                InterfacePainel *painel, const AppContext *ctx ) {
   g_return_if_fail( dir_compile != NULL && painel != NULL && ctx != NULL );

   if ( ctx->prova.limite_subtemas <= 0 || ctx->prova.subtemas == NULL ) {
      g_printerr( "[AVISO] Nenhum subtema disponível para compilação em '%s'.\n", dir_compile );
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
   async_data->qtd_subtemas           = ctx->prova.limite_subtemas;
   async_data->btn_compilar           = ctx->latex.btn_compilar;
   async_data->btn_pdflatex           = ctx->latex.btn_pdflatex;
   async_data->widget                 = widget;
   async_data->painel                 = painel;
   async_data->dir_compile            = g_strdup( dir_compile );
   async_data->caminho_banco_questoes = g_strdup( ctx->caminho.banco_questoes );

   // Deep Copy real do vetor de strings (+1 para o terminador NULL exigido por g_strfreev)
   async_data->subtemas = g_new0( gchar *, ( gsize )async_data->qtd_subtemas + 1 );
   for ( int i = 0; i < async_data->qtd_subtemas; i++ ) {
      async_data->subtemas[i] = g_strdup( ctx->prova.subtemas[i] );
   }

   guint num_cores = g_get_num_processors();
   g_autoptr( GError ) erro = NULL;
   GPid pid = 0;

   g_autofree char *comando_interno = g_strdup_printf(
                                         "parallel -j %u nice -n 5 pdflatex -synctex=1 -interaction=nonstopmode ::: *.tex || true",
                                         num_cores
                                      );

   g_auto( GStrv ) argv = g_new0( gchar *, 4 );
   argv[0] = g_strdup( "sh" );
   argv[1] = g_strdup( "-c" );
   argv[2] = g_strdup( comando_interno );

   if ( !g_spawn_async( dir_compile, argv, NULL,
                        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                        NULL, NULL, &pid, &erro ) ) {

      g_printerr( "[ERRO FATAL] Falha ao compilar assíncrono: %s\n", erro->message );

      if ( async_data->btn_pdflatex ) {
         gtk_widget_set_sensitive( async_data->btn_pdflatex, TRUE );
      }
      if ( async_data->btn_compilar ) {
         gtk_widget_set_sensitive( async_data->btn_compilar, TRUE );
      }

      painel->format_titulo    = meu_gerador_variadico( "✘ Erro de Processamento" );
      painel->format_subtitulo = meu_gerador_variadico( "Não foi possível iniciar o GNU Parallel." );
      painel->format_instrucao = meu_gerador_variadico( "Verifique a instalação do ambiente TeX/Parallel." );
      criar_mensagem_painel( ERRO, painel );

      liberar_dados_compilacao_async( async_data );
   } else {
      g_child_watch_add( pid, ao_terminar_compilacao_banco, async_data );
   }
}







// ====================================================================================
// ESTRUTURA DE TRANSPORTE E GESTÃO DO SUBPROCESSO DE CORREÇÃO
// ====================================================================================
typedef struct {
   GArray          *map_array;   // Posse transferida de corrigir_prova()
   AppContext      *ctx;         // Contexto mestre do aplicativo
   InterfacePainel *painel;      // Painel de notificações Toast
   char            *dir_compile; // Diretório temporário de compilação
   GTimer          *cronometro;  // Medidor de performance do lote
} DadosCorrecaoAsync;

/* Destrutor centralizado para evitar vazamento de campos em qualquer fluxo de saída */
static void liberar_dados_correcao_async( DadosCorrecaoAsync *async ) {
   if ( !async ) {
      return;
   }
   if ( async->cronometro ) {
      g_timer_destroy( async->cronometro );
      async->cronometro = NULL;
   }
   if ( async->map_array ) {
      g_array_free( async->map_array, TRUE );
      async->map_array = NULL;
   }
   g_free( async->dir_compile );
   g_free( async );
}

// ====================================================================================
// EXPORTAÇÃO INSTITUCIONAL E CONVERSÃO NÃO PRESENCIAL
// ====================================================================================
static void copiar_arquivos_correcao_externamente( const InterfaceDados *dados, const CaminhoDiretorio *caminho,
      const char *arquivo_saida ) {
   g_return_if_fail( dados != NULL && caminho != NULL && arquivo_saida != NULL );

   g_autofree char *nome_arquivo_escola = NULL;
   if ( dados->periodo[0] == 'R' ) {
      nome_arquivo_escola = g_strdup_printf( "Correção Recuperação Final - %s - %s - %s.pdf",
                                             dados->ano, dados->turma, dados->disciplina );
   } else {
      int inicial_per = ( dados->periodo[0] != '\0' ) ? dados->periodo[0] : '1';
      nome_arquivo_escola = g_strdup_printf( "Correção %s Prova - %s_%c - %s - %s.pdf",
                                             dados->prova_sequencia, dados->ano, inicial_per,
                                             dados->turma, dados->disciplina );
   }

   g_autofree char *pasta_provas_escola = g_build_filename( caminho->externo_escola, "Correções", NULL );
   g_autofree char *destino_escola      = g_build_filename( pasta_provas_escola, nome_arquivo_escola, NULL );

   if ( g_mkdir_with_parents( pasta_provas_escola, 0755 ) != 0 ) {
      g_printerr( "[ERRO] Não foi possível criar a pasta externa de correções: %s\n", pasta_provas_escola );
      return;
   }

   if ( !gio_copiar_arquivo( arquivo_saida, destino_escola ) ) {
      g_printerr( "[ERRO] Falha ao salvar a cópia institucional na pasta Correções da Escola!\n" );
   }
}

//------------------------------------------------------------------------------------------------------
static void copiar_arquivos_correcao_nao_presencial( const MapeamentoGabarito *map, guint qtd_linhas,
      const AppContext *ctx ) {
   g_return_if_fail( map != NULL && ctx != NULL && ctx->diario.fichas != NULL );

   const InterfaceDados   *dados   = &ctx->dados;
   const CaminhoDiretorio *caminho = &ctx->caminho;

   g_autofree char *diretorio_imagens = NULL;

   if ( ctx->cabecalho.foco_periodo == RECUPERACAO_FINAL ) {
      diretorio_imagens = g_build_filename( caminho->externo, "Notas", "Recuperação Final Imagens Corrigidas", NULL );
   } else {
      g_autofree char *pasta_imagens = g_strdup_printf( "%s Prova Imagens Corrigidas", dados->prova_sequencia );
      diretorio_imagens = g_build_filename( caminho->externo, "Notas", pasta_imagens, NULL );
   }

   if ( g_mkdir_with_parents( diretorio_imagens, 0755 ) != 0 ) {
      g_printerr( "[ERRO CRÍTICO] Falha ao criar a hierarquia externa de pastas: %s\n", diretorio_imagens );
      return;
   }

   guint total_fichas = ctx->diario.fichas->len;

   #pragma omp parallel for schedule(static)
   for ( guint i = 0; i < qtd_linhas; i++ ) {
      if ( map[i].status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
         int num_aluno = map[i].num;

         if ( num_aluno > 0 && ( guint )num_aluno <= total_fichas ) {
            const FichaAluno *ficha = ( const FichaAluno * )g_ptr_array_index( ctx->diario.fichas, ( guint )( num_aluno - 1 ) );
            if ( !ficha ) {
               continue;
            }

            g_autofree char *nome_pdf           = g_strdup_printf( "%.2d.pdf", num_aluno );
            g_autofree char *thread_caminho_pdf = g_build_filename( caminho->raiz, "dados", "temporarios", nome_pdf, NULL );
            g_autofree char *nome_arquivo_png   = g_strdup_printf( "%.2d - %s.png", num_aluno, ficha->aluno );
            g_autofree char *thread_caminho_png = g_build_filename( diretorio_imagens, nome_arquivo_png, NULL );

            if ( !pdf2png( thread_caminho_pdf, thread_caminho_png, 1.5 ) ) {
               g_printerr( "[AVISO] Falha ao converter e mover imagem %s\n", thread_caminho_png );
            }
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

   AppContext       *ctx       = async->ctx;
   InterfaceDados   *dados     = &ctx->dados;
   CaminhoDiretorio *caminho   = &ctx->caminho;
   InterfacePainel  *painel    = async->painel;
   GArray           *map_array = async->map_array;

#if GLIB_CHECK_VERSION( 2, 70, 0 )
   gboolean proc_ok = g_spawn_check_wait_status( wait_status, NULL );
#else
   gboolean proc_ok = g_spawn_check_exit_status( wait_status, NULL );
#endif

   if ( proc_ok && map_array != NULL ) {
      g_print( "[SUCESSO] Processamento assíncrono do LaTeX concluído.\n" );

      // 1. Exportação de imagens corrigidas para modalidade não presencial
      if ( dados->nao_presencial && map_array->len > 0 ) {
         copiar_arquivos_correcao_nao_presencial(
            ( const MapeamentoGabarito * )( void * )map_array->data, map_array->len, ctx
         );
      }

      // 2. Unificação dos PDFs gerados e higienização paralela
      guint total_provas = map_array->len;
      g_auto( GStrv ) arquivos_pdf = g_new0( char *, ( gsize )total_provas + 1 );
      int qtd_sucessos = 0;

      for ( guint i = 0; i < total_provas; i++ ) {
         const MapeamentoGabarito *map = &g_array_index( map_array, MapeamentoGabarito, i );
         if ( map->status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
            arquivos_pdf[qtd_sucessos++] = g_strdup_printf( "%.2d.pdf", map->num );
         }
      }

      g_autofree char *nome_arquivo  = g_strdup_printf( "Correção_%d.pdf", dados->seq_prova );
      g_autofree char *arquivo_saida = g_build_filename( caminho->relatorios, nome_arquivo, NULL );

      g_remove( arquivo_saida );

      if ( qtd_sucessos > 0 ) {
         g_pdfunite( async->dir_compile, arquivos_pdf, qtd_sucessos, arquivo_saida );
      }

      // Limpeza paralela dos arquivos temporários (.tex, .aux, .log, .synctex.gz, .pdf)
      #pragma omp parallel for schedule(static)
      for ( guint i = 0; i < total_provas; i++ ) {
         const MapeamentoGabarito *map = &g_array_index( map_array, MapeamentoGabarito, i );
         if ( map->status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
            g_autofree gchar *nome_base = g_strdup_printf( "%.2d", map->num );
            apagar_arquivos_temporarios_latex_nativamente( async->dir_compile, nome_base, 5 );
         }
      }

      // 3. Exibição automática e notificação na UI
      if ( qtd_sucessos > 0 && g_file_test( arquivo_saida, G_FILE_TEST_EXISTS ) ) {
         if ( dados->expor_dados ) {
            copiar_arquivos_correcao_externamente( dados, caminho, arquivo_saida );
         }

         g_xdg_open( arquivo_saida );

         painel->format_titulo    = meu_gerador_variadico( "✔ Correção Finalizada" );
         painel->format_subtitulo = meu_gerador_variadico( "%d provas unificadas com sucesso.", qtd_sucessos );
         painel->format_instrucao = meu_gerador_variadico( "As imagens corrigidas estão prontas no diretório base." );
         criar_mensagem_painel( SUCESSO, painel );
      } else {
         painel->format_titulo    = meu_gerador_variadico( "✘ Falha na Geração do PDF" );
         painel->format_subtitulo = meu_gerador_variadico( "Ocorreu um erro ao unificar os arquivos." );
         painel->format_instrucao = meu_gerador_variadico( "Verifique se há provas válidas ou se o pdfunite falhou." );
         criar_mensagem_painel( ERRO, painel );
      }

   } else {
      g_printerr( "[ERRO FATAL] O comando LaTeX assíncrono falhou (Status: %d).\n", wait_status );
      painel->format_titulo    = meu_gerador_variadico( "✘ Falha na Compilação" );
      painel->format_subtitulo = meu_gerador_variadico( "O motor LaTeX abortou o processamento." );
      painel->format_instrucao = meu_gerador_variadico( "Verifique os logs do terminal." );
      criar_mensagem_painel( ERRO, painel );
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
   async_data->map_array   = map_array; // Assume a posse do GArray
   async_data->ctx         = ctx;
   async_data->painel      = painel;
   async_data->dir_compile = g_build_filename( ctx->caminho.raiz, "dados", "temporarios", NULL );
   async_data->cronometro  = g_timer_new();

   guint num_cores = g_get_num_processors();
   GPid pid = 0;

   g_autofree char *comando_interno = g_strdup_printf(
                                         "parallel -j %u nice -n 5 pdflatex -synctex=1 -interaction=nonstopmode ::: *.tex || true",
                                         num_cores
                                      );

   g_auto( GStrv ) argv = g_new0( gchar *, 4 );
   argv[0] = g_strdup( "sh" );
   argv[1] = g_strdup( "-c" );
   argv[2] = g_strdup( comando_interno );

   if ( !g_spawn_async( async_data->dir_compile, argv, NULL,
                        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                        NULL, NULL, &pid, &erro ) ) {

      g_printerr( "[ERRO FATAL] Falha ao iniciar correção assíncrona: %s\n", erro->message );

      // Limpa tudo com segurança (incluindo GTimer, GArray e dir_compile)
      liberar_dados_correcao_async( async_data );

      // Restaura ambos os botões bloqueados em corrigir_prova()
      if ( ctx->prova.btn_corrigir ) {
         gtk_widget_set_sensitive( ctx->prova.btn_corrigir, TRUE );
      }
      if ( ctx->prova.btn_processar ) {
         gtk_widget_set_sensitive( ctx->prova.btn_processar, TRUE );
      }

      painel->format_titulo    = meu_gerador_variadico( "✘ Erro de Processamento" );
      painel->format_subtitulo = meu_gerador_variadico( "Não foi possível iniciar o GNU Parallel." );
      painel->format_instrucao = meu_gerador_variadico( "Tente novamente." );
      criar_mensagem_painel( ERRO, painel );
   } else {
      g_child_watch_add( pid, ao_terminar_correcao_prova, async_data );
   }
}




