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
#include "tex.h"
#include "pds.h"
#include "gas.h"
#include "assincrono.h"



// ====================================================================================
// FUNÇÕES UTILITÁRIAS REFATORADAS
// ====================================================================================

/* Substitui a extensão de um arquivo retornando uma nova string alocada na Heap */
static char* trocar_extensao( const char *caminho_orig, const char *nova_ext ) {
   g_return_val_if_fail( caminho_orig != NULL && nova_ext != NULL, NULL );

   const char *ponto = g_strrstr( caminho_orig, "." );
   const char *barra = g_strrstr( caminho_orig, G_DIR_SEPARATOR_S );

   // Garante que o ponto pertence ao nome do arquivo e não a uma pasta oculta anterior
   if ( ponto != NULL && ( barra == NULL || ponto > barra ) ) {
      return g_strdup_printf( "%.*s.%s", ( int )( ponto - caminho_orig ), caminho_orig, nova_ext );
   }
   return g_strdup_printf( "%s.%s", caminho_orig, nova_ext );
}

/* Gera identificadores alfanuméricos na Heap (Usa GRand local para evitar lock contention no OpenMP) */
static char* gerar_nome_aleatorio_thread( gsize tam, GRand *rng ) {
   static const char alfanumerico[] = "0123456789abcdefghijklmnopqrstuvwxyz";
   char *nome = g_malloc( tam + 1 );

   for ( gsize j = 0; j < tam; j++ ) {
      gint32 idx = ( rng != NULL ) ? g_rand_int_range( rng, 0, 36 ) : g_random_int_range( 0, 36 );
      nome[j] = alfanumerico[idx];
   }
   nome[tam] = '\0';

   return nome;
}

/* Wrapper padrão para ordenar ou buscar strings em GPtrArray / bsearch */
static gint cmp_str_ptr( gconstpointer a, gconstpointer b ) {
   return g_strcmp0( *( const char * const * )a, *( const char * const * )b );
}

// ====================================================================================
// ROTINAS DE TRIAGEM E ESCALONAMENTO ESPACIAL
// ====================================================================================

