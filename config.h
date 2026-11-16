#ifndef CONFIG_H
#define CONFIG_H

#include "interface.h"

void inicializar_flowbox_atipicidades( AppContext *ctx );
void logica_alunos_atipicos( AppContext *ctx );
void logica_atipicidades( GtkWidget *widget, AppContext *ctx );
void logica_suporte_tea( GtkWidget *widget, AppContext *ctx );

void relatorio_atipicidades( const AppContext *ctx );

#endif
