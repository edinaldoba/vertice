/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

// SIAEP - Sistema Integrado de Administração de Escolas Públicas
// Este modulo faz a ponte de ida e volta entre o Vértice e o Siaep

#include "seduc-ma.h"
#include "comum.h"
#include "basicas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gstdio.h>
#include <omp.h>

static SituacaoAluno situacao_aluno( const char *sit ) {
   g_return_val_if_fail( sit, SEM_SITUACAO );

   if ( strcmp( sit, "MTR" ) == 0 ) return MATRICULA_REGULAR;
   if ( strcmp( sit, "MTE" ) == 0 ) return MATRICULA_EXTERNA;
   if ( strcmp( sit, "MTI" ) == 0 ) return MATRICULA_INTERNA;
   if ( strcmp( sit, "CTE" ) == 0 ) return TRANSFERENCIA_EXTERNA;
   if ( strcmp( sit, "CTI" ) == 0 ) return TRANSFERENCIA_INTERNA;
   if ( strcmp( sit, "CPG" ) == 0 ) return EVADIDO;

   return SEM_SITUACAO;
}

static void salvar_ficha_aluno_inicial( const char *nome_turma_padrao ) {
   g_return_if_fail( nome_turma_padrao );

   g_autofree gchar *acesso_turma = g_strdup_printf( "./planilhas/acesso/%s.bin", nome_turma_padrao );
   FILE *f_acesso = fopen( acesso_turma, "rb" );

   if ( !f_acesso ) {
      g_printerr( "Falha ao abrir o arquivo de acesso da turma: %s\n", acesso_turma );
      return;
   }

   AcessoTurmas siaep;
   int i = 0; // Substitui o antigo 'n_linhas' do loop for

   // O loop lê diretamente até o fim do arquivo (EOF), poupando uma varredura no disco
   while ( fread( &siaep, sizeof( AcessoTurmas ), 1, f_acesso ) == 1 ) {

      // Escopo da string de ORIGEM
      g_autofree gchar *arquivo_origem = g_strdup_printf( "./planilhas/alunos/%s.bin", siaep.cod_aluno );
      FILE *f_origem = fopen( arquivo_origem, "rb" );

      if ( !f_origem ) continue;

      FichaSiaep diario;
      if ( fread( &diario, sizeof( FichaSiaep ), 1, f_origem ) == 1 ) {

         FichaAlunoAux ficha = {0};
         ficha.idx = i;
         snprintf( ficha.aluno, sizeof( ficha.aluno ), "%s", diario.aluno );
         snprintf( ficha.nasc, sizeof( ficha.nasc ), "%s", diario.nasc );
         snprintf( ficha.sexo, sizeof( ficha.sexo ), "%s", ( diario.sexo == 'M' ) ? "Masculino" : "Feminino" );
         ficha.sit = situacao_aluno( siaep.sit );
         ficha.ativo = ( ficha.sit & ( MATRICULA_INTERNA | MATRICULA_EXTERNA | MATRICULA_REGULAR ) ) != 0;

         // Escopo da string de DESTINO isolado para o g_autofree atuar perfeitamente
         g_autofree gchar *arquivo_destino = g_strdup_printf( "./dados/alunos/%s.bin", siaep.cod_aluno );
         FILE *f_destino = fopen( arquivo_destino, "wb" );

         if ( f_destino ) {
            fwrite( &ficha, sizeof( FichaAlunoAux ), 1, f_destino );
            fclose( f_destino );
         } else {
            g_printerr( "Falha ao gravar a ficha inicial do aluno %s\n", siaep.cod_aluno );
         }
      }

      fclose( f_origem );
      i++; // Incrementa o índice alfabético preservado do SIAEP
   }

   fclose( f_acesso );
}

// Função interna equivalente ao sed do bash para remover aspas e substituir vírgulas
static void aplicar_sed_nativo( gchar *texto ) {
   gchar *leitura = texto;
   gchar *escrita = texto;
   while ( *leitura ) {
      if ( *leitura == '"' ) {
         leitura++; // Pula aspas duplas
         continue;
      }
      if ( *leitura == ',' ) {
         *escrita = ';'; // Substitui vírgula por ponto-e-vírgula
      } else {
         *escrita = *leitura;
      }
      leitura++;
      escrita++;
   }
   *escrita = '\0';
}

