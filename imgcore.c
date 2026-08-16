/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>
#include <poppler.h>
#include <cairo.h>

#include "comum.h"
#include "imgcore.h"






/* Função auxiliar para alocar a matriz de pixels usando GLib */
int** alocar_matriz_pixels( int nrow, int ncol ) {
   int **matriz = g_new( int*, nrow );
   for ( int i = 0; i < nrow; i++ ) {
      matriz[i] = g_new0( int, ncol ); // g_new0 já zera a memória
   }
   return matriz;
}

/* Função auxiliar para alocar a matriz de pixels usando GLib */
PixelRGB** alocar_matriz_pixels_colorida( int nrow, int ncol ) {
   PixelRGB **matriz = g_new( PixelRGB*, nrow );
   for ( int i = 0; i < nrow; i++ ) {
      matriz[i] = g_new0( PixelRGB, ncol ); // g_new0 já zera a memória
   }
   return matriz;
}

void liberar_matriz_pixels( int **matriz, int nrow ) {
   for ( int i = 0; i < nrow; i++ ) {
      g_free( matriz[i] );
   }
   g_free( matriz );
   matriz = NULL;
}

void liberar_matriz_pixels_colorida( PixelRGB **matriz, int nrow ) {
   for ( int i = 0; i < nrow; i++ ) {
      g_free( matriz[i] );
   }
   g_free( matriz );
   matriz = NULL;
}

void liberar_imagem_imread( ImagemColorida *img ) {
   if ( img && img->image ) {
      // img->image[0] é sempre o endereço raiz do nosso buffer_gigante!
      if ( img->image[0] ) {
         free( img->image[0] );
      }

      // Libera o esqueleto de ponteiros
      free( img->image );
      img->image = NULL;
   }
}




// void carregar_imagem_pixbuf(const char *caminho, ImagemColorida *img) {
//     g_return_if_fail(img != NULL); // Segurança GLib
//
//     GError *erro = NULL;
//
//     // O GdkPixbuf descobre o formato (PNG, JPEG, PPM) sozinho pelo cabeçalho do arquivo!
//     GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(caminho, &erro);
//
//     if (!pixbuf) {
//         g_printerr("[ERRO] Falha ao carregar imagem '%s': %s\n", caminho, erro->message);
//         g_clear_error(&erro);
//         return;
//     }
//
//     // Extrai dimensões e dados de layout da memória
//     int largura = gdk_pixbuf_get_width(pixbuf);
//     int altura = gdk_pixbuf_get_height(pixbuf);
//     int canais = gdk_pixbuf_get_n_channels(pixbuf);   // 3 (RGB) ou 4 (RGBA/PNG transparente)
//     int rowstride = gdk_pixbuf_get_rowstride(pixbuf); // Espaço real em bytes de uma linha
//     guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
//
//     // 1. Preenche os metadados da sua estrutura
//     img->ncol = largura;
//     img->nrow = altura;
//     img->max = 255; // Padrão GdkPixbuf é 8-bit por canal
//     g_strlcpy(img->key, "P6", sizeof(img->key)); // Mantemos P6 para não quebrar outras rotinas antigas
//
//     // 2. Aloca a matriz bidimensional usando sua função
//     img->image = alocar_matriz_pixels_colorida(img->nrow, img->ncol);
//
//     // 3. Conversão de 1D (GdkPixbuf) para 2D (Sua matriz)
//     for (int i = 0; i < altura; i++) {
//         // Encontra o ponteiro inicial para a linha 'i', respeitando o padding (rowstride)
//         guchar *linha = pixels + (i * rowstride);
//
//         for (int j = 0; j < largura; j++) {
//             // Encontra o pixel exato dentro da linha, saltando de 3 em 3 ou 4 em 4 bytes
//             guchar *p = linha + (j * canais);
//
//             // Copia apenas os canais RGB.
//             // Se canais == 4 (tiver canal Alpha de transparência em p[3]), ele será ignorado com segurança.
//             img->image[i][j].r = p[0];
//             img->image[i][j].g = p[1];
//             img->image[i][j].b = p[2];
//         }
//     }
//
//     // Libera a memória da biblioteca nativa SOMENTE APÓS ter copiado tudo
//     g_object_unref(pixbuf);
// }




