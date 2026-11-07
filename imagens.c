/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glib/gstdio.h> // Necessário para g_remove e g_mkdir_with_parents
#include <omp.h>

#include "comum.h"
#include "imagens.h"
#include "imgcore.h"
#include "basicas.h"   // Para nFile e Files
#include "omr.h" // Para mudar_numero_na_imagem, gabaritos e imagens_corrigidas
#include "interface.h"
#include "mensagens.h"
#include "glib_gio.h"
#include "latex.h"
#include "pds.h"
#include "gas.h"
#include "assincrono.h"



// =========================================================================
// FUNÇÕES UTILITÁRIAS REFATORADAS
// =========================================================================
// Retorna uma nova string alocada dinamicamente, liberada depois com g_autofree
static char* trocar_extensao( const char *caminho_orig, const char *nova_ext ) {
   if ( !caminho_orig || !nova_ext ) return NULL;

   const char *ponto = g_strrstr( caminho_orig, "." );
   if ( ponto ) {
      return g_strdup_printf( "%.*s.%s", ( int )( ponto - caminho_orig ), caminho_orig, nova_ext );
   }
   return g_strdup_printf( "%s.%s", caminho_orig, nova_ext );
}

// Em vez de preencher um buffer estático, retorna a string na Heap.
static char* gerar_nome_aleatorio( size_t tam ) {
   const char alfanumerico[] = "0123456789abcdefghijklmnopqrstuvwxyz";
   char *nome = g_malloc( tam + 1 );

   for ( size_t j = 0; j < tam; j++ ) {
      nome[j] = alfanumerico[g_random_int_range( 0, 36 )];
   }
   nome[tam] = '\0';

   return nome;
}

// Wrapper OBRIGATÓRIO para ordernar ou buscar strings em GPtrArray / bsearch
static gint cmp_str_ptr( gconstpointer a, gconstpointer b ) {
   return g_strcmp0( *( const char** )a, *( const char** )b );
}

// =========================================================================
// ROTINAS DE PROCESSAMENTO
// =========================================================================

// Novo protótipo: Retorna um GPtrArray contendo os nomes finais das imagens.
static GPtrArray* converter_e_copiar_imagens( const char *origem, const char *destino ) {
   if ( !origem || !destino ) return NULL;

   g_autoptr( GError ) error = NULL;
   g_autoptr( GDir ) dir = g_dir_open( origem, 0, &error );
   if ( dir == NULL ) {
      g_printerr( "Aviso: Não foi possível abrir a pasta de origem: %s (%s)\n", origem, error->message );
      return NULL;
   }

   // 1. FASE DE COLETA
   g_autoptr( GPtrArray ) arquivos_validos = g_ptr_array_new_with_free_func( g_free );
   const char *filename;

   while ( ( filename = g_dir_read_name( dir ) ) != NULL ) {
      g_autofree char *filename_lc = g_utf8_strdown( filename, -1 );

      if ( g_str_has_suffix( filename_lc, ".png" )  ||
            g_str_has_suffix( filename_lc, ".jpg" )  ||
            g_str_has_suffix( filename_lc, ".jpeg" ) ||
            g_str_has_suffix( filename_lc, ".bmp" ) ) {
         g_ptr_array_add( arquivos_validos, g_strdup( filename ) );
      }
   }

   if ( arquivos_validos->len == 0 ) {
      return NULL;
   }

   // 2. FASE DE PREPARAÇÃO PARALELA
   // Criamos o array de retorno e pré-alocamos o tamanho exato.
   // Isso previne reallocs e permite acesso thread-safe direto pelo índice (pdata[i]).
   GPtrArray *arquivos_gerados = g_ptr_array_new_with_free_func( g_free );
   g_ptr_array_set_size( arquivos_gerados, arquivos_validos->len );

   // 3. FASE DE PROCESSAMENTO PARALELO (Dynamic mantido para carga balanceada)
   #pragma omp parallel for schedule(dynamic)
   for ( guint i = 0; i < arquivos_validos->len; i++ ) {
      const char *file_atual = ( const char * ) g_ptr_array_index( arquivos_validos, i );

      g_autofree char *nome_img = gerar_nome_aleatorio( 20 );
      const char *ext = g_strrstr( file_atual, "." );

      // Cria a nova string do arquivo destino final e salva diretamente no slot pré-alocado
      char *nome_final = g_strdup_printf( "%s%s", nome_img, ( ext != NULL ) ? ext : "" );
      arquivos_gerados->pdata[i] = nome_final;

      g_autofree char *path_origem = g_build_filename( origem, file_atual, NULL );
      g_autofree char *path_destino = g_build_filename( destino, nome_final, NULL );

      if ( gio_copiar_arquivo( path_origem, path_destino ) ) {
         g_remove( path_origem );
      } else {
         g_printerr( "Falha no processamento da imagem: %s\n", file_atual );
      }
   }

   // O vetor 'arquivos_validos' será destruído automaticamente pelo g_autoptr ao sair do escopo
   return arquivos_gerados;
}
//========================================================================================================//







