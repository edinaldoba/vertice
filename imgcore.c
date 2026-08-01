/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <poppler.h>
#include <cairo.h>

#include "imgcore.h"



// 1. Salva o estado atual dos alertas do GCC
#pragma GCC diagnostic push

// 2. Manda o GCC ignorar a falta de protótipos temporariamente
#pragma GCC diagnostic ignored "-Wmissing-prototypes"

// 3. Inclui a biblioteca externa que causa o alerta
#define STB_IMAGE_IMPLEMENTATION
#include "./include/thirdparty/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./include/thirdparty/stb_image_write.h"

// 4. Restaura os alertas do GCC para o padrão restrito do seu Makefile
#pragma GCC diagnostic pop








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






// 1. A função agora retorna 'char *'
char *trocar_extensao( const char *entrada, const char *extensao ) {
   // Usa g_strrchr (procura char) que é mais rápido que g_strrstr (procura string)
   const char *ponto = g_strrstr( entrada, "." );

   if ( ponto != NULL ) {
      // Cria uma substring do início até o ponto e concatena a nova extensão
      char *base = g_strndup( entrada, ponto - entrada );
      char *resultado = g_strdup_printf( "%s.%s", base, extensao );
      g_free( base );
      return resultado;
   }

   // Se a imagem veio sem extensão, apenas anexa
   return g_strdup_printf( "%s.%s", entrada, extensao );
}






// Retorna TRUE se a conversão/cópia funcionou perfeitamente, FALSE caso contrário
gboolean converter_para_png( const char *origem, const char *destino ) {
   if ( !origem || !destino ) return FALSE;

   // Se já é PNG, fazemos uma cópia rápida de arquivo sem gastar CPU com re-encoding
   if ( g_str_has_suffix( origem, ".png" ) ) {
      gchar *conteudo = NULL;
      gsize tamanho = 0;

      // Lê o arquivo inteiro para a RAM
      if ( g_file_get_contents( origem, &conteudo, &tamanho, NULL ) ) {
         // Grava no destino
         gboolean sucesso = g_file_set_contents( destino, conteudo, tamanho, NULL );
         g_free( conteudo );

         if ( !sucesso ) {
            g_printerr( "Falha ao gravar a cópia PNG no destino: %s\n", destino );
            return FALSE;
         }
         return TRUE;
      } else {
         g_printerr( "Falha ao ler o arquivo PNG de origem: %s\n", origem );
         return FALSE;
      }
   }

   int largura, altura, canais;

   // 1. Carrega a imagem original forçando 3 canais (RGB) para padronizar a saída
   unsigned char *pixels = stbi_load( origem, &largura, &altura, &canais, 3 );
   if ( !pixels ) {
      g_printerr( "Erro ao carregar a imagem na RAM via stb: %s\n", origem );
      return FALSE;
   }

   // 2. Escreve os pixels diretamente no formato PNG
   int sucesso = stbi_write_png( destino, largura, altura, 3, pixels, largura * 3 );

   // 3. Limpa a matriz de pixels da memória imediatamente
   stbi_image_free( pixels );

   if ( !sucesso ) {
      g_printerr( "Erro ao gravar o arquivo PNG de destino: %s\n", destino );
      return FALSE;
   }

   return TRUE;
}





gboolean converter_para_png_otimizado( const char *origem, const char *destino ) {
   if ( !origem || !destino ) return FALSE;

   int largura, altura, canais;

   // 1. Carrega forçando TRÊS canais (RGB). Compatibilidade 100% com PPM P6.
   unsigned char *pixels = stbi_load( origem, &largura, &altura, &canais, 3 );
   if ( !pixels ) {
      g_printerr( "Erro ao carregar a imagem: %s\n", origem );
      return FALSE;
   }

   size_t total_pixels = ( size_t )largura * altura;

   #pragma omp parallel for schedule(static)
   for ( size_t i = 0; i < total_pixels; i++ ) {
      size_t idx = i * 3;
      unsigned char *p = &pixels[idx];

      // Limiar de limpeza do fundo (Quase branco vira Branco Absoluto)
      if ( p[0] >= 220 && p[1] >= 220 && p[2] >= 220 ) {
         p[0] = 255;
         p[1] = 255;
         p[2] = 255;
      } else {
         // =========================================================
         // ALGORITMO DE QUANTIZAÇÃO BITWISE DE ALTÍSSIMA VELOCIDADE
         // =========================================================
         // A máscara 0xE0 (11100000 em binário) zera os 5 bits inferiores.
         // Isso reduz cada canal de 256 tons para apenas 8 tons (8x8x8 = 512 cores totais).
         // Para comprimir ainda mais, use 0xC0 (reduz para 64 cores totais).
         p[0] = p[0] & 0xE0; // R
         p[1] = p[1] & 0xE0; // G
         p[2] = p[2] & 0xE0; // B
      }
   }

   stbi_write_png_compression_level = 9;

   // Escreve o arquivo mantendo os 3 canais
   int sucesso = stbi_write_png( destino, largura, altura, 3, pixels, largura * 3 );

   stbi_image_free( pixels );

   if ( !sucesso ) {
      g_printerr( "Erro ao gravar o arquivo PNG: %s\n", destino );
      return FALSE;
   }

   return TRUE;
}







