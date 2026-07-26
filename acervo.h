#ifndef BANCO_H
#define BANCO_H

#include <gtk/gtk.h>
#include "interface.h"

void compilar_questoes( GtkWidget *widget, InterfacePainel *painel, const AppContext *ctx );

void atualizar_questoes( InterfacePainel *painel, const AppContext *ctx );

void abrir_tema( GtkWidget *widget, InterfacePainel *painel, const AppContext *ctx );

#endif