static void normalizar_ancora( const ImagemColorida *img_rgb, const ImagemCinza *img_gray, IndiceMatriz *ancora ) {
   // Validação de segurança padrão GLib
   g_return_if_fail( img_rgb && img_gray && ancora );
   g_return_if_fail( img_gray->ncol > 0 && img_gray->nrow > 0 ); // Proteção vital!

   // 1. Fatores de escala diretos (Largura e Altura)
   double scale_x = ( double )img_rgb->ncol / ( double )img_gray->ncol;
   double scale_y = ( double )img_rgb->nrow / ( double )img_gray->nrow;

   // 2. Aplicação da escala em cada uma das 4 âncoras
   for ( int ii = 0; ii < 4; ii++ ) {
      double x_scaled = ancora[ii].j * scale_x;
      double y_scaled = ancora[ii].i * scale_y;

      // Atualiza a âncora convertendo de volta para inteiro com arredondamento seguro
      ancora[ii].j = ( int )round( x_scaled );
      ancora[ii].i = ( int )round( y_scaled );
   }
}






//========================================================================================================//
/**
 * Função principal de processamento em lote.
 * Executa a visão computacional multithread, cortes e leitura do payload/respostas.
 */
// =========================================================================
// FUNÇÃO: gas_processar_imagens (Visão Computacional Evolutiva - GAS)
// =========================================================================
int gas_processar_imagens( const InterfaceDados *dados, const InterfaceCabecalho *cabecalho ) {
   if ( !dados || !cabecalho ) return -1;

   const char *home = g_get_home_dir();
   if ( home == NULL ) home = ".";

   g_autofree char *origem = g_build_filename( home, "Downloads", "imagens", NULL );
   g_autofree char *destino = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "imagens", NULL );
   g_autofree char *respostas = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "respostas", NULL );
   g_autofree char *dir_rejeitadas = g_build_filename( destino, "rejeitadas", NULL );

   if ( g_mkdir_with_parents( destino, 0755 ) != 0 ||
         g_mkdir_with_parents( dir_rejeitadas, 0755 ) != 0 ||
         g_mkdir_with_parents( respostas, 0755 ) != 0 ) {
      g_printerr( "Erro crítico: Não foi possível criar os diretórios de destino.\n" );
      return -1;
   }

   g_autofree char *gabaritos = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "gabaritos", NULL );
   int qtd_bin = quantidade_arquivos_por_extensao( gabaritos, ".bin" );

   if ( qtd_bin == 0 ) {
      g_printerr( "[AVISO] Nenhuma prova foi gerada até o momento.\n" );
      return -1;
   }

   // --- Transição para GLib GPtrArray ---
   g_autoptr( GPtrArray ) imgs_orig = converter_e_copiar_imagens( origem, destino );

   if ( !imgs_orig || imgs_orig->len == 0 ) {
      g_printerr( "[AVISO] Nenhuma imagem de respostas foi encontrada em %s.\n", origem );
      return -2;
   }

   g_autoptr( GPtrArray ) resp_bin = carregar_arquivos_por_extensao( gabaritos, ".bin", qtd_bin );
   g_ptr_array_sort( resp_bin, cmp_str_ptr ); // Usando nosso wrapper seguro

   g_autofree FILE **f = g_new0( FILE *, resp_bin->len );
   for ( guint i = 0; i < resp_bin->len; i++ ) {
      const char *nome_bin = ( const char * ) g_ptr_array_index( resp_bin, i );
      g_autofree char *arquivo = g_build_filename( respostas, nome_bin, NULL );
      f[i] = fopen( arquivo, "ab" );
   }

   int n_rejeitadas = 0;

   // =========================================================================
   // PROCESSAMENTO PARALELO DAS IMAGENS (OpenMP)
   // =========================================================================
   #pragma omp parallel for schedule(dynamic) reduction(+:n_rejeitadas)
   for ( guint i = 0; i < imgs_orig->len; i++ ) {

      gboolean sucesso = FALSE;
      int tentativas = 0;

      MapeamentoGabarito map = {0};
      IndiceMatriz ancora[4] = {0};

      ImagemColorida img_rgb_orig = {0};
      ImagemColorida img_rgb_crop = {0};
      ImagemCinza img_gray_bin    = {0};
      ImagemCinza img_gray_crop   = {0};
      ImagemCinza img_gray_alloc  = {0};

      const char *img_str = ( const char * ) g_ptr_array_index( imgs_orig, i );

      g_autofree char *path_orig = g_build_filename( destino, img_str, NULL );
      g_autofree char *img_png = trocar_extensao( img_str, "png" );
      g_autofree char *path_png = g_build_filename( destino, img_png, NULL );

      carregar_imagem_colorida_nativa( path_orig, &img_rgb_orig );
      g_remove( path_orig );
      rgb2gray( &img_rgb_orig, &img_gray_bin );

      int dim = 960;
      redimensionar_imagem_bilinear( &img_gray_bin, &img_gray_alloc, dim );

      // FASE 3: Visão Computacional Evolutiva (Dupla Passada)
      do {
         if ( tentativas > 0 && img_gray_crop.image != NULL ) {
            liberar_matriz_pixels( img_gray_crop.image, img_gray_crop.nrow );
            img_gray_crop.image = NULL;
         }

         gas_mapear_ancoras( &img_gray_alloc, &map, ancora, tentativas );
         transformada_homografica( &img_gray_alloc, &img_gray_crop, ancora, map.direcao );
         binarizar_pgm_metodo_otsu( &img_gray_crop );
         map.payload = extrair_payload_matriz( &img_gray_crop, map.direcao );

         sucesso = decodificar_payload_matriz( &map, cabecalho );
         tentativas++;

      } while ( !sucesso && tentativas < 2 );

      if ( !sucesso ) {
         fprintf( stderr, "[FALHA CRÍTICA] Payload inválido mesmo após resgate. Imagem: %s\n", img_str );
      }

      // FASE 4: Processamento de Dados (Gravação Binária)
      if ( sucesso ) {
         g_autofree char *chave = g_malloc0( 256 );
         nome_base_gabaritos_bin( chave, 256, map.turma, map.disc, map.per, map.seq );

         char **found = bsearch( &chave, resp_bin->pdata, resp_bin->len, sizeof( char* ), cmp_str_ptr );

         if ( found && f[found - ( char** )resp_bin->pdata] != NULL ) {
            int j = found - ( char** )resp_bin->pdata;
            ler_respostas_gabarito( &img_gray_crop, map.direcao, map.resp );
            map.num = ler_numero_aluno( &img_gray_crop, map.direcao );
            g_strlcpy( map.nome_img, img_png, sizeof( map.nome_img ) );

            #pragma omp critical(escrita_binario)
            {
               if ( fwrite( &map, sizeof( MapeamentoGabarito ), 1, f[j] ) != 1 ) {
                  g_printerr( "[ERRO] O registro da imagem %s não foi salvo.\n", img_str );
               }
               // O fflush foi removido por otimização thread-safe. fclose faz isso.
            }
         } else {
            g_printerr( "[ALERTA] Binário '%s' não encontrado para: %s\n", chave, img_str );
            sucesso = FALSE;
         }
      }

      // FASE 5: Renderização do Crop Colorido
      if ( sucesso ) {
         normalizar_ancora( &img_rgb_orig, &img_gray_alloc, ancora );
         transformada_homografica_colorida( &img_rgb_orig, &img_rgb_crop, ancora, map.direcao );

         ImagemColorida img_rgb_limpa = {0};
         filtrar_fundo_magico_colorido( &img_rgb_crop, &img_rgb_limpa, 15 );

         salvar_imagem_png_nativa( path_png, &img_rgb_limpa );

         liberar_matriz_pixels_colorida( img_rgb_limpa.image, img_rgb_limpa.nrow );

      } else {
         g_autofree char *path_erro = g_build_filename( dir_rejeitadas, img_png, NULL );
         salvar_imagem_png_nativa( path_erro, &img_rgb_orig );
         n_rejeitadas++;
      }

      // FASE 6: Limpeza Segura de Memória
      if ( img_gray_crop.image )  liberar_matriz_pixels( img_gray_crop.image, img_gray_crop.nrow );
      if ( img_gray_bin.image )   liberar_matriz_pixels( img_gray_bin.image, img_gray_bin.nrow );
      if ( img_gray_alloc.image ) liberar_matriz_pixels( img_gray_alloc.image, img_gray_alloc.nrow );
      if ( img_rgb_crop.image )   liberar_matriz_pixels_colorida( img_rgb_crop.image, img_rgb_crop.nrow );
      if ( img_rgb_orig.image )   liberar_matriz_pixels_colorida( img_rgb_orig.image, img_rgb_orig.nrow );
   }

   // =========================================================================
   // FINALIZAÇÃO GLOBAL
   // =========================================================================
   for ( guint i = 0; i < resp_bin->len; i++ ) {
      if ( f[i] != NULL ) {
         fclose( f[i] );
         const char *nome_bin = ( const char * ) g_ptr_array_index( resp_bin, i );
         g_autofree char *arquivo = g_build_filename( respostas, nome_bin, NULL );

         if ( verificar_arquivo( arquivo ) == ARQUIVO_VAZIO ) {
            if ( g_remove( arquivo ) != 0 ) {
               g_printerr( "[ERRO] Falha ao tentar remover o arquivo vazio: %s\n", arquivo );
            }
         }
      }
   }

   puts( "Processamento das imagens GAS concluído com sucesso!" );
   return n_rejeitadas;
}