/* Transfere as imagens da pasta de entrada para o diretório de trabalho gerando nomes únicos */
static GPtrArray* converter_e_copiar_imagens( const char *origem, const char *destino ) {
   g_return_val_if_fail( origem != NULL && destino != NULL, NULL );

   g_autoptr( GError ) error = NULL;
   g_autoptr( GDir ) dir = g_dir_open( origem, 0, &error );
   if ( dir == NULL ) {
      g_printerr( "[AVISO] Não foi possível abrir a pasta de origem: %s (%s)\n",
                  origem, ( error != NULL ) ? error->message : "Erro desconhecido" );
      return NULL;
   }

   // 1. FASE DE COLETA
   g_autoptr( GPtrArray ) arquivos_validos = g_ptr_array_new_with_free_func( g_free );
   const char *filename = NULL;

   while ( ( filename = g_dir_read_name( dir ) ) != NULL ) {
      // ascii_strdown é mais rápido que utf8_strdown para checagem pura de extensão ASCII
      g_autofree char *filename_lc = g_ascii_strdown( filename, -1 );

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
   guint total_validos = arquivos_validos->len;
   g_autofree char **slots_gerados = g_new0( char *, ( gsize )total_validos );

   // 3. FASE DE TRANSFERÊNCIA PARALELA (Sem contenção de Mutex no gerador aleatório)
   #pragma omp parallel
   {
      // Cada thread instancia seu próprio gerador pseudoaleatório independente
      g_autoptr( GRand ) rng_local = g_rand_new();

      #pragma omp for schedule(dynamic)
      for ( guint i = 0; i < total_validos; i++ ) {
         const char *file_atual = ( const char * )g_ptr_array_index( arquivos_validos, i );

         g_autofree char *nome_img = gerar_nome_aleatorio_thread( 20, rng_local );
         const char *ext = g_strrstr( file_atual, "." );

         // Normaliza a extensão para minúsculo (ex: .JPG -> .jpg)
         g_autofree char *ext_lc = ( ext != NULL ) ? g_ascii_strdown( ext, -1 ) : g_strdup( "" );
         char *nome_final = g_strdup_printf( "%s%s", nome_img, ext_lc );

         g_autofree char *path_origem  = g_build_filename( origem, file_atual, NULL );
         g_autofree char *path_destino = g_build_filename( destino, nome_final, NULL );

         if ( gio_copiar_arquivo( path_origem, path_destino ) ) {
            g_remove( path_origem );
            slots_gerados[i] = nome_final; // Sucesso: transfere a posse do ponteiro para o slot
         } else {
            g_printerr( "[ERRO] Falha na transferência da imagem: %s\n", file_atual );
            g_free( nome_final );          // Falha: limpa a string e mantém o slot NULL
         }
      }
   }

   // 4. CONSOLIDAÇÃO SEGURA (Compacta apenas os arquivos copiados com sucesso)
   GPtrArray *arquivos_gerados = g_ptr_array_sized_new( total_validos );
   g_ptr_array_set_free_func( arquivos_gerados, g_free );

   for ( guint i = 0; i < total_validos; i++ ) {
      if ( slots_gerados[i] != NULL ) {
         g_ptr_array_add( arquivos_gerados, slots_gerados[i] );
      }
   }

   if ( arquivos_gerados->len == 0 ) {
      g_ptr_array_free( arquivos_gerados, TRUE );
      return NULL;
   }

   return arquivos_gerados;
}

/* Reprojeta as coordenadas das 4 âncoras da imagem reduzida (cinza) para a resolução original (RGB) */
static void normalizar_ancora( const ImagemColorida *img_rgb, const ImagemCinza *img_gray, IndiceMatriz *ancora ) {
   g_return_if_fail( img_rgb != NULL && img_gray != NULL && ancora != NULL );
   g_return_if_fail( img_gray->ncol > 0 && img_gray->nrow > 0 );

   // 1. Fatores de escala diretos (Largura e Altura)
   double scale_x = ( double )img_rgb->ncol / ( double )img_gray->ncol;
   double scale_y = ( double )img_rgb->nrow / ( double )img_gray->nrow;

   // Limites máximos seguros na imagem de alta resolução
   int max_x = ( int )img_rgb->ncol - 1;
   int max_y = ( int )img_rgb->nrow - 1;

   // 2. Aplicação da escala em cada uma das 4 âncoras com clamping de segurança
   for ( gsize k = 0; k < 4; k++ ) {
      double x_scaled = ( double )ancora[k].j * scale_x;
      double y_scaled = ( double )ancora[k].i * scale_y;

      int nx = ( int )round( x_scaled );
      int ny = ( int )round( y_scaled );

      ancora[k].j = CLAMP( nx, 0, max_x );
      ancora[k].i = CLAMP( ny, 0, max_y );
   }
}






// ====================================================================================
// FUNÇÕES AUXILIARES LOCAIS DE LIMPEZA (Evitam repetição e previnem vazamentos)
// ====================================================================================
static inline void _limpar_imagem_cinza_local( ImagemCinza *img ) {
   if ( !img ) {
      return;
   }
   if ( img->dados ) {
      g_free( img->dados );
      img->dados = NULL;
      g_free( img->image );
      img->image = NULL;
   } else if ( img->image ) {
      liberar_matriz_pixels( img->image, img->nrow );
      img->image = NULL;
   }
   img->nrow = 0;
   img->ncol = 0;
}
//------------------------------------------------------------------------------------------
static inline void _limpar_imagem_colorida_local( ImagemColorida *img ) {
   if ( !img ) {
      return;
   }
   if ( img->dados ) {
      g_free( img->dados );
      img->dados = NULL;
      g_free( img->image );
      img->image = NULL;
   } else if ( img->image ) {
      liberar_matriz_pixels_colorida( img->image, img->nrow );
      img->image = NULL;
   }
   img->nrow = 0;
   img->ncol = 0;
}
/**
 * Função principal de processamento em lote.
 * Executa a visão computacional multithread, cortes e leitura do payload/respostas.
 */
// ====================================================================================
// MOTOR 2: VISÃO COMPUTACIONAL EVOLUTIVA (GAS MULTITHREAD)
// ====================================================================================
int gas_processar_imagens( const InterfaceDados *dados, const InterfaceCabecalho *cabecalho ) {
   g_return_val_if_fail( dados != NULL && cabecalho != NULL, -1 );

   const char *home = g_get_home_dir();
   if ( home == NULL ) {
      home = ".";
   }

   g_autofree char *origem         = g_build_filename( home, "Downloads", "imagens", NULL );
   g_autofree char *destino        = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "imagens", NULL );
   g_autofree char *respostas      = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "respostas", NULL );
   g_autofree char *dir_rejeitadas = g_build_filename( destino, "rejeitadas", NULL );

   if ( g_mkdir_with_parents( destino, 0755 ) != 0 ||
         g_mkdir_with_parents( dir_rejeitadas, 0755 ) != 0 ||
         g_mkdir_with_parents( respostas, 0755 ) != 0 ) {
      g_printerr( "[ERRO CRÍTICO] Não foi possível criar os diretórios de destino.\n" );
      return -1;
   }

   g_autofree char *gabaritos = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "gabaritos", NULL );
   int qtd_bin = quantidade_arquivos_por_extensao( gabaritos, ".bin" );

   if ( qtd_bin <= 0 ) {
      g_printerr( "[AVISO] Nenhuma prova foi gerada até o momento.\n" );
      return -1;
   }

   g_autoptr( GPtrArray ) imgs_orig = converter_e_copiar_imagens( origem, destino );

   if ( !imgs_orig || imgs_orig->len == 0 ) {
      g_printerr( "[AVISO] Nenhuma imagem de respostas foi encontrada em %s.\n", origem );
      return -2;
   }

   g_autoptr( GPtrArray ) resp_bin = carregar_arquivos_por_extensao( gabaritos, ".bin", qtd_bin );
   g_ptr_array_sort( resp_bin, cmp_str_ptr );

   g_autofree FILE **f = g_new0( FILE *, ( gsize )resp_bin->len );
   for ( guint i = 0; i < resp_bin->len; i++ ) {
      const char *nome_bin = ( const char * )g_ptr_array_index( resp_bin, i );
      g_autofree char *arquivo = g_build_filename( respostas, nome_bin, NULL );
      f[i] = fopen( arquivo, "ab" );
   }

   int n_rejeitadas = 0;
   guint total_imgs = imgs_orig->len;

   // =========================================================================
   // PROCESSAMENTO PARALELO DAS IMAGENS (OpenMP)
   // =========================================================================
   #pragma omp parallel for schedule(dynamic) reduction(+:n_rejeitadas)
   for ( guint i = 0; i < total_imgs; i++ ) {

      gboolean sucesso = FALSE;
      int tentativas = 0;

      MapeamentoGabarito map = { 0 };
      IndiceMatriz ancora[4] = { { 0, 0 } };

      ImagemColorida img_rgb_orig = { 0 };
      ImagemColorida img_rgb_crop = { 0 };
      ImagemCinza img_gray_bin    = { 0 };
      ImagemCinza img_gray_crop   = { 0 };
      ImagemCinza img_gray_alloc  = { 0 };

      const char *img_str = ( const char * )g_ptr_array_index( imgs_orig, i );

      g_autofree char *path_orig = g_build_filename( destino, img_str, NULL );
      g_autofree char *img_png   = trocar_extensao( img_str, "png" );
      g_autofree char *path_png  = g_build_filename( destino, img_png, NULL );

      if ( carregar_imagem_colorida_nativa( path_orig, &img_rgb_orig ) ) {
         g_remove( path_orig );
         rgb2gray( &img_rgb_orig, &img_gray_bin );

         const gsize dim = 960;
         redimensionar_imagem_bilinear( &img_gray_bin, &img_gray_alloc, dim );

         // FASE 3: Visão Computacional Evolutiva (Dupla Passada)
         // transformada_homografica já gerencia a realocação de img_gray_crop internamente
         do {
            gas_mapear_ancoras( &img_gray_alloc, &map, ancora, tentativas );
            transformada_homografica( &img_gray_alloc, &img_gray_crop, ancora, map.direcao );
            binarizar_pgm_metodo_otsu( &img_gray_crop );
            map.payload = extrair_payload_matriz( &img_gray_crop, map.direcao );

            sucesso = decodificar_payload_matriz( &map, cabecalho );
            tentativas++;
         } while ( !sucesso && tentativas < 2 );
      }

      if ( !sucesso ) {
         g_printerr( "[FALHA CRÍTICA] Payload inválido mesmo após resgate. Imagem: %s\n", img_str );
      }

      // FASE 4: Processamento de Dados (Gravação Binária)
      if ( sucesso ) {
         char chave_buf[256] = { 0 };
         const char *chave_ptr = chave_buf;

         nome_base_gabaritos_bin( chave_buf, sizeof( chave_buf ), map.turma, map.disc, map.per, map.seq );

         const char * const *base_pdata = ( const char * const * )resp_bin->pdata;
         const char * const *found = ( const char * const * )bsearch(
                                        &chave_ptr, base_pdata, ( gsize )resp_bin->len, sizeof( char * ), cmp_str_ptr
                                     );

         if ( found != NULL ) {
            gsize idx_arq = ( gsize )( found - base_pdata );

            if ( f[idx_arq] != NULL ) {
               ler_respostas_gabarito( &img_gray_crop, map.direcao, map.resp );
               map.num = ler_numero_aluno( &img_gray_crop, map.direcao );
               g_strlcpy( map.nome_img, img_png, sizeof( map.nome_img ) );

               #pragma omp critical(escrita_binario)
               {
                  if ( fwrite( &map, sizeof( MapeamentoGabarito ), 1, f[idx_arq] ) != 1 ) {
                     g_printerr( "[ERRO] O registro da imagem %s não foi salvo.\n", img_str );
                  }
               }
            } else {
               g_printerr( "[ERRO] Descritor de arquivo inválido para o binário '%s' (%s)\n", chave_buf, img_str );
               sucesso = FALSE;
            }
         } else {
            g_printerr( "[ALERTA] Binário '%s' não encontrado para: %s\n", chave_buf, img_str );
            sucesso = FALSE;
         }
      }

      // FASE 5: Renderização do Crop Colorido
      if ( sucesso ) {
         normalizar_ancora( &img_rgb_orig, &img_gray_alloc, ancora );
         transformada_homografica_colorida( &img_rgb_orig, &img_rgb_crop, ancora, map.direcao );

         ImagemColorida img_rgb_limpa = { 0 };
         filtrar_fundo_magico_colorido( &img_rgb_crop, &img_rgb_limpa, 15 );

         salvar_imagem_png_nativa( path_png, &img_rgb_limpa );
         _limpar_imagem_colorida_local( &img_rgb_limpa );

      } else if ( img_rgb_orig.dados != NULL || img_rgb_orig.image != NULL ) {
         g_autofree char *path_erro = g_build_filename( dir_rejeitadas, img_png, NULL );
         salvar_imagem_png_nativa( path_erro, &img_rgb_orig );
         n_rejeitadas++;
      } else {
         n_rejeitadas++;
      }

      // FASE 6: Limpeza Segura de Memória
      _limpar_imagem_cinza_local( &img_gray_crop );
      _limpar_imagem_cinza_local( &img_gray_bin );
      _limpar_imagem_cinza_local( &img_gray_alloc );
      _limpar_imagem_colorida_local( &img_rgb_crop );
      _limpar_imagem_colorida_local( &img_rgb_orig );
   }

   // =========================================================================
   // FINALIZAÇÃO GLOBAL
   // =========================================================================
   for ( guint i = 0; i < resp_bin->len; i++ ) {
      if ( f[i] != NULL ) {
         fclose( f[i] );
         const char *nome_bin = ( const char * )g_ptr_array_index( resp_bin, i );
         g_autofree char *arquivo = g_build_filename( respostas, nome_bin, NULL );

         if ( verificar_arquivo( arquivo ) == ARQUIVO_VAZIO ) {
            if ( g_remove( arquivo ) != 0 ) {
               g_printerr( "[ERRO] Falha ao tentar remover o arquivo vazio: %s\n", arquivo );
            }
         }
      }
   }

   g_print( "Processamento das imagens GAS concluído com sucesso!\n" );
   return n_rejeitadas;
}


