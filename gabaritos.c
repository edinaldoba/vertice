/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include "gabaritos.h"
#include "mensagens.h"
#include "basicas.h"


gchar *nome_base_gabaritos_bin( int turma, int disciplina, int periodo, int prova ) {
   return g_strdup_printf( "%.3d%.2d%d%d.bin", turma, disciplina, periodo, prova );
}


// ====================================================================================
// FUNÇÃO AUXILIAR: Embaralhamento Fisher-Yates (Thread-Safe)
// ====================================================================================
static void chrperm_lc( char *str, gsize n, GRand *rng ) {
   g_return_if_fail( str != NULL && n > 0 && rng != NULL );

   if ( n < 2U ) {
      return;
   }

   // Fisher-Yates canônico: percorre até n - 1 (o último elemento já estará definido)
   for ( gsize i = 0; i < n - 1U; i++ ) {
      gint32 j_rand = g_rand_int_range( rng, ( gint32 )i, ( gint32 )n );
      gsize  j      = ( gsize )j_rand;

      char c = str[j];
      str[j] = str[i];
      str[i] = c;
   }
}

// ====================================================================================
// GERADOR DE GABARITOS (Motor em Bloco 1D Contíguo)
// ====================================================================================
static void gerar_gabaritos_lc( const gchar *arquivo, const int adicionados, const InterfaceDados *dados, const gchar *modo ) {
   g_return_if_fail( arquivo != NULL && dados != NULL && modo != NULL );

   // 1. Defesa rigorosa de limites
   if ( dados->total_quest_prova <= 0 || dados->total_quest_prova > MAX_QUEST_PROVA ) {
      g_printerr( "[ERRO] Quantidade de questões (%d) inválida.\n", dados->total_quest_prova );
      return;
   }

   if ( adicionados <= 0 ) {
      return;
   }

   FILE *p = fopen( arquivo, modo );
   if ( !p ) {
      g_printerr( "[ERRO] Não foi possível abrir o arquivo binário para gravação: %s\n", arquivo );
      return;
   }

   gsize num_provas  = ( gsize )adicionados;
   gsize total_quest = ( gsize )dados->total_quest_prova;
   gsize max_len     = ( gsize )MAX_QUEST_PROVA + 1U;

   // 2. Alocação Contígua Plana (já inicializada com '\0')
   char *bloco = g_malloc0_n( num_provas, max_len );
   g_autoptr( GRand ) rng_local = g_rand_new();

   // 3. Preenchimento Equilibrado e Embaralhamento por Prova
   for ( gsize i = 0; i < num_provas; i++ ) {
      gsize base_idx = i * max_len;
      gint32 offset  = g_rand_int_range( rng_local, 0, 5 );

      for ( gsize q = 0; q < total_quest; q++ ) {
         bloco[base_idx + q] = ( char )( 'A' + ( ( q + ( gsize )offset ) % 5U ) );
      }

      bloco[base_idx + total_quest] = '\0';

      // Embaralha a fatia atual passando o endereço indexado diretamente
      chrperm_lc( &bloco[base_idx], total_quest, rng_local );
   }

   // 4. I/O em Bloco Único
   gsize blocos_gravados = fwrite( bloco, max_len, num_provas, p );

   if ( blocos_gravados != num_provas ) {
      g_printerr( "[ERRO CRÍTICO] Falha de I/O: Apenas %" G_GSIZE_FORMAT " de %" G_GSIZE_FORMAT " registros foram salvos.\n",
                  blocos_gravados, num_provas );
   }

   g_free( bloco );
   fclose( p );
}


