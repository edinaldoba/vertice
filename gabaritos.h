#ifndef GABARITOS_H
#define GABARITOS_H

#include "interface.h"



gboolean decodificar_payload_matriz( MapeamentoGabarito *info, const LimitesFiltro *limite );

gboolean verificar_paridade_matriz( uint32_t payload_lido );

uint32_t extrair_payload_matriz( const ImagemCinza *IMG, char direcao );

void ler_respostas_gabarito( const ImagemCinza *IMG, char direcao, char *respostas_out );

int ler_numero_aluno( const ImagemCinza *IMG, char direcao );

void mudar_numero_na_imagem( float l, float h, float pp, int rot, Ponto2D *C, int n0,
                             int n, unsigned char t, char *img, const InterfaceDados *dados );


#endif