// =========================================================================
// FUNÇÃO: omr_processar_imagens (Pipeline Determinístico)
// =========================================================================
int omr_processar_imagens( const InterfaceDados *dados, const InterfaceCabecalho *cabecalho ) {
   if ( !dados || !cabecalho ) return -1;

   const char *home = g_get_home_dir();
   if ( home == NULL ) home = ".";

   g_autofree char *origem = g_build_filename( home, "Downloads", "imagens", NULL );
   g_autofree char *destino = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "imagens", NULL );
   g_autofree char *respostas = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "respostas", NULL );
   g_autofree char *dir_rejeitadas = g_build_filename( destino, "rejeitadas", NULL );

   if ( g_mkdir_with_parents( destino, 0755 ) != 0 ||
         g_mkdir_with_parents( dir_rejeitadas, 0755 ) != 0 ||
         g_mkdir_with_parents( respostas, 0755 ) != 0 ) {
      g_printerr( "Erro crítico: Não foi possível criar os diretórios de destino.\n" );
      return -1;
   }

   g_autofree char *gabaritos = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "gabaritos", NULL );
   int qtd_bin = quantidade_arquivos_por_extensao( gabaritos, ".bin" );

   if ( qtd_bin <= 0 ) {
      g_printerr( "[AVISO] Nenhuma prova foi gerada até o momento.\n" );
      return -1;
   }

   // --- Transição para GLib GPtrArray ---
   g_autoptr( GPtrArray ) imgs_orig = converter_e_copiar_imagens( origem, destino );

   if ( !imgs_orig || imgs_orig->len == 0 ) {
      g_printerr( "[AVISO] O sistema não encontrou fotografias ou digitalizações de respostas na pasta %s.\n", origem );
      return -2;
   }

   g_autoptr( GPtrArray ) resp_bin = carregar_arquivos_por_extensao( gabaritos, ".bin", qtd_bin );
   g_ptr_array_sort( resp_bin, cmp_str_ptr );

   g_autofree FILE **f = g_new0( FILE *, resp_bin->len );
   for ( guint i = 0; i < resp_bin->len; i++ ) {
      const char *nome_bin = ( const char * ) g_ptr_array_index( resp_bin, i );
      g_autofree char *arquivo = g_build_filename( respostas, nome_bin, NULL );
      f[i] = fopen( arquivo, "ab" );
   }

   int n_rejeitadas = 0;

   // =========================================================================
   // PROCESSAMENTO PARALELO DAS IMAGENS (OpenMP)
   // =========================================================================
   #pragma omp parallel for schedule(dynamic) reduction(+:n_rejeitadas)
   for ( guint i = 0; i < imgs_orig->len; i++ ) {

      gboolean sucesso = FALSE;
      MapeamentoGabarito map = {0};
      IndiceMatriz ancora[4] = {0};

      ImagemColorida img_rgb_orig = {0};
      ImagemColorida img_rgb_crop = {0};
      ImagemCinza img_gray_bin    = {0};
      ImagemCinza img_gray_alloc  = {0};
      ImagemCinza img_gray_fundo  = {0};
      ImagemCinza img_gray_blur   = {0};
      ImagemCinza img_gray_crop   = {0};

      const char *img_str = ( const char * ) g_ptr_array_index( imgs_orig, i );

      g_autofree char *path_orig = g_build_filename( destino, img_str, NULL );
      g_autofree char *img_png = trocar_extensao( img_str, "png" );
      g_autofree char *path_png = g_build_filename( destino, img_png, NULL );

      carregar_imagem_colorida_nativa( path_orig, &img_rgb_orig );
      g_remove( path_orig );
      rgb2gray( &img_rgb_orig, &img_gray_bin );

      int dim = 960;
      redimensionar_imagem_bilinear( &img_gray_bin, &img_gray_alloc, dim );

      // FASE 3: PIPELINE DETERMINÍSTICO OMR
      filtrar_fundo_magico_cinza( &img_gray_alloc, &img_gray_fundo, 30 );
      aplicar_filtro_gaussiano_2d( &img_gray_fundo, &img_gray_blur, 1.0f );
      binarizar_pgm_metodo_otsu( &img_gray_blur );
      sucesso = detectar_ancoras_omr( &img_gray_blur, ancora, &map.direcao, FALSE );

      if ( sucesso ) {
         transformada_homografica( &img_gray_blur, &img_gray_crop, ancora, map.direcao );
         map.payload = extrair_payload_matriz( &img_gray_crop, map.direcao );
         sucesso = decodificar_payload_matriz( &map, cabecalho );
      }

      // RESGATE
      if ( !sucesso ) {
         if ( img_gray_crop.image ) {
            liberar_matriz_pixels( img_gray_crop.image, img_gray_crop.nrow );
            img_gray_crop.image = NULL;
            img_gray_crop.nrow = 0;
            img_gray_crop.ncol = 0;
         }

         sucesso = detectar_ancoras_omr( &img_gray_blur, ancora, &map.direcao, TRUE );
         if ( sucesso ) {
            transformada_homografica( &img_gray_blur, &img_gray_crop, ancora, map.direcao );
            map.payload = extrair_payload_matriz( &img_gray_crop, map.direcao );
            sucesso = decodificar_payload_matriz( &map, cabecalho );
         }
      }

      if ( !sucesso ) {
         fprintf( stderr, "[FALHA CRÍTICA] OMR rejeitou a imagem ou Payload inválido: %s\n", img_str );
      }

      // FASE 4: Processamento de Dados (Gravação Binária)
      if ( sucesso ) {
         g_autofree char *chave = g_malloc0( 256 );
         nome_base_gabaritos_bin( chave, 256, map.turma, map.disc, map.per, map.seq );

         char **found = bsearch( &chave, resp_bin->pdata, resp_bin->len, sizeof( char* ), cmp_str_ptr );

         if ( found && f[found - ( char** )resp_bin->pdata] != NULL ) {
            int j = found - ( char** )resp_bin->pdata;
            ler_respostas_gabarito( &img_gray_crop, map.direcao, map.resp );
            map.num = ler_numero_aluno( &img_gray_crop, map.direcao );
            g_strlcpy( map.nome_img, img_png, sizeof( map.nome_img ) );

            #pragma omp critical(escrita_binario)
            {
               if ( fwrite( &map, sizeof( MapeamentoGabarito ), 1, f[j] ) != 1 ) {
                  g_printerr( "[ERRO] O registro da imagem %s não foi salvo.\n", img_str );
               }
            }
         } else {
            g_printerr( "[ALERTA] Binário '%s' não encontrado para: %s\n", chave, img_str );
            sucesso = FALSE;
         }
      }

      // FASE 5: Renderização do Crop Colorido
      if ( sucesso ) {
         normalizar_ancora( &img_rgb_orig, &img_gray_alloc, ancora );
         transformada_homografica_colorida( &img_rgb_orig, &img_rgb_crop, ancora, map.direcao );

         ImagemColorida img_rgb_limpa = {0};
         filtrar_fundo_magico_colorido( &img_rgb_crop, &img_rgb_limpa, 30 );
         realcar_cores_in_place( &img_rgb_limpa, 1.3f, 1.6f );

         salvar_imagem_png_nativa( path_png, &img_rgb_limpa );
         liberar_matriz_pixels_colorida( img_rgb_limpa.image, img_rgb_limpa.nrow );
      } else {
         g_autofree char *path_erro = g_build_filename( dir_rejeitadas, img_png, NULL );
         salvar_imagem_png_nativa( path_erro, &img_rgb_orig );
         n_rejeitadas++;
      }

      // FASE 6: Limpeza Segura de Memória
      if ( img_gray_fundo.image )  liberar_matriz_pixels( img_gray_fundo.image, img_gray_fundo.nrow );
      if ( img_gray_blur.image )   liberar_matriz_pixels( img_gray_blur.image, img_gray_blur.nrow );
      if ( img_gray_crop.image )   liberar_matriz_pixels( img_gray_crop.image, img_gray_crop.nrow );
      if ( img_gray_bin.image )    liberar_matriz_pixels( img_gray_bin.image, img_gray_bin.nrow );
      if ( img_gray_alloc.image )  liberar_matriz_pixels( img_gray_alloc.image, img_gray_alloc.nrow );
      if ( img_rgb_crop.image )    liberar_matriz_pixels_colorida( img_rgb_crop.image, img_rgb_crop.nrow );
      if ( img_rgb_orig.image )    liberar_matriz_pixels_colorida( img_rgb_orig.image, img_rgb_orig.nrow );
   }

   // =========================================================================
   // FINALIZAÇÃO GLOBAL
   // =========================================================================
   for ( guint i = 0; i < resp_bin->len; i++ ) {
      if ( f[i] != NULL ) {
         fclose( f[i] );
         const char *nome_bin = ( const char * ) g_ptr_array_index( resp_bin, i );
         g_autofree char *arquivo = g_build_filename( respostas, nome_bin, NULL );

         if ( verificar_arquivo( arquivo ) == ARQUIVO_VAZIO ) {
            if ( g_remove( arquivo ) != 0 ) {
               g_printerr( "[ERRO] Falha ao tentar remover o arquivo vazio: %s\n", arquivo );
            }
         }
      }
   }

   puts( "Processamento das imagens OMR (Determinístico) concluído com sucesso!" );
   return n_rejeitadas;
}