// ====================================================================================
// MOTOR 3: PIPELINE DETERMINÍSTICO COM PRÉ-FILTRAGEM (OMR MULTITHREAD)
// ====================================================================================
int omr_processar_imagens( const InterfaceDados *dados, const InterfaceCabecalho *cabecalho ) {
   g_return_val_if_fail( dados != NULL && cabecalho != NULL, -1 );

   const char *home = g_get_home_dir();
   if ( home == NULL ) {
      home = ".";
   }

   g_autofree char *origem         = g_build_filename( home, "Downloads", "imagens", NULL );
   g_autofree char *destino        = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "imagens", NULL );
   g_autofree char *respostas      = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "respostas", NULL );
   g_autofree char *dir_rejeitadas = g_build_filename( destino, "rejeitadas", NULL );

   if ( g_mkdir_with_parents( destino, 0755 ) != 0 ||
         g_mkdir_with_parents( dir_rejeitadas, 0755 ) != 0 ||
         g_mkdir_with_parents( respostas, 0755 ) != 0 ) {
      g_printerr( "[ERRO CRÍTICO] Não foi possível criar os diretórios de destino.\n" );
      return -1;
   }

   g_autofree char *gabaritos = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "gabaritos", NULL );
   int qtd_bin = quantidade_arquivos_por_extensao( gabaritos, ".bin" );

   if ( qtd_bin <= 0 ) {
      g_printerr( "[AVISO] Nenhuma prova foi gerada até o momento.\n" );
      return -1;
   }

   g_autoptr( GPtrArray ) imgs_orig = converter_e_copiar_imagens( origem, destino );

   if ( !imgs_orig || imgs_orig->len == 0 ) {
      g_printerr( "[AVISO] O sistema não encontrou fotografias ou digitalizações de respostas na pasta %s.\n", origem );
      return -2;
   }

   g_autoptr( GPtrArray ) resp_bin = carregar_arquivos_por_extensao( gabaritos, ".bin", qtd_bin );
   g_ptr_array_sort( resp_bin, cmp_str_ptr );

   g_autofree FILE **f = g_new0( FILE *, ( gsize )resp_bin->len );
   for ( guint i = 0; i < resp_bin->len; i++ ) {
      const char *nome_bin = ( const char * )g_ptr_array_index( resp_bin, i );
      g_autofree char *arquivo = g_build_filename( respostas, nome_bin, NULL );
      f[i] = fopen( arquivo, "ab" );
   }

   int n_rejeitadas = 0;
   guint total_imgs = imgs_orig->len;

   // =========================================================================
   // PROCESSAMENTO PARALELO DAS IMAGENS (OpenMP)
   // =========================================================================
   #pragma omp parallel for schedule(dynamic) reduction(+:n_rejeitadas)
   for ( guint i = 0; i < total_imgs; i++ ) {

      gboolean sucesso = FALSE;
      MapeamentoGabarito map = { 0 };
      IndiceMatriz ancora[4] = { { 0, 0 } };

      ImagemColorida img_rgb_orig = { 0 };
      ImagemColorida img_rgb_crop = { 0 };
      ImagemCinza img_gray_bin    = { 0 };
      ImagemCinza img_gray_alloc  = { 0 };
      ImagemCinza img_gray_fundo  = { 0 };
      ImagemCinza img_gray_blur   = { 0 };
      ImagemCinza img_gray_crop   = { 0 };

      const char *img_str = ( const char * )g_ptr_array_index( imgs_orig, i );

      g_autofree char *path_orig = g_build_filename( destino, img_str, NULL );
      g_autofree char *img_png   = trocar_extensao( img_str, "png" );
      g_autofree char *path_png  = g_build_filename( destino, img_png, NULL );

      if ( carregar_imagem_colorida_nativa( path_orig, &img_rgb_orig ) ) {
         g_remove( path_orig );
         rgb2gray( &img_rgb_orig, &img_gray_bin );

         const gsize dim = 960;
         redimensionar_imagem_bilinear( &img_gray_bin, &img_gray_alloc, dim );

         // FASE 3: PIPELINE DETERMINÍSTICO OMR COM REMOÇÃO DE FUNDO E SUAVIZAÇÃO
         filtrar_fundo_magico_cinza( &img_gray_alloc, &img_gray_fundo, 30 );
         aplicar_filtro_gaussiano_2d( &img_gray_fundo, &img_gray_blur, 1.0f );
         binarizar_pgm_metodo_otsu( &img_gray_blur );

         // 1ª Passagem: Contornos e Poda Ortogonal
         sucesso = detectar_ancoras_omr( &img_gray_blur, ancora, &map.direcao, FALSE );
         if ( sucesso ) {
            transformada_homografica( &img_gray_blur, &img_gray_crop, ancora, map.direcao );
            map.payload = extrair_payload_matriz( &img_gray_crop, map.direcao );
            sucesso = decodificar_payload_matriz( &map, cabecalho );
         }

         // 2ª Passagem (Resgate): Template Matching
         if ( !sucesso ) {
            sucesso = detectar_ancoras_omr( &img_gray_blur, ancora, &map.direcao, TRUE );
            if ( sucesso ) {
               transformada_homografica( &img_gray_blur, &img_gray_crop, ancora, map.direcao );
               map.payload = extrair_payload_matriz( &img_gray_crop, map.direcao );
               sucesso = decodificar_payload_matriz( &map, cabecalho );
            }
         }
      }

      if ( !sucesso ) {
         g_printerr( "[FALHA CRÍTICA] OMR rejeitou a imagem ou Payload inválido: %s\n", img_str );
      }

      // FASE 4: Processamento de Dados (Gravação Binária)
      if ( sucesso ) {
         char chave_buf[256] = { 0 };
         const char *chave_ptr = chave_buf;

         nome_base_gabaritos_bin( chave_buf, sizeof( chave_buf ), map.turma, map.disc, map.per, map.seq );

         const char * const *base_pdata = ( const char * const * )resp_bin->pdata;
         const char * const *found = ( const char * const * )bsearch(
                                        &chave_ptr, base_pdata, ( gsize )resp_bin->len, sizeof( char * ), cmp_str_ptr
                                     );

         if ( found != NULL ) {
            gsize idx_arq = ( gsize )( found - base_pdata );

            if ( f[idx_arq] != NULL ) {
               ler_respostas_gabarito( &img_gray_crop, map.direcao, map.resp );
               map.num = ler_numero_aluno( &img_gray_crop, map.direcao );
               g_strlcpy( map.nome_img, img_png, sizeof( map.nome_img ) );

               #pragma omp critical(escrita_binario)
               {
                  if ( fwrite( &map, sizeof( MapeamentoGabarito ), 1, f[idx_arq] ) != 1 ) {
                     g_printerr( "[ERRO] O registro da imagem %s não foi salvo.\n", img_str );
                  }
               }
            } else {
               g_printerr( "[ERRO] Descritor de arquivo inválido para o binário '%s' (%s)\n", chave_buf, img_str );
               sucesso = FALSE;
            }
         } else {
            g_printerr( "[ALERTA] Binário '%s' não encontrado para: %s\n", chave_buf, img_str );
            sucesso = FALSE;
         }
      }

      // FASE 5: Renderização do Crop Colorido
      if ( sucesso ) {
         normalizar_ancora( &img_rgb_orig, &img_gray_alloc, ancora );
         transformada_homografica_colorida( &img_rgb_orig, &img_rgb_crop, ancora, map.direcao );

         ImagemColorida img_rgb_limpa = { 0 };
         filtrar_fundo_magico_colorido( &img_rgb_crop, &img_rgb_limpa, 30 );
         realcar_cores_in_place( &img_rgb_limpa, 1.3f, 1.6f );

         salvar_imagem_png_nativa( path_png, &img_rgb_limpa );
         _limpar_imagem_colorida_local( &img_rgb_limpa );
      } else if ( img_rgb_orig.dados != NULL || img_rgb_orig.image != NULL ) {
         g_autofree char *path_erro = g_build_filename( dir_rejeitadas, img_png, NULL );
         salvar_imagem_png_nativa( path_erro, &img_rgb_orig );
         n_rejeitadas++;
      } else {
         n_rejeitadas++;
      }

      // FASE 6: Limpeza Segura de Memória
      _limpar_imagem_cinza_local( &img_gray_fundo );
      _limpar_imagem_cinza_local( &img_gray_blur );
      _limpar_imagem_cinza_local( &img_gray_crop );
      _limpar_imagem_cinza_local( &img_gray_bin );
      _limpar_imagem_cinza_local( &img_gray_alloc );
      _limpar_imagem_colorida_local( &img_rgb_crop );
      _limpar_imagem_colorida_local( &img_rgb_orig );
   }

   // =========================================================================
   // FINALIZAÇÃO GLOBAL
   // =========================================================================
   for ( guint i = 0; i < resp_bin->len; i++ ) {
      if ( f[i] != NULL ) {
         fclose( f[i] );
         const char *nome_bin = ( const char * )g_ptr_array_index( resp_bin, i );
         g_autofree char *arquivo = g_build_filename( respostas, nome_bin, NULL );

         if ( verificar_arquivo( arquivo ) == ARQUIVO_VAZIO ) {
            if ( g_remove( arquivo ) != 0 ) {
               g_printerr( "[ERRO] Falha ao tentar remover o arquivo vazio: %s\n", arquivo );
            }
         }
      }
   }

   g_print( "Processamento das imagens OMR (Determinístico) concluído com sucesso!\n" );
   return n_rejeitadas;
}





