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
#include <string.h>
#include <glib/gstdio.h>
#include <omp.h>
#include <inttypes.h>


static int quantidade_diretorios_aux( const char *diretorio ) {
   g_return_val_if_fail( diretorio != NULL, 0 );

   GError *error = NULL;
   GDir *dir = g_dir_open( diretorio, 0, &error );

   if ( dir == NULL ) {
      g_clear_error( &error );
      return 0;
   }

   int cont = 0;
   const gchar *nome_item;

   while ( ( nome_item = g_dir_read_name( dir ) ) != NULL ) {
      if ( nome_item[0] == '.' ) {
         continue;
      }

      g_autofree gchar *caminho_completo = g_build_filename( diretorio, nome_item, NULL );

      if ( g_file_test( caminho_completo, G_FILE_TEST_IS_DIR ) ) {
         cont++;
      }
   }

   g_dir_close( dir );

   return cont;
}


static uint32_t atou32_seguro( const char *num_string ) {
   if ( !num_string || num_string[0] == '\0' ) {
      return 0;
   }

   char *fim_da_leitura;
   guint64 numero_extraido = g_ascii_strtoull( num_string, &fim_da_leitura, 10 );

   if ( num_string == fim_da_leitura ) {
      return 0;
   }

   if ( numero_extraido > G_MAXUINT32 ) {
      return ( uint32_t ) G_MAXUINT32;
   }

   return ( uint32_t ) numero_extraido;
}


static SituacaoAluno situacao_aluno( const char *sit ) {
   g_return_val_if_fail( sit != NULL, SEM_SITUACAO );

   if ( g_strcmp0( sit, "MTR" ) == 0 ) {
      return MATRICULA_REGULAR;
   }
   if ( g_strcmp0( sit, "MTE" ) == 0 ) {
      return MATRICULA_EXTERNA;
   }
   if ( g_strcmp0( sit, "MTI" ) == 0 ) {
      return MATRICULA_INTERNA;
   }
   if ( g_strcmp0( sit, "CTE" ) == 0 ) {
      return TRANSFERENCIA_EXTERNA;
   }
   if ( g_strcmp0( sit, "CTI" ) == 0 ) {
      return TRANSFERENCIA_INTERNA;
   }
   if ( g_strcmp0( sit, "CPG" ) == 0 ) {
      return EVADIDO;
   }

   return SEM_SITUACAO;
}


static void aplicar_sed_nativo( gchar *texto ) {
   gchar *leitura = texto;
   gchar *escrita = texto;
   while ( *leitura ) {
      if ( *leitura == '"' ) {
         leitura++;
         continue;
      }
      if ( *leitura == ',' ) {
         *escrita = ';';
      } else {
         *escrita = *leitura;
      }
      leitura++;
      escrita++;
   }
   *escrita = '\0';
}


static double obter_largura_nome_mm( PangoLayout *layout, const char *texto ) {
   pango_layout_set_text( layout, texto, -1 );

   int largura_pango = 0;
   pango_layout_get_size( layout, &largura_pango, NULL );

   // Converte Pango Units para Milímetros ( Fator exato: 72 / 25.4 = 2.834645 )
   return ( double )largura_pango / ( PANGO_SCALE * 2.834645 );
}


static int obter_limite_corte_nome( PangoLayout *layout, const char *aluno ) {
   int max_chars = ( int )g_utf8_strlen( aluno, -1 );
   int limite_corte = calcular_len_limpo( aluno, max_chars );

   char buffer[64];
   g_snprintf( buffer, sizeof( buffer ), "%.*s", limite_corte, aluno );

   double largura = obter_largura_nome_mm( layout, buffer );

   while ( largura > 69.0 && max_chars > 0 ) {
      max_chars--;
      limite_corte = calcular_len_limpo( aluno, max_chars );
      g_snprintf( buffer, sizeof( buffer ), "%.*s", limite_corte, aluno );
      largura = obter_largura_nome_mm( layout, buffer );
   }

   return limite_corte;
}


