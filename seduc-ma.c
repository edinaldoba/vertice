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
#include "glib_gio.h"
#include "interface.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gstdio.h>
#include <omp.h>
#include <inttypes.h>



// Agora a função retorna explicitamente uint32_t
static uint32_t atou32_seguro( const char *num_string ) {
   // 1. Defesa básica
   if ( !num_string || num_string[0] == '\0' ) {
      return 0;
   }

   char *fim_da_leitura;

   // 2. Leitura de 64 bits sem sinal
   guint64 numero_extraido = g_ascii_strtoull( num_string, &fim_da_leitura, 10 );

   // 3. Validação de leitura
   if ( num_string == fim_da_leitura ) {
      return 0;
   }

   // 4. Proteção contra estouro exata para uint32_t (4.294.967.295)
   // G_MAXUINT32 é uma constante segura da GLib
   if ( numero_extraido > G_MAXUINT32 ) {
      return ( uint32_t ) G_MAXUINT32;
   }

   // 5. Conversão limpa e segura
   return ( uint32_t ) numero_extraido;
}


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



static double obter_largura_nome_mm( PangoLayout *layout, const char *texto ) {
   // Apenas atualiza o texto no layout já existente
   pango_layout_set_text( layout, texto, -1 );

   int largura_pango;
   pango_layout_get_size( layout, &largura_pango, NULL );

   // Converte Pango Units para Milímetros ( Fator exato: 72 / 25.4 = 2.834645 )
   return ( double )largura_pango / ( PANGO_SCALE * 2.834645 );
}

static int obter_limite_corte_nome( PangoLayout *layout, const char *aluno ) {
   int max_chars = g_utf8_strlen( aluno, -1 );
   int limite_corte = calcular_len_limpo( aluno, max_chars );

   char buffer[64];
   snprintf( buffer, sizeof( buffer ), "%.*s", limite_corte, aluno );

   // Mede a largura usando a função leve e o layout já instanciado
   double largura = obter_largura_nome_mm( layout, buffer );

   // O Loop de Ajuste Fino
   while ( largura > 69.0 && max_chars > 0 ) {
      max_chars--;
      limite_corte = calcular_len_limpo( aluno, max_chars );
      snprintf( buffer, sizeof( buffer ), "%.*s", limite_corte, aluno );
      largura = obter_largura_nome_mm( layout, buffer );
   }

   return limite_corte;
}

static gboolean eh_preposicao( const gchar *str ) {
   g_return_val_if_fail( str, FALSE );

   // Como a string raiz já foi convertida para minúscula previamente,
   // g_strcmp0 é computacionalmente mais leve que g_ascii_strcasecmp.
   return ( g_strcmp0( str, "de" ) == 0 ||
            g_strcmp0( str, "do" ) == 0 ||
            g_strcmp0( str, "da" ) == 0 ||
            g_strcmp0( str, "dos" ) == 0 ||
            g_strcmp0( str, "das" ) == 0 ||
            g_strcmp0( str, "e" ) == 0 );
}

static gchar* converter_nome_proprio( const gchar *nome_completo ) {
   if ( !nome_completo || *nome_completo == '\0' ) return NULL;

   // 1. Converte tudo para minúsculas (base padronizada)
   gchar *nome_minusculo = g_utf8_strdown( nome_completo, -1 );
   gchar **palavras = g_strsplit( nome_minusculo, " ", -1 );
   g_free( nome_minusculo );

   // 2. Processa cada palavra respeitando os bytes do UTF-8
   for ( int i = 0; palavras[i] != NULL; i++ ) {

      // Ignora múltiplos espaços (palavras vazias) ou preposições
      if ( palavras[i][0] == '\0' || eh_preposicao( palavras[i] ) ) {
         continue;
      }

      // 3. Isola e converte apenas a primeira letra da palavra
      // A aritmética de ponteiros aqui lida perfeitamente com caracteres acentuados (ex: Á, É)
      gssize tamanho_primeiro_char = g_utf8_next_char( palavras[i] ) - palavras[i];
      gchar *primeira_letra = g_utf8_strup( palavras[i], tamanho_primeiro_char );
      const gchar *resto_palavra = g_utf8_next_char( palavras[i] );

      // 4. Reconstrói a palavra capitalizada e recicla a memória
      gchar *palavra_capitalizada = g_strconcat( primeira_letra, resto_palavra, NULL );
      g_free( palavras[i] );
      palavras[i] = palavra_capitalizada;
      g_free( primeira_letra );
   }

   // 5. Remonta a string final limpa e libera o array
   gchar *resultado = g_strjoinv( " ", palavras );
   g_strfreev( palavras );

   return resultado;
}