// Função auxiliar privada para transformar a matriz 2D em um vetor 1D
static guchar* achatar_matriz_colorida( const ImagemColorida *img ) {
   if ( !img || !img->image ) return NULL;

   int largura = img->ncol;
   int altura = img->nrow;

   // O GdkPixbuf RGB exige exatamente 3 bytes por pixel.
   // O "rowstride" (passo da linha) é o tamanho exato de uma linha em bytes.
   int rowstride = largura * 3;

   // Alocamos o bloco único contíguo usando GLib
   guchar *pixels_1d = g_malloc( altura * rowstride );

   // Copiamos linha por linha em paralelo!
   // O memcpy faz a CPU transferir o bloco de memória de uma vez só (instruções SIMD).
   for ( int y = 0; y < altura; y++ ) {
      guchar *destino_linha = pixels_1d + ( y * rowstride );

      // Copiamos os pixels da linha Y da sua matriz 2D para a posição correta no vetor 1D
      memcpy( destino_linha, img->image[y], rowstride );
   }

   return pixels_1d;
}

gboolean salvar_imagem_png_nativa( const char *caminho, const ImagemColorida *img ) {
   if ( !caminho || !img || !img->image ) return FALSE;

   GError *erro = NULL;
   int largura = img->ncol;
   int altura = img->nrow;
   int rowstride = largura * 3; // 3 canais (R, G, B)

   // 1. Extração contígua: trazemos a sua matriz 2D para o formato nativo do GTK
   // O g_autofree blinda a função: não importa onde ocorra um "return", a RAM será limpa.
   g_autofree guchar *pixels_rgb = achatar_matriz_colorida( img );
   if ( !pixels_rgb ) return FALSE;

   // 2. Criação do envelope do GdkPixbuf (Ele apenas "aponta" para o nosso vetor)
   // GDK_COLORSPACE_RGB, FALSE (sem transparência/Alpha), 8 (bits por canal)
   GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data(
                          pixels_rgb, GDK_COLORSPACE_RGB, FALSE, 8,
                          largura, altura, rowstride, NULL, NULL
                       );

   if ( !pixbuf ) {
      g_printerr( "Falha crítica ao alocar o envelope GdkPixbuf.\n" );
      return FALSE;
   }

   // 3. I/O de Disco Nativo: Salva o arquivo comprimido usando a libpng do Linux
   gboolean sucesso = gdk_pixbuf_save( pixbuf, caminho, "png", &erro, NULL );

   if ( !sucesso ) {
      g_printerr( "Erro ao salvar PNG (%s): %s\n", caminho, erro->message );
      g_clear_error( &erro ); // Limpa a struct de erro da GLib
   }

   // 4. Limpeza: Destruímos o envelope.
   // O vetor 'pixels_rgb' será libertado silenciosamente no fechamento das chaves pelo g_autofree.
   g_object_unref( pixbuf );

   return sucesso;
}





