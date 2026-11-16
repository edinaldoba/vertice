#ifndef GLIB_GIO_H
#define GLIB_GIO_H

#include <glib.h>


gboolean gio_copiar_arquivo( const gchar *caminho_origem, const gchar *caminho_destino );

gboolean gio_mover_arquivo( const gchar *caminho_origem, const gchar *caminho_destino );

void g_xdg_open( const char *caminho );

void g_system_async( const char *comando );

void g_pdfunite( const char *diretorio, GStrv arquivos, const int qtd_arquivos, const char *saida );

void g_pdflatex_serial( const char *dir_compile, const char *arquivo_tex );

void g_pdflatex_parallel( const char *dir_compile, const char *prefixo );

GPtrArray *listar_subpastas_recursivo( const char *caminho_pai, GPtrArray *lista_caminhos );

char* trocar_extensao_arquivo( const char *caminho_orig, const char *nova_ext );

void apagar_temporarios_latex_serial( const gchar *diretorio, const gchar *nome_base, gint n_tipos );

void apagar_temporarios_latex_paralelo( const gchar *diretorio, GStrv nomes_base, gint n_tipos );


#endif
