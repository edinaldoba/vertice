/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>

#include "assincrono.h"
#include "comum.h"
#include "glib/gstdio.h"
#include "interface.h"
#include "mensagens.h"
#include "provas.h"
#include "basicas.h"
#include "imagens.h"
#include "glib_gio.h"
#include "imgcore.h"




typedef struct {
   InterfacePainel  *painel;         // ◄ PONTEIRO (Lê a UI viva, garantindo que o mouse_hover funcione)
   InterfaceDados   dados;           // ◄ Por valor (Cópia estática blindada)
   InterfaceListas  listas;          // ◄ Por valor
   FocoCoordenadas  foco;            // ◄ Por valor
   CaminhoDiretorio caminho;         // ◄ Por valor
   CalendarioData   data;            // ◄ Por valor
   GArray           *fichas;         // ◄ Ponteiro (Clone seguro na Heap para a Thread)
   GtkWidget        *botao_gerar;    // ◄ Ponteiro (Widget vivo)
   GtkWidget        *botao_corrigir; // ◄ Ponteiro (Widget vivo)
   GtkWidget        *botao_importar; // ◄ Ponteiro (Widget vivo)
   GtkWidget        *aba_latex;      // ◄ Ponteiro (Widget vivo)
   bool             sucesso;
} ProvaThreadArgs;

/**
 * Clona profundamente o diário de alunos na Heap para isolamento de threads (Deep Copy).
 * Retorna o ponteiro do novo vetor alocado ou NULL se houver falha.
 */
static GArray* clonar_diario_alunos( GArray *fichas_originais ) {
   if ( !fichas_originais || fichas_originais->len == 0 ) return NULL;

   // Criação do novo GArray independente (limpo com zeros)
   GArray *fichas_clone = g_array_sized_new( FALSE, TRUE, sizeof( FichaAluno ), fichas_originais->len );

   // Cópia física bruta (Block Copy) bit a bit dos elementos internos
   g_array_append_vals( fichas_clone, fichas_originais->data, fichas_originais->len );

   return fichas_clone;
}

// ============================================================================
// A NOVA FUNÇÃO DE ENTRADA DO MOTOR
// ============================================================================
void disparar_geracao_prova_assincrona( GtkWidget *widget, AppContext *ctx, void *( *funcao_background )( void * ) ) {
   ProvaThreadArgs *args = malloc( sizeof( ProvaThreadArgs ) );
   if ( !args ) return;

   // Snapshots por valor (Segurança Thread-Safe)
   args->dados   = ctx->dados;
   args->foco    = ctx->cascata.foco;
   args->caminho = ctx->caminho;
   args->data    = ctx->data;
   args->listas  = ctx->listas;

   // Snapshots por referência (Acesso UI Viva e Clone Heap)
   args->painel      = &ctx->painel; // Ponteiro para o painel atual do AppContext
   args->fichas      = clonar_diario_alunos( ctx->fichas );
   args->botao_gerar = widget;
   args->botao_corrigir = ctx->button.corrigir_prova;
   args->botao_importar = ctx->button.importar_dados;

   GtkWidget *aba_latex = gtk_notebook_get_nth_page( GTK_NOTEBOOK( ctx->notebook ), 1 ); // ABA LATEX
   args->aba_latex = aba_latex;

   pthread_t thread_id;
   pthread_attr_t attr;
   pthread_attr_init( &attr );
   pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

   gtk_widget_set_sensitive( widget, FALSE );
   gtk_widget_set_sensitive( ctx->button.corrigir_prova, FALSE );
   gtk_widget_set_sensitive( ctx->button.importar_dados, FALSE );
   gtk_widget_set_sensitive( aba_latex, FALSE );

   if ( pthread_create( &thread_id, &attr, funcao_background, args ) != 0 ) {

      gtk_widget_set_sensitive( widget, TRUE );
      gtk_widget_set_sensitive( ctx->button.corrigir_prova, TRUE );
      gtk_widget_set_sensitive( ctx->button.importar_dados, TRUE );
      gtk_widget_set_sensitive( aba_latex, TRUE );

      g_printerr( "ERRO: Falha ao criar a thread de geração de prova.\n" );
      if ( args->fichas ) g_array_unref( args->fichas );
      free( args );
   }

   pthread_attr_destroy( &attr );
}