// Função de reconstrução que lida com 3 canais (RGB) ou 4 canais (RGBA)
static void reconstruir_matriz_colorida( const guchar *pixels_1d, ImagemColorida *img,
      int largura, int altura,
      int rowstride, int canais ) {
   g_return_if_fail( pixels_1d != NULL );
   g_return_if_fail( img != NULL );

   if ( img->image != NULL ) {
      liberar_matriz_pixels_colorida( img->image, img->nrow );
   }

   img->ncol = largura;
   img->nrow = altura;
   img->max = 255;
   g_strlcpy( img->key, "P6", sizeof( img->key ) );

   img->image = alocar_matriz_pixels_colorida( altura, largura );
   if ( !img->image ) return;

   // Se for RGB puro (3 canais) e sem padding, podemos usar o memcpy ultraveloz!
   if ( canais == 3 && rowstride == ( largura * 3 ) ) {
      size_t tamanho_linha_util = largura * sizeof( PixelRGB );

      // #pragma omp parallel for
      for ( int y = 0; y < altura; y++ ) {
         const guchar *origem_linha = pixels_1d + ( y * rowstride );
         memcpy( img->image[y], origem_linha, tamanho_linha_util );
      }
   } else {
      // Caso a imagem tenha Alpha (4 canais) ou padding no rowstride,
      // copiamos pixel a pixel pulando o canal A (transparência)
      // #pragma omp parallel for
      for ( int y = 0; y < altura; y++ ) {
         const guchar *origem_linha = pixels_1d + ( y * rowstride );
         for ( int x = 0; x < largura; x++ ) {
            const guchar *p = origem_linha + ( x * canais );
            img->image[y][x].r = p[0];
            img->image[y][x].g = p[1];
            img->image[y][x].b = p[2];
         }
      }
   }
}

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

   int largura = gdk_pixbuf_get_width( pixbuf );
   int altura = gdk_pixbuf_get_height( pixbuf );
   int rowstride = gdk_pixbuf_get_rowstride( pixbuf );
   int canais = gdk_pixbuf_get_n_channels( pixbuf ); // 3 para RGB, 4 para RGBA
   const guchar *pixels_1d = gdk_pixbuf_read_pixels( pixbuf );

   // Passamos o número de canais para a função auxiliar
   reconstruir_matriz_colorida( pixels_1d, img, largura, altura, rowstride, canais );

   g_object_unref( pixbuf );
   return TRUE;
}








void salvar_imagem_pgm( ImagemCinza *IMG, const char *arquivo_destino ) {
   if ( !IMG || !IMG->image || !arquivo_destino ) return;

   FILE *out = fopen( arquivo_destino, "wb" );
   if ( !out ) {
      fprintf( stderr, "Erro: Não foi possível criar o arquivo de saída %s.\n", arquivo_destino );
      return;
   }

   // 1. Grava o cabeçalho
   fprintf( out, "%s\n%d %d\n%d\n", IMG->key, IMG->ncol, IMG->nrow, IMG->max );

   // 2. Otimização I/O: Buffer Gigante (aloca a imagem inteira de uma vez)
   size_t total_pixels = ( size_t )IMG->nrow * IMG->ncol;
   unsigned char *buffer_gigante = ( unsigned char * ) malloc( total_pixels );

   if ( !buffer_gigante ) {
      fprintf( stderr, "Erro: Falha de memória ao alocar o buffer de gravação.\n" );
      fclose( out );
      return;
   }

   // 3. Conversão CPU-Bound Paralelizada
   // As threads transformam a matriz 2D de 'ints' em um vetor 1D contíguo de bytes
   // #pragma omp parallel for schedule(static)
   for ( int i = 0; i < IMG->nrow; i++ ) {

      // Calcula o deslocamento da linha no buffer gigante
      size_t offset_linha = ( size_t )i * IMG->ncol;

      for ( int j = 0; j < IMG->ncol; j++ ) {
         // Cada thread acessa seu espaço exclusivo no buffer_gigante
         buffer_gigante[ offset_linha + j ] = ( unsigned char ) IMG->image[i][j];
      }
   }

   // 4. I/O em Bloco (Disk-Bound)
   // Grava a imagem inteira em uma única martelada no disco
   fwrite( buffer_gigante, 1, total_pixels, out );

   // 5. Limpeza
   free( buffer_gigante );
   fclose( out );
}





