/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>

#include "assincrono.h"
#include "glib/gstdio.h"
#include "mensagens.h"
#include "provas.h"
#include "basicas.h"
#include "imagens.h"
#include "glibrary.h"




typedef struct {
   InterfacePainel  painel;
   InterfaceDados   dados;    // ◄ Por valor (Cópia estática blindada)
   InterfaceListas  listas;
   FocoCoordenadas  foco;     // ◄ Por valor
   CaminhoDiretorio caminho;  // ◄ Por valor
   DataHoje         data;     // ◄ Por valor
   FichaAluno       *diario;   // Ponteiro (Seguro, pois o vetor global do ctx está vivo)
   GtkWidget        *botao_gerar;
   bool             sucesso;
} ProvaThreadArgs;

/**
 * Clona profundamente o diário de alunos na Heap para isolamento de threads (Deep Copy).
 * Retorna o ponteiro do novo vetor alocado ou NULL se houver falha ou se n_alunos == 0.
 */
static FichaAluno* clonar_diario_alunos( const FichaAluno *diario_original, int n_alunos ) {
   // 1. Validação geométrica elementar
   if ( !diario_original || n_alunos <= 0 ) {
      return NULL;
   }

   // 2. Cálculo exato do bloco de memória necessário para a turma toda
   size_t tamanho_total = ( size_t )n_alunos * sizeof( FichaAluno );

   // 3. Alocação isolada na Heap
   FichaAluno *diario_clonado = malloc( tamanho_total );

   if ( !diario_clonado ) {
      g_printerr( "ERRO CRÍTICO: Falha de memória (malloc) ao clonar diário de alunos.\n" );
      return NULL;
   }

   // 4. Cópia física bruta dos dados (Blindagem estática bit a bit)
   memcpy( diario_clonado, diario_original, tamanho_total );

   return diario_clonado;
}

// 🚀 A NOVA FUNÇÃO DE ENTRADA DO MOTOR:
void disparar_geracao_prova_assincrona( GtkWidget *widget, AppContext *ctx, void *( *funcao_background )( void * ) ) {
   // Aloca os argumentos, tira o snapshot por valor...
   ProvaThreadArgs *args = malloc( sizeof( ProvaThreadArgs ) );
   if ( !args ) return;

   args->dados   = ctx->dados;
   args->foco    = ctx->cascata.foco;
   args->caminho = ctx->caminho;
   args->data    = ctx->data;
   args->painel  = ctx->painel;
   args->listas  = ctx->listas;
   args->diario = clonar_diario_alunos( ctx->diario, ctx->dados.qtd_alunos_total );
   args->botao_gerar = widget;

   pthread_t thread_id;
   pthread_attr_t attr;
   pthread_attr_init( &attr );
   pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

   if ( pthread_create( &thread_id, &attr, funcao_background, args ) != 0 ) {
      g_printerr( "ERRO: Falha ao criar a thread de geração de prova.\n" );
      gtk_widget_set_sensitive( widget, TRUE );
      free( args );
   }
   pthread_attr_destroy( &attr );
}





/* =================================================================================================================
   CALLBACK DE RETORNO DO TRABALHADOR DE PROVAS (G-Source do Main Loop)
   ================================================================================================================= */