static gchar *padronizar_nome_turma( const gchar *coluna, FILE **f_acesso ) {
   g_return_val_if_fail( coluna && f_acesso, NULL );

   g_autofree gchar *coluna_minusculo = g_utf8_strdown( coluna, -1 );
   int len = strlen( coluna_minusculo );
   gchar *turma = NULL;

   if ( len > 9 ) { // Proteção de memória
      if ( coluna_minusculo[len - 3] == '1' ) {
         char c = coluna_minusculo[len - 4];
         const char *turno = ( c == 'm' ) ? "matutino" : ( c == 'n' ) ? "noturno" : "vespertino";

         turma = g_strdup_printf( "%.3s %s", &coluna_minusculo[len - 3], turno );
      } else {
         char c = coluna_minusculo[len - 9];
         const char *turno = ( c == 'm' ) ? "mat" : ( c == 'n' ) ? "not" : "vesp";

         turma = g_strdup_printf( "%.3s %.3s %s", &coluna_minusculo[len - 3], &coluna_minusculo[len - 7], turno );
      }

      // Prepara o caminho e abre o arquivo, atualizando o ponteiro original do chamador
      g_autofree gchar *acesso_turma = g_strdup_printf( "./planilhas/acesso/%s.bin", turma );
      *f_acesso = fopen( acesso_turma, "wb" );
   }

   // Retorna a string recém-criada. O chamador gerencia essa memória.
   return turma;
}

static gboolean siaep_processar_arquivo( const gchar *arquivo_xls ) {
   g_return_val_if_fail( arquivo_xls, FALSE );

   // 1. Extrai o diretório base para garantir que os arquivos renomeados fiquem na mesma pasta
   g_autofree gchar *diretorio = g_path_get_dirname( arquivo_xls );

   // Prepara o nome do CSV temporário
   g_autofree gchar *arquivo_csv = g_strdup( arquivo_xls );
   gchar *extensao = strrchr( arquivo_csv, '.' );
   if ( extensao ) strcpy( extensao, ".csv" );

   // Prepara o nome do DAT original
   g_autofree gchar *arquivo_dat = g_strdup( arquivo_xls );
   extensao = strrchr( arquivo_dat, '.' );
   if ( extensao ) strcpy( extensao, ".dat" );

   // 2. Executa a conversão via ssconvert (Substitui o script bash)
   g_autofree gchar *comando = g_strdup_printf( "ssconvert \"%s\" \"%s\"", arquivo_xls, arquivo_csv );
   g_autoptr( GError ) erro = NULL;
   if ( !g_spawn_command_line_sync( comando, NULL, NULL, NULL, &erro ) ) {
      g_printerr( "Erro ao invocar ssconvert: %s\n", erro->message );
      return FALSE;
   }

   // 3. Carrega o CSV na memória e aplica a limpeza (Substitui o sed)
   g_autofree gchar *conteudo_csv = NULL;
   if ( !g_file_get_contents( arquivo_csv, &conteudo_csv, NULL, &erro ) ) {
      g_printerr( "Erro ao ler CSV: %s\n", erro->message );
      return FALSE;
   }

   aplicar_sed_nativo( conteudo_csv );

   // 4. Garante que os diretórios de saída existam
   g_mkdir_with_parents( "./planilhas/acesso", 0755 );
   g_mkdir_with_parents( "./planilhas/alunos", 0755 );

   // 5. Processamento das linhas
   FILE *saida = fopen( arquivo_dat, "w" );
   if ( !saida ) return FALSE;

   FILE *f_acesso = NULL;
   AcessoTurmas siaep = {0};
   gboolean cabecalho_escrito = FALSE;

   // Variável para armazenar o nome da turma que será usado no renomeio final
   gchar *nome_turma_padrao = NULL;

   g_auto( GStrv ) linhas = g_strsplit( conteudo_csv, "\n", -1 );

   for ( guint i = 0; linhas[i] != NULL; i++ ) {
      gchar *linha = linhas[i];
      g_strstrip( linha ); // Remove quebras de linha e espaços nas pontas
      if ( strlen( linha ) == 0 ) continue;

      g_auto( GStrv ) colunas = g_strsplit( linha, ";", -1 );
      guint num_colunas = g_strv_length( colunas );

      // Identificação da turma usando a verificação de prefixo "TURMA: "
      if ( g_ascii_strncasecmp( colunas[0], "TURMA: ", 7 ) == 0 && num_colunas > 0 ) {

         nome_turma_padrao = padronizar_nome_turma( colunas[0], &f_acesso );

         if ( !nome_turma_padrao || !f_acesso ) {
            g_printerr( "[AVISO] Turma ou acesso vazios para este arquivo xls\n" );
         }
      }

      // Identificação do cabeçalho
      if ( !cabecalho_escrito && num_colunas > 7 && strstr( colunas[7], "NOME_ALUNO" ) ) {
         fprintf( saida, "Cod. Aluno;NOME_ALUNO;Sexo;Nascimento;Situação\n" );
         cabecalho_escrito = TRUE;
         continue;
      }

      // Processamento dos Alunos
      if ( cabecalho_escrito && num_colunas > 16 ) {
         if ( strlen( colunas[7] ) < 2 ) continue;

         if ( strstr( colunas[1], "Total" ) || strstr( colunas[0], "Legenda" ) ) break;

         // Gravação no arquivo .dat
         fprintf( saida, "%s;%s;%s;%s;%s\n",
                  colunas[2], colunas[7], colunas[11], colunas[14], colunas[16] );

         // Gravação do Binário Diário
         FichaSiaep diario = {0};
         snprintf( diario.aluno, sizeof( diario.aluno ), "%s", colunas[7] );
         diario.sexo = colunas[11][0]; // Pega a primeira letra do sexo
         snprintf( diario.nasc, sizeof( diario.nasc ), "%s", colunas[14] );

         g_autofree gchar *arquivo_bin = g_strdup_printf( "./planilhas/alunos/%s.bin", colunas[2] );

         #pragma omp critical(escreve_aluno)
         {
            FILE *p = fopen( arquivo_bin, "wb" );
            if ( p ) {
               fwrite( &diario, sizeof( FichaSiaep ), 1, p );
               fclose( p );
            }
         }

         // Gravação do Binário de Acesso da Turma
         if ( f_acesso ) {
            snprintf( siaep.cod_aluno, sizeof( siaep.cod_aluno ), "%s", colunas[2] );
            snprintf( siaep.sit, sizeof( siaep.sit ), "%s", colunas[16] );
            fwrite( &siaep, sizeof( AcessoTurmas ), 1, f_acesso );
         }
      }
   }

   fclose( saida );
   fclose( f_acesso );

   salvar_ficha_aluno_inicial( nome_turma_padrao );

   // 6. Limpeza e Renomeio Seguro
   g_remove( arquivo_csv ); // Remove o CSV temporário

   if ( nome_turma_padrao ) {
      // Monta os novos caminhos vinculando o nome extraído ao diretório de origem
      g_autofree gchar *novo_xls = g_strdup_printf( "%s/%s.xls", diretorio, nome_turma_padrao );
      g_autofree gchar *novo_dat = g_strdup_printf( "%s/%s.dat", diretorio, nome_turma_padrao );

      // Renomeia o .xls original e o .dat recém-criado
      g_rename( arquivo_xls, novo_xls );
      g_rename( arquivo_dat, novo_dat );

      g_free( nome_turma_padrao ); // Libera a string que foi alocada lá em padronizar_nome_turma
   }

   return TRUE;
}



