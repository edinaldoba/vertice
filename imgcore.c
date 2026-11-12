/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>
#include <poppler.h>
#include <cairo.h>
#include <math.h>

#include "comum.h"
#include "glibconfig.h"
#include "imgcore.h"






// ====================================================================================
// GERENCIAMENTO DE MEMÓRIA DAS IMAGENS (Arquitetura Contígua 1D + 2D)
// ====================================================================================

/* Função auxiliar para alocar a matriz de pixels em tons de cinza */
guint8** alocar_matriz_pixels( gsize nrow, gsize ncol ) {
   // Aloca o bloco gigante contíguo de pixels, já inicializado com zero (preto)
   guint8 *dados = g_malloc0( nrow * ncol * sizeof( guint8 ) );

   // Aloca apenas o esqueleto de ponteiros de linha
   guint8 **matriz = g_malloc( nrow * sizeof( guint8 * ) );

   // Mapeia os atalhos de linha para os deslocamentos corretos dentro do bloco 1D
   for ( gsize i = 0; i < nrow; i++ ) {
      matriz[i] = dados + ( i * ncol );
   }
   return matriz;
}

/* Função auxiliar para alocar a matriz de pixels RGB */
PixelRGB** alocar_matriz_pixels_colorida( gsize nrow, gsize ncol ) {
   PixelRGB *dados = g_malloc0( nrow * ncol * sizeof( PixelRGB ) );
   PixelRGB **matriz = g_malloc( nrow * sizeof( PixelRGB * ) );

   for ( gsize i = 0; i < nrow; i++ ) {
      matriz[i] = dados + ( i * ncol );
   }
   return matriz;
}

/* Liberação instantânea O(1) da matriz contígua (Cinza) */
void liberar_matriz_pixels( guint8 **matriz, gsize nrow ) {
   ( void )nrow; // Cast void suprime o alerta [-Wunused-parameter] do clangd mantendo a assinatura

   if ( matriz ) {
      if ( matriz[0] ) {
         g_free( matriz[0] ); // Libera todo o bloco mestre de pixels de uma vez
      }
      g_free( matriz );       // Libera o esqueleto de ponteiros
   }
}

/* Liberação instantânea O(1) da matriz contígua (RGB) */
void liberar_matriz_pixels_colorida( PixelRGB **matriz, gsize nrow ) {
   ( void )nrow;

   if ( matriz ) {
      if ( matriz[0] ) {
         g_free( matriz[0] );
      }
      g_free( matriz );
   }
}

/* Liberação completa da estrutura ImagemColorida gerada pelo imread */
void liberar_imagem_imread( ImagemColorida *img ) {
   if ( img ) {
      // Checagem robusta: previne double-free caso o novo campo explícito
      // 'dados' já tenha sido gerido ou liberado externamente.
      if ( img->dados ) {
         g_free( img->dados );
         img->dados = NULL;
      }
      // Fallback clássico: localiza o bloco gigante armazenado no índice 0
      else if ( img->image && img->image[0] ) {
         g_free( img->image[0] );
      }

      // Libera o vetor de ponteiros
      if ( img->image ) {
         g_free( img->image );
         img->image = NULL;
      }
   }
}






// ====================================================================================
// FUNÇÃO AUXILIAR (Otimizada para 1 linha com g_memdup2)
// Use apenas se precisar de uma CÓPIA INDEPENDENTE 1D para outro algoritmo.
// ====================================================================================
// static guchar* _achatar_matriz_colorida( const ImagemColorida *img ) {
//    g_return_val_if_fail( img != NULL && img->dados != NULL, NULL );
//
//    // Como a matriz já é 1D nativamente, basta clonar o bloco mestre inteiro
//    gsize bytes_totais = img->nrow * img->ncol * sizeof( PixelRGB );
//    return ( guchar * )g_memdup2( img->dados, bytes_totais );
// }