// ====================================================================================
// MOTOR PRINCIPAL DE PROCESSAMENTO EM LOTE (OMR MULTITHREAD)
// ====================================================================================
int cs_omr_processar_imagens( const InterfaceDados *dados, const InterfaceCabecalho *cabecalho ) {
   g_return_val_if_fail( dados != NULL && cabecalho != NULL, -1 );

   const char *home = g_get_home_dir();
   if ( home == NULL ) {
      home = ".";
   }

   g_autofree char *origem         = g_build_filename( home, "Downloads", "imagens", NULL );
   g_autofree char *destino        = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "imagens", NULL );
   g_autofree char *respostas      = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "respostas", NULL );
   g_autofree char *dir_rejeitadas = g_build_filename( destino, "rejeitadas", NULL );

   if ( g_mkdir_with_parents( destino, 0755 ) != 0 ||
         g_mkdir_with_parents( dir_rejeitadas, 0755 ) != 0 ||
         g_mkdir_with_parents( respostas, 0755 ) != 0 ) {
      g_printerr( "[ERRO CRÍTICO] Não foi possível criar os diretórios de destino.\n" );
      return -1;
   }

   g_autofree char *gabaritos = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "gabaritos", NULL );
   int qtd_bin = quantidade_arquivos_por_extensao( gabaritos, ".bin" );

   if ( qtd_bin <= 0 ) {
      g_printerr( "[AVISO] Nenhuma prova foi gerada até o momento.\n" );
      return -1;
   }

   g_autoptr( GPtrArray ) imgs_orig = converter_e_copiar_imagens( origem, destino );

   if ( !imgs_orig || imgs_orig->len == 0 ) {
      g_printerr( "[AVISO] O sistema não encontrou fotografias ou digitalizações de respostas na pasta %s.\n", origem );
      return -2;
   }

   g_autoptr( GPtrArray ) resp_bin = carregar_arquivos_por_extensao( gabaritos, ".bin", qtd_bin );
   g_ptr_array_sort( resp_bin, cmp_str_ptr );

   // Vetor dinâmico para os arquivos binários abertos em modo append
   g_autofree FILE **f = g_new0( FILE *, ( gsize )resp_bin->len );
   for ( guint i = 0; i < resp_bin->len; i++ ) {
      const char *nome_bin = ( const char * )g_ptr_array_index( resp_bin, i );
      g_autofree char *arquivo = g_build_filename( respostas, nome_bin, NULL );
      f[i] = fopen( arquivo, "ab" );
   }

   int n_rejeitadas = 0;
   guint total_imgs = imgs_orig->len;

   // =========================================================================
   // PROCESSAMENTO PARALELO (OpenMP)
   // =========================================================================
   #pragma omp parallel for schedule(dynamic) reduction(+:n_rejeitadas)
   for ( guint i = 0; i < total_imgs; i++ ) {

      gboolean sucesso = FALSE;
      MapeamentoGabarito map = { 0 };
      IndiceMatriz ancora[4] = { { 0, 0 } };

      ImagemColorida img_rgb_orig = { 0 };
      ImagemColorida img_rgb_crop = { 0 };
      ImagemCinza img_gray_bin    = { 0 };
      ImagemCinza img_gray_alloc  = { 0 };
      ImagemCinza img_gray_crop   = { 0 };

      const char *img_str = ( const char * )g_ptr_array_index( imgs_orig, i );

      g_autofree char *path_orig = g_build_filename( destino, img_str, NULL );
      g_autofree char *img_png   = trocar_extensao( img_str, "png" );
      g_autofree char *path_png  = g_build_filename( destino, img_png, NULL );

      // Verifica se a imagem original carregou com integridade antes de prosseguir
      if ( carregar_imagem_colorida_nativa( path_orig, &img_rgb_orig ) ) {
         g_remove( path_orig );
         rgb2gray( &img_rgb_orig, &img_gray_bin );

         const gsize dim = 960;
         redimensionar_imagem_bilinear( &img_gray_bin, &img_gray_alloc, dim );

         binarizar_pgm_metodo_otsu( &img_gray_alloc );

         // 1ª Passagem: Detecção rápida por contornos (BFS + poda ortogonal)
         sucesso = detectar_ancoras_omr( &img_gray_alloc, ancora, &map.direcao, FALSE );
         if ( sucesso ) {
            transformada_homografica( &img_gray_alloc, &img_gray_crop, ancora, map.direcao );
            map.payload = extrair_payload_matriz( &img_gray_crop, map.direcao );
            sucesso = decodificar_payload_matriz( &map, cabecalho );
         }

         // 2ª Passagem (Resgate): Correlação cruzada por Template se a 1ª falhar
         if ( !sucesso ) {
            sucesso = detectar_ancoras_omr( &img_gray_alloc, ancora, &map.direcao, TRUE );
            if ( sucesso ) {
               // transformada_homografica já limpa/realoca img_gray_crop internamente
               transformada_homografica( &img_gray_alloc, &img_gray_crop, ancora, map.direcao );

               // g_autofree char *img_ppm   = trocar_extensao( img_png, "ppm" );
               // salvar_imagem_pgm( &img_gray_alloc, img_ppm );

               map.payload = extrair_payload_matriz( &img_gray_crop, map.direcao );
               sucesso = decodificar_payload_matriz( &map, cabecalho );
            }
         }
      }

      if ( !sucesso ) {
         g_printerr( "[FALHA CRÍTICA] OMR rejeitou a imagem ou Payload inválido: %s\n", img_str );
      }

      if ( sucesso ) {
         // Buffer local na stack da thread (evita malloc/free repetitivo a cada imagem)
         char chave_buf[256] = { 0 };
         const char *chave_ptr = chave_buf;

         nome_base_gabaritos_bin( chave_buf, sizeof( chave_buf ), map.turma, map.disc, map.per, map.seq );

         const char * const *base_pdata = ( const char * const * )resp_bin->pdata;
         const char * const *found = ( const char * const * )bsearch(
                                        &chave_ptr, base_pdata, ( gsize )resp_bin->len, sizeof( char * ), cmp_str_ptr
                                     );

         if ( found != NULL ) {
            gsize idx_arq = ( gsize )( found - base_pdata );

            if ( f[idx_arq] != NULL ) {
               ler_respostas_gabarito( &img_gray_crop, map.direcao, map.resp );
               map.num = ler_numero_aluno( &img_gray_crop, map.direcao );

               g_strlcpy( map.nome_img, img_png, sizeof( map.nome_img ) );

               #pragma omp critical(escrita_binario)
               {
                  if ( fwrite( &map, sizeof( MapeamentoGabarito ), 1, f[idx_arq] ) != 1 ) {
                     g_printerr( "[ERRO] O registro da imagem %s não foi salvo.\n", img_str );
                  }
               }
            } else {
               g_printerr( "[ERRO] Descritor de arquivo inválido para o binário '%s' (%s)\n", chave_buf, img_str );
               sucesso = FALSE;
            }
         } else {
            g_printerr( "[ALERTA] Binário '%s' não encontrado para: %s\n", chave_buf, img_str );
            sucesso = FALSE;
         }
      }

      if ( sucesso ) {
         normalizar_ancora( &img_rgb_orig, &img_gray_alloc, ancora );
         transformada_homografica_colorida( &img_rgb_orig, &img_rgb_crop, ancora, map.direcao );
         salvar_imagem_png_nativa( path_png, &img_rgb_crop );
      } else if ( img_rgb_orig.dados != NULL || img_rgb_orig.image != NULL ) {
         g_autofree char *path_erro = g_build_filename( dir_rejeitadas, img_png, NULL );
         salvar_imagem_png_nativa( path_erro, &img_rgb_orig );
         n_rejeitadas++;
      } else {
         n_rejeitadas++;
      }

      // Limpeza unificada e segura da memória de cada imagem na thread
      _limpar_imagem_cinza_local( &img_gray_crop );
      _limpar_imagem_cinza_local( &img_gray_bin );
      _limpar_imagem_cinza_local( &img_gray_alloc );
      _limpar_imagem_colorida_local( &img_rgb_crop );
      _limpar_imagem_colorida_local( &img_rgb_orig );
   }

   // =========================================================================
   // FINALIZAÇÃO GLOBAL
   // =========================================================================
   for ( guint i = 0; i < resp_bin->len; i++ ) {
      if ( f[i] != NULL ) {
         fclose( f[i] );
         const char *nome_bin = ( const char * )g_ptr_array_index( resp_bin, i );
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







// ====================================================================================
// VALIDAÇÃO DE INTEGRIDADE DO MAPEAMENTO OMR
// ====================================================================================
static StatusMapeamento validar_prova_escaneada( const MapeamentoGabarito *map, const AppContext *ctx ) {
   g_return_val_if_fail( map != NULL && ctx != NULL, ERRO_NUMERO_ALUNO_INVALIDO );

   const InterfaceDiario *diario = &ctx->diario;
   const InterfaceDados  *dados  = &ctx->dados;

   // 1º TESTE (Crítico): O número de chamada protege o GPtrArray 'diario->fichas'
   if ( !diario->fichas || map->num <= 0 ||
         map->num > dados->qtd_alunos_total ||
         ( guint )map->num > diario->fichas->len ) {
      return ERRO_NUMERO_ALUNO_INVALIDO;
   }

   // 2º TESTE (Crítico): O ID (0-indexed) protege o deslocamento no bloco contíguo 'G'
   if ( ( int )map->id >= dados->qtd_alunos_ativos ) {
      return ERRO_ID_GABARITO_INVALIDO;
   }

   const FichaAluno *ficha = ( const FichaAluno * )g_ptr_array_index( diario->fichas, ( guint )( map->num - 1 ) );
   if ( !ficha ) {
      return ERRO_NUMERO_ALUNO_INVALIDO;
   }

   // 3º TESTE (Regra de Negócio): Memória validada, verifica se o aluno está ativo
   if ( !ficha->ativo ) {
      return ( StatusMapeamento )( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO );
   }

   return STATUS_PROVA_OK;
}

// ====================================================================================
// COMPARADOR PARA DEDUPLICAÇÃO POR CÓDIGO DO ALUNO
// ====================================================================================
static gint comparar_por_cod_aluno( gconstpointer a, gconstpointer b ) {
   const MapeamentoGabarito *m1 = ( const MapeamentoGabarito * )a;
   const MapeamentoGabarito *m2 = ( const MapeamentoGabarito * )b;

   return ( m1->cod_aluno > m2->cod_aluno ) - ( m1->cod_aluno < m2->cod_aluno );
}

// ====================================================================================
// MOTOR DE CORREÇÃO DE PROVAS E LANÇAMENTO AUTOMÁTICO DE NOTAS
// ====================================================================================
void corrigir_prova( InterfacePainel *painel, AppContext *ctx ) {
   g_return_if_fail( painel != NULL && ctx != NULL );

   const InterfaceDiario    *diario    = &ctx->diario;
   const InterfaceDados     *dados     = &ctx->dados;
   const InterfaceCabecalho *cabecalho = &ctx->cabecalho;

   if ( dados->total_quest_prova <= 0 || dados->max_quest_prova <= 0 || dados->qtd_alunos_ativos <= 0 ) {
      g_printerr( "[ERRO] Configuração de questões (%d/%d) ou alunos ativos (%d) inválida.\n",
                  dados->total_quest_prova, dados->max_quest_prova, dados->qtd_alunos_ativos );
      return;
   }

   char nome_bin[64] = { 0 };
   nome_base_gabaritos_bin( nome_bin, sizeof( nome_bin ),
                            cabecalho->foco_turma, cabecalho->foco_disciplina,
                            cabecalho->foco_periodo, dados->seq_prova );

   g_autofree char *path_resp = g_build_filename(
                                   dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "respostas", nome_bin, NULL
                                );

   g_autofree char *path_gab = g_build_filename(
                                  dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "gabaritos", nome_bin, NULL
                               );

   // =================================================================================
   // 1. VALIDAÇÃO DE EXISTÊNCIA E ABERTURA DOS ARQUIVOS BINÁRIOS
   // =================================================================================
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
      fclose( fr );
      painel->format_titulo    = meu_gerador_variadico( "⚠ Gabarito Mestre Ausente" );
      painel->format_subtitulo = meu_gerador_variadico( "Nenhum gabarito foi gerado para esta turma ainda." );
      painel->format_instrucao = meu_gerador_variadico( "Acesse a aba de geração de provas e crie o arquivo de gabaritos." );
      criar_mensagem_painel( ERRO, painel );
      return;
   }

   // =================================================================================
   // 2. LEITURA BINÁRIA PARA GARRAY E BLOCO 1D DE GABARITOS (Sem ItemTextoCurto)
   // =================================================================================
   int qtd_linhas = contar_registros_binarios( path_resp, sizeof( MapeamentoGabarito ) );
   if ( qtd_linhas <= 0 ) {
      fclose( fr );
      fclose( fg );
      g_printerr( "[AVISO] Arquivo de respostas vazio ou corrompido: %s\n", path_resp );
      painel->format_titulo    = meu_gerador_variadico( "⚠ Arquivo de Respostas Vazio" );
      painel->format_subtitulo = meu_gerador_variadico( "Nenhuma folha de resposta válida foi registrada em %s.", nome_bin );
      painel->format_instrucao = meu_gerador_variadico( "Verifique as imagens na pasta de entrada e processe novamente." );
      criar_mensagem_painel( ERRO, painel );
      return;
   }

   // Sem g_autoptr: a posse de 'map_array' é transferida para g_pdflatex_parallel_async_corrigir_prova
   GArray *map_array = g_array_sized_new( FALSE, FALSE, sizeof( MapeamentoGabarito ), ( guint )qtd_linhas );
   g_array_set_size( map_array, ( guint )qtd_linhas );

   gsize lidos_resp = fread( map_array->data, sizeof( MapeamentoGabarito ), ( gsize )qtd_linhas, fr );
   fclose( fr );

   if ( lidos_resp != ( gsize )qtd_linhas ) {
      g_array_set_size( map_array, ( guint )lidos_resp );
   }

   // Alocação Contígua 1D para os gabaritos (idêntica à usada em gerar_gabaritos e thread_gerar_prova)
   gsize num_ativos = ( gsize )dados->qtd_alunos_ativos;
   gsize max_len    = ( gsize )dados->max_quest_prova + 1;

   g_autofree char *G = g_malloc0( num_ativos * max_len );
   gsize lidos_gab = fread( G, max_len, num_ativos, fg );
   fclose( fg );

   if ( lidos_gab != num_ativos ) {
      g_printerr( "[AVISO] Esperava %" G_GSIZE_FORMAT " gabaritos, mas leu %" G_GSIZE_FORMAT " em %s.\n",
                  num_ativos, lidos_gab, path_gab );
   }

   // =================================================================================
   // 3. SANITIZAÇÃO E CORRESPONDÊNCIA ABSOLUTA DO CÓDIGO DO ALUNO
   // =================================================================================
   if ( diario->fichas != NULL ) {
      for ( guint i = 0; i < map_array->len; i++ ) {
         MapeamentoGabarito *map = &g_array_index( map_array, MapeamentoGabarito, i );
         int idx_esperado = map->num - 1;

         if ( idx_esperado >= 0 && ( guint )idx_esperado < diario->fichas->len ) {
            const FichaAluno *ficha = ( const FichaAluno * )g_ptr_array_index( diario->fichas, ( guint )idx_esperado );

            if ( ficha && ( map->cod_aluno == 0 || map->cod_aluno == ficha->cod_aluno ) ) {
               map->cod_aluno = ficha->cod_aluno;
            } else {
               for ( guint j = 0; j < diario->fichas->len; j++ ) {
                  const FichaAluno *f_busca = ( const FichaAluno * )g_ptr_array_index( diario->fichas, j );
                  if ( f_busca && f_busca->cod_aluno == map->cod_aluno ) {
                     map->num = ( int )( j + 1 );
                     break;
                  }
               }
            }
         }
      }
   }

   // =================================================================================
   // 4. DEDUPLICAÇÃO BLINDADA VIA COD_ALUNO
   // =================================================================================
   g_array_sort( map_array, comparar_por_cod_aluno );

   if ( map_array->len > 1 ) {
      for ( guint i = map_array->len - 1; i > 0; i-- ) {
         const MapeamentoGabarito *atual    = &g_array_index( map_array, MapeamentoGabarito, i );
         const MapeamentoGabarito *anterior = &g_array_index( map_array, MapeamentoGabarito, i - 1 );

         if ( atual->cod_aluno > 0 && atual->cod_aluno == anterior->cod_aluno ) {
            g_autofree char *imagem = g_build_filename(
                                         dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "imagens", anterior->nome_img, NULL
                                      );
            g_remove( imagem );
            g_array_remove_index( map_array, i - 1 );
         }
      }
   }

   g_array_sort( map_array, comparar_mapeamento_gabarito );

   // =================================================================================
   // 5. LAÇO PARALELO BLINDADO COM INJEÇÃO DIRETA NA MEMÓRIA
   // =================================================================================
   guint total_provas = map_array->len;
   float peso_questao = 10.0f / ( float )dados->total_quest_prova;

   #pragma omp parallel for schedule(static)
   for ( guint i = 0; i < total_provas; i++ ) {

      MapeamentoGabarito *map = &g_array_index( map_array, MapeamentoGabarito, i );
      StatusMapeamento status = validar_prova_escaneada( map, ctx );
      map->status = status;

      if ( status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {

         g_autofree gchar *nome_base = g_strdup_printf( "%.2d", map->num );

         // Deslocamento aritmético O(1) no bloco 1D de gabaritos
         const char *gab = G + ( ( gsize )map->id * max_len );

         int acertos = imagens_corrigidas( gab, map, ctx, nome_base );
         map->acertos = acertos;

         int idx_aluno = map->num - 1;

         if ( diario->fichas && idx_aluno >= 0 && ( guint )idx_aluno < diario->fichas->len ) {
            FichaAluno *ficha = ( FichaAluno * )g_ptr_array_index( diario->fichas, ( guint )idx_aluno );
            int f_disc    = cabecalho->foco_disciplina;
            int f_per     = map->per;
            int seq_prova = map->seq;
            int idx_av    = ( 2 * seq_prova ) - 2;
            float nota    = peso_questao * ( float )acertos;

            // Proteção completa de limites antes de gravar na ficha do diário
            if ( ficha &&
                  f_disc >= 0 && f_disc < cabecalho->limite_disciplinas &&
                  f_per >= 0 && f_per <= 4 &&
                  idx_av >= 0 ) {
               ficha->disciplina[f_disc].periodo[f_per].avaliacoes[idx_av].av = nota;
               ficha->ficha_modificada = TRUE;
            }
         }

      } else {
         int thread_id = omp_get_thread_num();

         if ( status & ERRO_NUMERO_ALUNO_INVALIDO ) {
            g_printerr( "[ALERTA Thread %d] Número da prova (%d) corrompido ou fora dos limites.\n",
                        thread_id, map->num );
         } else if ( status & ERRO_ID_GABARITO_INVALIDO ) {
            g_printerr( "[ALERTA Thread %d] O ID lido (%d) na prova Nº %d não possui gabarito.\n",
                        thread_id, ( int )map->id, map->num );
         }
      }
   }

   // =================================================================================
   // 6. ATUALIZAÇÃO DO ARQUIVO BINÁRIO E COMPILAÇÃO (LaTeX Assíncrono)
   // =================================================================================
   g_autoptr( GError ) erro = NULL;
   gssize bytes_resp = ( gssize )( ( gsize )map_array->len * sizeof( MapeamentoGabarito ) );

   if ( !g_file_set_contents( path_resp, map_array->data, bytes_resp, &erro ) ) {
      g_printerr( "[ERRO CRÍTICO] Falha ao atualizar o arquivo de respostas: %s\n", erro->message );
   }

   if ( ctx->prova.btn_corrigir ) {
      gtk_widget_set_sensitive( ctx->prova.btn_corrigir, FALSE );
   }
   if ( ctx->prova.btn_processar ) {
      gtk_widget_set_sensitive( ctx->prova.btn_processar, FALSE );
   }

   painel->format_titulo    = meu_gerador_variadico( "⏳ Processando Provas..." );
   painel->format_subtitulo = meu_gerador_variadico( "O Motor LaTeX está gerando os PDFs em background." );
   painel->format_instrucao = meu_gerador_variadico( "Por favor, aguarde a unificação." );
   criar_mensagem_painel( INFO, painel );

   g_pdflatex_parallel_async_corrigir_prova( painel, map_array, ctx );
}