// Retorna TRUE se a conversão funcionou perfeitamente, FALSE caso contrário
gboolean converter_para_ppm( const char *origem, const char *destino ) {
   // 1. Validações defensivas robustas via GLib
   g_return_val_if_fail( origem != NULL, FALSE );
   g_return_val_if_fail( destino != NULL, FALSE );

   int largura, altura, canais;

   // 2. Força o carregamento em 3 canais (RGB), ideal para o PPM P6
   unsigned char *pixels = stbi_load( origem, &largura, &altura, &canais, 3 );
   if ( !pixels ) {
      g_printerr( "Erro ao carregar a imagem na RAM via stb: %s\n", origem );
      return FALSE;
   }

   // 3. Abertura segura de arquivo
   FILE *fp = fopen( destino, "wb" );
   if ( !fp ) {
      g_printerr( "Erro ao criar o arquivo de destino: %s\n", destino );
      stbi_image_free( pixels );
      return FALSE;
   }

   // 4. Escreve o cabeçalho do formato genérico PPM (P6, dimensões e valor máximo)
   fprintf( fp, "P6\n%d %d\n255\n", largura, altura );

   // 5. Escreve os dados binários. Cast para size_t previne overflow em imagens gigantes
   size_t tamanho_buffer = ( size_t )largura * altura * 3;
   size_t gravados = fwrite( pixels, 1, tamanho_buffer, fp );

   if ( gravados != tamanho_buffer ) {
      g_printerr( "Aviso: Falha ao gravar todos os bytes da imagem %s.\n", destino );
      fclose( fp );
      stbi_image_free( pixels );
      return FALSE; // Aborta retornando erro de gravação
   }

   // 6. Limpeza de recursos
   fclose( fp );
   stbi_image_free( pixels );
   // Nota: O g_free(destino) foi removido, pois o gerenciamento da string agora pertence ao chamador da função.

   return TRUE;
}






void salvar_imagem_ppm_p6( const ImagemColorida *IMG, const char *caminho ) {
   if ( !IMG || !caminho ) return;

   FILE *fp = fopen( caminho, "wb" );
   if ( !fp ) return;

   // Grava o cabeçalho P6 padrão
   fprintf( fp, "P6\n%d %d\n%d\n", IMG->ncol, IMG->nrow, IMG->max );

   // Grava as linhas contíguas de pixels
   for ( int i = 0; i < IMG->nrow; i++ ) {
      fwrite( IMG->image[i], sizeof( PixelRGB ), IMG->ncol, fp );
   }

   fclose( fp );
}



void salvar_imagem_png( const ImagemColorida *IMG, const char *caminho ) {
   if ( !IMG || !caminho ) return;

   size_t tamanho_buffer = ( size_t )IMG->ncol * IMG->nrow * 3;
   unsigned char *buffer_linear = ( unsigned char* ) malloc( tamanho_buffer );

   // Serializa a matriz 2D em um vetor 1D contíguo
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < IMG->nrow; i++ ) {
      size_t offset = ( size_t )i * IMG->ncol * 3;
      memcpy( &buffer_linear[offset], IMG->image[i], IMG->ncol * sizeof( PixelRGB ) );
   }

   // Executa a compressão nativa do PNG pela biblioteca de terceiros
   // Parâmetros: caminho, largura, altura, canais (3 = RGB), buffer, stride (largura * 3)
   int sucesso = stbi_write_png( caminho, IMG->ncol, IMG->nrow, 3, buffer_linear, IMG->ncol * 3 );

   if ( !sucesso ) {
      g_printerr( "Erro ao exportar arquivo PNG para: %s\n", caminho );
   }

   free( buffer_linear );
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
   #pragma omp parallel for schedule(static)
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
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < altura; i++ ) {
      memcpy( img->image[i], &IMG->image[y_ini + i][x_ini], largura * sizeof( int ) );
   }
}










void cortar_imagem_ortogonal_colorida( const ImagemColorida *IMG, ImagemColorida *img,
                                       int x_ini, int y_ini, int largura, int altura )
{
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
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < altura; i++ ) {
      memcpy( img->image[i], &IMG->image[y_ini + i][x_ini], largura * sizeof( PixelRGB ) );
   }
}















void binarizar_pgm( ImagemCinza *IMG ) {
   if ( !IMG || !IMG->image ) return;

   // Cache local das variáveis para evitar acesso contínuo à estrutura via ponteiro
   int limiar = IMG->max / 2;
   int max_val = IMG->max;

   // Paralelização limpa e estática
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < IMG->nrow; i++ ) {
      for ( int j = 0; j < IMG->ncol; j++ ) {
         // O processador avalia a condição em tempo recorde na memória cache
         IMG->image[i][j] = ( IMG->image[i][j] > limiar ) ? max_val : 0;
      }
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

// #define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < PGM->nrow; i++ ) {
      for ( int j = 0; j < PGM->ncol; j++ ) {

         unsigned char r = PPM->image[i][j].r;
         unsigned char g = PPM->image[i][j].g;
         unsigned char b = PPM->image[i][j].b;

         // PGM->image[i][j] = MIN3( r, g, b );

         // Luminância Rec.709
         PGM->image[i][j] = ( 2126*r + 7152*g + 722*b ) / 10000;
      }
   }

// #undef MIN3
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

         int luminancia = ( 2126*r + 7152*g + 722*b ) / 10000;
         IMG->image[i][j] = luminancia;
      }
   }

   // Limpeza do buffer temporário
   free( buffer_gigante );
}
//========================================================================================================//







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