// ============================================================================
// CALLBACK: Oculta o painel Toast e libera a Heap (Executa na Main Thread)
// ============================================================================
static gboolean ocultar_painel_e_desalocar_args_cb( gpointer user_data ) {
   ProvaThreadArgs *args = ( ProvaThreadArgs * )user_data;
   if ( !args ) return G_SOURCE_REMOVE;

   InterfacePainel *painel = args->painel; // Usa o ponteiro direto!

   if ( painel ) {
      // 1. Hover check em tempo real no ponteiro vivo
      if ( painel->mouse_hover ) {
         return G_SOURCE_CONTINUE; // Adia a ocultação!
      }

      // 2. Oculta o Revealer suavemente
      if ( painel->revealer_painel ) {
         gtk_revealer_set_reveal_child( GTK_REVEALER( painel->revealer_painel ), FALSE );
      }
      painel->timeout_id = 0;
   }

   // 3. DESALOCAÇÃO SEGURA NA HEAP (Somente quando a UI terminar)
   if ( args->fichas != NULL ) {
      g_array_unref( args->fichas );
   }
   free( args );

   g_print( "[GLib Timer] Toast ocultado. Memory cleanup de ProvaThreadArgs concluído.\n" );
   return G_SOURCE_REMOVE;
}

// ============================================================================
// CALLBACK: Retorno da Thread (Executa na Main Thread via g_idle_add)
// ============================================================================
static gboolean reativar_botao_gerar_prova( gpointer user_data ) {
   ProvaThreadArgs *args = ( ProvaThreadArgs * )user_data;
   if ( !args ) return FALSE;

   if ( args->botao_gerar )    gtk_widget_set_sensitive( args->botao_gerar,    TRUE );
   if ( args->botao_corrigir ) gtk_widget_set_sensitive( args->botao_corrigir, TRUE );
   if ( args->botao_importar ) gtk_widget_set_sensitive( args->botao_importar, TRUE );
   if ( args->aba_latex )      gtk_widget_set_sensitive( args->aba_latex,      TRUE );

   char instrucao[256] = {0};
   InterfacePainel *painel = args->painel;

   if ( args->sucesso ) {
      snprintf( instrucao, sizeof( instrucao ), "Pronto para impressão: %s Prova.pdf (em tela)",
                args->dados.prova_sequencia );

      painel->format_titulo    = meu_gerador_variadico( "✔ Prova Gerada com Sucesso!" );
      painel->format_subtitulo = meu_gerador_variadico( "O arquivo PDF foi compilado e estruturado corretamente via TeX." );
      painel->format_instrucao = meu_gerador_variadico( "%s", instrucao );

      criar_mensagem_painel( SUCESSO, painel );
   } else {
      snprintf( instrucao, sizeof( instrucao ), "Verifique o arquivo lista.dat do \"%s\" ou os logs do compilador.",
                args->dados.periodo );

      painel->format_titulo    = meu_gerador_variadico( "✘ Erro no Motor TeX" );
      painel->format_subtitulo = meu_gerador_variadico( "Falha crítica na estruturação ou compilação assíncrona dos gabaritos." );
      painel->format_instrucao = meu_gerador_variadico( "%s", instrucao );

      criar_mensagem_painel( ERRO, painel );
   }

   // REPROGRAMA O TIMER COM O CONTEXTO ISOLADO
   if ( painel->timeout_id > 0 ) {
      g_source_remove( painel->timeout_id );
   }

   guint tempo_exibicao = ( args->sucesso ) ? 4000 : 6000;
   painel->timeout_id = g_timeout_add( tempo_exibicao, ocultar_painel_e_desalocar_args_cb, args );

   return FALSE; // Remove da fila Idle
}





void* thread_gerar_prova_background( void *data ) {

   g_autoptr( GTimer ) cronometro = g_timer_new();

   if ( !data ) return NULL;
   ProvaThreadArgs *args = ( ProvaThreadArgs * )data;

   args->sucesso = true;

   g_print( "[Thread] Iniciando compilação com contexto clonado...\n" );

   g_autofree
   char *destino = g_build_filename( ".", "dados", "gabaritos", args->dados.ano, args->dados.escola, "gabaritos", NULL );

   char nome[32] = {0};
   nome_base_gabaritos_bin( nome, sizeof( nome ), args->foco.turma, args->foco.disciplina,
                            args->foco.periodo, args->dados.iprova );

   g_autofree char *arquivo = g_build_filename( destino, nome, NULL );

   FILE *p = fopen( arquivo, "rb" );
   if ( !p ) {
      g_printerr( "Erro ao abrir o arquivo para leitura: %s\n", arquivo );
      return NULL;
   }

   ItemTextoCurto *G = g_new0( ItemTextoCurto, args->dados.qtd_alunos_ativos );

   size_t lidos = fread( G, sizeof( ItemTextoCurto ), args->dados.qtd_alunos_ativos, p );

   if ( lidos != ( size_t )args->dados.qtd_alunos_ativos ) {
      g_printerr( "Aviso de I/O: Esperava %d alunos, mas só consegui ler %zu.\n",
                  args->dados.qtd_alunos_ativos, lidos );
   }

   fclose( p );


   if ( args->sucesso ) {

      prova( &args->dados, &args->foco, args->fichas, &args->caminho, &args->data, G );

      salvar_estado_aplicativo( &args->dados, &args->foco, &args->caminho );
   }

   g_free( G );

   // Agenda a execução gráfica passando o pacote com o veredito
   g_idle_add( reativar_botao_gerar_prova, args );



   display_tempo( "Geração de prova", cronometro );

   return NULL;
}












