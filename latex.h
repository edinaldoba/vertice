#ifndef LATEX_H
#define LATEX_H

#include "interface.h"

void quadro_de_respostas( GString *tex, const char *aluno, int numero, const uint8_t id, char direcao,
                          gboolean assinalar_nome_numero, const InterfaceDados *dados, const InterfaceCabecalho *cabecalho );

int imagens_corrigidas( const char *gab, const MapeamentoGabarito *info, const AppContext *ctx, const char *nome_base );


void anexar_circulo_alternativa_colorida( GString *tex, gint intensidade, const gchar *texto );



void anexar_tema_pagina_quadrados( GString *tex, gint k );
void anexar_cabecalho_questao_quadrados( GString *tex, const gchar *cor_texto, gint k, gint num_questao );

void anexar_tema_pagina_linhas( GString *tex, gint k );
void anexar_cabecalho_questao_linhas( GString *tex, const gchar *cor_texto, gint k, gint num_questao );

void anexar_tema_pagina_senoides( GString *tex, gint k );
void anexar_cabecalho_questao_senoides( GString *tex, const gchar *cor_texto, gint k, gint num_questao );



void anexar_tema_pagina_trapezios( GString *tex, gint k );
void anexar_cabecalho_questao_trapezios( GString *tex, gint k, gint num_questao );

void anexar_tema_pagina_ondas( GString *tex, gint k );
void anexar_cabecalho_questao_ondas( GString *tex, gint k, gint num_questao );

void anexar_tema_pagina_circulos( GString *tex, gint k );
void anexar_cabecalho_questao_circulos( GString *tex, gint k, gint num_questao );

void anexar_tema_pagina_hexagonos( GString *tex, gint k );
void anexar_cabecalho_questao_hexagonos( GString *tex, gint k, gint num_questao );


#endif