static gboolean eh_preposicao( const gchar *str ) {
   g_return_val_if_fail( str != NULL, FALSE );

   return ( g_strcmp0( str, "de" ) == 0 ||
            g_strcmp0( str, "do" ) == 0 ||
            g_strcmp0( str, "da" ) == 0 ||
            g_strcmp0( str, "dos" ) == 0 ||
            g_strcmp0( str, "das" ) == 0 ||
            g_strcmp0( str, "e" ) == 0 );
}


static gchar *converter_nome_proprio( const gchar *nome_completo ) {
   if ( !nome_completo || *nome_completo == '\0' ) {
      return NULL;
   }

   gchar *nome_minusculo = g_utf8_strdown( nome_completo, -1 );
   gchar **palavras = g_strsplit( nome_minusculo, " ", -1 );
   g_free( nome_minusculo );

   for ( int i = 0; palavras[i] != NULL; i++ ) {
      if ( palavras[i][0] == '\0' || eh_preposicao( palavras[i] ) ) {
         continue;
      }

      gssize tamanho_primeiro_char = g_utf8_next_char( palavras[i] ) - palavras[i];
      gchar *primeira_letra = g_utf8_strup( palavras[i], tamanho_primeiro_char );
      const gchar *resto_palavra = g_utf8_next_char( palavras[i] );

      gchar *palavra_capitalizada = g_strconcat( primeira_letra, resto_palavra, NULL );
      g_free( palavras[i] );
      palavras[i] = palavra_capitalizada;
      g_free( primeira_letra );
   }

   gchar *resultado = g_strjoinv( " ", palavras );
   g_strfreev( palavras );

   return resultado;
}


static gchar *padronizar_nome_turma( const gchar *coluna, FILE **f_acesso, const gchar *diretorio ) {
   g_return_val_if_fail( coluna != NULL && f_acesso != NULL && diretorio != NULL, NULL );

   g_autofree gchar *coluna_minusculo = g_utf8_strdown( coluna, -1 );
   int len = ( int )strlen( coluna_minusculo );
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

      g_autofree gchar *dir_acesso = g_build_filename( diretorio, "acessos", NULL );
      g_mkdir_with_parents( dir_acesso, 0755 );

      g_autofree gchar *acesso_turma = g_strdup_printf( "%s/%s.bin", dir_acesso, turma );
      *f_acesso = fopen( acesso_turma, "wb" );
   }

   return turma;
}


