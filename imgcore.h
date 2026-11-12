#ifndef IMGCORE_H
#define IMGCORE_H

#include <glib.h>
#include "comum.h"



guint8** alocar_matriz_pixels( gsize nrow, gsize ncol );
PixelRGB** alocar_matriz_pixels_colorida( gsize nrow, gsize ncol );
void liberar_matriz_pixels( guint8 **matriz, gsize nrow );
void liberar_matriz_pixels_colorida( PixelRGB **matriz, gsize nrow );
void liberar_imagem_imread( ImagemColorida *img );

gboolean salvar_imagem_png_nativa( const char *caminho, const ImagemColorida *img );
gboolean carregar_imagem_colorida_nativa( const char *caminho, ImagemColorida *img );

void salvar_imagem_pgm( const ImagemCinza *IMG, const char *arquivo_destino );

void cortar_imagem_ortogonal( const ImagemCinza *IMG, ImagemCinza *img, gsize x_ini, gsize y_ini, gsize largura, gsize altura );
void cortar_imagem_ortogonal_colorida( const ImagemColorida *IMG, ImagemColorida *img,
                                       gsize x_ini, gsize y_ini, gsize largura, gsize altura );

int pdf_para_png_multiplo( const char *caminho_pdf, const char *pasta_destino, const char *prefixo_arquivo,
                           int inicio, int fim, double escala );

gboolean pdf2png( const char *caminho_pdf, const char *caminho_png, double escala );

void rgb2gray( const ImagemColorida *PPM, ImagemCinza *PGM );
void imread_gray( ImagemCinza *IMG, const char *arquivo );
void imread_pgm( ImagemCinza *IMG, const char *arquivo );
void imread( ImagemColorida *img, const char *arquivo );



#endif // IMGCORE_H
