#ifndef LATEX_H
#define LATEX_H

#include "interface.h"

void quadro_de_respostas( FILE *p, const char *aluno, int numero, const uint8_t id, char direcao, bool assinalar_nome_numero,
                          const InterfaceDados *dados, const FocoCoordenadas *foco );

int imagens_corrigidas( const char *gab, const MapeamentoGabarito *info, const AppContext *ctx, const char *nome_base );

#endif