static gchar *padronizar_nome_turma( const gchar *coluna, FILE **f_acesso, const gchar *diretorio ) {
   g_return_val_if_fail( coluna && f_acesso && diretorio, NULL );

   g_autofree gchar *coluna_minusculo = g_utf8_strdown( coluna, -1 );
   int len = strlen( coluna_minusculo );
   gchar *turma = NULL;

   if ( len > 9 ) {
      if ( coluna_minusculo[len - 3] == '1' ) {
         char c = coluna_minusculo[len - 4];
         const char *turno = ( c == 'm' ) ? "matutino" : ( c == 'n' ) ? "noturno" : "vespertino";
         turma = g_strdup_printf( "%.3s %s", &coluna_minusculo[len - 3], turno );
      } else {
         char c = coluna_minusculo[len - 9];
         const char *turno = ( c == 'm' ) ? "mat" : ( c == 'n' ) ? "not" : "vesp";
         turma = g_strdup_printf( "%.3s %.3s %s", &coluna_minusculo[len - 3], &coluna_minusculo[len - 7], turno );
      }

      // Cria a subpasta acesso no local correto da hierarquia
      g_autofree gchar *dir_acesso = g_build_filename( diretorio, "acessos", NULL );
      g_mkdir_with_parents( dir_acesso, 0755 );

      g_autofree gchar *acesso_turma = g_strdup_printf( "%s/%s.bin", dir_acesso, turma );
      *f_acesso = fopen( acesso_turma, "wb" );
   }

   return turma;
}