// ====================================================================================
// I/O DE IMAGEM: SALVAR PNG (Otimizado com Zero-Copy)
// ====================================================================================
gboolean salvar_imagem_png_nativa( const char *caminho, const ImagemColorida *img ) {
   g_return_val_if_fail( caminho != NULL && img != NULL && img->dados != NULL, FALSE );

   GError *erro = NULL;

   // A API do GdkPixbuf exige 'int'. O cast silencia o [-Wconversion] do clangd.
   int largura = ( int )img->ncol;
   int altura  = ( int )img->nrow;
   int rowstride = largura * 3; // 3 canais (R, G, B) sem transparência

   // 1. Envelopamento Zero-Copy: O Pixbuf apenas "aponta" para o seu bloco img->dados.
   // Passar 'NULL' nos dois últimos argumentos impede que o GTK tente dar free() na sua memória.
   GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data(
                          ( const guchar * )img->dados,
                          GDK_COLORSPACE_RGB, FALSE, 8,
                          largura, altura, rowstride,
                          NULL, NULL
                       );

   if ( !pixbuf ) {
      g_printerr( "[ERRO] Falha crítica ao alocar o envelope GdkPixbuf.\n" );
      return FALSE;
   }

   // 2. I/O de Disco Nativo: Salva o arquivo comprimido usando a libpng do Linux
   gboolean sucesso = gdk_pixbuf_save( pixbuf, caminho, "png", &erro, NULL );

   if ( !sucesso ) {
      g_printerr( "[ERRO] Erro ao salvar PNG (%s): %s\n", caminho, erro->message );
      g_clear_error( &erro );
   }

   // 3. Limpeza: Destrói apenas o envelope Pixbuf.
   // O bloco 'img->dados' permanece intacto e sob o controle do Vértice.
   g_object_unref( pixbuf );

   return sucesso;
}





// ====================================================================================
// FUNÇÃO DE RECONSTRUÇÃO: Integra GdkPixbuf ao Padrão Contíguo (1D + 2D)
// ====================================================================================
static void _reconstruir_matriz_colorida( const guchar *pixels_1d, ImagemColorida *img,
      gsize largura, gsize altura, gsize rowstride, guint16 canais ) {

   g_return_if_fail( pixels_1d != NULL );
   g_return_if_fail( img != NULL );

   // 1. Limpeza segura do buffer anterior (suporta tanto o padrão contíguo quanto legados)
   if ( img->dados ) {
      g_free( img->dados );
      img->dados = NULL;
      g_free( img->image );
      img->image = NULL;
   } else if ( img->image ) {
      liberar_matriz_pixels_colorida( img->image, img->nrow );
      img->image = NULL;
   }

   img->ncol = largura;
   img->nrow = altura;
   img->max = 255;
   g_strlcpy( img->key, "P6", sizeof( img->key ) );

   // 2. Alocação unificada no novo padrão contíguo (Bloco mestre + ponteiros de linha)
   img->dados = g_malloc( altura * largura * sizeof( PixelRGB ) );
   img->image = g_malloc( altura * sizeof( PixelRGB * ) );

   for ( gsize y = 0; y < altura; y++ ) {
      img->image[y] = img->dados + ( y * largura );
   }

   // 3. Preenchimento otimizado (Cópia Direta vs Conversão RGBA paralela)
   if ( canais == 3 && rowstride == ( largura * 3 ) ) {
      // Bloco contíguo puro sem padding: cópia mestre de uma só vez para máxima velocidade SIMD!
      memcpy( img->dados, pixels_1d, ( size_t )( altura * largura * 3 ) );
   } else if ( canais == 3 ) {
      // RGB com padding de alinhamento no rowstride
      // #pragma omp parallel for schedule(static)
      for ( gsize y = 0; y < altura; y++ ) {
         const guchar *origem_linha = pixels_1d + ( y * ( size_t )rowstride );
         memcpy( img->image[y], origem_linha, ( size_t )largura * 3 );
      }
   } else {
      // RGBA (4 canais) ou outros formatos: descarrega o canal Alpha em paralelo
      // #pragma omp parallel for schedule(static)
      for ( gsize y = 0; y < altura; y++ ) {
         const guchar *origem_linha = pixels_1d + ( y * ( size_t )rowstride );
         for ( gsize x = 0; x < largura; x++ ) {
            const guchar *p = origem_linha + ( x * canais );
            img->image[y][x].r = p[0];
            img->image[y][x].g = p[1];
            img->image[y][x].b = p[2];
         }
      }
   }
}

