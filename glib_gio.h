#ifndef GLIB_GIO_H
#define GLIB_GIO_H

#include <glib.h>
#include "interface.h"


gboolean gio_copiar_arquivo( const gchar *caminho_origem, const gchar *caminho_destino );

gboolean gio_mover_arquivo( const gchar *caminho_origem, const gchar *caminho_destino );

void g_xdg_open( const char *caminho );

void g_system_async( const char *comando );

void g_pdfunite( const char *diretorio, const char **arquivos, const int qtd_arquivos, const char *saida );

void g_pdflatex_parallel( const char *dir_compile );

GPtrArray *listar_subpastas_recursivo( const char *caminho_pai, GPtrArray *lista_caminhos );


#endif