static void salvar_ficha_aluno_inicial( const char *nome_turma_padrao, int fonte, const gchar *diretorio ) {
   g_return_if_fail( nome_turma_padrao && diretorio );

   // =========================================================================
   // 1. ESPELHAMENTO DINÂMICO DE DIRETÓRIOS ( ./planilhas -> ./dados/listas )
   // =========================================================================
   gchar *dir_destino = NULL;
   if ( g_str_has_prefix( diretorio, "./planilhas" ) ) {
      // Pula exatamente os 11 caracteres de "./planilhas" e acopla o resto do caminho
      dir_destino = g_strdup_printf( "./dados/listas%s", diretorio + 11 );
   } else {
      dir_destino = g_strdup( "./dados/listas" ); // Fallback de segurança
   }

   // 2. CRIA AS SUBPASTAS DE DESTINO ESPELHADAS
   g_autofree gchar *dest_acesso = g_build_filename( dir_destino, "acessos", NULL );
   g_autofree gchar *dest_alunos = g_build_filename( dir_destino, "alunos", NULL );
   g_mkdir_with_parents( dest_acesso, 0755 );
   g_mkdir_with_parents( dest_alunos, 0755 );

   // 3. ABERTURA DOS ARQUIVOS DE ACESSO (Lendo da origem, gravando no destino)
   g_autofree gchar *arquivo_acesso_destino = g_strdup_printf( "%s/%s.bin", dest_acesso, nome_turma_padrao );
   FILE *f = fopen( arquivo_acesso_destino, "wb" );
   if ( !f ) {
      g_printerr( "Falha ao criar binário de acesso: %s\n", arquivo_acesso_destino );
      g_free( dir_destino );
      return;
   }

   g_autofree gchar *arquivo_acesso_origem = g_strdup_printf( "%s/acessos/%s.bin", diretorio, nome_turma_padrao );
   FILE *f_acesso = fopen( arquivo_acesso_origem, "rb" );
   if ( !f_acesso ) {
      g_printerr( "Falha ao abrir acesso de origem: %s\n", arquivo_acesso_origem );
      fclose( f );
      g_free( dir_destino );
      return;
   }

   // =========================================================================
   // 4. INICIALIZAÇÃO ÚNICA DO PANGO
   // =========================================================================
   PangoFontMap *font_map = pango_cairo_font_map_get_default();
   PangoContext *context = pango_font_map_create_context( font_map );
   PangoLayout *layout = pango_layout_new( context );

   const char *fonte_latex = (fonte == 1) ? "CMU Bright 11" : "CMU Serif 11";
   PangoFontDescription *desc = pango_font_description_from_string( fonte_latex );
   pango_layout_set_font_description( layout, desc );
   pango_font_description_free( desc );

   AcessoTurmas turmas;
   AcessoFicha acesso;
   int i = 0;

   // 5. LOOP DE PROCESSAMENTO ALUNO A ALUNO
   while ( fread( &turmas, sizeof( AcessoTurmas ), 1, f_acesso ) == 1 ) {

      acesso.sit = situacao_aluno( turmas.sit );
      acesso.ativo = ( acesso.sit & ( MATRICULA_INTERNA | MATRICULA_EXTERNA | MATRICULA_REGULAR ) ) != 0;
      acesso.cod_aluno = turmas.cod_aluno;

      fwrite( &acesso, sizeof( AcessoFicha ), 1, f );

      // Busca a FichaSiaep (CRUA) no diretório original dinâmico
      g_autofree gchar *arquivo_aluno_origem = g_strdup_printf( "%s/alunos/%" PRIu32 ".bin", diretorio, acesso.cod_aluno );
      FILE *f_origem = fopen( arquivo_aluno_origem, "rb" );

      if ( !f_origem ) continue;

      FichaSiaep siaep;
      if ( fread( &siaep, sizeof( FichaSiaep ), 1, f_origem ) == 1 ) {

         FichaAluno ficha = {0};
         ficha.idx = i;

         g_autofree char *nome_formatado = converter_nome_proprio( siaep.aluno );
         ficha.limite_corte = obter_limite_corte_nome( layout, nome_formatado );

         snprintf( ficha.aluno, sizeof( ficha.aluno ), "%s", nome_formatado );
         snprintf( ficha.nasc, sizeof( ficha.nasc ), "%s", siaep.nasc );
         snprintf( ficha.sexo, sizeof( ficha.sexo ), "%s", ( siaep.sexo == 'M' ) ? "Masculino" : "Feminino" );

         // Zera o status periódico (será gerido dinamicamente durante o uso do Vértice)
         ficha.sit = SEM_SITUACAO;
         ficha.ativo = FALSE;

         // Grava a FichaAluno (RICA) no diretório espelhado
         g_autofree gchar *arquivo_aluno_destino = g_strdup_printf( "%s/%" PRIu32 ".bin", dest_alunos, acesso.cod_aluno );
         FILE *f_destino = fopen( arquivo_aluno_destino, "wb" );

         if ( f_destino ) {
            fwrite( &ficha, sizeof( FichaAluno ), 1, f_destino );
            fclose( f_destino );
         }
      }

      fclose( f_origem );
      i++;
   }

   fclose( f_acesso );
   fclose( f );

   g_free( dir_destino );
   g_object_unref( layout );
   g_object_unref( context );
}