// ====================================================================================
// CARREGAMENTO NATIVO DE IMAGEM COLORIDA VIA GDK-PIXBUF
// ====================================================================================
gboolean carregar_imagem_colorida_nativa( const char *caminho, ImagemColorida *img ) {
   g_return_val_if_fail( caminho != NULL, FALSE );
   g_return_val_if_fail( img != NULL, FALSE );

   GError *erro = NULL;
   GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file( caminho, &erro );

   if ( !pixbuf ) {
      g_printerr( "[ERRO] Ao carregar imagem (%s): %s\n", caminho, erro->message );
      g_clear_error( &erro );
      return FALSE;
   }

   gsize largura = ( gsize )gdk_pixbuf_get_width( pixbuf );
   gsize altura = ( gsize )gdk_pixbuf_get_height( pixbuf );
   gsize rowstride = ( gsize )gdk_pixbuf_get_rowstride( pixbuf );
   guint16 canais = ( guint16 )gdk_pixbuf_get_n_channels( pixbuf ); // 3 para RGB, 4 para RGBA
   const guchar *pixels_1d = gdk_pixbuf_read_pixels( pixbuf );

   // Reconstrói a matriz aplicando o mapeamento contíguo otimizado
   _reconstruir_matriz_colorida( pixels_1d, img, largura, altura, rowstride, canais );

   g_object_unref( pixbuf );
   return TRUE;
}








// ====================================================================================
// I/O DE IMAGEM: SALVAR PGM (Otimizado com Zero-Copy)
// ====================================================================================
void salvar_imagem_pgm( const ImagemCinza *IMG, const char *arquivo_destino ) {
   g_return_if_fail( IMG != NULL && IMG->dados != NULL && arquivo_destino != NULL );

   FILE *out = fopen( arquivo_destino, "wb" );
   if ( !out ) {
      g_printerr( "[ERRO] Não foi possível criar o arquivo de saída %s.\n", arquivo_destino );
      return;
   }

   // 1. Grava o cabeçalho
   // %hu é usado para o IMG->max (guint16). G_GSIZE_FORMAT blinda os gsize.
   fprintf( out, "%s\n%" G_GSIZE_FORMAT " %" G_GSIZE_FORMAT "\n%hu\n",
            IMG->key, IMG->ncol, IMG->nrow, IMG->max );

   // 2. I/O em Bloco Direto (Disk-Bound Puro)
   // Como os dados já estão contíguos na RAM, enviamos tudo de uma vez para o disco.
   gsize total_pixels = IMG->nrow * IMG->ncol;
   gsize gravados = fwrite( IMG->dados, sizeof( uint8_t ), total_pixels, out );

   if ( gravados != total_pixels ) {
      g_printerr( "[ERRO] Falha de I/O: Nem todos os pixels foram salvos em %s.\n", arquivo_destino );
   }

   // 3. Limpeza
   fclose( out );
}




// ====================================================================================
// CORTE ORTOGONAL - ESCALA DE CINZA (Bounding Box)
// ====================================================================================
void cortar_imagem_ortogonal( const ImagemCinza *IMG, ImagemCinza *img, gsize x_ini, gsize y_ini, gsize largura, gsize altura ) {
   g_return_if_fail( IMG != NULL && IMG->image != NULL && img != NULL );

   // Como gsize é unsigned (sem sinal), <= 0 é logicamente equivalente a == 0
   if ( largura == 0 || altura == 0 ) {
      return;
   }

   // Proteção de limites rigorosa
   if ( x_ini + largura > IMG->ncol ) {
      largura = IMG->ncol - x_ini;
   }
   if ( y_ini + altura > IMG->nrow ) {
      altura = IMG->nrow - y_ini;
   }

   // Limpeza e Realocação no Padrão Contíguo (1D real + 2D pointers)
   if ( img->dados ) {
      g_free( img->dados );
      g_free( img->image );
   }

   img->ncol = largura;
   img->nrow = altura;
   img->max = IMG->max;
   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );

   // Alocação em bloco único para vetorização SIMD
   img->dados = g_malloc( img->nrow * img->ncol * sizeof( uint8_t ) );
   img->image = g_malloc( img->nrow * sizeof( uint8_t * ) );
   for ( gsize i = 0; i < img->nrow; i++ ) {
      img->image[i] = img->dados + ( i * img->ncol );
   }

   // Cópia ultra-rápida de blocos de memória linha a linha (Multithread)
   // Diretiva OpenMP reativada com schedule(static)
   // #pragma omp parallel for schedule(static)
   for ( gsize i = 0; i < altura; i++ ) {
      // CORREÇÃO CRÍTICA: sizeof( uint8_t ) no lugar de sizeof( int )
      memcpy( img->image[i], &IMG->image[y_ini + i][x_ini], largura * sizeof( uint8_t ) );
   }
}


