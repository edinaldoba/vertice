#ifndef RGBA_H
#define RGBA_H

#include <gtk/gtk.h>
#include "comum.h" // Presumo que contenha TipoRegistroDiario e StatusAssiduidade

// Funções Wrapper de Conveniência (Traduzem o negócio para a cor semântica)
int cor_fundo_celula_pendente( int tema_ativo, GdkRGBA *cor_out );

gboolean cor_texto_observacao( gboolean eh_inativo, int tema_ativo, int pendencias_e_status, GdkRGBA *cor_out );

int cor_texto_aluno_inativo( int eh_inativo, int tema_ativo, GdkRGBA *cor_out );

int cor_texto_nota_vermelha( double nota, int tema_ativo, GdkRGBA *cor_out );

int cor_texto_tipo_registro( int tipo_registro, int tema_ativo, GdkRGBA *cor_out );

int cor_texto_linha_frequencia( StatusAssiduidade status, int tema_ativo, GdkRGBA *cor_out );

#endif
