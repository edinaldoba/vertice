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
   GPtrArray         *fichas; // Clone na Heap
} MotorProvaSnapshot;
//==========================================================================


void definir_titulo_documento( char *titulo, const InterfaceDados *dados );

void gerar_tex_lista_frequencia( const char *caminho_saida, char *titulo_prova, const GPtrArray *fichas,
                                 const InterfaceDados *dados, const CalendarioData *hoje );

void imagens_para_prova( const int i, int numero, const GPtrArray *fichas,
                         const InterfaceDados *dados, const InterfaceCabecalho *cabecalho );

void provinha( FILE *pm, FILE **pb, const int i, char *titulo_prova, const InterfaceDados *dados, const InterfaceCabecalho *cabecalho,
               const GArray *fichas, const CalendarioData *hoje, const ItemTextoCurto *G );

void prova( const MotorProvaSnapshot *snap, const ItemTextoCurto *G );

void compilacao_latex_e_manipulacao_de_arquivos( const GPtrArray *fichas, const InterfaceDados *dados,
      const CaminhoDiretorio *caminho );



#endif
