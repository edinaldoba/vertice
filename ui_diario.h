#ifndef UI_DIARIO_H
#define UI_DIARIO_H

#include "interface.h"

void iniciar_autosave_diario( AppContext *ctx, guint intervalo_minutos );

gchar *validar_data( const gchar *texto );

void remover_registro_diario_selecionado( AppContext *ctx, int indice_remocao, GtkTreeModel *model, GtkTreeIter *iter );
void registrar_aula( AppContext *ctx );
void carregar_registro_para_edicao( AppContext *ctx, GtkTreeIter *iter );
void modificar_registro_aula( AppContext *ctx );

void ui_restaurar_registros_de_aula( const char *caminho_arquivo, InterfaceRegistroDiario *ui_diario,
                                     const int foco_estilo, gboolean rolagem );

void registrar_status_assiduidade_frequencia( InterfacePainel *painel, AppContext *ctx, StatusAssiduidade status );
void renderizar_frequencia_modo_normal( AppContext *ctx, gboolean style_changed );
void renderizar_frequencia_modo_por_aluno( AppContext *ctx, gboolean style_changed );

void treeview_frequencia_navegar_modo_por_aluno( const AppContext *ctx, int indice_linha );
void treeview_frequencia_navegar_modo_normal( const AppContext *ctx, GtkTreeView *treeview, int indice_linha );

void rolagem_automatica_treeview_frequencia( const AppContext *ctx );
void selecionar_combo_status( const AppContext *ctx );

void carregar_diario( AppContext *ctx );

void selecionar_avaliacao( AppContext *ctx, GtkComboBox *combo, int item_ativo );

void carregar_avaliacoes( AppContext *ctx );

void salvar_diario( AppContext *ctx, gboolean final_save );
void salvar_avaliacoes( AppContext *ctx, gboolean final_save );

void desativar_avaliacao( AppContext *ctx, gboolean estado );

// AVALIAÇÕES
void popover_adicionar_avaliacao( AppContext *ctx, const char *texto );
void popover_editar_avaliacao( AppContext *ctx, const char *texto );

gboolean salvar_fichas( AppContext *ctx, gboolean final_save );

gboolean calcular_destino_cursor( guint keyval, GtkTreeModel *model, GtkTreePath *path,
                                  GList *current_col_node, GtkTreeViewColumn **nova_coluna );
void confirmar_edicao_e_mover_cursor( GtkTreeView *tree_view, GtkTreePath *path_destino, GtkTreeViewColumn *col_destino );

void renderizar_nota( AppContext *ctx, GtkCellRendererText *renderer, gchar *path_string, gchar *new_text );
void colar_notas_em_lote( AppContext *ctx, GtkWidget *widget, GtkTreePath *start_path, GtkTreeViewColumn *column );
void carregar_notas_ui_por_periodo( AppContext *ctx );

#endif