//=================================================================================================
typedef struct {
   gchar *dir_destino;
   gchar *dir_turma;
   gchar *dest_acesso;
   gchar *dest_alunos;
} EspelhamentoDiretorios;
//-------------------------------------------------------------------------------------------------
static EspelhamentoDiretorios *criar_estrutura_diretorios( const gchar *diretorio, const char *raiz, const char *turma ) {
   EspelhamentoDiretorios *paths = g_malloc0( sizeof( EspelhamentoDiretorios ) );

   g_autofree gchar *plan = g_build_filename( raiz, "planilhas", NULL );
   if ( g_str_has_prefix( diretorio, plan ) ) {
      const char *subdirs = diretorio + strlen( plan );
      if ( g_str_has_prefix( subdirs, G_DIR_SEPARATOR_S ) ) {
         subdirs++;
      }
      paths->dir_destino = g_build_filename( raiz, "dados", "listas", subdirs, NULL );
   } else {
      paths->dir_destino = g_build_filename( raiz, "dados", "listas", NULL );
   }

   if ( !g_str_has_prefix( paths->dir_destino, G_DIR_SEPARATOR_S ) ) {
      gchar *temp_absoluto = g_strdup_printf( "%s%s", G_DIR_SEPARATOR_S, paths->dir_destino );
      g_free( paths->dir_destino );
      paths->dir_destino = temp_absoluto;
   }

   gchar **partes = g_strsplit( paths->dir_destino, G_DIR_SEPARATOR_S, -1 );
   guint n_partes = g_strv_length( partes );
   if ( n_partes > 0 ) {
      g_free( partes[n_partes - 1] );
      partes[n_partes - 1] = g_strdup( turma );

      for ( guint i = 0; i < n_partes; i++ ) {
         if ( g_strcmp0( partes[i], "listas" ) == 0 ) {
            g_free( partes[i] );
            partes[i] = g_strdup( "informados" );
            break;
         }
      }

      paths->dir_turma = g_strjoinv( G_DIR_SEPARATOR_S, partes );
      g_strfreev( partes );
   }

   paths->dest_acesso = g_build_filename( paths->dir_destino, "acessos", NULL );
   paths->dest_alunos = g_build_filename( paths->dir_destino, "fichas", NULL );

   g_mkdir_with_parents( paths->dest_acesso, 0755 );
   g_mkdir_with_parents( paths->dest_alunos, 0755 );

   return paths;
}
//-------------------------------------------------------------------------------------------------
static void liberar_estrutura_diretorios( EspelhamentoDiretorios *paths ) {
   if ( !paths ) {
      return;
   }
   g_free( paths->dir_destino );
   g_free( paths->dir_turma );
   g_free( paths->dest_acesso );
   g_free( paths->dest_alunos );
   g_free( paths );
}
//-------------------------------------------------------------------------------------------------
static FichaAluno *criar_ficha_zerada( int lim_disc ) {
   gsize tamanho_total = sizeof( FichaAluno ) + ( gsize )lim_disc * sizeof( HistoricoDisciplina );

   FichaAluno *ficha = g_malloc0( tamanho_total );

   for ( int k = 0; k < lim_disc; k++ ) {
      for ( int ii = 0; ii < 4; ii++ ) {
         for ( int jj = 0; jj < 5; jj++ ) {
            ficha->disciplina[k].periodo[ii].avaliacoes[jj].av  = -1.0f;
            ficha->disciplina[k].periodo[ii].avaliacoes[jj].rec = -1.0f;
         }
         ficha->disciplina[k].relatorio.notas_periodos[ii] = -1.0f;
      }
      ficha->disciplina[k].relatorio.soma        = -1.0f;
      ficha->disciplina[k].relatorio.media_anual = -1.0f;
      ficha->disciplina[k].relatorio.rec_final   = -1.0f;
      ficha->disciplina[k].relatorio.conselho    = -1.0f;
   }

   ficha->sit = SEM_SITUACAO;
   ficha->ativo = FALSE;

   return ficha;
}
//--------------------------------------------------------------------------------------------
static void salvar_ficha_aluno_inicial( const char *nome_turma_padrao, int fonte,
                                        const gchar *diretorio, const char *raiz ) {
   g_return_if_fail( nome_turma_padrao != NULL && diretorio != NULL && raiz != NULL );

   EspelhamentoDiretorios *paths = criar_estrutura_diretorios( diretorio, raiz, nome_turma_padrao );
   int lim_disc = quantidade_diretorios_aux( paths->dir_turma );

   g_autofree gchar *nome_bin = g_strdup_printf( "%s.bin", nome_turma_padrao );
   g_autofree gchar *arquivo_acesso_destino = g_build_filename( paths->dest_acesso, nome_bin, NULL );
   g_autofree gchar *arquivo_acesso_origem  = g_build_filename( diretorio, "acessos", nome_bin, NULL );

   FILE *f = g_fopen( arquivo_acesso_destino, "wb" );
   if ( !f ) {
      g_printerr( "[ERRO] Falha ao criar binário de acesso: %s\n", arquivo_acesso_destino );
      liberar_estrutura_diretorios( paths );
      return;
   }

   FILE *f_acesso = g_fopen( arquivo_acesso_origem, "rb" );
   if ( !f_acesso ) {
      g_printerr( "[ERRO] Falha ao abrir acesso de origem: %s\n", arquivo_acesso_origem );
      fclose( f );
      liberar_estrutura_diretorios( paths );
      return;
   }

   const char *fonte_latex = ( fonte == 1 ) ? "CMU Bright 11" : "CMU Serif 11";
   gsize tamanho_gravacao  = sizeof( FichaAluno ) + ( ( gsize )lim_disc * sizeof( HistoricoDisciplina ) );

   #pragma omp critical(motor_pango)
   {
      // Instancia um FontMap próprio [transfer full] em vez do singleton global (_get_default).
      // Ao sair do bloco, g_autoptr destrói layout, context e font_map sem deixar cache órfão.
      g_autoptr( PangoFontMap )         font_map = pango_cairo_font_map_new();
      PangoContext                     *context  = pango_font_map_create_context( font_map );
      g_autoptr( PangoLayout )          layout   = pango_layout_new( context );
      g_object_unref( context );
      g_autoptr( PangoFontDescription ) desc     = pango_font_description_from_string( fonte_latex );

      pango_layout_set_font_description( layout, desc );

      AcessoTurmas turmas = { 0 };
      AcessoFicha  acesso = { 0 };
      int idx = 0;

      while ( fread( &turmas, sizeof( AcessoTurmas ), 1, f_acesso ) == 1 ) {
         acesso.sit       = situacao_aluno( turmas.sit );
         acesso.ativo     = ( acesso.sit & ( MATRICULA_INTERNA | MATRICULA_EXTERNA | MATRICULA_REGULAR ) ) != 0;
         acesso.cod_aluno = turmas.cod_aluno;
         fwrite( &acesso, sizeof( AcessoFicha ), 1, f );

         g_autofree gchar *nome_ficha_bin       = g_strdup_printf( "%" PRIu32 ".bin", acesso.cod_aluno );
         g_autofree gchar *arquivo_aluno_origem = g_build_filename( diretorio, "fichas", nome_ficha_bin, NULL );

         FILE *f_origem = g_fopen( arquivo_aluno_origem, "rb" );
         if ( !f_origem ) {
            g_printerr( "[ERRO] Não foi possível abrir a origem: %s\n", arquivo_aluno_origem );
            continue;
         }

         FichaSiaep siaep = { 0 };
         if ( fread( &siaep, sizeof( FichaSiaep ), 1, f_origem ) == 1 ) {
            g_autofree FichaAluno *ficha = criar_ficha_zerada( lim_disc );
            ficha->idx_siaep = ( uint8_t )idx;

            g_autofree char *nome_formatado = converter_nome_proprio( siaep.aluno );
            const char *nome_seguro = ( nome_formatado != NULL ) ? nome_formatado : siaep.aluno;

            ficha->limite_corte = ( uint8_t )obter_limite_corte_nome( layout, nome_seguro );

            g_strlcpy( ficha->aluno, nome_seguro, sizeof( ficha->aluno ) );
            g_strlcpy( ficha->nasc,  siaep.nasc,  sizeof( ficha->nasc ) );
            g_strlcpy( ficha->sexo, ( siaep.sexo == 'M' ) ? "Masculino" : "Feminino", sizeof( ficha->sexo ) );

            g_autofree gchar *arquivo_aluno_destino = g_build_filename( paths->dest_alunos, nome_ficha_bin, NULL );
            FILE *f_destino = g_fopen( arquivo_aluno_destino, "wb" );

            if ( f_destino ) {
               fwrite( ficha, tamanho_gravacao, 1, f_destino );
               fclose( f_destino );
            }
         }

         fclose( f_origem );
         idx++;
      }
   }

   fclose( f_acesso );
   fclose( f );
   liberar_estrutura_diretorios( paths );
}
//=====================================================================================================


