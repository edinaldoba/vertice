#ifndef PROVA_H
#define PROVA_H

#include "interface.h"



void definir_titulo_documento( char *titulo, const InterfaceDados *dados );

void gerar_tex_lista_frequencia( const char *caminho_saida, char *titulo_prova, const FichaAluno *ficha,
                                 const InterfaceDados *dados, const CalendarioData *data );

void imagens_para_prova( const int i, int numero, const FichaAluno *ficha, const InterfaceDados *dados, const FocoCoordenadas *foco );

void provinha( FILE *pm, FILE **pb, const int i, char *titulo_prova, const InterfaceDados *dados, const FocoCoordenadas *foco,
               const FichaAluno *ficha, const CalendarioData *data, const ItemTextoCurto *G );

void prova( const InterfaceDados *dados, const FocoCoordenadas *foco, const FichaAluno *ficha,
            const CaminhoDiretorio *caminho, const CalendarioData *data, const ItemTextoCurto *G );

void compilacao_latex_e_manipulacao_de_arquivos( const FichaAluno *ficha, const InterfaceDados *dados,
      const CaminhoDiretorio *caminho );



#endif