// ============================================================================
// ESTRUTURA REFINADA (Deep Copy + Ponteiro de UI)
// ============================================================================
typedef struct {
   InterfacePainel  *painel;          // ◄ PONTEIRO (Lê a UI viva, garantindo que o mouse_hover funcione)
   InterfaceDados   dados;            // ◄ Por valor (Cópia estática blindada)
   LimitesFiltro    limite;           // ◄ Por valor (Cópia estática blindada)
   GtkWidget        *botao_processar; // Para reativar o clique ao final
   GtkWidget        *botao_corrigir; // Para reativar o clique ao final
   int              n_rejeitadas;     // Número de imagens rejeitadas e movidas para a quarentena
} ProcessarThreadArgs;


// ============================================================================
// DISPARADOR ASSÍNCRONO
// ============================================================================
void disparar_processamento_imagens_assincrono( GtkWidget *widget, AppContext *ctx, void *( *funcao_background )( void * ) ) {
   ProcessarThreadArgs *args = malloc( sizeof( ProcessarThreadArgs ) );
   if ( !args ) return;

   // Deep Copy: Isolando os dados da Thread Principal GTK
   args->dados  = ctx->dados;
   args->limite = ctx->cascata.limite;

   // Referências Vivas
   args->painel          = &ctx->painel; // Passagem por referência para acesso em tempo real
   args->botao_processar = widget;
   args->botao_corrigir = ctx->button.corrigir_prova;
   args->n_rejeitadas    = -1;

   gtk_widget_set_sensitive( widget, FALSE );
   gtk_widget_set_sensitive( ctx->button.corrigir_prova, FALSE );

   pthread_t thread_id;
   pthread_attr_t attr;
   pthread_attr_init( &attr );
   pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

   if ( pthread_create( &thread_id, &attr, funcao_background, args ) != 0 ) {
      g_printerr( "ERRO: Falha ao criar a thread de processamento de imagens.\n" );
      gtk_widget_set_sensitive( widget, TRUE );
      gtk_widget_set_sensitive( ctx->button.corrigir_prova, TRUE );
      gtk_widget_set_sensitive( ctx->button.importar_dados, TRUE );
      free( args );
   }

   pthread_attr_destroy( &attr );
}


// ============================================================================
// CALLBACK: Oculta o painel Toast e libera a Heap (Executa na Main Thread)
// ============================================================================
static gboolean ocultar_painel_cv_cb( gpointer user_data ) {
   ProcessarThreadArgs *args = ( ProcessarThreadArgs * )user_data;
   if ( !args ) return G_SOURCE_REMOVE;

   InterfacePainel *painel = args->painel;

   if ( painel ) {
      // 1. Se o professor estiver com o mouse sobre o Toast, NÃO recolhe nem desaloca ainda!
      if ( painel->mouse_hover ) {
         return G_SOURCE_CONTINUE;
      }

      // 2. Oculta o Revealer de forma suave
      if ( painel->revealer_painel ) {
         gtk_revealer_set_reveal_child( GTK_REVEALER( painel->revealer_painel ), FALSE );
      }
      painel->timeout_id = 0;
   }

   // 3. DESALOCAÇÃO SEGURA NA HEAP
   free( args );
   g_print( "[GLib Timer] Toast CV ocultado. Memory cleanup de ProcessarThreadArgs concluído.\n\n" );

   return G_SOURCE_REMOVE;
}