// ====================================================================================
// CORTE ORTOGONAL - RGB COLORIDO (Bounding Box)
// ====================================================================================
void cortar_imagem_ortogonal_colorida( const ImagemColorida *IMG, ImagemColorida *img,
                                       gsize x_ini, gsize y_ini, gsize largura, gsize altura ) {
   g_return_if_fail( IMG != NULL && IMG->image != NULL && img != NULL );

   if ( largura == 0 || altura == 0 ) {
      return;
   }

   // Proteção de limites rigorosa
   if ( x_ini + largura > IMG->ncol ) {
      largura = IMG->ncol - x_ini;
   }
   if ( y_ini + altura > IMG->nrow ) {
      altura = IMG->nrow - y_ini;
   }

   // Limpeza e Realocação no Padrão Contíguo (1D real + 2D pointers)
   if ( img->dados ) {
      g_free( img->dados );
      g_free( img->image );
   }

   img->ncol = largura;
   img->nrow = altura;
   img->max = IMG->max;
   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );

   // Alocação em bloco único de structs PixelRGB
   img->dados = g_malloc( img->nrow * img->ncol * sizeof( PixelRGB ) );
   img->image = g_malloc( img->nrow * sizeof( PixelRGB * ) );
   for ( gsize i = 0; i < img->nrow; i++ ) {
      img->image[i] = img->dados + ( i * img->ncol );
   }

   // Cópia pura de memória, muito mais rápida que interpolação para cortes retos
   // #pragma omp parallel for schedule(static)
   for ( gsize i = 0; i < altura; i++ ) {
      // Remoção do cast (guint) que causaria alertas -Wconversion. gsize -> size_t é direto.
      memcpy( img->image[i], &IMG->image[y_ini + i][x_ini], largura * sizeof( PixelRGB ) );
   }
}





