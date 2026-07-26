#ifndef ASSINCRONO_H
#define ASSINCRONO_H

#include "comum.h"
#include "interface.h"

void disparar_geracao_prova_assincrona( GtkWidget *widget, AppContext *ctx, void *( *funcao_background )( void * ) );
void* thread_gerar_prova_background( void *data );

void disparar_processamento_imagens_assincrono( GtkWidget *widget, AppContext *ctx, void *( *funcao_background )( void * ) );
void* thread_processar_imagens_background( void *data );

void g_pdflatex_parallel_async( GtkWidget *widget, const char *dir_compile, InterfacePainel *painel, const AppContext *ctx );



#endif