// =================================================================================================================
// GERENCIAMENTO DE FLUXO DE GABARITOS - Versão Otimizada (Painel de Feedback)
// =================================================================================================================
void gerenciar_fluxo_gabaritos( GtkWidget *widget, InterfacePainel *painel, const AppContext *ctx ) {
   g_return_if_fail( widget != NULL && painel != NULL && ctx != NULL );

   const InterfaceDados     *dados     = &ctx->dados;
   const InterfaceCabecalho *cabecalho = &ctx->cabecalho;

   if ( dados->qtd_alunos_ativos <= 0 ) {
      g_printerr( "[ERRO] Não é possível gerenciar gabaritos com %d alunos ativos.\n", dados->qtd_alunos_ativos );
      return;
   }

   g_autofree char *destino = g_build_filename(
                                 dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "gabaritos", NULL
                              );

   if ( g_mkdir_with_parents( destino, 0755 ) != 0 ) {
      g_printerr( "[ERRO CRÍTICO] Não foi possível criar os diretórios de destino: %s\n", destino );
      return;
   }

   g_autofree gchar *nome = nome_base_gabaritos_bin( cabecalho->foco_turma, cabecalho->foco_disciplina,
                            cabecalho->foco_periodo, dados->seq_prova );

   g_autofree char *arquivo = g_build_filename( destino, nome, NULL );

   // Cenário A: O arquivo não existe (Primeira compilação da prova)
   if ( !g_file_test( arquivo, G_FILE_TEST_EXISTS ) ) {
      gerar_gabaritos_lc( arquivo, dados->qtd_alunos_ativos, dados, "wb" );
      return;
   }

   // O tamanho de 1 registro equivale à capacidade máxima de caracteres + 1 (para o '\0')
   gsize tamanho_registro = ( gsize )MAX_QUEST_PROVA + 1U;
   int contador = contar_registros_binarios( arquivo, tamanho_registro );

   // Sub-cenário B1: Alunos adicionados (Seguro - Não sobrescreve, apenas anexa)
   if ( contador > 0 && contador < dados->qtd_alunos_ativos ) {
      int adicionados = dados->qtd_alunos_ativos - contador;

      gerar_gabaritos_lc( arquivo, adicionados, dados, "ab" );

      painel->format_titulo    = meu_gerador_variadico( "⚠ Lista de Alunos Expandida" );
      painel->format_subtitulo = meu_gerador_variadico( "%d novos alunos ativos foram adicionados desde a última compilação.", adicionados );
      painel->format_instrucao = meu_gerador_variadico( "%d novos gabaritos foram anexados mantendo os antigos intactos.", adicionados );
      criar_mensagem_painel( AVISO, painel );

      gchar *f_titulo    = meu_gerador_variadico( "⚙️ Gerando Provas e Gabaritos" );
      gchar *f_subtitulo = meu_gerador_variadico( "O processo leva aproximadamente 9 segundos." );
      gchar *f_instrucao = meu_gerador_variadico( "Compilando lote para <b>%s</b> (%s)... Por favor, aguarde.", ctx->dados.escola, ctx->dados.ano );
      agendar_ou_exibir_mensagem_painel( INFO, painel, f_titulo, f_subtitulo, f_instrucao );
   }
   // Sub-cenário B2: Risco de Perda de Dados (Alunos removidos, qtde idêntica, ou erro de leitura)
   else {
      GtkWindow *janela_principal = GTK_WINDOW( gtk_widget_get_toplevel( widget ) );
      char *msg_gabarito = NULL;

      if ( contador > dados->qtd_alunos_ativos ) {
         int removidos = contador - dados->qtd_alunos_ativos;
         msg_gabarito = meu_gerador_variadico(
                           "<b>ESCOLA:</b> %s\n"
                           "<b>TURMA:</b> %s\n\n"
                           "A turma atual possui <b>%d aluno(s) a menos</b> que o arquivo de gabaritos existente.\n\n"
                           "Deseja sobrescrever o arquivo para refletir a nova quantidade exata da turma?",
                           dados->escola, dados->turma, removidos
                        );
      } else if ( contador == 0 ) {
         msg_gabarito = meu_gerador_variadico(
                           "<b>Atenção:</b> O arquivo de gabaritos %s parece estar corrompido ou vazio.\n"
                           "Deseja gerar um novo arquivo substituindo-o?", nome
                        );
      } else {
         msg_gabarito = meu_gerador_variadico(
                           "<b>ESCOLA:</b> %s\n"
                           "<b>TURMA:</b> %s\n\n"
                           "O arquivo de <b>Gabaritos</b> da <b>%s Prova</b> do <b>%s</b> já existe.\n\n"
                           "Deseja criar um novo arquivo e sobrescrever o anterior?",
                           dados->escola, dados->turma, dados->prova_sequencia, dados->periodo
                        );
      }

      gboolean emitir_gabarito = mostrar_popup_confirmacao( janela_principal, "Aviso de Sobrescrita", msg_gabarito );
      g_free( msg_gabarito );

      if ( !emitir_gabarito && contador > 0 ) {
         painel->format_titulo    = meu_gerador_variadico( "✔ Reaproveitando Histórico" );
         painel->format_subtitulo = meu_gerador_variadico( "Gerando provas com o uso de gabaritos gerados em compilação anterior." );
         painel->format_instrucao = meu_gerador_variadico( "Os dados originais dos alunos foram preservados com sucesso." );
      } else {
         gerar_gabaritos_lc( arquivo, dados->qtd_alunos_ativos, dados, "wb" );
         painel->format_titulo    = meu_gerador_variadico( "✔ Novos Gabaritos Alocados" );
         painel->format_subtitulo = meu_gerador_variadico( "Gerando provas com o uso de uma nova sequência de gabaritos." );
         painel->format_instrucao = meu_gerador_variadico( "O arquivo anterior foi sobrescrito conforme solicitado." );
      }

      criar_mensagem_painel( SUCESSO, painel );

      gchar *f_titulo    = meu_gerador_variadico( "⚙️ Gerando Provas e Gabaritos" );
      gchar *f_subtitulo = meu_gerador_variadico( "O processo leva aproximadamente 9 segundos." );
      gchar *f_instrucao = meu_gerador_variadico( "Compilando lote para <b>%s</b> (%s)... Por favor, aguarde.", ctx->dados.escola, ctx->dados.ano );
      agendar_ou_exibir_mensagem_painel( INFO, painel, f_titulo, f_subtitulo, f_instrucao );
   }
}