static gboolean siaep_processar_arquivo( const gchar *arquivo_xls, int fonte ) {
   g_return_val_if_fail( arquivo_xls, FALSE );

   // 1. Extrai o diretório base (onde o .xls foi encontrado na hierarquia)
   g_autofree gchar *diretorio = g_path_get_dirname( arquivo_xls );

   // Prepara o nome do CSV temporário
   g_autofree gchar *arquivo_csv = g_strdup( arquivo_xls );
   gchar *extensao = strrchr( arquivo_csv, '.' );
   if ( extensao ) strcpy( extensao, ".csv" );

   // Prepara o nome do DAT original
   g_autofree gchar *arquivo_dat = g_strdup( arquivo_xls );
   extensao = strrchr( arquivo_dat, '.' );
   if ( extensao ) strcpy( extensao, ".dat" );

   // 2. Executa a conversão via ssconvert
   g_autofree gchar *comando = g_strdup_printf( "ssconvert \"%s\" \"%s\"", arquivo_xls, arquivo_csv );
   g_autoptr( GError ) erro = NULL;
   if ( !g_spawn_command_line_sync( comando, NULL, NULL, NULL, &erro ) ) {
      g_printerr( "Erro ao invocar ssconvert: %s\n", erro->message );
      return FALSE;
   }

   // 3. Carrega o CSV na memória e aplica a limpeza (sed nativo)
   g_autofree gchar *conteudo_csv = NULL;
   if ( !g_file_get_contents( arquivo_csv, &conteudo_csv, NULL, &erro ) ) {
      g_printerr( "Erro ao ler CSV: %s\n", erro->message );
      return FALSE;
   }

   aplicar_sed_nativo( conteudo_csv );

   // 4. Cria as subpastas dinamicamente DENTRO do diretório de origem atual
   g_autofree gchar *dir_acesso = g_build_filename( diretorio, "acessos", NULL );
   g_autofree gchar *dir_alunos = g_build_filename( diretorio, "alunos", NULL );
   g_mkdir_with_parents( dir_acesso, 0755 );
   g_mkdir_with_parents( dir_alunos, 0755 );

   // 5. Processamento das linhas
   FILE *saida = fopen( arquivo_dat, "w" );
   if ( !saida ) return FALSE;

   FILE *f_acesso = NULL;
   AcessoTurmas turmas = {0};
   gboolean cabecalho_escrito = FALSE;
   gchar *nome_turma_padrao = NULL;

   g_auto( GStrv ) linhas = g_strsplit( conteudo_csv, "\n", -1 );

   for ( guint i = 0; linhas[i] != NULL; i++ ) {
      gchar *linha = linhas[i];
      g_strstrip( linha );
      if ( strlen( linha ) == 0 ) continue;

      g_auto( GStrv ) colunas = g_strsplit( linha, ";", -1 );
      guint num_colunas = g_strv_length( colunas );

      // Identificação da turma
      if ( g_ascii_strncasecmp( colunas[0], "TURMA: ", 7 ) == 0 && num_colunas > 0 ) {

         // Conforme a sua arquitetura: a padronização e o acesso não recebem o parâmetro diretorio
         nome_turma_padrao = padronizar_nome_turma( colunas[0], &f_acesso, diretorio );

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

         // Gravação do Binário de Origem (FichaSiaep)
         FichaSiaep siaep = {0};
         snprintf( siaep.aluno, sizeof( siaep.aluno ), "%s", colunas[7] );
         siaep.sexo = colunas[11][0];
         snprintf( siaep.nasc, sizeof( siaep.nasc ), "%s", colunas[14] );

         // Monta o caminho do binário do aluno vinculando à subpasta dinâmica
         g_autofree gchar *arquivo_bin = g_strdup_printf( "%s/%s.bin", dir_alunos, colunas[2] );

         #pragma omp critical(escreve_aluno)
         {
            FILE *p = fopen( arquivo_bin, "wb" );
            if ( p ) {
               fwrite( &siaep, sizeof( FichaSiaep ), 1, p );
               fclose( p );
            }
         }

         // Gravação do Binário de Acesso da Turma (AcessoTurmas)
         if ( f_acesso ) {
            turmas.cod_aluno = atou32_seguro( colunas[2] );
            snprintf( turmas.sit, sizeof( turmas.sit ), "%s", colunas[16] );
            fwrite( &turmas, sizeof( AcessoTurmas ), 1, f_acesso );
         }
      }
   }

   fclose( saida );
   if ( f_acesso ) fclose( f_acesso );

   // 6. Chamada para a função que espelha os dados transformados para ./dados/listas
   salvar_ficha_aluno_inicial( nome_turma_padrao, fonte, diretorio );

   // 7. Limpeza e Renomeio Seguro
   g_remove( arquivo_csv ); // Remove o CSV temporário

   if ( nome_turma_padrao ) {
      // Monta os novos caminhos vinculando o nome extraído ao diretório de origem
      g_autofree gchar *novo_xls = g_strdup_printf( "%s/%s.xls", diretorio, nome_turma_padrao );
      g_autofree gchar *novo_dat = g_strdup_printf( "%s/%s.dat", diretorio, nome_turma_padrao );

      // Renomeia o .xls original e o .dat recém-criado
      g_rename( arquivo_xls, novo_xls );
      g_rename( arquivo_dat, novo_dat );

      g_free( nome_turma_padrao );
   }

   return TRUE;
}