void siaep_atualizar_alunos( InterfacePainel *painel, FichaAluno *diario ) {
   g_return_if_fail( painel && diario );

   g_autofree gchar *diretorio_origem = g_strdup("./planilhas");

   g_autoptr( GError ) erro = NULL;
   g_autoptr( GDir ) dir = g_dir_open( diretorio_origem, 0, &erro );

   if ( !dir ) {
      g_printerr( "Erro ao abrir diretório %s: %s\n", diretorio_origem, erro->message );
      return;
   }

   // 1. Cria um array dinâmico da GLib para guardar os caminhos
   // O g_free garante que a memória das strings será liberada automaticamente no fim
   g_autoptr( GPtrArray ) lista_arquivos = g_ptr_array_new_with_free_func( g_free );

   const gchar *nome_arquivo;
   while ( ( nome_arquivo = g_dir_read_name( dir ) ) != NULL ) {
      // Obs: Alterei .xml para .xls acompanhando a lógica original do seu conversor
      if ( g_str_has_suffix( nome_arquivo, ".xls" ) ) {
         gchar *caminho_completo = g_build_filename( diretorio_origem, nome_arquivo, NULL );
         g_ptr_array_add( lista_arquivos, caminho_completo );
      }
   }

   int num_arquivos = lista_arquivos->len;

   if ( num_arquivos > 0 ) {
      g_print( "Iniciando processamento paralelo de %d arquivos...\n", num_arquivos );

      // 2. Dispara as threads do OpenMP
      // Usamos schedule(dynamic, 1) porque o ssconvert de uma planilha grande
      // pode demorar mais que o de uma planilha pequena. Assim as threads ociosas pegam o próximo.
      #pragma omp parallel for schedule( dynamic, 1 )
      for ( int i = 0; i < num_arquivos; i++ ) {

         // Resgata a string do array fazendo o cast
         const gchar *arquivo_atual = ( const gchar * ) g_ptr_array_index( lista_arquivos, i );

         g_print( "[Thread %d] Processando: %s\n", omp_get_thread_num(), arquivo_atual );

         siaep_processar_arquivo( arquivo_atual );
      }
   } else {
      g_print( "Nenhum arquivo .xls encontrado no diretório: %s\n", diretorio_origem );
   }
}




// Salva o estado atual do aluno da RAM direto para o disco
void salvar_ficha_aluno( const FichaAluno *aluno, const gchar *cod_aluno ) {
   g_return_if_fail( aluno && cod_aluno );

   g_autofree gchar *caminho = g_strdup_printf( "./dados/alunos/%s.bin", cod_aluno );

   // 1. Abre, 2. Despeja a memória, 3. Fecha (Tudo em microssegundos)
   FILE *p = fopen( caminho, "wb" );
   if ( p ) {
      fwrite( aluno, sizeof( FichaAluno ), 1, p );
      fclose( p );
   } else {
      g_printerr( "Falha ao sincronizar o aluno %s no disco.\n", cod_aluno );
   }
}