static gboolean reativar_botao_gerar_prova( gpointer user_data ) {
   ProvaThreadArgs *args = ( ProvaThreadArgs * )user_data;
   if ( !args ) return FALSE;

   // 1. Reativação física do botão de controle da interface
   if ( args->botao_gerar ) {
      gtk_widget_set_sensitive( args->botao_gerar, TRUE );
   }

   char instrucao[256] = {0};

   // 2. Fluxo Baseado no Retorno da Compilação Assíncrona do TeX
   if ( args->sucesso ) {
      snprintf( instrucao, sizeof( instrucao ), "Pronto para impressão: %s Prova.pdf (em tela)",
                args->dados.prova_sequencia );

      // Alimenta os buffers do painel para o cenário de SUCESSO
      args->painel.format_titulo    = meu_gerador_variadico( "✔ Prova Gerada com Sucesso!" );
      args->painel.format_subtitulo = meu_gerador_variadico(
                                         "O arquivo PDF foi compilado e estruturado corretamente via TeX." );
      args->painel.format_instrucao = meu_gerador_variadico( "%s", instrucao );

      // Dispara o motor central para pintar as etiquetas e gerenciar o Heap
      criar_mensagem_painel( SUCESSO, &args->painel );
   } else {
      snprintf( instrucao, sizeof( instrucao ), "Verifique o arquivo lista.dat do \"%s\" ou os logs do compilador.",
                args->dados.periodo );

      // Alimenta os buffers do painel para o cenário de ERRO crítico
      args->painel.format_titulo    = meu_gerador_variadico( "✘ Erro no Motor TeX" );
      args->painel.format_subtitulo = meu_gerador_variadico(
                                         "Falha crítica na estruturação ou compilação assíncrona dos gabaritos." );
      args->painel.format_instrucao = meu_gerador_variadico( "%s", instrucao );

      // Dispara o motor central injetando o estilo do tema (Light, Deep Blue ou Dark Green)
      criar_mensagem_painel( ERRO, &args->painel );
   }


   // =========================================================================
   // 🛡️ HIGIENE DA HEAP DA THREAD (Obrigatório antes do return/pthread_exit)
   // =========================================================================
   g_print( "[Thread] Trabalho concluído. Iniciando desalocação do snapshot...\n" );

   // A. Libera o diário clonado que foi gerado especificamente para esta thread
   if ( args->diario != NULL ) {
      free( args->diario );
      args->diario = NULL;
      g_print( "   ✔ Clone do Diário de Alunos desalocado da Heap.\n" );
   }

   // C. Libera a própria estrutura de argumentos da thread
   g_print( "   ✔ Estrutura ProvaThreadArgs desalocada.\n" );

   // 3. Desalocação segura da estrutura de argumentos que veio da Thread secundária
   free( args );

   g_print( "[Thread] Finalizada com 100%% de segurança na memória.\n\n" );

   // Retornamos FALSE para avisar ao g_idle_add ou g_timeout_add que este callback deve rodar apenas uma vez!
   return FALSE;
   // return G_SOURCE_REMOVE; // Diz ao GTK para executar essa função apenas UMA vez
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

      prova( &args->dados, &args->foco, args->diario, &args->caminho, &args->data, G );

      salvar_estado_aplicativo( &args->dados, &args->foco, &args->caminho );
   }

   g_free( G );

   // Agenda a execução gráfica passando o pacote com o veredito
   g_idle_add( reativar_botao_gerar_prova, args );



   display_tempo( "Geração de prova", cronometro );

   return NULL;
}












// Estrutura atualizada para garantir o Deep Copy e controle de UI
typedef struct {
   InterfacePainel  painel;           // Para atualizar a interface no retorno
   InterfaceDados   dados;            // ◄ Por valor (Cópia estática blindada)
   LimitesFiltro    limite;           // ◄ Por valor (Cópia estática blindada)
   GtkWidget        *botao_processar; // Para reativar o clique ao final
   int              n_rejeitadas;     // Número de imagens rejeitadas e movidas para a quarentena
} ProcessarThreadArgs;

void disparar_processamento_imagens_assincrono( GtkWidget *widget, AppContext *ctx, void *( *funcao_background )( void * ) ) {

   // 1. Alocação do pacote de argumentos para a Thread
   ProcessarThreadArgs *args = malloc( sizeof( ProcessarThreadArgs ) );
   if ( !args ) return;

   // 2. Deep Copy: Isolando os dados da Thread Principal GTK
   args->dados  = ctx->dados;
   args->limite = ctx->cascata.limite; // Ajuste para o local exato do seu LimitesFiltro no ctx
   args->painel = ctx->painel;
   args->botao_processar = widget;
   args->n_rejeitadas = -1;

   // 3. Desativa o botão para o usuário não clicar duas vezes enquanto processa
   gtk_widget_set_sensitive( widget, FALSE );

   // 4. Criação e disparo da Thread Detached
   pthread_t thread_id;
   pthread_attr_t attr;
   pthread_attr_init( &attr );
   pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED ); //

   if ( pthread_create( &thread_id, &attr, funcao_background, args ) != 0 ) {
      g_printerr( "ERRO: Falha ao criar a thread de processamento de imagens.\n" );
      gtk_widget_set_sensitive( widget, TRUE );
      free( args );
   }
   pthread_attr_destroy( &attr );

}