// ====================================================================================
// CONVERSÃO DE PDF PARA PNG (MÚLTIPLAS PÁGINAS)
// ====================================================================================
int pdf_para_png_multiplo( const char *caminho_pdf, const char *pasta_destino, const char *prefixo_arquivo,
                           int inicio, int fim, double escala ) {
   g_return_val_if_fail( caminho_pdf != NULL && pasta_destino != NULL && prefixo_arquivo != NULL, -1 );
   g_return_val_if_fail( escala > 0.0, -1 );

   GError *erro = NULL;

   // 1. Padronização: Uso do GFile (mesma segurança da pdf2png) para caminhos
   g_autoptr( GFile ) arquivo = g_file_new_for_path( caminho_pdf );
   g_autofree char *uri = g_file_get_uri( arquivo );

   if ( !uri ) {
      g_printerr( "[ERRO] Falha ao criar URI: %s\n", caminho_pdf );
      return -1;
   }

   PopplerDocument *doc = poppler_document_new_from_file( uri, NULL, &erro );
   if ( !doc ) {
      g_printerr( "[ERRO] Falha ao carregar o PDF: %s\n", erro->message );
      g_clear_error( &erro );
      return -1;
   }

   int total_paginas = poppler_document_get_n_pages( doc );
   if ( total_paginas <= 0 ) {
      g_printerr( "[ERRO] O arquivo PDF está vazio.\n" );
      g_object_unref( doc );
      return -1;
   }

   if ( inicio < 0 ) {
      inicio = 0;
   }
   if ( fim < 0 || fim >= total_paginas ) {
      fim = total_paginas - 1;
   }

   if ( inicio > fim ) {
      g_printerr( "[ERRO] Intervalo de páginas inválido (%d a %d).\n", inicio, fim );
      g_object_unref( doc );
      return -1;
   }

   for ( int i = inicio; i <= fim; i++ ) {
      PopplerPage *pagina = poppler_document_get_page( doc, i );
      if ( !pagina ) {
         g_printerr( "[AVISO] Não foi possível ler a página %d.\n", i + 1 );
         continue;
      }

      double largura, altura;
      poppler_page_get_size( pagina, &largura, &altura );

      // Arredondamento e cast explícito: silencia -Wconversion e melhora a precisão dos pixels
      int largura_px = ( int )round( largura * escala );
      int altura_px  = ( int )round( altura * escala );

      cairo_surface_t *superficie = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, largura_px, altura_px );
      cairo_t *cr = cairo_create( superficie );

      cairo_set_source_rgb( cr, 1.0, 1.0, 1.0 );
      cairo_paint( cr );

      cairo_scale( cr, escala, escala );
      poppler_page_render( pagina, cr );

      // Montagem de caminho blindada contra duplicação de barras (ex: "pasta//arquivo.png")
      g_autofree char *nome_arquivo = g_strdup_printf( "%s_%03d.png", prefixo_arquivo, i + 1 );
      g_autofree char *caminho_png = g_build_filename( pasta_destino, nome_arquivo, NULL );

      cairo_status_t status = cairo_surface_write_to_png( superficie, caminho_png );

      if ( status != CAIRO_STATUS_SUCCESS ) {
         g_printerr( "[ERRO] Falha ao gravar %s: %s\n", caminho_png, cairo_status_to_string( status ) );
      }

      // Limpeza estrita de memória local do Cairo/Poppler
      cairo_destroy( cr );
      cairo_surface_destroy( superficie );
      g_object_unref( pagina );
   }

   g_object_unref( doc );
   return total_paginas;
}


// ====================================================================================
// CONVERSÃO DE PDF PARA PNG (PÁGINA ÚNICA)
// ====================================================================================
gboolean pdf2png( const char *caminho_pdf, const char *caminho_png, double escala ) {
   g_return_val_if_fail( caminho_pdf != NULL && caminho_png != NULL, FALSE );
   g_return_val_if_fail( escala > 0.0, FALSE );

   if ( !g_file_test( caminho_pdf, G_FILE_TEST_EXISTS ) ) {
      g_printerr( "[ERRO] Arquivo PDF não encontrado: %s\n", caminho_pdf );
      return FALSE;
   }

   GError *erro = NULL;

   g_autoptr( GFile ) arquivo = g_file_new_for_path( caminho_pdf );
   g_autofree char *uri = g_file_get_uri( arquivo );

   if ( !uri ) {
      g_printerr( "[ERRO] Falha ao criar URI para o PDF.\n" );
      return FALSE;
   }

   PopplerDocument *doc = poppler_document_new_from_file( uri, NULL, &erro );
   if ( !doc ) {
      g_printerr( "[ERRO] Falha ao carregar o PDF: %s\n", erro->message );
      g_clear_error( &erro );
      return FALSE;
   }

   PopplerPage *pagina = poppler_document_get_page( doc, 0 );
   if ( !pagina ) {
      g_printerr( "[ERRO] O PDF parece estar vazio ou sem páginas.\n" );
      g_object_unref( doc );
      return FALSE;
   }

   double largura, altura;
   poppler_page_get_size( pagina, &largura, &altura );

   // Arredondamento e cast explícito para cairo_image_surface_create
   int largura_px = ( int )round( largura * escala );
   int altura_px  = ( int )round( altura * escala );

   cairo_surface_t *superficie = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, largura_px, altura_px );
   cairo_t *cr = cairo_create( superficie );

   cairo_set_source_rgb( cr, 1.0, 1.0, 1.0 );
   cairo_paint( cr );

   cairo_scale( cr, escala, escala );
   poppler_page_render( pagina, cr );

   cairo_status_t status = cairo_surface_write_to_png( superficie, caminho_png );

   // Destruição dos objetos imediatamente após a renderização
   cairo_destroy( cr );
   cairo_surface_destroy( superficie );
   g_object_unref( pagina );
   g_object_unref( doc );

   if ( status != CAIRO_STATUS_SUCCESS ) {
      g_printerr( "[ERRO] Falha ao gravar o PNG: %s\n", cairo_status_to_string( status ) );
      return FALSE;
   }

   return TRUE;
}