void cortar_imagem_ortogonal( const ImagemCinza *IMG, ImagemCinza *img, int x_ini, int y_ini, int largura, int altura ) {
   if ( !IMG || !img || largura <= 0 || altura <= 0 ) return;

   // Proteção de limites
   if ( x_ini < 0 ) x_ini = 0;
   if ( y_ini < 0 ) y_ini = 0;
   if ( x_ini + largura > IMG->ncol ) largura = IMG->ncol - x_ini;
   if ( y_ini + altura > IMG->nrow ) altura = IMG->nrow - y_ini;

   if ( img->image != NULL ) liberar_matriz_pixels( img->image, img->nrow );

   img->ncol = largura;
   img->nrow = altura;
   img->image = alocar_matriz_pixels( img->nrow, img->ncol );
   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );
   img->max = IMG->max;

   // Cópia ultra-rápida de blocos de memória linha a linha (sem interpolação)
   // #pragma omp parallel for schedule(static)
   for ( int i = 0; i < altura; i++ ) {
      memcpy( img->image[i], &IMG->image[y_ini + i][x_ini], largura * sizeof( int ) );
   }
}





void cortar_imagem_ortogonal_colorida( const ImagemColorida *IMG, ImagemColorida *img,
                                       int x_ini, int y_ini, int largura, int altura ) {
   if ( !IMG || !img || largura <= 0 || altura <= 0 ) return;

   // Proteção de limites rigorosa
   if ( x_ini < 0 ) x_ini = 0;
   if ( y_ini < 0 ) y_ini = 0;
   if ( x_ini + largura > IMG->ncol ) largura = IMG->ncol - x_ini;
   if ( y_ini + altura > IMG->nrow ) altura = IMG->nrow - y_ini;

   if ( img->image != NULL ) liberar_matriz_pixels_colorida( img->image, img->nrow );

   img->ncol = largura;
   img->nrow = altura;
   img->image = alocar_matriz_pixels_colorida( img->nrow, img->ncol );
   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );
   img->max = IMG->max;

   // Cópia pura de memória, muito mais rápida que interpolação para cortes retos
   // #pragma omp parallel for schedule(static)
   for ( int i = 0; i < altura; i++ ) {
      memcpy( img->image[i], &IMG->image[y_ini + i][x_ini], largura * sizeof( PixelRGB ) );
   }
}





/*
 * Converte um intervalo de páginas de um PDF para arquivos PNG.
 * - inicio e fim: Índices baseados em 0 (0 = primeira página).
 * - Se fim for -1, a função converte até a última página disponível.
 *
 * Retorna o total de páginas contidas no PDF, ou -1 em caso de erro.
 */
int pdf_para_png_multiplo( const char *caminho_pdf, const char *pasta_destino, const char *prefixo_arquivo,
                           int inicio, int fim, double escala ) {
   GError *erro = NULL;

   g_autofree char *uri = g_filename_to_uri( caminho_pdf, NULL, &erro );
   if ( !uri ) {
      g_printerr( "[ERRO] Falha ao criar URI: %s\n", erro->message );
      g_clear_error( &erro );
      return -1;
   }

   PopplerDocument *doc = poppler_document_new_from_file( uri, NULL, &erro );
   if ( !doc ) {
      g_printerr( "[ERRO] Falha ao carregar o PDF: %s\n", erro->message );
      g_clear_error( &erro );
      return -1;
   }

   // A MÁGICA ACONTECE AQUI: Obtemos o total de páginas
   int total_paginas = poppler_document_get_n_pages( doc );
   if ( total_paginas <= 0 ) {
      g_printerr( "[ERRO] O arquivo PDF está vazio.\n" );
      g_object_unref( doc );
      return -1;
   }

   // Tratamento defensivo dos limites do intervalo
   if ( inicio < 0 ) inicio = 0;
   if ( fim < 0 || fim >= total_paginas ) fim = total_paginas - 1;

   if ( inicio > fim ) {
      g_printerr( "[ERRO] Intervalo de páginas inválido (%d a %d).\n", inicio, fim );
      g_object_unref( doc );
      return -1;
   }

   // O Loop de extração
   for ( int i = inicio; i <= fim; i++ ) {
      PopplerPage *pagina = poppler_document_get_page( doc, i );
      if ( !pagina ) {
         g_printerr( "[AVISO] Não foi possível ler a página %d.\n", i + 1 );
         continue; // Pula para a próxima em caso de corrupção isolada
      }

      double largura, altura;
      poppler_page_get_size( pagina, &largura, &altura );

      int largura_px = ( int )( largura * escala );
      int altura_px  = ( int )( altura * escala );

      cairo_surface_t *superficie = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, largura_px, altura_px );
      cairo_t *cr = cairo_create( superficie );

      // Fundo branco sólido
      cairo_set_source_rgb( cr, 1.0, 1.0, 1.0 );
      cairo_paint( cr );

      // Escala e renderização
      cairo_scale( cr, escala, escala );
      poppler_page_render( pagina, cr );

      // Monta o nome do arquivo dinamicamente (ex: "./dados/prova_001.png")
      g_autofree char *caminho_png = g_strdup_printf( "%s/%s_%03d.png", pasta_destino, prefixo_arquivo, i + 1 );

      cairo_status_t status = cairo_surface_write_to_png( superficie, caminho_png );

      if ( status != CAIRO_STATUS_SUCCESS ) {
         g_printerr( "[ERRO] Falha ao gravar %s: %s\n", caminho_png, cairo_status_to_string( status ) );
      }

      // Limpeza do ciclo atual
      cairo_destroy( cr );
      cairo_surface_destroy( superficie );
      g_object_unref( pagina );
   }

   g_object_unref( doc );

   // Retorna o total de páginas originais do documento para o fluxo do programa
   return total_paginas;
}