// ============================================================================
// 1. ESTRUTURA E NORMALIZAÇÃO DE CABEÇALHOS DINÂMICOS
// ============================================================================
typedef struct {
   int idx_codigo;
   int idx_nome;
   int idx_sexo;
   int idx_nascimento;
   int idx_situacao;
   gboolean mapeado;
} MapaSiaep;

static void inicializar_mapa_siaep( MapaSiaep *mapa ) {
   mapa->idx_codigo = -1;
   mapa->idx_nome = -1;
   mapa->idx_sexo = -1;
   mapa->idx_nascimento = -1;
   mapa->idx_situacao = -1;
   mapa->mapeado = FALSE;
}

static void tentar_mapear_cabecalho( gchar **colunas, guint num_colunas, MapaSiaep *mapa ) {
   for ( guint i = 0; i < num_colunas; i++ ) {
      if ( !colunas[i] || colunas[i][0] == '\0' ) {
         continue;
      }

      g_autofree gchar *col_up = g_utf8_strup( colunas[i], -1 );

      if ( strstr( col_up, "COD" ) && strstr( col_up, "ALUNO" ) ) {
         mapa->idx_codigo = ( int )i;
      } else if ( strstr( col_up, "NOME" ) ) {
         mapa->idx_nome = ( int )i;
      } else if ( strstr( col_up, "SEXO" ) ) {
         mapa->idx_sexo = ( int )i;
      } else if ( strstr( col_up, "NASCIMENTO" ) || strstr( col_up, "NASC" ) ) {
         mapa->idx_nascimento = ( int )i;
      } else if ( strstr( col_up, "SITUA" ) ) {
         mapa->idx_situacao = ( int )i;
      }
   }

   if ( mapa->idx_codigo != -1 && mapa->idx_nome != -1 && mapa->idx_sexo != -1 &&
         mapa->idx_nascimento != -1 && mapa->idx_situacao != -1 ) {
      mapa->mapeado = TRUE;
   }
}