// ====================================================================================
// CONVERSÃO DE RGB PARA TONS DE CINZA (Totalmente Contíguo e Paralelizado)
// ====================================================================================
void rgb2gray( const ImagemColorida *PPM, ImagemCinza *PGM ) {
   g_return_if_fail( PPM != NULL && PPM->dados != NULL );
   g_return_if_fail( PGM != NULL );

   g_strlcpy( PGM->key, "P5", sizeof( PGM->key ) );
   PGM->ncol = PPM->ncol;
   PGM->nrow = PPM->nrow;
   PGM->max  = PPM->max;

   // 1. Limpeza e Realocação no Padrão Contíguo (1D real + 2D pointers)
   if ( PGM->dados ) {
      g_free( PGM->dados );
      g_free( PGM->image );
   }

   PGM->dados = g_malloc( PGM->nrow * PGM->ncol * sizeof( uint8_t ) );
   PGM->image = g_malloc( PGM->nrow * sizeof( uint8_t * ) );
   for ( gsize i = 0; i < PGM->nrow; i++ ) {
      PGM->image[i] = PGM->dados + ( i * PGM->ncol );
   }

   // 2. Conversão de Luminância Rec.709 Otimizada (Varredura Linear 1D)
   // Com a memória contígua, o CPU ignora linhas/colunas e avança direto pelo buffer mestre
   gsize total_pixels = PGM->nrow * PGM->ncol;

   // #pragma omp parallel for schedule(static)
   for ( gsize i = 0; i < total_pixels; i++ ) {
      uint8_t r = PPM->dados[i].r;
      uint8_t g = PPM->dados[i].g;
      uint8_t b = PPM->dados[i].b;

      PGM->dados[i] = ( uint8_t )( ( 2126 * r + 7152 * g + 722 * b ) / 10000 );
   }
}

// ====================================================================================
// LEITURA DIRETA DO DISCO PARA ESCALA DE CINZA (Conversão On-the-fly)
// ====================================================================================
void imread_gray( ImagemCinza *IMG, const char *arquivo ) {
   g_return_if_fail( IMG != NULL && arquivo != NULL );

   FILE *p = fopen( arquivo, "rb" );
   if ( !p ) {
      g_printerr( "[ERRO] Não foi possível abrir o arquivo %s.\n", arquivo );
      return;
   }

   if ( fscanf( p, "%9s\n%" G_GSIZE_FORMAT " %" G_GSIZE_FORMAT "\n%hu\n",
                IMG->key, &IMG->ncol, &IMG->nrow, &IMG->max ) != 4 ) {
      g_printerr( "[ERRO] Falha ao ler o cabeçalho PNM/PPM do arquivo.\n" );
      fclose( p );
      return;
   }

   g_strlcpy( IMG->key, "P5", sizeof( IMG->key ) );

   if ( IMG->dados ) {
      g_free( IMG->dados );
      g_free( IMG->image );
   }

   // 1. Alocação Contígua Definitiva
   IMG->dados = g_malloc( IMG->nrow * IMG->ncol * sizeof( uint8_t ) );
   IMG->image = g_malloc( IMG->nrow * sizeof( uint8_t * ) );
   for ( gsize i = 0; i < IMG->nrow; i++ ) {
      IMG->image[i] = IMG->dados + ( i * IMG->ncol );
   }

   // 2. Buffer temporário de leitura para o arquivo colorido
   gsize total_pixels = IMG->ncol * IMG->nrow;
   g_autofree PixelRGB *buffer_rgb = g_malloc( total_pixels * sizeof( PixelRGB ) );

   if ( fread( buffer_rgb, sizeof( PixelRGB ), total_pixels, p ) != total_pixels ) {
      g_printerr( "[AVISO] Fim de arquivo inesperado. A imagem %s pode estar cortada.\n", arquivo );
   }
   fclose( p );

   // 3. Conversão de Cor Imediata (1D puro e multithread)
   // #pragma omp parallel for schedule(static)
   for ( gsize i = 0; i < total_pixels; i++ ) {
      uint8_t r = buffer_rgb[i].r;
      uint8_t g = buffer_rgb[i].g;
      uint8_t b = buffer_rgb[i].b;

      IMG->dados[i] = ( uint8_t )( ( 2126 * r + 7152 * g + 722 * b ) / 10000 );
   }
}