// ============================================================================
// CALLBACK: Retorno da Thread (Executa na Main Thread via g_idle_add)
// ============================================================================
static gboolean reativar_botao_processar_imagens( gpointer user_data ) {
   ProcessarThreadArgs *args = ( ProcessarThreadArgs * )user_data;
   if ( !args ) return FALSE;

   if ( args->botao_processar ) {
      gtk_widget_set_sensitive( args->botao_processar, TRUE );
      gtk_widget_set_sensitive( args->botao_corrigir, TRUE );
   }

   InterfacePainel *painel = args->painel;

   // Textos conforme o status
   if ( args->n_rejeitadas == 0 ) {
      painel->format_titulo    = meu_gerador_variadico( "✔ Processamento Concluído!" );
      painel->format_subtitulo = meu_gerador_variadico( "Todas as provas foram lidas, alinhadas e validadas sem falhas." );
      painel->format_instrucao = meu_gerador_variadico( "O lote está pronto. Você já pode prosseguir para a etapa de Correção." );
      criar_mensagem_painel( SUCESSO, painel );

   } else if ( args->n_rejeitadas > 0 ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠️ Processamento com Alertas" );
      painel->format_subtitulo = meu_gerador_variadico( "Não foi possível ler os marcadores ou identificar %d imagem(ns).", args->n_rejeitadas );
      painel->format_instrucao = meu_gerador_variadico( "Estes arquivos foram isolados na pasta 'rejeitadas' para sua verificação manual." );
      criar_mensagem_painel( AVISO, painel );

   } else if ( args->n_rejeitadas == -1 ) {
      painel->format_titulo    = meu_gerador_variadico( "✘ Dados Não Encontrados" );
      painel->format_subtitulo = meu_gerador_variadico( "O sistema não localizou gabaritos estruturais de nenhuma avaliação." );
      painel->format_instrucao = meu_gerador_variadico( "Certifique-se de gerar os cadernos de prova antes de tentar processar as imagens." );
      criar_mensagem_painel( ERRO, painel );

   } else if ( args->n_rejeitadas <= -2 ) {
      painel->format_titulo    = meu_gerador_variadico( "✘ Nenhuma Imagem Localizada" );
      painel->format_subtitulo = meu_gerador_variadico( "O sistema não encontrou fotografias ou digitalizações de respostas na pasta de entrada." );
      painel->format_instrucao = meu_gerador_variadico( "Transfira os ficheiros de imagem das provas para a pasta 'Downloads/imagens' antes de iniciar o processamento." );
      criar_mensagem_painel( ERRO, painel );
   }

   // REPROGRAMA O TIMER DO PAINEL PASSANDO 'args' COMO CONTEXTO
   if ( painel->timeout_id > 0 ) {
      g_source_remove( painel->timeout_id );
   }

   guint tempo_exibicao = ( args->n_rejeitadas == 0 ) ? 4000 : 6000;
   painel->timeout_id = g_timeout_add( tempo_exibicao, ocultar_painel_cv_cb, args );

   g_print( "[Thread CV] Mensagem emitida. Aguardando o término do Toast para desalocar Heap...\n" );

   return FALSE;
}


// ============================================================================
// FUNÇÃO DA THREAD SECUNDÁRIA
// ============================================================================
void* thread_processar_imagens_background( void *data ) {
   g_autoptr( GTimer ) cronometro = g_timer_new();

   if ( !data ) return NULL;
   ProcessarThreadArgs *args = ( ProcessarThreadArgs * )data;

   g_print( "[Thread CV] Iniciando o processamento assíncrono das imagens...\n" );

   args->n_rejeitadas = omr_processar_imagens( &args->dados, &args->limite );

   g_print( "[Thread CV] Processamento concluído. Retornando o controle para a UI.\n" );

   // Agenda a execução do retorno gráfico na Thread Principal do GTK
   g_idle_add( reativar_botao_processar_imagens, args );

   display_tempo( "Processamento", cronometro );

   return NULL;
}










// Estrutura para empacotar o contexto e as cópias seguras
typedef struct {
   char *tema;                  // ctx->dados.tema
   int qtd_subtemas;            // ctx->cascata.limite.subtemas
   GtkWidget *botao_compilar;   // ctx->button.compilar_latex_acervo
   GtkWidget *botao_abrir;      // ctx->button.abrir_pdf_acervo
   GtkWidget *widget;
   ItemCombo *subtemas;         // ctx->listas.subtemas (typedef struct {char str[64];} ItemCombo;)
   InterfacePainel *painel;     // ctx->painel
   char *dir_compile;           // dir_compile
   char *caminho_banco_questoes;// ctx->caminho.banco_questoes
} DadosCompilacaoAsync;