int cs_omr_processar_imagens( const InterfaceDados *dados, const InterfaceCabecalho *cabecalho ) {
   if ( !dados || !cabecalho ) return -1;

   const char *home = g_get_home_dir();
   if ( home == NULL ) home = ".";

   g_autofree char *origem         = g_build_filename( home, "Downloads", "imagens", NULL );
   g_autofree char *destino        = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "imagens", NULL );
   g_autofree char *respostas      = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "respostas", NULL );
   g_autofree char *dir_rejeitadas = g_build_filename( destino, "rejeitadas", NULL );

   if ( g_mkdir_with_parents( destino, 0755 ) != 0 ||
         g_mkdir_with_parents( dir_rejeitadas, 0755 ) != 0 ||
         g_mkdir_with_parents( respostas, 0755 ) != 0 ) {
      g_printerr( "Erro crítico: Não foi possível criar os diretórios de destino.\n" );
      return -1;
   }

   g_autofree char *gabaritos = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "gabaritos", NULL );
   int qtd_bin = quantidade_arquivos_por_extensao( gabaritos, ".bin" );

   if ( qtd_bin <= 0 ) {
      g_printerr( "[AVISO] Nenhuma prova foi gerada até o momento.\n" );
      return -1;
   }

   // === MUDANÇA: O array de strings final é trazido diretamente, sem ItemTextoCurto ===
   g_autoptr( GPtrArray ) imgs_orig = converter_e_copiar_imagens( origem, destino );

   if ( !imgs_orig || imgs_orig->len == 0 ) {
      g_printerr( "[AVISO] O sistema não encontrou fotografias ou digitalizações de respostas na pasta %s.\n", origem );
      return -2;
   }

   g_autoptr( GPtrArray ) resp_bin = carregar_arquivos_por_extensao( gabaritos, ".bin", qtd_bin );
   g_ptr_array_sort( resp_bin, cmp_str_ptr );

   // Vetor dinâmico para os arquivos binários (gerenciado por autofree)
   g_autofree FILE **f = g_new0( FILE *, resp_bin->len );
   for ( guint i = 0; i < resp_bin->len; i++ ) {
      const char *nome_bin = ( const char * ) g_ptr_array_index( resp_bin, i );
      g_autofree char *arquivo = g_build_filename( respostas, nome_bin, NULL );
      f[i] = fopen( arquivo, "ab" );
   }

   int n_rejeitadas = 0;

   // =========================================================================
   // PROCESSAMENTO PARALELO (OpenMP)
   // =========================================================================
   #pragma omp parallel for schedule(dynamic) reduction(+:n_rejeitadas)
   for ( guint i = 0; i < imgs_orig->len; i++ ) {

      gboolean sucesso = FALSE;
      MapeamentoGabarito map = {0}; // Necessário por conta da gravação binária
      IndiceMatriz ancora[4] = {0};

      ImagemColorida img_rgb_orig = {0};
      ImagemColorida img_rgb_crop = {0};
      ImagemCinza img_gray_bin    = {0};
      ImagemCinza img_gray_alloc  = {0};
      ImagemCinza img_gray_crop   = {0};

      // Resgatamos o nome do GPtrArray (já é thread-safe via leitura iterada em guint i)
      const char *img_str = ( const char * ) g_ptr_array_index( imgs_orig, i );

      g_autofree char *path_orig = g_build_filename( destino, img_str, NULL );
      g_autofree char *img_png   = trocar_extensao( img_str, "png" );
      g_autofree char *path_png  = g_build_filename( destino, img_png, NULL );

      carregar_imagem_colorida_nativa( path_orig, &img_rgb_orig );
      g_remove( path_orig );
      rgb2gray( &img_rgb_orig, &img_gray_bin );

      int dim = 960;
      redimensionar_imagem_bilinear( &img_gray_bin, &img_gray_alloc, dim );

      binarizar_pgm_metodo_otsu( &img_gray_alloc );
      sucesso = detectar_ancoras_omr( &img_gray_alloc, ancora, &map.direcao, FALSE );

      if ( sucesso ) {
         transformada_homografica( &img_gray_alloc, &img_gray_crop, ancora, map.direcao );
         map.payload = extrair_payload_matriz( &img_gray_crop, map.direcao );
         sucesso = decodificar_payload_matriz( &map, cabecalho );
      }

      if ( !sucesso ) {
         if ( img_gray_crop.image ) {
            liberar_matriz_pixels( img_gray_crop.image, img_gray_crop.nrow );
            img_gray_crop.image = NULL;
            img_gray_crop.nrow = 0;
            img_gray_crop.ncol = 0;
         }
         sucesso = detectar_ancoras_omr( &img_gray_alloc, ancora, &map.direcao, TRUE );
         if ( sucesso ) {
            transformada_homografica( &img_gray_alloc, &img_gray_crop, ancora, map.direcao );
            map.payload = extrair_payload_matriz( &img_gray_crop, map.direcao );
            sucesso = decodificar_payload_matriz( &map, cabecalho );
         }
      }

      if ( !sucesso ) {
         g_printerr( "[FALHA CRÍTICA] OMR rejeitou a imagem ou Payload inválido: %s\n", img_str );
      }

      if ( sucesso ) {
         // Eliminamos o ItemTextoCurto. Se nome_base_gabaritos_bin ainda requer preencher
         // um ponteiro fornecido pelo usuário, fornecemos memória da heap usando g_malloc0.
         g_autofree char *chave = g_malloc0( 256 );
         nome_base_gabaritos_bin( chave, 256, map.turma, map.disc, map.per, map.seq );

         // Busca rápida via bsearch usando nosso novo wrapper
         char **found = bsearch( &chave, resp_bin->pdata, resp_bin->len, sizeof( char* ), cmp_str_ptr );

         if ( found && f[found - ( char** )resp_bin->pdata] != NULL ) {
            int j = found - ( char** )resp_bin->pdata;
            ler_respostas_gabarito( &img_gray_crop, map.direcao, map.resp );
            map.num = ler_numero_aluno( &img_gray_crop, map.direcao );

            // Salvando dentro da struct persistente
            g_strlcpy( map.nome_img, img_png, sizeof( map.nome_img ) );

            #pragma omp critical(escrita_binario)
            {
               if ( fwrite( &map, sizeof( MapeamentoGabarito ), 1, f[j] ) != 1 ) {
                  g_printerr( "[ERRO] O registro da imagem %s não foi salvo.\n", img_str );
               }
            }
         } else {
            g_printerr( "[ALERTA] Binário '%s' não encontrado para: %s\n", chave, img_str );
            sucesso = FALSE;
         }
      }

      if ( sucesso ) {
         normalizar_ancora( &img_rgb_orig, &img_gray_alloc, ancora );
         transformada_homografica_colorida( &img_rgb_orig, &img_rgb_crop, ancora, map.direcao );
         salvar_imagem_png_nativa( path_png, &img_rgb_crop );
      } else {
         g_autofree char *path_erro = g_build_filename( dir_rejeitadas, img_png, NULL );
         salvar_imagem_png_nativa( path_erro, &img_rgb_orig );
         n_rejeitadas++;
      }

      // Limpezas locais que não usam GLib
      if ( img_gray_crop.image )  liberar_matriz_pixels( img_gray_crop.image, img_gray_crop.nrow );
      if ( img_gray_bin.image )   liberar_matriz_pixels( img_gray_bin.image, img_gray_bin.nrow );
      if ( img_gray_alloc.image ) liberar_matriz_pixels( img_gray_alloc.image, img_gray_alloc.nrow );
      if ( img_rgb_crop.image )   liberar_matriz_pixels_colorida( img_rgb_crop.image, img_rgb_crop.nrow );
      if ( img_rgb_orig.image )   liberar_matriz_pixels_colorida( img_rgb_orig.image, img_rgb_orig.nrow );
   }

   // =========================================================================
   // FINALIZAÇÃO GLOBAL
   // =========================================================================
   for ( guint i = 0; i < resp_bin->len; i++ ) {
      if ( f[i] != NULL ) {
         fclose( f[i] );
         const char *nome_bin = ( const char * ) g_ptr_array_index( resp_bin, i );
         g_autofree char *arquivo = g_build_filename( respostas, nome_bin, NULL );

         if ( verificar_arquivo( arquivo ) == ARQUIVO_VAZIO ) {
            if ( g_remove( arquivo ) != 0 ) {
               g_printerr( "[ERRO] Falha ao tentar remover o arquivo vazio: %s\n", arquivo );
            }
         }
      }
   }

   g_print( "Processamento das imagens OMR concluído com sucesso!\n" );
   return n_rejeitadas;
}







