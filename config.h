#ifndef CONFIG_H
#define CONFIG_H

#include "interface.h"

void mapear_sub_aba_atipicidades( AppContext *ctx );
void logica_alunos_atipicos( AppContext *ctx );
void logica_atipicidades( GtkWidget *widget, AppContext *ctx );
void logica_suporte_tea( GtkWidget *widget, AppContext *ctx );

void localizar_cid_atipicidade( InterfacePainel *painel, const InterfaceAtipicidades *atip );

void relatorio_atipicidades( const AppContext *ctx );

#endif
