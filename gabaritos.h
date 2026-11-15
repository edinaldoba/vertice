#ifndef GABARITOS_H
#define GABARITOS_H

#include "interface.h"

gchar *nome_base_gabaritos_bin( int turma, int disciplina, int periodo, int prova );

void gerenciar_fluxo_gabaritos( GtkWidget *widget, InterfacePainel *painel, const AppContext *ctx );

#endif
