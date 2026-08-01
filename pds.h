#ifndef PDS_H
#define PDS_H

#include "comum.h"



void extrair_regiao_por_ancoras( const ImagemCinza *IMG, ImagemCinza *img, const IndiceMatriz *ancora );

void extrair_regiao_colorida_por_ancoras( const ImagemColorida *IMG, ImagemColorida *img, const IndiceMatriz *ancora );

void rotacionar_imagem( const ImagemCinza *IMG, ImagemCinza *img, float angulo_graus );

void rotacionar_imagem_colorida( const ImagemColorida *IMG, ImagemColorida *img, float angulo_graus );

void cortar_imagem_bilinear( const ImagemCinza *IMG, ImagemCinza *img, const IndiceMatriz *ancora );

void cortar_imagem_colorida_bilinear( const ImagemColorida *IMG, ImagemColorida *img, const IndiceMatriz *ancora );

void redimensionar_imagem_bilinear( ImagemCinza *origem, ImagemCinza *destino, int dim );

void redimensionar_imagem_colorida_bilinear( ImagemColorida *origem, ImagemColorida *destino, int dim );

void aplicar_filtro_gaussiano_2d( const ImagemCinza *IMG, ImagemCinza *img, float sigma );

void binarizar_pgm_metodo_otsu( ImagemCinza *IMG );

int calcular_limiar_ancoras( const ImagemCinza *IMG );




#endif