/*
 * Converte a primeira página de um PDF para PNG.
 * 'escala' controla a resolução: 1.0 equivale a 72 DPI. Para matrizes de gabarito,
 * valores como 2.0 (144 DPI) ou 4.0 (288 DPI) garantem precisão nos quadradinhos.
 */
gboolean pdf2png( const char *caminho_pdf, const char *caminho_png, double escala ) {
   if ( !caminho_pdf || !caminho_png || escala <= 0 ) return FALSE;

   if ( !g_file_test( caminho_pdf, G_FILE_TEST_EXISTS ) ) {
      return FALSE;
   }

   GError *erro = NULL;

   // 1. Uso do GFile para aceitar caminhos relativos e absolutos com segurança
   g_autoptr( GFile ) arquivo = g_file_new_for_path( caminho_pdf );
   g_autofree char *uri = g_file_get_uri( arquivo );

   if ( !uri ) {
      g_printerr( "[ERRO] Falha ao criar URI para o PDF.\n" );
      return FALSE;
   }

   // 2. Carrega o documento PDF na memória
   PopplerDocument *doc = poppler_document_new_from_file( uri, NULL, &erro );
   if ( !doc ) {
      g_printerr( "[ERRO] Falha ao carregar o PDF: %s\n", erro->message );
      g_clear_error( &erro );
      return FALSE;
   }

   // 3. Pega a primeira página (índice 0)
   PopplerPage *pagina = poppler_document_get_page( doc, 0 );
   if ( !pagina ) {
      g_printerr( "[ERRO] O PDF parece estar vazio ou sem páginas.\n" );
      g_object_unref( doc );
      return FALSE;
   }

   // 4. Obtém as dimensões originais da página em pontos
   double largura, altura;
   poppler_page_get_size( pagina, &largura, &altura );

   int largura_px = ( int )( largura * escala );
   int altura_px  = ( int )( altura * escala );

   // 5. Cria uma superfície de desenho do Cairo
   cairo_surface_t *superficie = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, largura_px, altura_px );
   cairo_t *cr = cairo_create( superficie );

   // 6. Fundo branco sólido para garantir leitura da matriz
   cairo_set_source_rgb( cr, 1.0, 1.0, 1.0 );
   cairo_paint( cr );

   // 7. Renderiza o PDF em alta resolução
   cairo_scale( cr, escala, escala );
   poppler_page_render( pagina, cr );

   // 8. Exporta diretamente para PNG no disco
   cairo_status_t status = cairo_surface_write_to_png( superficie, caminho_png );

   // 9. Limpeza de memória local
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