static bool verificar_pdfs_latex_acervo_questoes( InterfacePainel *painel, const char *tema, int qtd_subtemas, int falhas ) {

   if ( falhas == 0 ) {
      // --- ESTADO 1: SUCESSO TOTAL ---
      painel->format_titulo    = meu_gerador_variadico( "✔ Sucesso Total!" );
      painel->format_subtitulo = meu_gerador_variadico( "Todos os %d PDFs foram gerados corretamente.", qtd_subtemas );
      painel->format_instrucao = meu_gerador_variadico( "Iniciando concatenação dos PDFs para %s.pdf...", tema );
      criar_mensagem_painel( SUCESSO, painel );
      return true;

   } else if ( falhas < qtd_subtemas ) {
      // --- ESTADO 2: ALERTA (SUCESSO PARCIAL) ---
      int sucessos = qtd_subtemas - falhas;
      painel->format_titulo    = meu_gerador_variadico( "⚠ Atenção (Sucesso Parcial):" );
      painel->format_subtitulo = meu_gerador_variadico( "Processados %d de %d arquivos com sucesso.", sucessos, qtd_subtemas );
      painel->format_instrucao = meu_gerador_variadico( "Houve falha em %d item(ns). Unindo os PDFs disponíveis...", falhas );
      criar_mensagem_painel( AVISO, painel );
      return true; // Sua sacada implementada!

   } else {
      // --- ESTADO 3: ERRO CRÍTICO (NADA FOI GERADO) ---
      painel->format_titulo    = meu_gerador_variadico( "✘ Erro de Processamento:" );
      painel->format_subtitulo = meu_gerador_variadico( "Nenhum arquivo PDF foi gerado pelo LaTeX." );
      painel->format_instrucao = meu_gerador_variadico( "Verifique a pasta %s.", tema );
      criar_mensagem_painel( ERRO, painel );
      return false;
   }
}

static void ao_terminar_compilacao_banco( GPid pid, gint status, gpointer user_data ) {
   DadosCompilacaoAsync *async_data = ( DadosCompilacaoAsync * ) user_data;

   if ( status == 0 ) {
      g_print( "[SUCESSO] O tema '%s' foi compilado perfeitamente!\n", async_data->tema );

      // ======================================================================
      // FASE DE PÓS-PROCESSAMENTO
      // ======================================================================

      // 1. Prepara o array dinâmico com os nomes exatos dos PDFs gerados
      g_auto( GStrv ) arquivos_pdf = g_new0( char *, async_data->qtd_subtemas + 1 );
      int qtd_sucessos_reais = 0; // Contador paralelo

      for ( int i = 0; i < async_data->qtd_subtemas; i++ ) {
         g_autofree char *nome_arquivo = g_strdup_printf( "%s.pdf", async_data->subtemas[i].str );
         g_autofree char *path_teste = g_build_filename( async_data->dir_compile, nome_arquivo, NULL );

         // Só adiciona na lista do pdfunite se o arquivo existir!
         if ( g_file_test( path_teste, G_FILE_TEST_EXISTS ) ) {
            arquivos_pdf[qtd_sucessos_reais] = g_strdup( nome_arquivo );
            qtd_sucessos_reais++;
         }
      }

      int falhas = async_data->qtd_subtemas - qtd_sucessos_reais;

      if ( verificar_pdfs_latex_acervo_questoes( async_data->painel, async_data->tema, async_data->qtd_subtemas, falhas ) ) {

         // 2. Prepara o caminho absoluto e definitivo do arquivo final unificado
         g_autofree char *nome_arquivo = g_strdup_printf( "%s.pdf", async_data->tema );
         g_autofree char *arquivo_saida = g_build_filename( async_data->caminho_banco_questoes, nome_arquivo, NULL );

         // 3. Chama a sua função nativa passando o arquivo_saida blindado
         g_remove( arquivo_saida );
         g_pdfunite( async_data->dir_compile, ( const char ** )arquivos_pdf, qtd_sucessos_reais, arquivo_saida );

         // 4. Limpeza dos arquivos temporários
         for ( int i = 0; i < async_data->qtd_subtemas; i++ ) {
            apagar_arquivos_temporarios_latex_nativamente( async_data->dir_compile, async_data->subtemas[i].str, 5 );
         }

         if ( async_data->widget == async_data->botao_abrir ) {
            g_xdg_open( arquivo_saida );
         }
      }

   } else {
      g_printerr( "[AVISO] Compilação do tema '%s' interrompida ou com erros (status %d).\n", async_data->tema, status );
   }

   // Desbloqueia os botões diretamente pelo ponteiro salvo na struct
   gtk_widget_set_sensitive( async_data->botao_abrir, TRUE );
   gtk_widget_set_sensitive( async_data->botao_compilar, TRUE );

   // Libera as cópias de memória enxutas
   g_free( async_data->tema );
   g_free( async_data->dir_compile );
   g_free( async_data->caminho_banco_questoes );
   g_free( async_data->subtemas ); // Libera a cópia do array
   g_free( async_data );

   g_spawn_close_pid( pid );
}


