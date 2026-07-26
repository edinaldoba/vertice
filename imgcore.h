#ifndef IMGCORE_H
#define IMGCORE_H

#include <glib.h>
#include "comum.h"



int** alocar_matriz_pixels( int nrow, int ncol );
PixelRGB** alocar_matriz_pixels_colorida( int nrow, int ncol );
void liberar_matriz_pixels( int **matriz, int nrow );
void liberar_matriz_pixels_colorida( PixelRGB **matriz, int nrow );
void liberar_imagem_imread( ImagemColorida *img );
char *trocar_extensao( const char *entrada, const char *extensao );

gboolean converter_para_png( const char *origem, const char *destino );
gboolean converter_para_png_otimizado( const char *origem, const char *destino );
gboolean converter_para_ppm( const char *origem, const char *destino );

void salvar_imagem_ppm_p6( const ImagemColorida *IMG, const char *caminho );
void salvar_imagem_png( const ImagemColorida *IMG, const char *caminho );
void salvar_imagem_pgm( ImagemCinza *IMG, const char *arquivo_destino );

void cortar_imagem_ortogonal( const ImagemCinza *IMG, ImagemCinza *img, int x_ini, int y_ini, int largura, int altura );
void cortar_imagem_ortogonal_colorida( const ImagemColorida *IMG, ImagemColorida *img,
                                       int x_ini, int y_ini, int largura, int altura );

void binarizar_pgm( ImagemCinza *IMG );

int pdf_para_png_multiplo( const char *caminho_pdf, const char *pasta_destino, const char *prefixo_arquivo,
                           int inicio, int fim, double escala );

gboolean pdf2png( const char *caminho_pdf, const char *caminho_png, double escala );

void rgb2gray( ImagemColorida *PPM, ImagemCinza *PGM );
void imread_gray( ImagemCinza *IMG, const char *arquivo );
void imread( ImagemColorida *img, const char *arquivo );



#endif // IMGCORE_H