//------------------------------------------------------------------------------------------------------
static StatusMapeamento validar_prova_escaneada( const MapeamentoGabarito *map, const AppContext *ctx ) {

   const InterfaceDiario *ui_diario = &ctx->ui_diario;
   const InterfaceDados *dados = &ctx->dados;

   // 1º TESTE (Crítico): O número da folha protege o array 'diario'
   if ( map->num <= 0 || map->num > dados->qtd_alunos_total ) {
      return ERRO_NUMERO_ALUNO_INVALIDO;
   }

   // 2º TESTE (Crítico): O ID protege o array 'G' (Gabaritos)
   if ( map->id >= dados->qtd_alunos_ativos ) {
      return ERRO_ID_GABARITO_INVALIDO;
   }

   FichaAluno *ficha = &g_array_index( ui_diario->fichas, FichaAluno, map->num - 1 );

   // 3º TESTE (Regra de Negócio): Se chegou aqui, a memória está segura!
   if ( ficha->ativo == FALSE ) {
      return STATUS_PROVA_OK | AVISO_ALUNO_INATIVO; // Acumula os estados perfeitamente
   }

   return STATUS_PROVA_OK;
}
//------------------------------------------------------------------------------------------------------
static gint comparar_por_cod_aluno( gconstpointer a, gconstpointer b ) {
   const MapeamentoGabarito *m1 = ( const MapeamentoGabarito * )a;
   const MapeamentoGabarito *m2 = ( const MapeamentoGabarito * )b;

   if ( m1->cod_aluno < m2->cod_aluno ) return -1;
   if ( m1->cod_aluno > m2->cod_aluno ) return 1;
   return 0;
}
//------------------------------------------------------------------------------------------------------
void corrigir_prova( InterfacePainel *painel, AppContext *ctx ) {
   if ( !painel || !ctx ) return;

   const InterfaceDiario *ui_diario = &ctx->ui_diario;
   const InterfaceDados   *dados   = &ctx->dados;
   const InterfaceCabecalho  *cabecalho    = &ctx->cabecalho;
   // const CaminhoDiretorio *caminho = &ctx->caminho;

   char nome_bin[64];
   nome_base_gabaritos_bin( nome_bin, sizeof( nome_bin ), cabecalho->foco_turma, cabecalho->foco_disciplina, cabecalho->foco_periodo, dados->seq_prova );

   g_autofree char *path_resp = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola,
                                "respostas", nome_bin, NULL );

   g_autofree char *path_gab  = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola,
                                "gabaritos", nome_bin, NULL );

   // ====================================================================================
   // 1. VALIDAÇÃO DE SEGURANÇA
   // ====================================================================================
   FILE *fr = g_fopen( path_resp, "rb" );
   if ( !fr ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Respostas Não Encontradas" );
      painel->format_subtitulo = meu_gerador_variadico( "O arquivo de escaneamento %s não existe.", nome_bin );
      painel->format_instrucao = meu_gerador_variadico( "Realize o processamento das folhas da turma antes de corrigir." );
      criar_mensagem_painel( AVISO, painel );
      return;
   }

   FILE *fg = g_fopen( path_gab, "rb" );
   if ( !fg ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Gabarito Mestre Ausente" );
      painel->format_subtitulo = meu_gerador_variadico( "Nenhum gabarito foi gerado para esta turma ainda." );
      painel->format_instrucao = meu_gerador_variadico( "Acesse a aba de geração de provas e crie o arquivo de gabaritos." );
      criar_mensagem_painel( ERRO, painel );
      fclose( fr );
      return;
   }

   // ====================================================================================
   // 2. LEITURA BINÁRIA PARA GARRAY E ORDENAÇÃO
   // ====================================================================================
   int qtd_linhas = contar_registros_binarios( path_resp, sizeof( MapeamentoGabarito ) );
   if ( qtd_linhas <= 0 ) {
      g_printerr( "Aviso: Arquivo de respostas vazio ou corrompido.\n" );
      painel->format_titulo    = meu_gerador_variadico( "⚠ Gabarito Mestre Ausente" );
      painel->format_subtitulo = meu_gerador_variadico( "Nenhum gabarito foi gerado para esta turma ainda." );
      painel->format_instrucao = meu_gerador_variadico( "Acesse a aba de geração de provas e crie o arquivo de gabaritos." );
      criar_mensagem_painel( ERRO, painel );
      fclose( fr );
      fclose( fg );
      return;
   }

   // ⚠️ ATENÇÃO: Sem g_autoptr! A memória do GArray agora é transferida para a callback assíncrona.
   GArray *map_array = g_array_sized_new( FALSE, FALSE, sizeof( MapeamentoGabarito ), qtd_linhas );
   g_array_set_size( map_array, qtd_linhas );

   size_t lidos_resp = fread( map_array->data, sizeof( MapeamentoGabarito ), qtd_linhas, fr );
   fclose( fr );

   if ( lidos_resp != ( size_t )qtd_linhas ) {
      g_array_set_size( map_array, lidos_resp );
   }

   g_autofree ItemTextoCurto *G = g_new0( ItemTextoCurto, dados->qtd_alunos_ativos );
   size_t lidos_gab = fread( G, sizeof( ItemTextoCurto ), dados->qtd_alunos_ativos, fg );
   fclose( fg );

   if ( lidos_gab != ( size_t )dados->qtd_alunos_ativos ) {
      g_printerr( "Aviso de I/O: Tamanhos lidos do gabarito não batem com ativos.\n" );
   }

   // ====================================================================================
   // 3. SANITIZAÇÃO E CORRESPONDÊNCIA ABSOLUTA DO CÓDIGO DO ALUNO
   // ====================================================================================
   for ( guint i = 0; i < map_array->len; i++ ) {
      MapeamentoGabarito *map = &g_array_index( map_array, MapeamentoGabarito, i );
      int idx_esperado = map->num - 1;

      if ( ui_diario->fichas && idx_esperado >= 0 && ( guint )idx_esperado < ui_diario->fichas->len ) {
         FichaAluno *ficha = &g_array_index( ui_diario->fichas, FichaAluno, idx_esperado );

         if ( map->cod_aluno == 0 || map->cod_aluno == ficha->cod_aluno ) {
            map->cod_aluno = ficha->cod_aluno;

         } else {
            for ( guint j = 0; j < ui_diario->fichas->len; j++ ) {
               FichaAluno *f_busca = &g_array_index( ui_diario->fichas, FichaAluno, j );
               if ( f_busca->cod_aluno == map->cod_aluno ) {
                  map->num = j + 1;
                  break;
               }
            }
         }
      }
   }

   // ====================================================================================
   // DEDUPLICAÇÃO BLINDADA VIA COD_ALUNO
   // ====================================================================================
   g_array_sort( map_array, comparar_por_cod_aluno );

   if ( map_array->len > 1 ) {
      for ( int i = map_array->len - 1; i > 0; i-- ) {
         MapeamentoGabarito *atual = &g_array_index( map_array, MapeamentoGabarito, i );
         MapeamentoGabarito *anterior = &g_array_index( map_array, MapeamentoGabarito, i - 1 );

         if ( atual->cod_aluno > 0 && atual->cod_aluno == anterior->cod_aluno ) {
            MapeamentoGabarito *map = &g_array_index( map_array, MapeamentoGabarito, i - 1 );

            g_autofree char *imagem = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola,
                                      "imagens", map->nome_img, NULL );
            g_remove( imagem );
            g_array_remove_index( map_array, i - 1 );
         }
      }
   }

   g_array_sort( map_array, comparar_mapeamento_gabarito );

   // ====================================================================================
   // 4. LAÇO PARALELO BLINDADO COM INJEÇÃO DIRETA NA MEMÓRIA
   // ====================================================================================
   int total_provas = map_array->len;

   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < total_provas; i++ ) {

      MapeamentoGabarito *map = &g_array_index( map_array, MapeamentoGabarito, i );
      StatusMapeamento status = validar_prova_escaneada( map, ctx );
      map->status = status;

      if ( status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {

         g_autofree gchar *nome_base = g_strdup_printf( "%.2d", map->num );
         const char *gab = G[ map->id ].str;

         int nota = imagens_corrigidas( gab, map, ctx, nome_base );
         map->nota = nota;

         int idx_aluno = map->num - 1;

         if ( ui_diario->fichas && idx_aluno >= 0 && ( guint )idx_aluno < ui_diario->fichas->len ) {
            FichaAluno *ficha = &g_array_index( ui_diario->fichas, FichaAluno, idx_aluno );
            int periodo = map->per;
            int seq_prova = map->seq;

            if ( periodo >= 0 && periodo <= 4 ) {
               ficha->disciplina[cabecalho->foco_disciplina].periodo[periodo].avaliacoes[2 * seq_prova - 2].av  = ( float )nota;
               ficha->ficha_modificada = TRUE;
            }
         }

      } else {
         int thread_id = omp_get_thread_num();

         if ( status & ERRO_NUMERO_ALUNO_INVALIDO ) {
            g_printerr( "Alerta [Thread %d]: Número da prova (%d) corrompido ou fora dos limites.\n",
                        thread_id, map->num );
         } else if ( status & ERRO_ID_GABARITO_INVALIDO ) {
            g_printerr( "Alerta [Thread %d]: O ID lido (%d) na prova Nº %d não possui gabarito.\n",
                        thread_id, map->id, map->num );
         }
      }
   }

   // ====================================================================================
   // 5. ATUALIZAÇÃO DO ARQUIVO BINÁRIO E COMPILAÇÃO (LaTeX Assíncrono)
   // ====================================================================================
   GError *erro = NULL;
   if ( !g_file_set_contents( path_resp, map_array->data, map_array->len * sizeof( MapeamentoGabarito ), &erro ) ) {
      g_printerr( "Erro crítico ao atualizar o arquivo de respostas: %s\n", erro->message );
      g_clear_error( &erro );
   }

   // -------------------------------------------------------------------------
   // BLOQUEIO DA INTERFACE: (Descomente a linha abaixo e insira o widget do seu botão)
   gtk_widget_set_sensitive( ctx->ui_prova.btn_corrigir, FALSE );
   gtk_widget_set_sensitive( ctx->ui_prova.btn_processar, FALSE );
   // -------------------------------------------------------------------------

   painel->format_titulo    = meu_gerador_variadico( "⏳ Processando Provas..." );
   painel->format_subtitulo = meu_gerador_variadico( "O Motor LaTeX está gerando os PDFs em background." );
   painel->format_instrucao = meu_gerador_variadico( "Por favor, aguarde a unificação." );
   criar_mensagem_painel( INFO, painel );

   g_pdflatex_parallel_async_corrigir_prova( painel, map_array, ctx );
}
