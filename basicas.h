#ifndef BASICAS_H
#define BASICAS_H

#include "comum.h"
#include "interface.h"


void gerar_sementes( guint32 *sementes );
int* randperm( int n );
void file_permute( FILE **pb, int nquest );
void chrperm( char *str, int n );
void nome_base_gabaritos_bin( char *nome, size_t tam, int turma, int disciplina, int periodo, int prova );


int comparar_item_texto_curto( const void* a, const void* b );

int comparar_item_combo( const void* a, const void* b );

int comparar_mapeamento_gabarito( const void* a, const void* b );

int buscar_indice_bsearch( const void *chave, const void *vetor, size_t n, size_t tamanho_elemento,
                           int ( *comparador )( const void *, const void * ) );

int contar_registros_binarios( const char *filepath, size_t tam );

void display_tempo( const char *descricao, GTimer *cronometro );

int quantidade_arquivos_por_extensao( const char *diretorio, const char *ext );
ItemTextoCurto *carregar_arquivos_por_extensao( const char *diretorio, const char *ext, int n );
int extrair_id_widget( const char *string_id );
int atoi_seguro( const char *num_string );

void ajustar_nomes( const char *arquivo, AppContext *ctx );

int datefind( int a, int *v, int n );
int sdatefind( char a, char *v, int n );

int dia_da_semana( int dia, int mes, int ano );

EstadoArquivo verificar_arquivo( const char *nome_arquivo );
int copiar_arquivo( const char *origem, const char *destino );
int criar_diretorios_cascata( const char *caminho );
int origem_mais_recente( const char *origem, const char *destino );
void preparar_ambiente_latex( const char *recursos_prefix );
void preparar_ambiente_latex_no_disco( void );

void apagar_arquivos_temporarios_latex( void );
void apagar_arquivos_temporarios_latex_nativamente( const char *diretorio, const char *nome_base, int n_tipos );

bool detectar_ubuntu( void );

void obter_resolucao_tela( GtkWidget *widget, int *largura, int *altura );
void obter_resolucao_global( int *largura, int *altura );

int tamanho_fonte( GtkWidget *window );
int tamanho_fonte_px( GtkWidget *window );

int calcular_len_limpo( const char *str, int len_maximo );


#endif