static gboolean reativar_botao_processar_imagens( gpointer user_data ) {
   ProcessarThreadArgs *args = ( ProcessarThreadArgs * )user_data;
   if ( !args ) return FALSE;

   // 1. Reativação física do botão de controle da interface
   if ( args->botao_processar ) {
      gtk_widget_set_sensitive( args->botao_processar, TRUE );
   }

   // 2. Fluxo Baseado no Retorno do Processamento das Imagens
   if ( args->n_rejeitadas == 0 ) {
      // Cenário Perfeito: Transmite segurança de que o trabalho pesado terminou bem.
      args->painel.format_titulo    = meu_gerador_variadico( "✔ Processamento Concluído!" );
      args->painel.format_subtitulo = meu_gerador_variadico( "Todas as provas foram lidas, alinhadas e validadas sem falhas." );
      args->painel.format_instrucao = meu_gerador_variadico( "O lote está pronto. Você já pode prosseguir para a etapa de Correção." );
      criar_mensagem_painel( SUCESSO, &args->painel );

   } else if ( args->n_rejeitadas > 0 ) {
      // E removi "Payload/Âncoras" por uma explicação mais visual ("marcadores/identificar").
      args->painel.format_titulo    = meu_gerador_variadico( "⚠️ Processamento com Alertas" );
      args->painel.format_subtitulo = meu_gerador_variadico( "Não foi possível ler os marcadores ou identificar %d imagem(ns).", args->n_rejeitadas );
      args->painel.format_instrucao = meu_gerador_variadico( "Estes arquivos foram isolados na pasta 'rejeitadas' para sua verificação manual." );
      criar_mensagem_painel( AVISO, &args->painel );

   } else if ( args->n_rejeitadas < 0 ) {
      // Cenário de Erro Crítico (Falta do .bin): Explica a causa raiz de forma simples.
      args->painel.format_titulo    = meu_gerador_variadico( "✘ Dados Não Encontrados" );
      args->painel.format_subtitulo = meu_gerador_variadico( "O sistema não localizou gabaritos estruturais de nenhuma avaliação." );
      args->painel.format_instrucao = meu_gerador_variadico( "Certifique-se de gerar os cadernos de prova antes de tentar processar as imagens." );
      criar_mensagem_painel( ERRO, &args->painel );
   }

   // 3. Desalocação segura da estrutura de argumentos que veio da Thread secundária[cite: 1]
   free( args );

   g_print( "[GTK] Memória da thread CV desalocada com sucesso.\n" );

   // Retorna FALSE para avisar ao g_idle_add que este callback deve rodar apenas uma vez[cite: 1]
   return FALSE;
}




void* thread_processar_imagens_background( void *data ) {

   g_autoptr( GTimer ) cronometro = g_timer_new();

   if ( !data ) return NULL;
   ProcessarThreadArgs *args = ( ProcessarThreadArgs * )data;

   g_print( "[Thread CV] Iniciando o processamento assíncrono das imagens...\n" );

   args->n_rejeitadas = processar_imagens( &args->dados, &args->limite );

   g_print( "[Thread CV] Processamento concluído. Retornando o controle para a UI.\n" );

   // Agenda a execução do retorno gráfico na Thread Principal do GTK[cite: 1]
   g_idle_add( reativar_botao_processar_imagens, args );



   display_tempo( "Processamento", cronometro );

   return NULL;
}










// Estrutura para empacotar o contexto e as cópias seguras
typedef struct {
   char *tema;                  // ctx->dados.tema
   int qtd_subtemas;            // ctx->cascata.limite.subtemas
   GtkWidget *botao_compilar;   // ctx->botao.compilar_latex_acervo
   GtkWidget *botao_abrir;      // ctx->botao.abrir_pdf_acervo
   GtkWidget *widget;
   ItemCombo *subtemas;         // ctx->listas.subtemas (typedef struct {char str[64];} ItemCombo;)
   InterfacePainel *painel;     // ctx->painel
   char *dir_compile;           // dir_compile
   char *caminho_banco_questoes;// ctx->caminho.banco_questoes
} DadosCompilacaoAsync;

static bool verificar_pdfs_latex_acervo_questoes(  InterfacePainel *painel, const char *tema, int qtd_subtemas, int falhas ) {

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
      g_auto(GStrv) arquivos_pdf = g_new0( char *, async_data->qtd_subtemas + 1 );
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

      if ( verificar_pdfs_latex_acervo_questoes(  async_data->painel, async_data->tema, async_data->qtd_subtemas, falhas ) ) {

         // 2. Prepara o caminho absoluto e definitivo do arquivo final unificado
         g_autofree char *nome_arquivo = g_strdup_printf( "%s.pdf", async_data->tema );
         g_autofree char *arquivo_saida = g_build_filename( async_data->caminho_banco_questoes, nome_arquivo, NULL );

         // 3. Chama a sua função nativa passando o arquivo_saida blindado
         g_remove( arquivo_saida );
         g_pdfunite( async_data->dir_compile, (const char **)arquivos_pdf, qtd_sucessos_reais, arquivo_saida );

         // 4. Limpeza dos arquivos temporários
         for ( int i = 0; i < async_data->qtd_subtemas; i++ ) {
            apagar_arquivos_temporarios_latex_nativamente( async_data->dir_compile, async_data->subtemas[i].str, 5 );
         }

         if( async_data->widget == async_data->botao_abrir ) {
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

   gtk_widget_set_sensitive( ctx->botao.abrir_pdf_acervo, FALSE );
   gtk_widget_set_sensitive( ctx->botao.compilar_latex_acervo, FALSE );

   DadosCompilacaoAsync *async_data = g_new( DadosCompilacaoAsync, 1 );

   // Mapeamento direto das variáveis essenciais
   async_data->tema                   = g_strdup( ctx->dados.tema );
   async_data->qtd_subtemas           = ctx->cascata.limite.subtemas;
   async_data->botao_compilar         = ctx->botao.compilar_latex_acervo;
   async_data->botao_abrir            = ctx->botao.abrir_pdf_acervo;
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