// ============================================================================
// 2. PROCESSAMENTO UNITÁRIO DO ALUNO
// ============================================================================
static gboolean processar_linha_aluno( gchar **colunas, guint num_colunas, const MapaSiaep *mapa,
                                       FILE *saida_dat, FILE *f_acesso, const gchar *dir_alunos ) {

   int max_idx = MAX( mapa->idx_situacao, MAX( mapa->idx_nascimento, MAX( mapa->idx_sexo, MAX( mapa->idx_codigo, mapa->idx_nome ) ) ) );
   if ( ( int )num_colunas <= max_idx ) {
      return FALSE;
   }

   const char *nome = colunas[mapa->idx_nome];
   if ( !nome || strlen( nome ) < 2 ) {
      return FALSE;
   }

   const char *codigo = colunas[mapa->idx_codigo];
   const char *sexo   = colunas[mapa->idx_sexo];
   const char *nasc   = colunas[mapa->idx_nascimento];
   const char *sit    = colunas[mapa->idx_situacao];

   if ( saida_dat ) {
      fprintf( saida_dat, "%s;%s;%s;%s;%s\n", codigo, nome, sexo, nasc, sit );
   }

   FichaSiaep siaep = {0};
   g_strlcpy( siaep.aluno, nome, sizeof( siaep.aluno ) );
   siaep.sexo = sexo[0];
   g_strlcpy( siaep.nasc, nasc, sizeof( siaep.nasc ) );

   g_autofree gchar *arquivo_bin = g_strdup_printf( "%s/%s.bin", dir_alunos, codigo );

   #pragma omp critical(escreve_aluno)
   {
      FILE *p = fopen( arquivo_bin, "wb" );
      if ( p ) {
         fwrite( &siaep, sizeof( FichaSiaep ), 1, p );
         fclose( p );
      }
   }

   if ( f_acesso ) {
      AcessoTurmas turmas = {0};
      turmas.cod_aluno = atou32_seguro( codigo );
      g_strlcpy( turmas.sit, sit, sizeof( turmas.sit ) );
      fwrite( &turmas, sizeof( AcessoTurmas ), 1, f_acesso );
   }

   return TRUE;
}