void rgb2gray( ImagemColorida *PPM, ImagemCinza *PGM ) {
   if ( !PPM || !PGM ) return;

   g_strlcpy( PGM->key, "P5", sizeof( PGM->key ) );
   PGM->ncol = PPM->ncol;
   PGM->nrow = PPM->nrow;
   PGM->max  = PPM->max;

   // 1. Pré-alocação segura da matriz 2D
   PGM->image = alocar_matriz_pixels( PGM->nrow, PGM->ncol );

   // #pragma omp parallel for schedule(static)
   for ( int i = 0; i < PGM->nrow; i++ ) {
      for ( int j = 0; j < PGM->ncol; j++ ) {

         unsigned char r = PPM->image[i][j].r;
         unsigned char g = PPM->image[i][j].g;
         unsigned char b = PPM->image[i][j].b;

         // Luminância Rec.709
         PGM->image[i][j] = ( 2126 * r + 7152 * g + 722 * b ) / 10000;
      }
   }

}



//========================================================================================================//
void imread_gray( ImagemCinza *IMG, const char *arquivo ) {
   if ( !IMG || !arquivo ) return;

   FILE *p = fopen( arquivo, "rb" );
   if ( !p ) {
      fprintf( stderr, "Erro: Não foi possível abrir o arquivo %s.\n", arquivo );
      return;
   }

   // Limite no %s para evitar buffer overflow caso o arquivo esteja corrompido
   if ( fscanf( p, "%9s\n%d %d\n%d\n", IMG->key, &IMG->ncol, &IMG->nrow, &IMG->max ) != 4 ) {
      fprintf( stderr, "Erro: Falha ao ler o cabeçalho PPM do arquivo.\n" );
      fclose( p );
      return;
   }

   sprintf( IMG->key, "P5" );

   // 1. Pré-alocação segura da matriz 2D
   IMG->image = ( int** ) malloc( IMG->nrow * sizeof( int* ) );
   for ( int i = 0; i < IMG->nrow; i++ ) {
      IMG->image[i] = ( int* ) malloc( IMG->ncol * sizeof( int ) );
   }

   // 2. I/O em Bloco (Leitura Massiva)
   size_t total_pixels = ( size_t )IMG->ncol * IMG->nrow;
   unsigned char *buffer_gigante = ( unsigned char * ) malloc( total_pixels * 3 );

   // Lemos todos os pixels de uma única vez. O disco rígido adora isso.
   if ( fread( buffer_gigante, 3, total_pixels, p ) != total_pixels ) {
      fprintf( stderr, "Aviso: Fim de arquivo inesperado. A imagem pode estar cortada.\n" );
   }

   fclose( p ); // O disco já fez o seu trabalho, liberamos o recurso imediatamente!

   // 3. Processamento CPU-Bound Paralelizado (100% Seguro)
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < IMG->nrow; i++ ) {

      // Cada thread calcula matematicamente onde começa a sua linha no buffer gigante
      size_t offset_linha = ( size_t )i * IMG->ncol * 3;

      for ( int j = 0; j < IMG->ncol; j++ ) {
         size_t idx = offset_linha + ( j * 3 );

         // Acesso seguro: cada thread lê de um índice exclusivo, sem colisão
         unsigned char r = buffer_gigante[idx];
         unsigned char g = buffer_gigante[idx + 1];
         unsigned char b = buffer_gigante[idx + 2];

         int luminancia = ( 2126 * r + 7152 * g + 722 * b ) / 10000;
         IMG->image[i][j] = luminancia;
      }
   }

   // Limpeza do buffer temporário
   free( buffer_gigante );
}
//========================================================================================================//



