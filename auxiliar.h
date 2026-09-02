#ifndef AUXILIAR_H
#define AUXILIAR_H

#include <gtk/gtk.h>

#include "comum.h"
#include "interface.h"

/* ESTE ARQUIVO É EXCLUSIVO PARA DEPENDÊNCIAS DE INTERFACE.C */


CalendarioData data_de_hoje( void );
long int mapear_data_para_id( int dia, int mes, int ano );
void mapear_id_para_data( long int w, int *dia, int *mes, int *ano );
int foco_periodo_corrente( int escalar_hoje );

void mapear_datas_frequencia( GtkListStore *store, GtkTreeIter *iter, const void *dados, int i );

int obter_foco_inicial( const int limite, const FichaAluno *ficha );
void mapear_alunos( GtkListStore *store, GtkTreeIter *iter, const void *ficha, int i );

int quantidade_diretorios( const char *diretorio );

bool diretorio_existe( const char *caminhodir );

ItemCombo *carregar_diretorios_temas( int qtd_dir, const char *diretorio, int ( *comparadora )( const void *, const void * ) );

void gerar_gabaritos( const char *arquivo, const int qtd_linhas, const int total_questoes, const char *modo );

void renderizar_combo_box_ellipsize( GtkWidget *widget, int qtd_caracteres );

void caminhos_uteis_de_diretorios( const InterfaceDados *dados, CaminhoDiretorio *caminho );

void acessar_e_carregar_ficha_dos_alunos_da_turma( AppContext *ctx );
void remover_conteudo_por_indice( const char *caminho_arquivo, const int indice );
void remover_frequencia_por_data( const char *arquivo_freq, const char *data_alvo );
gchar *formatar_data_extenso( const gchar *data_iso );

int ordenar_turmas_novo_em( const void* a, const void* b );

gint comparar_datas_diario( gconstpointer a, gconstpointer b );
int gravar_diario_binario( const char *caminho_arquivo, const RegistroDiario *registro, int indice_edicao );
int cor_texto_linha_liststore(const RegistroDiario *diario, int tema_ativo, GdkRGBA *cor_out);

int cor_texto_linha_frequencia( StatusAssiduidade status, int tema_ativo, GdkRGBA *cor_out );



#endif