static void atualizar_acesso_e_adicionar_novos_alunos( const AppContext *ctx ) {
   g_return_if_fail( ctx );
   const InterfaceListas  *listas  = &ctx->listas;
   const InterfaceDados   *dados   = &ctx->dados;

   int limite_turmas = ctx->cascata.limite.turmas;

   // 1. Atualização dos arquivos de acesso
   for ( int i = 0; i < limite_turmas; i++ ) {
      g_autofree char *acesso_bin = g_strdup_printf( "%s.bin", listas->turmas[i].str );
      g_autofree char *acesso_bin_orig = g_build_filename( ".", "dados", "listas", dados->ano, dados->escola, dados->periodo,
                                                           "acessos", acesso_bin, NULL );

      // Monta o diretório base da turma, onde as disciplinas devem estar alocadas
      g_autofree char *dir_turma_base = g_build_filename( ".", "dados", "informados", dados->ano, dados->escola,
                                                          listas->turmas[i].str, NULL );

      g_autoptr( GError ) erro = NULL;
      g_autoptr( GDir ) dir_turma = g_dir_open( dir_turma_base, 0, &erro );

      if ( !dir_turma ) {
         g_printerr( "[Aviso] Nenhuma disciplina encontrada para a turma %s: %s\n", listas->turmas[i].str, erro->message );
         continue; // Se a pasta da turma ainda não existir, pula para a próxima
      }

      const gchar *nome_disciplina;
      // Lê o conteúdo dentro da pasta da turma (que serão as pastas das disciplinas)
      while ( ( nome_disciplina = g_dir_read_name( dir_turma ) ) != NULL ) {

         g_autofree char *caminho_disciplina = g_build_filename( dir_turma_base, nome_disciplina, NULL );

         // Verifica se a entrada atual é realmente um diretório (ignora arquivos)
         if ( g_file_test( caminho_disciplina, G_FILE_TEST_IS_DIR ) ) {

            // Monta o diretório final de destino (agora incluindo a disciplina e o período)
            g_autofree char *dir_periodo_dest = g_build_filename( caminho_disciplina, dados->periodo, NULL );

            g_autofree char *acesso_bin_dest = g_build_filename( dir_periodo_dest, "acesso.bin", NULL );

            // Faz a cópia do acesso.bin original para dentro desta disciplina específica
            gio_copiar_arquivo( acesso_bin_orig, acesso_bin_dest );
         }
      }
   }

   // 2. Caminhos dos alunos
   g_autofree char *dir_alunos_orig = g_build_filename( ".", "dados", "listas", dados->ano, dados->escola, dados->periodo,
                                                        "alunos", NULL );
   g_autofree char *dir_alunos_dest = g_build_filename( ".", "dados", "informados", dados->ano, dados->escola,
                                                        "alunos", NULL );

   // Garante que a árvore de pastas de destino já exista antes de iniciar as cópias
   g_mkdir_with_parents( dir_alunos_dest, 0755 );

   // 3. Sincronização Otimizada dos Novos Alunos
   g_autoptr( GError ) erro = NULL;
   g_autoptr( GDir ) dir = g_dir_open( dir_alunos_orig, 0, &erro );

   if ( !dir ) {
      g_printerr( "Erro ao abrir diretório de alunos origem: %s\n", erro->message );
      return;
   }

   const gchar *nome_arquivo;
   // Varre a pasta original pegando apenas os nomes dos arquivos
   while ( ( nome_arquivo = g_dir_read_name( dir ) ) != NULL ) {

      if ( g_str_has_suffix( nome_arquivo, ".bin" ) ) {
         // Monta o caminho de onde esse arquivo DEVERIA estar no destino
         g_autofree char *arquivo_dest = g_build_filename( dir_alunos_dest, nome_arquivo, NULL );

         // g_file_test é extremamente rápido (não abre o arquivo, só lê a tabela do sistema de arquivos)
         if ( !g_file_test( arquivo_dest, G_FILE_TEST_EXISTS ) ) {

            // O arquivo não existe no destino! É um aluno novo. Montamos a origem e copiamos.
            g_autofree char *arquivo_orig = g_build_filename( dir_alunos_orig, nome_arquivo, NULL );

            gio_copiar_arquivo( arquivo_orig, arquivo_dest );
         }
      }
   }
}