void g_pdflatex_parallel_async( GtkWidget *widget, const char *dir_compile, InterfacePainel *painel, const AppContext *ctx ) {
   g_return_if_fail( dir_compile != NULL );
   g_return_if_fail( ctx != NULL );

   gtk_widget_set_sensitive( ctx->button.abrir_pdf_acervo, FALSE );
   gtk_widget_set_sensitive( ctx->button.compilar_latex_acervo, FALSE );

   DadosCompilacaoAsync *async_data = g_new( DadosCompilacaoAsync, 1 );

   // Mapeamento direto das variáveis essenciais
   async_data->tema                   = g_strdup( ctx->dados.tema );
   async_data->qtd_subtemas           = ctx->cascata.limite.subtemas;
   async_data->botao_compilar         = ctx->button.compilar_latex_acervo;
   async_data->botao_abrir            = ctx->button.abrir_pdf_acervo;
   async_data->widget                 = widget;
   async_data->painel                 = painel;
   async_data->dir_compile            = g_strdup( dir_compile );
   async_data->caminho_banco_questoes = g_strdup( ctx->caminho.banco_questoes );

   // Cópia absoluta do array (Blindagem contra alterações na interface durante a compilação)
   async_data->subtemas = g_new( ItemCombo, async_data->qtd_subtemas );
   memcpy( async_data->subtemas, ctx->listas.subtemas, sizeof( ItemCombo ) * async_data->qtd_subtemas );

   int num_cores = ( int ) g_get_num_processors();
   GError *erro = NULL;
   GPid pid;

   g_autofree char *comando_interno = g_strdup_printf(
                                         "parallel -j %d nice -n 5 pdflatex -synctex=1 -interaction=nonstopmode ::: *.tex || true", num_cores );
   char *argv[] = { ( char * )"sh", ( char * )"-c", comando_interno, NULL };

   if ( !g_spawn_async( dir_compile, argv, NULL,
                        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                        NULL, NULL, &pid, &erro ) ) {

      g_printerr( "[ERRO FATAL] Falha ao compilar assíncrono: %s\n", erro->message );
      g_clear_error( &erro );

      gtk_widget_set_sensitive( async_data->botao_abrir, TRUE );
      gtk_widget_set_sensitive( async_data->botao_compilar, TRUE );

      g_free( async_data->tema );
      g_free( async_data->dir_compile );
      g_free( async_data->caminho_banco_questoes );
      g_free( async_data->subtemas );
      g_free( async_data );

   } else {
      g_child_watch_add( pid, ao_terminar_compilacao_banco, async_data );
   }
}