// ====================================================================================
// LEITURA PURA DE IMAGEM PGM (ZERO-COPY DISCO -> RAM)
// ====================================================================================
void imread_pgm( ImagemCinza *IMG, const char *arquivo ) {
   g_return_if_fail( IMG != NULL && arquivo != NULL );

   FILE *p = fopen( arquivo, "rb" );
   if ( !p ) {
      g_printerr( "[ERRO] Não foi possível abrir o arquivo %s.\n", arquivo );
      return;
   }

   if ( fscanf( p, "%9s\n%" G_GSIZE_FORMAT " %" G_GSIZE_FORMAT "\n%hu\n",
                IMG->key, &IMG->ncol, &IMG->nrow, &IMG->max ) != 4 ) {
      g_printerr( "[ERRO] Falha ao ler o cabeçalho PGM do arquivo.\n" );
      fclose( p );
      return;
   }
   g_strlcpy( IMG->key, "P5", sizeof( IMG->key ) );

   if ( IMG->dados ) {
      g_free( IMG->dados );
      g_free( IMG->image );
   }

   // 1. Alocação Contígua
   IMG->dados = g_malloc( IMG->nrow * IMG->ncol * sizeof( uint8_t ) );
   IMG->image = g_malloc( IMG->nrow * sizeof( uint8_t * ) );
   for ( gsize i = 0; i < IMG->nrow; i++ ) {
      IMG->image[i] = IMG->dados + ( i * IMG->ncol );
   }

   // 2. Zero-Copy I/O: O disco injeta a imagem DIRETO no nosso bloco final da struct
   gsize total_pixels = IMG->ncol * IMG->nrow;
   if ( fread( IMG->dados, sizeof( uint8_t ), total_pixels, p ) != total_pixels ) {
      g_printerr( "[AVISO] Fim de arquivo inesperado. A imagem %s pode estar cortada.\n", arquivo );
   }

   fclose( p );
}

// ====================================================================================
// LEITURA PURA DE IMAGEM PPM COLORIDA (ZERO-COPY DISCO -> RAM)
// ====================================================================================
void imread( ImagemColorida *img, const char *arquivo ) {
   g_return_if_fail( img != NULL && arquivo != NULL );

   FILE *p = fopen( arquivo, "rb" );
   if ( !p ) {
      g_printerr( "[ERRO] Não foi possível abrir o arquivo %s.\n", arquivo );
      return;
   }

   if ( fscanf( p, "%9s\n%" G_GSIZE_FORMAT " %" G_GSIZE_FORMAT "\n%hu\n",
                img->key, &img->ncol, &img->nrow, &img->max ) != 4 ) {
      g_printerr( "[ERRO] Falha ao ler o cabeçalho PPM do arquivo.\n" );
      fclose( p );
      return;
   }

   if ( img->dados ) {
      g_free( img->dados );
      g_free( img->image );
   }

   // 1. Alocação Contígua (O "Músculo" e o "Esqueleto")
   img->dados = g_malloc( img->nrow * img->ncol * sizeof( PixelRGB ) );
   img->image = g_malloc( img->nrow * sizeof( PixelRGB * ) );
   for ( gsize i = 0; i < img->nrow; i++ ) {
      img->image[i] = img->dados + ( i * img->ncol );
   }

   // 2. Zero-Copy I/O: O disco injeta a imagem DIRETO no nosso bloco final da struct
   gsize total_pixels = img->ncol * img->nrow;
   if ( fread( img->dados, sizeof( PixelRGB ), total_pixels, p ) != total_pixels ) {
      g_printerr( "[AVISO] Imagem truncada ou erro de leitura em %s.\n", arquivo );
   }

   fclose( p );
}



//========================================================================================================//