void siaep_atualizar_alunos( InterfacePainel *painel, const AppContext *ctx ) {
   g_return_if_fail( painel && ctx );
   const InterfaceDados *dados   = &ctx->dados;

   g_autofree gchar *dir_origem = g_build_filename( ".", "planilhas", dados->ano, dados->escola, dados->periodo, NULL );
   g_autoptr( GPtrArray ) lista_arquivos = g_ptr_array_new_with_free_func( g_free );

   g_autoptr( GError ) erro = NULL;
   g_autoptr( GDir ) dir = g_dir_open( dir_origem, 0, &erro );

   if ( dir ) {
      const gchar *nome_arquivo;
      while ( ( nome_arquivo = g_dir_read_name( dir ) ) != NULL ) {
         if ( g_str_has_suffix( nome_arquivo, ".xls" ) || g_str_has_suffix( nome_arquivo, ".XLS" ) ) {
            gchar *caminho_completo = g_build_filename( dir_origem, nome_arquivo, NULL );
            g_ptr_array_add( lista_arquivos, caminho_completo );
         }
      }
   } else {
      g_printerr( "Erro ao abrir diretório %s: %s\n", dir_origem, erro->message );
      return;
   }

   int num_arquivos = lista_arquivos->len;

   if ( num_arquivos == 0 ) {
      g_print( "Nenhum arquivo .xls encontrado no diretório: %s\n", dir_origem );
      return;
   }

   g_print( "Iniciando processamento paralelo de %d arquivos...\n", num_arquivos );

   #pragma omp parallel for schedule( dynamic, 1 )
   for ( int i = 0; i < num_arquivos; i++ ) {
      const gchar *arquivo_atual = ( const gchar * )g_ptr_array_index( lista_arquivos, i );

      g_print( "[Thread %d] Processando: %s\n", omp_get_thread_num(), arquivo_atual );

      siaep_processar_arquivo( arquivo_atual, dados->fonte_latex );
   }

   atualizar_acesso_e_adicionar_novos_alunos( ctx );
}




// Salva o estado atual do aluno da RAM direto para o disco
void salvar_ficha_aluno( const FichaAluno *aluno, uint32_t cod_aluno ) {
   g_return_if_fail( aluno && cod_aluno );

   g_autofree gchar *caminho = g_strdup_printf( "./dados/alunos/%" PRIu32 ".bin", cod_aluno );

   // 1. Abre, 2. Despeja a memória, 3. Fecha (Tudo em microssegundos)
   FILE *p = fopen( caminho, "wb" );
   if ( p ) {
      fwrite( aluno, sizeof( FichaAluno ), 1, p );
      fclose( p );
   } else {
      g_printerr( "Falha ao sincronizar o aluno %" PRIu32 " no disco.\n", cod_aluno );
   }
}
