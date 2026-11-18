#ifndef PROVA_H
#define PROVA_H

#include "interface.h"


//======= GERAÇÃO ASSÍNCRONA DE PROVAS ==================================
// Sub-estrutura que encapsula exclusivamente o "estado isolado" para a Thread.
// ISSO REDUZ A ASSINATURA DA FUNÇÃO prova() DRASTICAMENTE.
typedef struct {
   InterfaceDados     dados;
   InterfaceCabecalho cabecalho;
   CaminhoDiretorio   caminho;
   CalendarioData     hoje;
   GPtrArray         *fichas; // Clone profundo na Heap
} MotorProvaSnapshot;
//==========================================================================


void prova( const MotorProvaSnapshot *snap, const char *G );


#endif
