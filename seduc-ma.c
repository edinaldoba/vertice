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


static int _quantidade_diretorios( const char *diretorio ) {
   g_return_val_if_fail( diretorio != NULL, 0 );

   GError *error = NULL;
   GDir *dir = g_dir_open( diretorio, 0, &error );

   if ( dir == NULL ) {
      // Falhou ao abrir (ex: pasta não existe). Limpamos o erro silenciosamente
      // para evitar memory leaks e retornamos 0, mantendo seu contrato original.
      g_clear_error( &error );
      return 0;
   }

   int cont = 0;
   const gchar *nome_item;

   // 1. Varre o diretório (g_dir_read_name já ignora "." e "..")
   while ( ( nome_item = g_dir_read_name( dir ) ) != NULL ) {

      // 2. Substitui o buffer fixo arriscado por alocação dinâmica e segura de caminhos
      g_autofree gchar *caminho_completo = g_build_filename( diretorio, nome_item, NULL );

      // 3. Interroga o S.O. de forma unificada, substituindo a struct stat e S_ISDIR
      if ( g_file_test( caminho_completo, G_FILE_TEST_IS_DIR ) ) {
         cont++;
      }
   }

   // 4. Fecha o descritor de diretório de forma limpa
   g_dir_close( dir );

   return cont;
}



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

   if ( g_strcmp0( sit, "MTR" ) == 0 ) return MATRICULA_REGULAR;
   if ( g_strcmp0( sit, "MTE" ) == 0 ) return MATRICULA_EXTERNA;
   if ( g_strcmp0( sit, "MTI" ) == 0 ) return MATRICULA_INTERNA;
   if ( g_strcmp0( sit, "CTE" ) == 0 ) return TRANSFERENCIA_EXTERNA;
   if ( g_strcmp0( sit, "CTI" ) == 0 ) return TRANSFERENCIA_INTERNA;
   if ( g_strcmp0( sit, "CPG" ) == 0 ) return EVADIDO;

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
   int max_chars = (int)g_utf8_strlen( aluno, -1 );
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
   int len = (int)strlen( coluna_minusculo );
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