// ============================================================================
// 3. MAESTRO ORQUESTRADOR
// ============================================================================
static gboolean siaep_processar_arquivo( const gchar *arquivo_xls, int fonte, const char *raiz ) {
   g_return_val_if_fail( arquivo_xls != NULL, FALSE );

   g_autofree gchar *diretorio = g_path_get_dirname( arquivo_xls );

   g_autofree gchar *arquivo_csv = g_strdup( arquivo_xls );
   gchar *ext_csv = strrchr( arquivo_csv, '.' );
   if ( ext_csv && strlen( ext_csv ) >= 4 ) {
      memcpy( ext_csv, ".csv", 5 );
   }

   g_autofree gchar *arquivo_dat = g_strdup( arquivo_xls );
   gchar *ext_dat = strrchr( arquivo_dat, '.' );
   if ( ext_dat && strlen( ext_dat ) >= 4 ) {
      memcpy( ext_dat, ".dat", 5 );
   }

   // Extração Física (xls -> csv via ssconvert) - 100% paralela
   g_autofree gchar *comando = g_strdup_printf( "ssconvert \"%s\" \"%s\"", arquivo_xls, arquivo_csv );
   if ( !g_spawn_command_line_sync( comando, NULL, NULL, NULL, NULL ) ) {
      return FALSE;
   }

   g_autofree gchar *conteudo_csv = NULL;
   if ( !g_file_get_contents( arquivo_csv, &conteudo_csv, NULL, NULL ) ) {
      return FALSE;
   }

   aplicar_sed_nativo( conteudo_csv );

   g_autofree gchar *dir_acesso = g_build_filename( diretorio, "acessos", NULL );
   g_autofree gchar *dir_alunos = g_build_filename( diretorio, "fichas", NULL );
   g_mkdir_with_parents( dir_acesso, 0755 );
   g_mkdir_with_parents( dir_alunos, 0755 );

   FILE *saida_dat = fopen( arquivo_dat, "w" );
   FILE *f_acesso = NULL;
   g_autofree gchar *nome_turma_padrao = NULL;

   MapaSiaep mapa;
   inicializar_mapa_siaep( &mapa );

   g_auto( GStrv ) linhas = g_strsplit( conteudo_csv, "\n", -1 );

   for ( guint i = 0; linhas[i] != NULL; i++ ) {
      g_strstrip( linhas[i] );
      if ( linhas[i][0] == '\0' ) {
         continue;
      }

      g_auto( GStrv ) colunas = g_strsplit( linhas[i], ";", -1 );
      guint num_colunas = g_strv_length( colunas );

      if ( g_ascii_strncasecmp( colunas[0], "TURMA: ", 7 ) == 0 && num_colunas > 0 ) {
         g_clear_pointer( &nome_turma_padrao, g_free );
         if ( f_acesso ) {
            fclose( f_acesso );
            f_acesso = NULL;
         }
         nome_turma_padrao = padronizar_nome_turma( colunas[0], &f_acesso, diretorio );
         continue;
      }

      if ( !mapa.mapeado ) {
         tentar_mapear_cabecalho( colunas, num_colunas, &mapa );
         if ( mapa.mapeado && saida_dat ) {
            fprintf( saida_dat, "Cod. Aluno;NOME_ALUNO;Sexo;Nascimento;Situação\n" );
         }
         continue;
      }

      if ( mapa.mapeado ) {
         if ( strstr( colunas[0], "Legenda" ) || ( num_colunas > 1 && strstr( colunas[1], "Total" ) ) ) {
            break;
         }
         processar_linha_aluno( colunas, num_colunas, &mapa, saida_dat, f_acesso, dir_alunos );
      }
   }

   if ( saida_dat ) {
      fclose( saida_dat );
   }
   if ( f_acesso ) {
      fclose( f_acesso );
   }
   g_remove( arquivo_csv );

   if ( nome_turma_padrao ) {
      salvar_ficha_aluno_inicial( nome_turma_padrao, fonte, diretorio, raiz );

      g_autofree gchar *novo_xls = g_strdup_printf( "%s/%s.xls", diretorio, nome_turma_padrao );
      g_autofree gchar *novo_dat = g_strdup_printf( "%s/%s.dat", diretorio, nome_turma_padrao );

      g_rename( arquivo_xls, novo_xls );
      g_rename( arquivo_dat, novo_dat );
   }

   return TRUE;
}


