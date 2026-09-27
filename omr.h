#ifndef OMR_H
#define  OMR_H

#include "interface.h"



// Estrutura auxiliar interna para a heurística OMR
typedef struct {
   int min_i, max_i;
   int min_j, max_j;
   int area;
   double centro_i;
   double centro_j;
} BlobInfo;



void ordenar_polar_inline( const BlobInfo *candidatos[4], const BlobInfo *ordenados[4] );

int extrair_quadrados_pretos( const ImagemCinza *img_bin, BlobInfo ancora_final[4], char *direcao_inferida );

gboolean detectar_ancoras_omr( const ImagemCinza *img_bin, IndiceMatriz ancora[4], char *direcao_inferida );

gboolean decodificar_payload_matriz( MapeamentoGabarito *info, const LimitesFiltro *limite );

gboolean verificar_paridade_matriz( uint32_t payload_lido );

uint32_t extrair_payload_matriz( const ImagemCinza *IMG, char direcao );

void ler_respostas_gabarito( const ImagemCinza *IMG, char direcao, char *respostas_out );

int ler_numero_aluno( const ImagemCinza *IMG, char direcao );

void mudar_numero_na_imagem( float l, float h, float pp, int rot, Ponto2D *C, int n0,
                             int n, unsigned char t, char *img, const InterfaceDados *dados );


#endif
