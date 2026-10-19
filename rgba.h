#ifndef RGBA_H
#define RGBA_H

#include <gtk/gtk.h>
#include "comum.h" // Presumo que contenha TipoRegistroDiario e StatusAssiduidade

// Funções Wrapper de Conveniência (Traduzem o negócio para a cor semântica)
int cor_fundo_celula_pendente( int tema_ativo, GdkRGBA *cor_out );
int cor_texto_inativo_ou_nota( int eh_inativo, double nota, int tema_ativo, GdkRGBA *cor_out );
int cor_texto_tipo_registro( int tipo_registro, int tema_ativo, GdkRGBA *cor_out );
int cor_texto_linha_frequencia( StatusAssiduidade status, int tema_ativo, GdkRGBA *cor_out );

#endif