static void atualizar_acesso_e_adicionar_novos_alunos( const AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );
   const InterfaceCabecalho *cabecalho = &ctx->cabecalho;
   const InterfaceDados     *dados     = &ctx->dados;

   int limite_turmas = ctx->cabecalho.limite_turmas;

   for ( int i = 0; i < limite_turmas; i++ ) {
      g_autofree char *acesso_bin = g_strdup_printf( "%s.bin", cabecalho->turmas[i] );
      g_autofree char *acesso_bin_orig = g_build_filename( dados->raiz, "dados", "listas", dados->ano, dados->escola, dados->periodo,
                                         "acessos", acesso_bin, NULL );

      g_autofree char *dir_turma_base = g_build_filename( dados->raiz, "dados", "informados", dados->ano, dados->escola,
                                        cabecalho->turmas[i], NULL );

      g_autoptr( GError ) erro = NULL;
      g_autoptr( GDir ) dir_turma = g_dir_open( dir_turma_base, 0, &erro );

      if ( !dir_turma ) {
         g_printerr( "[Aviso] Nenhuma disciplina encontrada para a turma %s: %s\n", cabecalho->turmas[i], erro->message );
         continue;
      }

      const gchar *nome_disciplina;
      while ( ( nome_disciplina = g_dir_read_name( dir_turma ) ) != NULL ) {
         if ( nome_disciplina[0] == '.' ) {
            continue;
         }

         g_autofree char *caminho_disciplina = g_build_filename( dir_turma_base, nome_disciplina, NULL );

         if ( !g_file_test( caminho_disciplina, G_FILE_TEST_IS_DIR ) || diretorio_esta_vazio( caminho_disciplina ) ) {
            continue;
         }

         g_autofree char *dir_periodo_dest = g_build_filename( caminho_disciplina, dados->periodo, NULL );
         g_autofree char *acesso_bin_dest  = g_build_filename( dir_periodo_dest, "acesso.bin", NULL );

         gio_copiar_arquivo( acesso_bin_orig, acesso_bin_dest );
      }
   }

   g_autofree char *dir_alunos_orig = g_build_filename( dados->raiz, "dados", "listas", dados->ano,
                                      dados->escola, dados->periodo, "fichas", NULL );

   g_autofree char *dir_alunos_dest = g_build_filename( dados->raiz, "dados", "informados", dados->ano,
                                      dados->escola, "fichas", NULL );

   g_mkdir_with_parents( dir_alunos_dest, 0755 );

   g_autoptr( GError ) erro = NULL;
   g_autoptr( GDir ) dir = g_dir_open( dir_alunos_orig, 0, &erro );

   if ( !dir ) {
      g_printerr( "Erro ao abrir diretório de alunos origem: %s\n", erro->message );
      return;
   }

   const gchar *nome_arquivo;
   while ( ( nome_arquivo = g_dir_read_name( dir ) ) != NULL ) {
      if ( g_str_has_suffix( nome_arquivo, ".bin" ) ) {
         g_autofree char *arquivo_dest = g_build_filename( dir_alunos_dest, nome_arquivo, NULL );

         if ( !g_file_test( arquivo_dest, G_FILE_TEST_EXISTS ) ) {
            g_autofree char *arquivo_orig = g_build_filename( dir_alunos_orig, nome_arquivo, NULL );
            gio_copiar_arquivo( arquivo_orig, arquivo_dest );
         }
      }
   }
}


gboolean siaep_atualizar_alunos( InterfacePainel *painel, const AppContext *ctx ) {
   g_return_val_if_fail( painel != NULL && ctx != NULL, FALSE );
   if ( ctx->cabecalho.foco_periodo == RECUPERACAO_FINAL ) {
      return FALSE;
   }

   const InterfaceDados *dados = &ctx->dados;

   g_autofree gchar *dir_origem = g_build_filename( dados->raiz, "planilhas", dados->ano, dados->escola, dados->periodo, NULL );
   g_mkdir_with_parents( dir_origem, 0755 );

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
      return FALSE;
   }

   int num_arquivos = ( int )lista_arquivos->len;

   if ( num_arquivos == 0 ) {
      g_print( "Nenhum arquivo .xls encontrado no diretório: %s\n", dir_origem );
      return FALSE;
   }

   g_print( "Iniciando processamento paralelo de %d arquivos...\n", num_arquivos );

   #pragma omp parallel for schedule( dynamic, 1 )
   for ( int i = 0; i < num_arquivos; i++ ) {
      const gchar *arquivo_atual = ( const gchar * )g_ptr_array_index( lista_arquivos, i );
      int th = omp_get_thread_num();
      g_print( "[Thread %d] Processando: %s\n", th, arquivo_atual );

      siaep_processar_arquivo( arquivo_atual, dados->fonte_latex, dados->raiz );
   }

   atualizar_acesso_e_adicionar_novos_alunos( ctx );

   return TRUE;
}
