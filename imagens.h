#ifndef IMAGENS_H
#define IMAGENS_H

#include "interface.h"


int gas_processar_imagens( const InterfaceDados *dados, const InterfaceCabecalho *cabecalho );

int omr_processar_imagens( const InterfaceDados *dados, const InterfaceCabecalho *cabecalho );

int cs_omr_processar_imagens( const InterfaceDados *dados, const InterfaceCabecalho *cabecalho );

void corrigir_prova( InterfacePainel *painel, AppContext *ctx );


#endif