//=========================================================================================================
// Estrutura para transportar o contexto da correção para a callback
typedef struct {
   GArray *map_array;        // O array com os dados dos alunos
   AppContext *ctx;           // O contexto global do app
   InterfacePainel *painel;   // O painel para dar o feedback
   char *dir_compile;         // Diretório temporário
   GTimer *cronometro;
} DadosCorrecaoAsync;
//------------------------------------------------------------------------------------------------------
static void copiar_arquivos_correcao_externamente( const InterfaceDados *dados, const CaminhoDiretorio *caminho,
      const char *arquivo_saida ) {
   g_autofree char *nome_arquivo_escola = NULL;
   if ( dados->periodo[0] == 'R' ) {
      nome_arquivo_escola = g_strdup_printf( "Correção Recuperação Final - %s - %s - %s.pdf",
                                             dados->ano, dados->turma, dados->disciplina );
   } else {
      nome_arquivo_escola = g_strdup_printf( "Correção %s Prova - %s_%c - %s - %s.pdf",
                                             dados->prova_sequencia, dados->ano, dados->periodo[0],
                                             dados->turma, dados->disciplina );
   }

   g_autofree char *pasta_provas_escola = g_build_filename( caminho->externo_escola, "Correções", NULL );
   g_autofree char *destino_escola      = g_build_filename( pasta_provas_escola, nome_arquivo_escola, NULL );

   // Garante que a pasta "Provas" exista lá no drive/nuvem da escola
   g_mkdir_with_parents( pasta_provas_escola, 0777 );

   if ( !gio_copiar_arquivo( arquivo_saida, destino_escola ) ) {
      g_printerr( "Erro ao salvar a cópia institucional na pasta Provas da Escola!\n" );
   }
}
//------------------------------------------------------------------------------------------------------
static void copiar_arquivos_correcao_nao_presencial( const MapeamentoGabarito *map, const int qtd_linhas,
      const AppContext *ctx ) {

   const InterfaceDados *dados = &ctx->dados;
   const CaminhoDiretorio *caminho = &ctx->caminho;

   g_autofree char *diretorio_imagens = NULL;

   // 1. Construção simplificada: Não precisamos criar o diretório "Notas" separadamente.
   // O g_mkdir_with_parents já cria toda a árvore genealógica de pastas se não existirem.
   if ( dados->periodo[0] == 'R' ) {
      diretorio_imagens = g_build_filename( caminho->externo, "Notas", "Recuperação Final Imagens Corrigidas", NULL );

   } else {
      g_autofree char *pasta_imagens = g_strdup_printf( "%s Prova Imagens Corrigidas", dados->prova_sequencia );
      diretorio_imagens = g_build_filename( caminho->externo, "Notas", pasta_imagens, NULL );
   }

   // 2. Apenas uma chamada de criação de pasta resolve tudo
   if ( g_mkdir_with_parents( diretorio_imagens, 0777 ) != 0 ) {
      g_printerr( "ERRO CRÍTICO: Falha ao criar a hierarquia externa de pastas: %s\n", diretorio_imagens );
      return;
   }

   // 3. Libere o poder do OpenMP! A conversão de PDF para PNG consome muita CPU.
   // Fazer isso em paralelo para 40 alunos economiza dezenas de segundos.
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < qtd_linhas; i++ ) {

      if ( map[i].status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
         int num_aluno = map[i].num;
         FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, num_aluno - 1 );

         g_autofree char *thread_caminho_pdf = g_strdup_printf( "./dados/temporarios/%.2d.pdf", num_aluno );
         g_autofree char *nome_arquivo_png   = g_strdup_printf( "%.2d - %s.png", num_aluno, ficha->aluno );
         g_autofree char *thread_caminho_png = g_build_filename( diretorio_imagens, nome_arquivo_png, NULL );

         if ( !pdf2png( thread_caminho_pdf, thread_caminho_png, 1.5 ) ) {
            // Em laços OpenMP, evite GTK, mas g_printerr é seguro.
            g_printerr( "[AVISO] Falha ao converter e mover imagem %s\n", thread_caminho_png );
         }
      }
   }

}
//------------------------------------------------------------------------------------------------------
// Callback disparada quando o GNU Parallel + pdflatex terminam a correção
static void ao_terminar_correcao_prova( GPid pid, gint status, gpointer user_data ) {
   DadosCorrecaoAsync *async = ( DadosCorrecaoAsync * ) user_data;

   AppContext *ctx = async->ctx;
   InterfaceDados *dados = &ctx->dados;
   CaminhoDiretorio *caminho = &ctx->caminho;
   InterfacePainel *painel = async->painel;
   GArray *map_array = async->map_array;

   if ( status == 0 ) {
      g_print( "[SUCESSO] Processamento assíncrono do LaTeX concluído.\n" );

      // 5.1. Arquivos Não Presenciais
      if ( dados->naopresencial ) {
         copiar_arquivos_correcao_nao_presencial( ( MapeamentoGabarito * )map_array->data, map_array->len, ctx );
      }

      // ====================================================================================
      // 6. UNIFICAÇÃO DOS PDFS E LIMPEZA NATIVA
      // ====================================================================================
      int total_provas = map_array->len;
      g_auto( GStrv ) arquivos_pdf = g_new0( char *, total_provas + 1 );
      int qtd_sucessos = 0;

      for ( int i = 0; i < total_provas; i++ ) {
         MapeamentoGabarito *map = &g_array_index( map_array, MapeamentoGabarito, i );
         if ( map->status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
            arquivos_pdf[qtd_sucessos] = g_strdup_printf( "%.2d.pdf", map->num );
            qtd_sucessos++;
         }
      }

      g_autofree char *nome_arquivo  = g_strdup_printf( "Correção_%d.pdf", dados->iprova );
      g_autofree char *arquivo_saida = g_build_filename( caminho->relatorios, nome_arquivo, NULL );

      g_remove( arquivo_saida );

      // Unifica os PDFs
      g_pdfunite( async->dir_compile, ( const char ** )arquivos_pdf, qtd_sucessos, arquivo_saida );

      // Limpeza com OpenMP
      #pragma omp parallel for schedule(static)
      for ( int i = 0; i < total_provas; i++ ) {
         MapeamentoGabarito *map = &g_array_index( map_array, MapeamentoGabarito, i );
         if ( map->status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
            g_autofree gchar *nome_base = g_strdup_printf( "%.2d", map->num );
            apagar_arquivos_temporarios_latex_nativamente( async->dir_compile, nome_base, 5 );
         }
      }

      // ====================================================================================
      // 7. EXIBIÇÃO AUTOMÁTICA E FEEDBACK NA INTERFACE
      // ====================================================================================
      if ( g_file_test( arquivo_saida, G_FILE_TEST_EXISTS ) ) {
         if ( dados->expor ) {
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
         painel->format_instrucao = meu_gerador_variadico( "Verifique se o LaTeX apresentou erros ou se o pdfunite falhou." );
         criar_mensagem_painel( ERRO, painel );
      }

   } else {
      g_printerr( "[ERRO FATAL] O comando LaTeX assíncrono falhou (Status: %d).\n", status );
      painel->format_titulo    = meu_gerador_variadico( "✘ Falha na Compilação" );
      painel->format_subtitulo = meu_gerador_variadico( "O motor LaTeX abortou o processamento." );
      painel->format_instrucao = meu_gerador_variadico( "Verifique os logs do terminal." );
      criar_mensagem_painel( ERRO, painel );
   }

   // -------------------------------------------------------------------------
   // DESBLOQUEIO DE INTERFACE:
   // Se você bloqueia o botão de "Corrigir" lá na função principal,
   // desbloqueie-o aqui. Exemplo:
   gtk_widget_set_sensitive( ctx->button.corrigir_prova, TRUE );
   gtk_widget_set_sensitive( ctx->button.processar_imagens, TRUE );
   // -------------------------------------------------------------------------

   if ( async->cronometro ) {
      display_tempo( "PDF de Correção", async->cronometro );
      g_timer_destroy( async->cronometro ); // <--- LIBERA O GTIMER AQUI
   }

   // LIBERAÇÃO SEGURA DE MEMÓRIA (O map_array morre AQUI, e não na corrigir_prova)
   g_array_free( async->map_array, TRUE );
   g_free( async->dir_compile );
   g_free( async );

   g_spawn_close_pid( pid );
}
//------------------------------------------------------------------------------------------------------
void g_pdflatex_parallel_async_corrigir_prova( InterfacePainel *painel, GArray *map_array, AppContext *ctx ) {
   // g_autoptr( GTimer ) cronometro = g_timer_new();

   // Empacota os dados para enviar à callback
   GError *erro = NULL;

   DadosCorrecaoAsync *async_data = g_new0( DadosCorrecaoAsync, 1 );
   async_data->map_array  = map_array; // Transferência da posse da memória
   async_data->ctx         = ctx;
   async_data->painel      = painel;
   async_data->dir_compile = g_strdup( "./dados/temporarios" );
   async_data->cronometro = g_timer_new();

   int num_cores = ( int ) g_get_num_processors();
   GPid pid;

   g_autofree char *comando_interno = g_strdup_printf(
            "parallel -j %d nice -n 5 pdflatex -synctex=1 -interaction=nonstopmode ::: *.tex || true", num_cores );
   char *argv[] = { ( char * )"sh", ( char * )"-c", comando_interno, NULL };

   if ( !g_spawn_async( async_data->dir_compile, argv, NULL,
                        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                        NULL, NULL, &pid, &erro ) ) {

      g_printerr( "[ERRO FATAL] Falha ao iniciar correção assíncrona: %s\n", erro->message );
      g_clear_error( &erro );

      // Como falhou ao iniciar a thread, liberamos a memória do array e da struct agora
      g_array_free( async_data->map_array, TRUE );
      g_free( async_data->dir_compile );
      g_free( async_data );

      gtk_widget_set_sensitive( ctx->button.corrigir_prova, TRUE );

      painel->format_titulo    = meu_gerador_variadico( "✘ Erro de Processamento" );
      painel->format_subtitulo = meu_gerador_variadico( "Não foi possível iniciar o GNU Parallel." );
      painel->format_instrucao = meu_gerador_variadico( "Tente novamente." );
      criar_mensagem_painel( ERRO, painel );

   } else {
      // O GNU Parallel iniciou! A callback `ao_terminar_correcao_prova` assume o controle.
      g_child_watch_add( pid, ao_terminar_correcao_prova, async_data );
   }
}