//=================================================================================================
typedef struct {
   gchar *dir_destino;
   gchar *dir_turma;
   gchar *dest_acesso;
   gchar *dest_alunos;
} EspelhamentoDiretorios;
//-------------------------------------------------------------------------------------------------
static EspelhamentoDiretorios* criar_estrutura_diretorios(const gchar *diretorio, const char *raiz, const char *turma) {
   EspelhamentoDiretorios *paths = g_malloc0(sizeof(EspelhamentoDiretorios));

   // 1. Caminho 'listas'
   g_autofree gchar *plan = g_build_filename(raiz, "planilhas", NULL);
   if (g_str_has_prefix(diretorio, plan)) {
      const char *subdirs = diretorio + strlen(plan);
      if (g_str_has_prefix(subdirs, G_DIR_SEPARATOR_S)) subdirs++;
      paths->dir_destino = g_build_filename(raiz, "dados", "listas", subdirs, NULL);
   } else {
      paths->dir_destino = g_build_filename(raiz, "dados", "listas", NULL);
   }

   // 🛡️ GARANTIA ABSOLUTA: Se o destino não começou com '/', forçamos a barra inicial.
   // Isso blinda a função caso a 'raiz' tenha vindo sem a barra do sistema operacional.
   if (!g_str_has_prefix(paths->dir_destino, G_DIR_SEPARATOR_S)) {
      gchar *temp_absoluto = g_strdup_printf("%s%s", G_DIR_SEPARATOR_S, paths->dir_destino);
      g_free(paths->dir_destino);
      paths->dir_destino = temp_absoluto;
   }

   // 2. Caminho 'informados'
   gchar **partes = g_strsplit(paths->dir_destino, G_DIR_SEPARATOR_S, -1);
   guint n_partes = g_strv_length(partes);
   if (n_partes > 0) {
      g_free(partes[n_partes - 1]);
      partes[n_partes - 1] = g_strdup(turma);

      for (guint i = 0; i < n_partes; i++) {
         if (g_strcmp0(partes[i], "listas") == 0) {
            g_free(partes[i]);
            partes[i] = g_strdup("informados");
            break;
         }
      }

      // 🎯 CORREÇÃO CRÍTICA: g_strjoinv preserva a string vazia [0] gerada pela
      // explosão do caminho absoluto, restaurando a '/' inicial perfeitamente.
      paths->dir_turma = g_strjoinv(G_DIR_SEPARATOR_S, partes);
      g_strfreev(partes);
   }

   // 3. Subpastas alvo
   paths->dest_acesso = g_build_filename(paths->dir_destino, "acessos", NULL);
   paths->dest_alunos = g_build_filename(paths->dir_destino, "fichas", NULL);

   g_mkdir_with_parents(paths->dest_acesso, 0755);
   g_mkdir_with_parents(paths->dest_alunos, 0755);

   return paths;
}
//-------------------------------------------------------------------------------------------------
static void liberar_estrutura_diretorios(EspelhamentoDiretorios *paths) {
   if (!paths) return;
   g_free(paths->dir_destino);
   g_free(paths->dir_turma);
   g_free(paths->dest_acesso);
   g_free(paths->dest_alunos);
   g_free(paths);
}
//-------------------------------------------------------------------------------------------------
static FichaAluno* criar_ficha_zerada(int lim_disc) {
   gsize tamanho_total = sizeof(FichaAluno) + (gsize)lim_disc * sizeof(HistoricoDisciplina);

   // g_malloc0 garante que toda a base e o array de disciplinas nasçam zerados
   FichaAluno *ficha = g_malloc0(tamanho_total);

   for (int k = 0; k < lim_disc; k++) {
      for (int ii = 0; ii < 4; ii++) {
         for (int jj = 0; jj < 5; jj++) {
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
static void salvar_ficha_aluno_inicial( const char *nome_turma_padrao, int fonte, const gchar *diretorio, char *raiz ) {
   g_return_if_fail( nome_turma_padrao && diretorio && raiz );

   // 1. Configura a rede de caminhos
   EspelhamentoDiretorios *paths = criar_estrutura_diretorios(diretorio, raiz, nome_turma_padrao);

   int lim_disc = _quantidade_diretorios(paths->dir_turma);

   // 2. Abertura do Arquivo Central de Acessos
   g_autofree gchar *arquivo_acesso_destino = g_strdup_printf("%s/%s.bin", paths->dest_acesso, nome_turma_padrao);
   g_autofree gchar *arquivo_acesso_origem  = g_strdup_printf("%s/acessos/%s.bin", diretorio, nome_turma_padrao);

   FILE *f = fopen(arquivo_acesso_destino, "wb");
   if (!f) {
      g_printerr("Falha ao criar binário de acesso: %s\n", arquivo_acesso_destino);
      liberar_estrutura_diretorios(paths);
      return;
   }

   FILE *f_acesso = fopen(arquivo_acesso_origem, "rb");
   if (!f_acesso) {
      g_printerr("Falha ao abrir acesso de origem: %s\n", arquivo_acesso_origem);
      fclose(f);
      liberar_estrutura_diretorios(paths);
      return;
   }

   // 3. Inicialização do Motor Tipográfico (Pango)
   PangoFontMap *font_map = pango_cairo_font_map_get_default();
   PangoContext *context = pango_font_map_create_context(font_map);
   PangoLayout *layout = pango_layout_new(context);

   const char *fonte_latex = (fonte == 1) ? "CMU Bright 11" : "CMU Serif 11";
   PangoFontDescription *desc = pango_font_description_from_string(fonte_latex);
   pango_layout_set_font_description(layout, desc);
   pango_font_description_free(desc);

   // 4. Loop de Transmutação de Arquivos (SIAEP Cru -> Vértice Dinâmico)
   AcessoTurmas turmas = {0};
   AcessoFicha acesso = {0};
   int idx = 0;

   gsize tamanho_gravacao = sizeof(FichaAluno) + (gsize)lim_disc * sizeof(HistoricoDisciplina);

   while (fread(&turmas, sizeof(AcessoTurmas), 1, f_acesso) == 1) {

      acesso.sit = situacao_aluno(turmas.sit);
      acesso.ativo = (acesso.sit & (MATRICULA_INTERNA | MATRICULA_EXTERNA | MATRICULA_REGULAR)) != 0;
      acesso.cod_aluno = turmas.cod_aluno;
      fwrite(&acesso, sizeof(AcessoFicha), 1, f);

      // g_autofree gchar *arquivo_aluno_origem = g_strdup_printf("%s/alunos/%" PRIu32 ".bin", diretorio, acesso.cod_aluno);
      // Altere esta linha dentro do loop while:
      g_autofree gchar *arquivo_aluno_origem = g_strdup_printf("%s/fichas/%" PRIu32 ".bin", diretorio, acesso.cod_aluno);
      FILE *f_origem = fopen(arquivo_aluno_origem, "rb" );

      if ( !f_origem ) {
         // ⚠️ SE CAIR AQUI, ELE PASSA PARA O PRÓXIMO ALUNO EM SILÊNCIO
         g_printerr( "[ERRO] Não foi possível abrir a origem: %s\n", arquivo_aluno_origem );
         continue;
      }

      FichaSiaep siaep;
      if (fread(&siaep, sizeof(FichaSiaep), 1, f_origem) == 1) {

         // Delega a criação segura (e do tamanho correto) para a função auxiliar
         FichaAluno *ficha = criar_ficha_zerada(lim_disc);
         ficha->idx_siaep = (guint8)idx;

         g_autofree char *nome_formatado = converter_nome_proprio(siaep.aluno);
         ficha->limite_corte = (guint8)obter_limite_corte_nome(layout, nome_formatado);

         snprintf(ficha->aluno, sizeof(ficha->aluno), "%s", nome_formatado);
         snprintf(ficha->nasc, sizeof(ficha->nasc), "%s", siaep.nasc);
         snprintf(ficha->sexo, sizeof(ficha->sexo), "%s", (siaep.sexo == 'M') ? "Masculino" : "Feminino");

         g_autofree gchar *arquivo_aluno_destino = g_strdup_printf("%s/%" PRIu32 ".bin", paths->dest_alunos, acesso.cod_aluno);
         FILE *f_destino = fopen(arquivo_aluno_destino, "wb");

         if (f_destino) {
            // GRAVAÇÃO BLINDADA: Escreve a base + FAM no disco
            fwrite(ficha, tamanho_gravacao, 1, f_destino);
            fclose(f_destino);
         }

         g_free(ficha); // Previne memory leak após gravação de cada aluno
      }

      fclose(f_origem);
      idx++;
   }

   // 5. Encerramento e Limpeza
   fclose(f_acesso);
   fclose(f);

   liberar_estrutura_diretorios(paths);
   g_object_unref(layout);
   g_object_unref(context);
   g_object_unref(font_map);
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
      if ( !colunas[i] || colunas[i][0] == '\0' ) continue;

      // Converte para uppercase apenas para a validação (Case-Insensitive)
      g_autofree gchar *col_up = g_utf8_strup( colunas[i], -1 );

      if ( strstr( col_up, "COD" ) && strstr( col_up, "ALUNO" ) ) mapa->idx_codigo = (guint8)i;
      else if ( strstr( col_up, "NOME" ) ) mapa->idx_nome = (guint8)i;
      else if ( strstr( col_up, "SEXO" ) ) mapa->idx_sexo = (guint8)i;
      else if ( strstr( col_up, "NASCIMENTO" ) || strstr( col_up, "NASC" ) ) mapa->idx_nascimento = (guint8)i;
      else if ( strstr( col_up, "SITUA" ) ) mapa->idx_situacao = (guint8)i;
   }

   // Só declara sucesso quando as 5 âncoras vitais forem rastreadas
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

   // Defesa 1: Garante que a linha fatiada seja maior que o maior índice do nosso mapa
   int max_idx = MAX( mapa->idx_situacao, MAX( mapa->idx_nascimento, MAX( mapa->idx_sexo, MAX( mapa->idx_codigo, mapa->idx_nome ) ) ) );
   if ( (int)num_colunas <= max_idx ) return FALSE;

   const char *nome = colunas[mapa->idx_nome];
   if ( !nome || strlen( nome ) < 2 ) return FALSE; // Linha vazia ou lixo de fim de tabela

   const char *codigo = colunas[mapa->idx_codigo];
   const char *sexo   = colunas[mapa->idx_sexo];
   const char *nasc   = colunas[mapa->idx_nascimento];
   const char *sit    = colunas[mapa->idx_situacao];

   // 1. Grava no log histórico (.dat)
   if ( saida_dat ) {
      fprintf( saida_dat, "%s;%s;%s;%s;%s\n", codigo, nome, sexo, nasc, sit );
   }

   // 2. Transmuta para a FichaSiaep em binário cru
   FichaSiaep siaep = {0};
   snprintf( siaep.aluno, sizeof( siaep.aluno ), "%s", nome );
   siaep.sexo = sexo[0];
   snprintf( siaep.nasc, sizeof( siaep.nasc ), "%s", nasc );

   g_autofree gchar *arquivo_bin = g_strdup_printf( "%s/%s.bin", dir_alunos, codigo );

   #pragma omp critical(escreve_aluno)
   {
      FILE *p = fopen( arquivo_bin, "wb" );
      if ( p ) {
         fwrite( &siaep, sizeof( FichaSiaep ), 1, p );
         fclose( p );
      }
   }

   // 3. Alimenta o índice binário da turma (.bin de acesso)
   if ( f_acesso ) {
      AcessoTurmas turmas = {0};
      turmas.cod_aluno = atou32_seguro( codigo );
      snprintf( turmas.sit, sizeof( turmas.sit ), "%s", sit );
      fwrite( &turmas, sizeof( AcessoTurmas ), 1, f_acesso );
   }

   return TRUE;
}

// ============================================================================
// 3. MAESTRO ORQUESTRADOR
// ============================================================================
static gboolean siaep_processar_arquivo( const gchar *arquivo_xls, int fonte, char *raiz ) {
   g_return_val_if_fail( arquivo_xls, FALSE );

   // 1. Abstração de Nomes e Caminhos
   g_autofree gchar *diretorio = g_path_get_dirname( arquivo_xls );

   g_autofree gchar *arquivo_csv = g_strdup( arquivo_xls );
   gchar *ext_csv = strrchr( arquivo_csv, '.' );
   if ( ext_csv ) strcpy( ext_csv, ".csv" );

   g_autofree gchar *arquivo_dat = g_strdup( arquivo_xls );
   gchar *ext_dat = strrchr( arquivo_dat, '.' );
   if ( ext_dat ) strcpy( ext_dat, ".dat" );

   // 2. Extração Física (xls -> csv via ssconvert)
   g_autofree gchar *comando = g_strdup_printf( "ssconvert \"%s\" \"%s\"", arquivo_xls, arquivo_csv );
   if ( !g_spawn_command_line_sync( comando, NULL, NULL, NULL, NULL ) ) return FALSE;

   g_autofree gchar *conteudo_csv = NULL;
   if ( !g_file_get_contents( arquivo_csv, &conteudo_csv, NULL, NULL ) ) return FALSE;

   aplicar_sed_nativo( conteudo_csv );

   // 3. Preparação das Pastas Transitórias
   g_autofree gchar *dir_acesso = g_build_filename( diretorio, "acessos", NULL );
   g_autofree gchar *dir_alunos = g_build_filename( diretorio, "fichas", NULL );
   g_mkdir_with_parents( dir_acesso, 0755 );
   g_mkdir_with_parents( dir_alunos, 0755 );

   // 4. Configuração do Motor de Parsing
   FILE *saida_dat = fopen( arquivo_dat, "w" );
   FILE *f_acesso = NULL;
   g_autofree gchar *nome_turma_padrao = NULL;

   MapaSiaep mapa;
   inicializar_mapa_siaep( &mapa );

   g_auto( GStrv ) linhas = g_strsplit( conteudo_csv, "\n", -1 );

   // 5. Laço de Triagem de Linhas (O Pipeline)
   for ( guint i = 0; linhas[i] != NULL; i++ ) {
      g_strstrip( linhas[i] );
      if ( linhas[i][0] == '\0' ) continue;

      g_auto( GStrv ) colunas = g_strsplit( linhas[i], ";", -1 );
      guint num_colunas = g_strv_length( colunas );

      // Estágio A: Identificou a tag de Turma
      if ( g_ascii_strncasecmp( colunas[0], "TURMA: ", 7 ) == 0 && num_colunas > 0 ) {
         g_clear_pointer( &nome_turma_padrao, g_free );
         if ( f_acesso ) { fclose( f_acesso ); f_acesso = NULL; }
         nome_turma_padrao = padronizar_nome_turma( colunas[0], &f_acesso, diretorio );
         continue;
      }

      // Estágio B: Busca e Ativação do Mapa de Cabeçalho
      if ( !mapa.mapeado ) {
         tentar_mapear_cabecalho( colunas, num_colunas, &mapa );
         if ( mapa.mapeado && saida_dat ) {
            fprintf( saida_dat, "Cod. Aluno;NOME_ALUNO;Sexo;Nascimento;Situação\n" );
         }
         continue;
      }

      // Estágio C: Processamento contínuo de Alunos
      if ( mapa.mapeado ) {
         // Rodapé do SIAEP ("Legenda" ou "Total") indica o fim dos dados da turma
         if ( strstr( colunas[0], "Legenda" ) || ( num_colunas > 1 && strstr( colunas[1], "Total" ) ) ) {
            break;
         }
         processar_linha_aluno( colunas, num_colunas, &mapa, saida_dat, f_acesso, dir_alunos );
      }
   }

   // 6. Limpeza Operacional de Arquivos
   if ( saida_dat ) fclose( saida_dat );
   if ( f_acesso ) fclose( f_acesso );
   g_remove( arquivo_csv );

   // 7. Salto Final de Espelhamento (SIAEP -> Vértice)
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
   g_return_if_fail( ctx );
   const InterfaceCabecalho *cabecalho = &ctx->cabecalho;
   const InterfaceDados   *dados   = &ctx->dados;

   int limite_turmas = ctx->cabecalho.limite_turmas;

   // 1. Atualização dos arquivos de acesso
   for ( int i = 0; i < limite_turmas; i++ ) {
      g_autofree char *acesso_bin = g_strdup_printf( "%s.bin", cabecalho->turmas[i] );
      g_autofree char *acesso_bin_orig = g_build_filename( dados->raiz, "dados", "listas", dados->ano, dados->escola, dados->periodo,
                                         "acessos", acesso_bin, NULL );

      // Monta o diretório base da turma, onde as disciplinas devem estar alocadas
      g_autofree char *dir_turma_base = g_build_filename( dados->raiz, "dados", "informados", dados->ano, dados->escola,
                                        cabecalho->turmas[i], NULL );

      g_autoptr( GError ) erro = NULL;
      g_autoptr( GDir ) dir_turma = g_dir_open( dir_turma_base, 0, &erro );

      if ( !dir_turma ) {
         g_printerr( "[Aviso] Nenhuma disciplina encontrada para a turma %s: %s\n", cabecalho->turmas[i], erro->message );
         continue; // Se a pasta da turma ainda não existir, pula para a próxima
      }

      const gchar *nome_disciplina;
      // Lê o conteúdo dentro da pasta da turma (que serão as pastas das disciplinas)
      while ( ( nome_disciplina = g_dir_read_name( dir_turma ) ) != NULL ) {

         g_autofree char *caminho_disciplina = g_build_filename( dir_turma_base, nome_disciplina, NULL );

         if ( diretorio_esta_vazio( caminho_disciplina ) ) continue;

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
   g_autofree char *dir_alunos_orig = g_build_filename( dados->raiz, "dados", "listas", dados->ano,
                                                        dados->escola, dados->periodo, "fichas", NULL );

   g_autofree char *dir_alunos_dest = g_build_filename( dados->raiz, "dados", "informados", dados->ano,
                                                        dados->escola, "fichas", NULL );

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

gboolean siaep_atualizar_alunos( InterfacePainel *painel, const AppContext *ctx ) {
   g_return_val_if_fail( painel && ctx, FALSE );
   if ( ctx->cabecalho.foco_periodo == RECUPERACAO_FINAL ) return FALSE;

   const InterfaceDados *dados   = &ctx->dados;

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

   int num_arquivos = (int)lista_arquivos->len;

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

      g_autofree char *raiz = g_strdup( ctx->dados.raiz );
      siaep_processar_arquivo( arquivo_atual, dados->fonte_latex, raiz );
   }

   atualizar_acesso_e_adicionar_novos_alunos( ctx );

   return TRUE;
}




