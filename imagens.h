#ifndef IMAGENS_H
#define IMAGENS_H

#include "interface.h"


int processar_imagens( const InterfaceDados *dados, const LimitesFiltro *limite );

void corrigir_prova( InterfacePainel *painel, const AppContext *ctx );


#endif