void imread_pgm( ImagemCinza *IMG, const char *arquivo ) {
   if ( !IMG || !arquivo ) return;

   FILE *p = fopen( arquivo, "rb" );
   if ( !p ) {
      g_printerr( "[ERRO] Não foi possível abrir o arquivo %s.\n", arquivo );
      return;
   }

   // 1. Segurança reativada: Se falhar, fecha o arquivo e aborta antes de alocar lixo.
   if ( fscanf( p, "%9s\n%d %d\n%d\n", IMG->key, &IMG->ncol, &IMG->nrow, &IMG->max ) != 4 ) {
      g_printerr( "[ERRO] Falha ao ler o cabeçalho PGM do arquivo %s.\n", arquivo );
      fclose( p );
      return;
   }

   snprintf( IMG->key, sizeof( IMG->key ), "P5" ); // Mais seguro que sprintf

   // 2. Pré-alocação segura da matriz 2D com GLib
   IMG->image = g_new0( int*, IMG->nrow );
   for ( int i = 0; i < IMG->nrow; i++ ) {
      IMG->image[i] = g_new0( int, IMG->ncol );
   }

   // 3. I/O em Bloco (Leitura Massiva)
   size_t total_pixels = ( size_t )IMG->ncol * IMG->nrow;
   unsigned char *buffer_gigante = g_new( unsigned char, total_pixels );

   if ( fread( buffer_gigante, 1, total_pixels, p ) != total_pixels ) {
      g_printerr( "[AVISO] Fim de arquivo inesperado. A imagem %s pode estar cortada.\n", arquivo );
   }
   fclose( p );

   // 4. Processamento CPU-Bound Paralelizado via OpenMP
   // schedule(static) reativado para dividir linhas igualmente entre os núcleos
   // #pragma omp parallel for schedule(static)
   for ( int i = 0; i < IMG->nrow; i++ ) {

      // CORREÇÃO: Em PGM (tons de cinza), temos exatos 1 byte por pixel, sem o '* 3'
      size_t offset_linha = ( size_t )i * IMG->ncol;

      for ( int j = 0; j < IMG->ncol; j++ ) {
         // O acesso é perfeitamente linear e contíguo
         size_t idx = offset_linha + j;

         IMG->image[i][j] = ( int )buffer_gigante[idx];
      }
   }

   g_free( buffer_gigante ); // Limpeza via GLib
}







//========================================================================================================//
void imread( ImagemColorida *img, const char *arquivo ) {
   if ( img == NULL || arquivo == NULL ) {
      fprintf( stderr, "Erro: Ponteiros nulos passados para a função.\n" );
      return;
   }

   FILE *p = fopen( arquivo, "rb" );
   if ( p == NULL ) {
      fprintf( stderr, "Erro: Não foi possível abrir o arquivo %s.\n", arquivo );
      return;
   }

   if ( fscanf( p, "%9s\n%d %d\n%d\n", img->key, &img->ncol, &img->nrow, &img->max ) != 4 ) {
      fprintf( stderr, "Erro ao ler cabeçalho.\n" );
      return;
   }

   // 1. Aloca apenas o "esqueleto" (a coluna de ponteiros das linhas)
   img->image = ( PixelRGB** ) malloc( img->nrow * sizeof( PixelRGB* ) );

   // 2. A MÁGICA: Aloca TODOS os pixels da imagem em um único bloco contíguo (A "carne")
   size_t total_pixels = ( size_t )img->nrow * img->ncol;
   PixelRGB *buffer_gigante = ( PixelRGB* ) malloc( total_pixels * sizeof( PixelRGB ) );

   if ( img->image == NULL || buffer_gigante == NULL ) {
      fprintf( stderr, "Erro: Falha catastrófica de memória.\n" );
      free( img->image );
      free( buffer_gigante );
      return;
   }

   // 3. I/O Absoluto: O disco despeja o arquivo inteiro direto no bloco final
   if ( fread( buffer_gigante, sizeof( PixelRGB ), total_pixels, p ) != total_pixels ) {
      fprintf( stderr, "Aviso: Imagem truncada ou erro de leitura.\n" );
   }

   // 4. Mapeamento dos Ponteiros (Zero-Copy)
   // Extremamente rápido, dispensa paralelização.
   for ( int i = 0; i < img->nrow; i++ ) {
      // A linha 'i' aponta para o endereço exato dentro do buffer gigante
      img->image[i] = &buffer_gigante[i * img->ncol];
   }
}



//========================================================================================================//





