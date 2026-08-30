#ifndef SEDUC_MA_H
#define SEDUC_MA_H

#include "interface.h"
#include <stdint.h>



void siaep_atualizar_alunos( InterfacePainel *painel, const AppContext *ctx );

void salvar_ficha_aluno( const FichaAluno *aluno, uint32_t cod_aluno );




#endif
