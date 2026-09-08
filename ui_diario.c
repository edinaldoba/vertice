/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include "ui_diario.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "comum.h"
#include "interface.h"
#include "basicas.h"
#include "callbacks.h"
#include "mensagens.h"
#include "dinamica.h"



// Callback executada periodicamente em segundo plano pelo loop principal da GLib
static gboolean _autosave_diario_cb( gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;

   // Proteção básica contra ponteiros nulos ou encerramento
   if ( !ctx ) return G_SOURCE_REMOVE;

   // Só grava no disco se realmente houver alterações pendentes na RAM
   if ( ctx->dados_modificados && ctx->path_save && ctx->diarios ) {
      salvar_diario( ctx, FALSE );
      ctx->dados_modificados = FALSE; // Reseta a flag após o sucesso
      g_print( "[Autosave] Backup automático de '%s' realizado com sucesso.\n", ctx->path_save );
   }

   // Retorna G_SOURCE_CONTINUE para manter o temporizador rodando
   return G_SOURCE_CONTINUE;
}

// Interrompe o temporizador da GLib com segurança
void parar_autosave_diario( AppContext *ctx ) {
   g_return_if_fail( ctx );

   if ( ctx->autosave_timer_id > 0 ) {
      g_source_remove( ctx->autosave_timer_id );
      ctx->autosave_timer_id = 0;
   }
}

// Inicia a contagem regressiva do Autosave (ex: a cada 5 minutos)
void iniciar_autosave_diario( AppContext *ctx, guint intervalo_minutos ) {
   g_return_if_fail( ctx );

   // Cancela um temporizador anterior se já estiver rodando
   parar_autosave_diario( ctx );

   // Converte minutos para milissegundos (5 min * 60 s * 1000 ms)
   guint intervalo_ms = intervalo_minutos * 60 * 1000;

   // Registra a callback no Main Loop da GLib
   ctx->autosave_timer_id = g_timeout_add( intervalo_ms, _autosave_diario_cb, ctx );
   ctx->dados_modificados = FALSE;
}

// Função centralizada para marcar a RAM como alterada
static void _marcar_diario_modificado( AppContext *ctx ) {
   if ( ctx ) {
      ctx->dados_modificados = TRUE;
   }
}




static void _mapear_datas_frequencia( GtkListStore *store, GtkTreeIter *iter, const void *dados, int i ) {
   g_return_if_fail( store && iter && dados );

   // Cast elegante com 'const' para respeitar a imutabilidade do callback
   const GArray *diarios = ( const GArray * )dados;

   // Proteção de limites de memória
   if ( ( guint )i >= diarios->len ) return;

   // Ponteiro de trabalho local (exatamente como combinamos)
   const RegistroDiario *diario = &g_array_index( diarios, RegistroDiario, i );

   gboolean tem_chamada = ( diario->tipo_registro == TIPO_REGISTRO_AULA_NORMAL ||
                            diario->tipo_registro == TIPO_REGISTRO_AULA_EXTRA );

   gtk_list_store_set( store, iter,
                       0, diario->data,
                       1, ( guint )diario->qtd_aulas,
                       2, tem_chamada,   // FALSE (risca nome/desativa) se for Feriado/Pedagógico
                       3, !tem_chamada,  // TRUE (trava seleção visual) se for Feriado/Pedagógico
                       -1 );
}


/**
 * Converte uma data no formato "AAAA/MM/DD" para "DD - mon - AAAA" (ex: "1983/10/11" -> "11 - out - 1983").
 * Retorna uma nova string alocada dinamicamente que deve ser liberada com g_free() ou usada com g_autofree.
 */
static gchar *_formatar_data_extenso( const gchar *data_iso ) {
   if ( !data_iso ) return NULL;

   // 1. Divide a string "AAAA/MM/DD" usando as barras como separadores
   g_auto( GStrv ) tokens = g_strsplit( data_iso, "/", 3 );

   // Valida se a string possui exatamente 3 partes (Ano, Mês, Dia)
   if ( !tokens || !tokens[0] || !tokens[1] || !tokens[2] ) {
      return NULL;
   }

   int ano = ( int )g_ascii_strtoll( tokens[0], NULL, 10 );
   int mes = ( int )g_ascii_strtoll( tokens[1], NULL, 10 );
   int dia = ( int )g_ascii_strtoll( tokens[2], NULL, 10 );

   // 2. Instancia um GDateTime local com os inteiros extraídos
   g_autoptr( GDateTime ) dt = g_date_time_new_local( ano, mes, dia, 0, 0, 0.0 );
   if ( !dt ) return NULL;

   // 3. Formata usando os especificadores de formatação (%d = dia, %b = mês abreviado, %Y = ano)
   // Por padrão, %b usa a localização do sistema (ex: "out" para outubro em pt_BR)
   return g_date_time_format( dt, "%d - %b - %Y" );
}



static gint _comparar_datas_diario( gconstpointer a, gconstpointer b ) {
   const RegistroDiario *d1 = (const RegistroDiario *)a;
   const RegistroDiario *d2 = (const RegistroDiario *)b;

   int dia1, mes1, ano1, dia2, mes2, ano2;
   // Converte a string "27/08/2026" para inteiros separadamente
   sscanf( d1->data, "%d/%d/%d", &dia1, &mes1, &ano1 );
   sscanf( d2->data, "%d/%d/%d", &dia2, &mes2, &ano2 );

   // Ordenação: Ano -> Mês -> Dia
   if ( ano1 != ano2 ) return ano1 - ano2;
   if ( mes1 != mes2 ) return mes1 - mes2;
   return dia1 - dia2;
}




// Supondo que você tenha um enum para os temas no seu AppContext ou cabeçalho:
// typedef enum { TEMA_DARK_GREEN, TEMA_DEEP_BLUE, TEMA_LIGHT } TemaAtual;

/*
 * Retorna 1 (TRUE) se uma cor customizada foi atribuída,
 * ou 0 (FALSE) se for aula normal (devendo usar a cor padrão).
 */
static int _cor_texto_linha_liststore(const RegistroDiario *diario, int tema_ativo, GdkRGBA *cor_out) {
   if (!diario || !cor_out) return 0;

   // Retorna imediatamente se for Aula Normal (usa a cor padrão do tema)
   if (diario->tipo_registro == 0) return 0;

   // =====================================================================
   // 1. TIPO: Atividade Pedagógica
   // =====================================================================
   if (diario->tipo_registro == 1) {
      if (tema_ativo == 1) { // Deep Blue
         *cor_out = (GdkRGBA){ 0.50, 0.88, 1.00, 1.0 };
      } else if (tema_ativo == 2) { // Light
         *cor_out = (GdkRGBA){ 0.00, 0.00, 1.00, 1.0 };
      } else { // Dark Green (Padrão)
         *cor_out = (GdkRGBA){ 0.39, 0.71, 0.96, 1.0 };
      }
      return 1;
   }

   // =====================================================================
   // 2. TIPO: Feriado
   // =====================================================================
   if (diario->tipo_registro == 2) {
      if (tema_ativo == 1) { // Deep Blue
         *cor_out = (GdkRGBA){ 0.96, 0.40, 0.50, 1.0 };
      } else if (tema_ativo == 2) { // Light
         *cor_out = (GdkRGBA){ 0.85, 0.00, 0.00, 1.0 };
      } else { // Dark Green (Padrão)
         *cor_out = (GdkRGBA){ 0.90, 0.45, 0.45, 1.0 };
      }
      return 1;
   }

   // =====================================================================
   // 3. TIPO: Aula Extra
   // =====================================================================
   if (diario->tipo_registro == 3) {
      if (tema_ativo == 1) { // Deep Blue
         *cor_out = (GdkRGBA){ 1.00, 0.79, 0.16, 1.0 }; // #FFCA28
      } else if (tema_ativo == 2) { // Light
         *cor_out = (GdkRGBA){ 0.62, 0.49, 0.10, 1.0 }; // ##9d7c19
      } else { // Dark Green (Padrão)
         *cor_out = (GdkRGBA){ 1.00, 0.84, 0.31, 1.0 }; // #FFD54F
      }
      return 1;
   }

   return 0; // Fallback de segurança
}



static int _cor_texto_linha_frequencia( StatusAssiduidade status, int tema_ativo, GdkRGBA *cor_out ) {
   if ( !cor_out ) return 0;

   // =====================================================================
   // 1. STATUS COM COR PADRÃO (Retorna 0 para usar a cor nativa do tema)
   // =====================================================================
   if ( status == PRESENTE ) {
      return 0;
   }

   // =====================================================================
   // 2. SEM STATUS: Alunos inativos
   // =====================================================================
   if ( status == SEM_STATUS ) {
      if ( tema_ativo == 1 ) { // Deep Blue (Cinza azulado escuro)
         *cor_out = (GdkRGBA){ 0.40, 0.45, 0.55, 1.0 };
      } else if ( tema_ativo == 2 ) { // Light (Cinza padrão)
         *cor_out = (GdkRGBA){ 0.60, 0.60, 0.60, 1.0 };
      } else { // Dark Green - Padrão (Cinza esverdeado escuro)
         *cor_out = (GdkRGBA){ 0.45, 0.50, 0.45, 1.0 };
      }
      return 1;
   }

   // =====================================================================
   // 3. ALERTAS AVERMELHADOS: Ausente e Suspenso
   // =====================================================================
   if ( status == AUSENTE || status == SUSPENSO || status == FOI_EMBORA ) {
      if ( tema_ativo == 1 ) { // Deep Blue
         *cor_out = (GdkRGBA){ 0.96, 0.40, 0.50, 1.0 };
      } else if ( tema_ativo == 2 ) { // Light
         *cor_out = (GdkRGBA){ 0.85, 0.00, 0.00, 1.0 };
      } else { // Dark Green (Padrão)
         *cor_out = (GdkRGBA){ 0.90, 0.45, 0.45, 1.0 };
      }
      return 1;
   }

   // =====================================================================
   // 4. AVISOS AMARELADOS: Fora de sala e Foi embora
   // =====================================================================
   if ( status == FORA_DE_SALA || status == DISPENSADO ) {
      if ( tema_ativo == 1 ) { // Deep Blue
         *cor_out = (GdkRGBA){ 1.00, 0.79, 0.16, 1.0 }; // #FFCA28
      } else if ( tema_ativo == 2 ) { // Light
         *cor_out = (GdkRGBA){ 0.62, 0.49, 0.10, 1.0 }; // #9D7C19
      } else { // Dark Green (Padrão)
         *cor_out = (GdkRGBA){ 1.00, 0.84, 0.31, 1.0 }; // #FFD54F
      }
      return 1;
   }

   // =====================================================================
   // 5. DESTAQUE (ROXO/PÚRPURA): Atividade Domiciliar (Requer ação extra)
   // =====================================================================
   if ( status == ATIVIDADE_DOMICILIAR ) {
      if ( tema_ativo == 1 ) { // Deep Blue (Lilás vibrante)
         *cor_out = (GdkRGBA){ 0.80, 0.60, 0.95, 1.0 };
      } else if ( tema_ativo == 2 ) { // Light (Roxo escuro)
         *cor_out = (GdkRGBA){ 0.45, 0.15, 0.60, 1.0 };
      } else { // Dark Green - Padrão (Púrpura suave)
         *cor_out = (GdkRGBA){ 0.75, 0.55, 0.85, 1.0 };
      }
      return 1;
   }

   // =====================================================================
   // 5. DESTAQUE (ROXO/PÚRPURA): Atividade Domiciliar (Requer ação extra)
   // =====================================================================
   if ( status == FALTA_JUSTIFICADA ) {
      if ( tema_ativo == 1 ) { // Deep Blue (Verde-azulado / Turquesa vibrante)
         *cor_out = (GdkRGBA){ 0.25, 0.85, 0.75, 1.0 };
      } else if ( tema_ativo == 2 ) { // Light (Verde escuro / Floresta - alto contraste em fundo claro)
         *cor_out = (GdkRGBA){ 0.15, 0.55, 0.25, 1.0 };
      } else { // Dark Green - Padrão (Verde suave / Menta claro - legível em fundo escuro)
         *cor_out = (GdkRGBA){ 0.50, 0.85, 0.55, 1.0 };
      }
      return 1;
   }

   return 0; // Fallback de segurança
}



// ============================================================================
// FUNÇÃO AUXILIAR DE NEGÓCIO (Independente da Interface)
// ============================================================================
// Avalia a string digitada, formata corretamente e retorna uma nova string.
// Se a data for inválida, retorna a data de hoje como fallback.
// O chamador é responsável por liberar a memória (use g_autofree).
gchar* validar_data( const gchar *texto ) {
   g_return_val_if_fail( texto, NULL );

   int dia = 0, mes = 0, ano = 0;
   gboolean data_valida = FALSE;

   if ( texto != NULL ) {
      // Extração flexível (Barras, 6 dígitos ou 8 dígitos)
      if ( sscanf( texto, "%d/%d/%d", &dia, &mes, &ano ) == 3 ) {
         data_valida = TRUE;
      } else if ( sscanf( texto, "%02d%02d%02d", &dia, &mes, &ano ) == 3 && strlen( texto ) == 6 ) {
         data_valida = TRUE;
      } else if ( sscanf( texto, "%02d%02d%04d", &dia, &mes, &ano ) == 3 && strlen( texto ) == 8 ) {
         data_valida = TRUE;
      }
   }

   if ( data_valida ) {
      // Expansão do ano de 2 dígitos para 4 dígitos
      if ( ano < 100 ) {
         ano += ( ano <= 69 ) ? 2000 : 1900;
      }

      // Valida pelo calendário gregoriano da GLib
      if ( ano >= 1900 && ano <= 2100 && g_date_valid_dmy( dia, mes, ano ) ) {
         return g_strdup_printf( "%02d/%02d/%04d", dia, mes, ano ); // SUCESSO
      }
   }

   // MODO FALLBACK: Digitou algo não reconhecido ou inválido (ex: 31/02).
   g_autoptr( GDateTime ) agora = g_date_time_new_now_local();
   return g_date_time_format( agora, "%d/%m/%Y" );
}




void remover_registro_diario_selecionado( AppContext *ctx, int indice_remocao, GtkTreeModel *model, GtkTreeIter *iter ) {
   g_return_if_fail( ctx && ctx->diarios && model && iter );

   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;

   // 1. Proteção de limites da memória
   if ( indice_remocao < 0 || ( guint )indice_remocao >= ctx->diarios->len ) return;

   // 2. Cancela o modo de edição se o professor estiver apagando a aula que está editando
   if ( ui_diario->editando ) {
      g_autoptr( GtkTreePath ) path_remocao = gtk_tree_path_new_from_indices( indice_remocao, -1 );
      g_autoptr( GtkTreePath ) path_edicao  = gtk_tree_model_get_path( model, &ui_diario->iter_em_edicao );

      if ( path_edicao && gtk_tree_path_compare( path_remocao, path_edicao ) == 0 ) {
         ui_diario->editando = FALSE;
         RegistroDiario *d = &g_array_index( ctx->diarios, RegistroDiario, indice_remocao );
         gboolean tipo_feriado    = ( d->tipo_registro == TIPO_REGISTRO_FERIADO );
         gboolean tipo_pedagogico = ( d->tipo_registro == TIPO_REGISTRO_PEDAGOGICO );

         if ( tipo_feriado || tipo_pedagogico ) {
            gtk_entry_set_text( GTK_ENTRY( ui_diario->tema ), "" );
         }
         gtk_entry_set_text( GTK_ENTRY( ui_diario->descricao ), "" );
      }
   }

   // 3. REMOÇÃO ATÔMICA: RAM
   g_array_remove_index( ctx->diarios, indice_remocao );

   // 4. ATUALIZAÇÃO VISUAL: Remove da lista e limpa a seleção
   gtk_list_store_remove( GTK_LIST_STORE( model ), iter );
   GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( ui_diario->treeview_conteudo ) );
   gtk_tree_selection_unselect_all( selection );

}


void registrar_aula( AppContext *ctx ) {
   g_return_if_fail( ctx && ctx->diarios );

   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;
   const int foco_estilo = ctx->dados.interface_style;

   const gchar *tema = gtk_entry_get_text( GTK_ENTRY( ui_diario->tema ) );
   const gchar *descricao = gtk_entry_get_text( GTK_ENTRY( ui_diario->descricao ) );

   if ( g_strcmp0( tema, "" ) == 0 && g_strcmp0( descricao, "" ) == 0 ) return;

   // =====================================================================
   // 1. MONTA A NOVA AULA (A frequência nasce zerada por padrão)
   // =====================================================================
   RegistroDiario nova_aula = {0};
   g_strlcpy( nova_aula.tema, tema, sizeof( nova_aula.tema ) );
   g_strlcpy( nova_aula.descricao, descricao, sizeof( nova_aula.descricao ) );
   nova_aula.tipo_registro = gtk_combo_box_get_active( GTK_COMBO_BOX( ui_diario->tipo_registro ) );

   const gchar *str_data = gtk_entry_get_text( GTK_ENTRY( ui_diario->entry_data ) );
   if ( str_data ) g_strlcpy( nova_aula.data, str_data, sizeof( nova_aula.data ) );

   const gchar *str_ch = gtk_label_get_text( GTK_LABEL( ui_diario->qtd_aulas ) );
   if ( str_ch ) {
      int temp_qtd = 0;
      sscanf( str_ch, "%d", &temp_qtd );
      nova_aula.qtd_aulas = temp_qtd; // Atribuição direta evita pegar o endereço (&) de membro packed
   }

   // INSERE OS CÓDIGOS DOS ALUNOS ANTES DE FAZER A CHAMADA
   // O código do aluno é necessário para a verificação de novos alunos adicionados a turma
   for ( int i = 0; i < ctx->dados.qtd_alunos_total; i++ ) {
      const FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, i );
      nova_aula.chamada[i].cod_aluno = ficha->cod_aluno;
   }

   // =====================================================================
   // 2. INSERE NA RAM E ORDENA
   // =====================================================================
   g_array_append_val( ctx->diarios, nova_aula );
   g_array_sort( ctx->diarios, _comparar_datas_diario );

   _marcar_diario_modificado( ctx ); // Para salvamento automático

   // =====================================================================
   // 3. DESCOBRE A POSIÇÃO PÓS-ORDENAÇÃO
   // =====================================================================
   int novo_indice = ctx->diarios->len - 1; // Fallback para o final
   for ( guint i = 0; i < ctx->diarios->len; i++ ) {
      RegistroDiario *r = &g_array_index( ctx->diarios, RegistroDiario, i );
      // Checa data e tema para garantir que achou a aula certa em dias com múltiplas aulas
      if ( g_strcmp0( r->data, nova_aula.data ) == 0 && g_strcmp0( r->tema, nova_aula.tema ) == 0 ) {
         novo_indice = (int)i;
         break;
      }
   }

   // =====================================================================
   // 4. INSERE CIRURGICAMENTE NA INTERFACE VISUAL
   // =====================================================================
   GtkTreeView *tree_view = GTK_TREE_VIEW( ui_diario->treeview_conteudo );
   GtkListStore *store_view = GTK_LIST_STORE( gtk_tree_view_get_model( tree_view ) );
   GtkTreeIter iter;

   gtk_list_store_insert( store_view, &iter, novo_indice );

   GdkRGBA cor_texto;
   int r = _cor_texto_linha_liststore( &nova_aula, foco_estilo, &cor_texto );

   gtk_list_store_set( store_view, &iter,
                       0, nova_aula.data,      1, nova_aula.qtd_aulas,    2, nova_aula.tema,
                       3, nova_aula.descricao, 4, nova_aula.tipo_registro, 5, (r==0) ? NULL : &cor_texto, -1 );

   // =====================================================================
   // 5. AJUSTES FINAIS DE UI
   // =====================================================================
   GtkTreePath *path_novo = gtk_tree_path_new_from_indices( novo_indice, -1 );
   if ( path_novo ) {
      gtk_tree_view_scroll_to_cell( GTK_TREE_VIEW( ui_diario->treeview_conteudo ), path_novo, NULL, FALSE, 0.0, 0.0 );
      gtk_tree_path_free( path_novo );
   }

   gtk_tree_selection_unselect_all( gtk_tree_view_get_selection( GTK_TREE_VIEW( ui_diario->treeview_conteudo ) ) );

   gtk_entry_set_text( GTK_ENTRY( ui_diario->descricao ), "" );

   RegistroDiario *d = &g_array_index( ctx->diarios, RegistroDiario, novo_indice );
   gboolean tipo_feriado    = ( d->tipo_registro == TIPO_REGISTRO_FERIADO );
   gboolean tipo_pedagogico = ( d->tipo_registro == TIPO_REGISTRO_PEDAGOGICO );
   if ( tipo_feriado || tipo_pedagogico ) {
      gtk_entry_set_text( GTK_ENTRY( ui_diario->tema ), "" );
      gtk_widget_grab_focus( ctx->ui_diario.tema );
   } else {
      gtk_widget_grab_focus( ctx->ui_diario.descricao );
   }

}



// 1. A FUNÇÃO MODULAR (Pode ir para um arquivo .c separado, como diario_ui.c)
void carregar_registro_para_edicao( AppContext *ctx, GtkTreeIter *iter ) {
   g_return_if_fail( ctx && ctx->diarios && iter );

   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;

   GtkTreeView *tree_view = GTK_TREE_VIEW( ui_diario->treeview_conteudo );
   GtkTreeModel *model_view = gtk_tree_view_get_model( tree_view );

   // 1. Descobre o índice da linha clicada na TreeView
   GtkTreePath *path = gtk_tree_model_get_path( model_view, iter );
   if ( !path ) return;

   int indice = gtk_tree_path_get_indices( path )[0];
   gtk_tree_path_free( path );

   // Proteção de limites de segurança
   if ( indice < 0 || ( guint )indice >= ctx->diarios->len ) return;

   // 2. Busca a aula completa direto na memória RAM em O(1)
   RegistroDiario *diario_edicao = &g_array_index( ctx->diarios, RegistroDiario, indice );

   // 3. Popula a interface visual rapidamente
   gtk_entry_set_text( GTK_ENTRY( ui_diario->entry_data ), diario_edicao->data );

   g_autofree gchar *str_ch = g_strdup_printf( "%d h", diario_edicao->qtd_aulas );
   gtk_label_set_text( GTK_LABEL( ui_diario->qtd_aulas ), str_ch );

   gtk_combo_box_set_active( GTK_COMBO_BOX( ui_diario->tipo_registro ), diario_edicao->tipo_registro );
   gtk_entry_set_text( GTK_ENTRY( ui_diario->tema ), diario_edicao->tema );
   gtk_entry_set_text( GTK_ENTRY( ui_diario->descricao ), diario_edicao->descricao );

   // 4. Atualiza o estado da aplicação
   ui_diario->iter_em_edicao = *iter;
   ui_diario->editando = TRUE;
}



void modificar_registro_aula( AppContext *ctx ) {
   g_return_if_fail( ctx && ctx->diarios );

   InterfaceRegistroDiario *ui = &ctx->ui_diario;
   GtkTreeView *tree_view = GTK_TREE_VIEW( ui->treeview_conteudo );
   GtkTreeModel *model_view = gtk_tree_view_get_model( tree_view );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );

   g_autoptr(GtkTreePath) path_antigo = gtk_tree_model_get_path( model_view, &ui->iter_em_edicao );
   if ( !path_antigo ) return;

   int idx = gtk_tree_path_get_indices( path_antigo )[0];
   if ( idx < 0 || ( guint )idx >= ctx->diarios->len ) return;

   RegistroDiario *reg = &g_array_index( ctx->diarios, RegistroDiario, idx );

   const gchar *tema = gtk_entry_get_text( GTK_ENTRY( ui->tema ) );
   const gchar *desc = gtk_entry_get_text( GTK_ENTRY( ui->descricao ) );

   // Avaliação direta de buffer vazio
   if ( !tema[0] && !desc[0] ) return;

   g_strlcpy( reg->tema, tema, sizeof( reg->tema ) );
   g_strlcpy( reg->descricao, desc, sizeof( reg->descricao ) );
   reg->tipo_registro = gtk_combo_box_get_active( GTK_COMBO_BOX( ui->tipo_registro ) );

   const gchar *str_data = gtk_entry_get_text( GTK_ENTRY( ui->entry_data ) );
   if ( str_data ) g_strlcpy( reg->data, str_data, sizeof( reg->data ) );

   const gchar *str_ch = gtk_label_get_text( GTK_LABEL( ui->qtd_aulas ) );
   if ( str_ch ) reg->qtd_aulas = (int)g_ascii_strtoll( str_ch, NULL, 10 );

   // Clone na stack: g_array_sort invalida o ponteiro 'reg' ao mover blocos de memória
   RegistroDiario reg_clone = *reg;

   gtk_list_store_remove( store_view, &ui->iter_em_edicao );
   g_array_sort( ctx->diarios, _comparar_datas_diario );

   int novo_indice = 0;
   for ( guint i = 0; i < ctx->diarios->len; i++ ) {
      RegistroDiario *r = &g_array_index( ctx->diarios, RegistroDiario, i );
      if ( g_strcmp0( r->data, reg_clone.data ) == 0 &&
            g_strcmp0( r->tema, reg_clone.tema ) == 0 &&
            g_strcmp0( r->descricao, reg_clone.descricao ) == 0 ) {
         novo_indice = ( int )i;
         break;
      }
   }

   GtkTreeIter iter;
   gtk_list_store_insert( store_view, &iter, novo_indice );

   GdkRGBA cor_texto;
   int r_estilo = _cor_texto_linha_liststore( &reg_clone, ctx->dados.interface_style, &cor_texto );

   gtk_list_store_set( store_view, &iter,
                     0, reg_clone.data,
                     1, reg_clone.qtd_aulas,
                     2, reg_clone.tema,
                     3, reg_clone.descricao,
                     4, reg_clone.tipo_registro,
                     5, ( r_estilo == 0 ) ? NULL : &cor_texto,
                     -1 );

   g_autoptr(GtkTreePath) path_novo = gtk_tree_path_new_from_indices( novo_indice, -1 );
   if ( path_novo ) {
      gtk_tree_view_scroll_to_cell( GTK_TREE_VIEW( ui->treeview_conteudo ), path_novo, NULL, FALSE, 0.0, 0.0 );
   }

   ui->editando = FALSE;
   gtk_tree_selection_unselect_all( gtk_tree_view_get_selection( GTK_TREE_VIEW( ui->treeview_conteudo ) ) );
   gtk_entry_set_text( GTK_ENTRY( ui->descricao ), "" );
   // gtk_entry_set_text( GTK_ENTRY( ui->tema ), "" );
}




void ui_restaurar_registros_de_aula( const char *caminho_arquivo, InterfaceRegistroDiario *ui_diario,
                                           const int foco_estilo, gboolean rolagem ) {

   g_return_if_fail( caminho_arquivo && ui_diario );

   GtkTreeView *tree_view = GTK_TREE_VIEW( ui_diario->treeview_conteudo );
   GtkTreeModel *model_view = gtk_tree_view_get_model( tree_view );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );

   // =====================================================================
   // 1. SALVA A POSIÇÃO EXATA DA TELA (Antes do clear)
   // =====================================================================
   GtkTreePath *path_topo = NULL;
   if ( !rolagem ) {
      // Captura o path da linha que está perfeitamente no topo visível no momento
      gtk_tree_view_get_visible_range( tree_view, &path_topo, NULL );
   }

   gtk_list_store_clear( store_view );

   EstadoArquivo estado = verificar_arquivo( caminho_arquivo );
   if ( estado & ( ARQUIVO_INEXISTENTE | ARQUIVO_VAZIO ) ) {
      if ( path_topo ) gtk_tree_path_free( path_topo );
      return;
   }

   // =====================================================================
   // 2. LEITURA ATÔMICA DA GLIB
   // =====================================================================
   g_autofree gchar *conteudo = NULL;
   gsize tamanho = 0;

   if ( !g_file_get_contents( caminho_arquivo, &conteudo, &tamanho, NULL ) ) {
      if ( path_topo ) gtk_tree_path_free( path_topo );
      return;
   }

   int total_registros = tamanho / sizeof( RegistroDiario );
   RegistroDiario *registros = ( RegistroDiario * )conteudo;
   GtkTreeIter iter;

   // =====================================================================
   // 3. RENDERIZAÇÃO
   // =====================================================================
   for ( int i = 0; i < total_registros; i++ ) {
      gtk_list_store_append( store_view, &iter );

      GdkRGBA cor_texto;
      int r = _cor_texto_linha_liststore( &registros[i], foco_estilo, &cor_texto );

      gtk_list_store_set( store_view, &iter,
                          0, registros[i].data,
                          1, registros[i].qtd_aulas,
                          2, registros[i].tema,
                          3, registros[i].descricao,
                          4, registros[i].tipo_registro,
                          5, (r == 0) ? NULL : &cor_texto, -1 );
   }

   // =====================================================================
   // 4. RESTAURA A ROLAGEM
   // =====================================================================
   if ( rolagem && total_registros > 0 ) {
      // Comportamento normal: rola para o último item adicionado
      g_autoptr( GtkTreePath ) path_fim = gtk_tree_model_get_path( model_view, &iter );
      if ( path_fim ) {
         gtk_tree_view_scroll_to_cell( tree_view, path_fim, NULL, FALSE, 0.0, 0.0 );
      }
   } else if ( !rolagem && path_topo ) {
      // Comportamento de atualização visual: devolve para a mesma posição!
      gtk_tree_view_scroll_to_cell( tree_view, path_topo, NULL, TRUE, 0.0, 0.0 );
      gtk_tree_path_free( path_topo ); // Libera a memória após o uso
   }
}





void popular_datas( AppContext *ctx ) {
   g_return_if_fail( ctx && ctx->diarios );

   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;
   int qtd_itens = ( int )ctx->diarios->len;
   int foco = -1; // -1 indica que nenhuma data válida foi encontrada ainda

   // Busca retroativa (do último para o primeiro) usando um laço 'for' limpo e seguro
   for ( int i = qtd_itens - 1; i >= 0; i-- ) {
      RegistroDiario *diario = &g_array_index( ctx->diarios, RegistroDiario, i );

      if ( diario->tipo_registro != TIPO_REGISTRO_FERIADO &&
           diario->tipo_registro != TIPO_REGISTRO_PEDAGOGICO ) {
         foco = i;
         break;
      }
   }

   if ( qtd_itens > 0 && foco >= 0 ) {
      popular_combo_box_generico( ui_diario->combo_data, ctx->diarios, qtd_itens, foco,
                                  ui_diario->handler_combo_data, _mapear_datas_frequencia );

      GtkTreeIter iter;
      GtkComboBox *combo = GTK_COMBO_BOX( ui_diario->combo_data );

      if ( gtk_combo_box_get_active_iter( combo, &iter ) ) {
         GtkTreeModel *model_view = gtk_combo_box_get_model( combo );
         guint qtd_aulas = 0;
         gtk_tree_model_get( model_view, &iter, 1, &qtd_aulas, -1 );

         g_autofree gchar *str_qtd_aulas = meu_gerador_variadico( "<b>%u h</b>", qtd_aulas );
         gtk_label_set_markup( GTK_LABEL( ui_diario->label_ch ), str_qtd_aulas );
      }

   } else {
      // Limpa visualmente se a turma estiver vazia ou só tiver feriados
      popular_combo_box_generico( ui_diario->combo_data, NULL, 0, 0,
                                  ui_diario->handler_combo_data, _mapear_datas_frequencia );

      gtk_label_set_text( GTK_LABEL( ui_diario->label_ch ), "0 h" );

      // Limpa a visualização da TreeView de forma segura
      GtkTreeView *tree_view = GTK_TREE_VIEW( ui_diario->treeview_frequencia );
      GtkTreeModel *model_view = gtk_tree_view_get_model( tree_view );
      if ( model_view ) {
         gtk_list_store_clear( GTK_LIST_STORE( model_view ) );
      }

      // SE NÃO HÁ AULAS REGISTRADAS, ENTÃO O COMBO DOS ALUNOS DEVE SER OCULTADO
      gtk_list_store_clear( GTK_LIST_STORE( gtk_tree_view_get_model( GTK_TREE_VIEW(ctx->ui_diario.treeview_frequencia) ) ) );
      ctx->ui_diario.foco_combo_alunos = -1;
      ctx->ui_diario.limite_combo_alunos = 0;
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ), ctx->ui_diario.foco_combo_alunos );

   }
}


static int _indexar_liststore_modo_por_aluno( const GArray *diarios, int idx_aula ) {
   int idx_linha_liststore = 0;
   for ( int i = 0; i < idx_aula; i++ ) {
      const RegistroDiario *d = &g_array_index( diarios, RegistroDiario, i );
      if ( d->tipo_registro == TIPO_REGISTRO_AULA_NORMAL ||
            d->tipo_registro == TIPO_REGISTRO_AULA_EXTRA ) {
         idx_linha_liststore++;
      }
   }
   return idx_linha_liststore;
}

static void _processar_modo_por_aluno( AppContext *ctx, int idx_aula, const char *str_status,
                                       int tem_cor, GdkRGBA *cor_texto ) {

   GtkTreeView *tree_view = GTK_TREE_VIEW( ctx->ui_diario.treeview_frequencia );
   GtkTreeModel *model_view = gtk_tree_view_get_model( tree_view );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );

   // 1. Atualiza o status visual usando o índice filtrado (idx_linha_liststore)
   int idx_linha_liststore = _indexar_liststore_modo_por_aluno( ctx->diarios, idx_aula );
   g_autoptr( GtkTreePath ) path = gtk_tree_path_new_from_indices( idx_linha_liststore, -1 );
   GtkTreeIter iter;

   if ( gtk_tree_model_get_iter( model_view, &iter, path ) ) {
      gtk_list_store_set( store_view, &iter,
                          3, str_status,
                          5, tem_cor ? cor_texto : NULL,
                          -1 );
   }

   // 2. Busca o índice da próxima aula letiva válida usando o índice absoluto (idx_aula)
   for ( idx_aula = idx_aula + 1; ( guint )idx_aula < ctx->diarios->len; idx_aula++ ) {
      const RegistroDiario *diario = &g_array_index( ctx->diarios, RegistroDiario, idx_aula );

      if ( diario->tipo_registro == TIPO_REGISTRO_AULA_NORMAL ||
           diario->tipo_registro == TIPO_REGISTRO_AULA_EXTRA ) {
         break;
      }
   }

   // 3. Avança o combo absoluto
   if ( ( guint )idx_aula < ctx->diarios->len ) {
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_data ), idx_aula );
   }
}


static void _processar_modo_normal( AppContext *ctx, RegistroDiario *diario, int idx_aluno,
                                    const char *str_status, int tem_cor, GdkRGBA *cor_texto ) {

   GtkTreeView *tree_view = GTK_TREE_VIEW( ctx->ui_diario.treeview_frequencia );
   GtkTreeModel *model_view = gtk_tree_view_get_model( tree_view );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );

   gboolean modo_edicao = FALSE;
   GtkTreeIter iter_view;

   // 1. Verifica se o aluno já está na tela
   if ( gtk_tree_model_get_iter_first( model_view, &iter_view ) ) {
      do {
         int num_lista = 0;
         gtk_tree_model_get( model_view, &iter_view, 0, &num_lista, -1 );

         if ( num_lista == idx_aluno + 1 ) {
            modo_edicao = TRUE;
            break;
         }
      } while ( gtk_tree_model_iter_next( model_view, &iter_view ) );
   }

   // 2. Atualiza a interface e processa os inativos
   if ( modo_edicao ) {
      gtk_list_store_set( store_view, &iter_view, 3, str_status, 5, tem_cor ? cor_texto : NULL, -1 );

      // Inicia a busca pelo próximo aluno ativo
      for ( idx_aluno = idx_aluno + 1; idx_aluno < ctx->dados.qtd_alunos_total; idx_aluno++ ) {
         FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, idx_aluno );
         if ( ficha->ativo ) break;
      }

   } else {
      FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, idx_aluno );
      g_autofree gchar *nasc = _formatar_data_extenso( ficha->nasc );

      gtk_list_store_append( store_view, &iter_view );
      gtk_list_store_set( store_view, &iter_view,
                          0, idx_aluno + 1,
                          1, ficha->aluno,
                          2, nasc,
                          3, str_status,
                          4, !ficha->ativo,
                          5, tem_cor ? cor_texto : NULL, -1 );

      // Processa e renderiza os inativos subsequentes na sequência da chamada
      for ( idx_aluno = idx_aluno + 1; idx_aluno < ctx->dados.qtd_alunos_total; idx_aluno++ ) {
         ficha = &g_array_index( ctx->fichas, FichaAluno, idx_aluno );
         if ( ficha->ativo ) break;

         diario->chamada[idx_aluno].status = SEM_STATUS;

         g_autofree gchar *nasc_inativo = _formatar_data_extenso( ficha->nasc );
         GdkRGBA cor_inativo;
         int tem_cor_inativo = _cor_texto_linha_frequencia( 0, ctx->dados.interface_style, &cor_inativo );

         GtkTreeIter iter_inativo;
         gtk_list_store_append( store_view, &iter_inativo );
         gtk_list_store_set( store_view, &iter_inativo,
                             0, idx_aluno + 1,
                             1, ficha->aluno,
                             2, nasc_inativo,
                             3, ctx->listas.status_assiduidade[0].str,
                             4, !ficha->ativo, // Será TRUE
                             5, tem_cor_inativo ? &cor_inativo : NULL, -1 );
      }
   }

   // 3. Atualiza o limite e avança o combo
   ctx->ui_diario.limite_combo_alunos = gtk_tree_model_iter_n_children( model_view, NULL ) + 1;

   if ( idx_aluno < ctx->dados.qtd_alunos_total ) {
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ), idx_aluno );
   }
}


void registrar_status_assiduidade_frequencia( InterfacePainel *painel, AppContext *ctx, StatusAssiduidade status ) {
   g_return_if_fail( ctx && ctx->diarios && painel );

   // 1. Validação de segurança
   if ( status == SEM_STATUS ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Aviso:" );
      painel->format_subtitulo = meu_gerador_variadico( "Status de assiduidade inválido" );
      painel->format_instrucao = meu_gerador_variadico( "Selecione uma justificativa válida no menu antes de registrar a frequência." );
      criar_mensagem_painel( AVISO, painel );
      return;
   }

   // 2. Extrai os índices ativos
   int idx_aluno = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ) );
   if ( idx_aluno < 0 || idx_aluno >= ctx->dados.qtd_alunos_total ) return;

   int idx_aula = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_data ) );
   if ( idx_aula < 0 || ( guint )idx_aula >= ctx->diarios->len ) return;

   // 3. Atualiza a RAM e aciona o gatilho do Autosave
   RegistroDiario *diario = &g_array_index( ctx->diarios, RegistroDiario, idx_aula );
   diario->chamada[idx_aluno].status = status;
   _marcar_diario_modificado( ctx );

   // 4. Prepara a formatação visual (Texto e Cor)
   const char *str_status = ctx->listas.status_assiduidade[status].str;
   GdkRGBA cor_texto;
   int tem_cor = _cor_texto_linha_frequencia( status, ctx->dados.interface_style, &cor_texto );

   // 5. Roteia para o módulo correspondente
   gboolean modo_por_aluno = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( ctx->ui_diario.check_por_aluno ) );

   if ( modo_por_aluno ) {
      _processar_modo_por_aluno( ctx, idx_aula, str_status, tem_cor, &cor_texto );
   } else {
      _processar_modo_normal( ctx, diario, idx_aluno, str_status, tem_cor, &cor_texto );
   }
}





void renderizar_frequencia_modo_normal( AppContext *ctx, gboolean style_changed ) {
   g_return_if_fail( ctx );

   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;

   int foco = gtk_combo_box_get_active( GTK_COMBO_BOX( ui_diario->combo_data ) );

   if ( foco < 0 || ( guint )foco >= ctx->diarios->len ) {
      return;
   }

   RegistroDiario *diario = &g_array_index( ctx->diarios, RegistroDiario, foco );

   // NOVA TRAVA: Se estiver no modo Aluno, atualizamos o ponteiro e rolamos a tela, mas abortamos a renderização geral.
   if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( ui_diario->check_por_aluno ) ) ) {
      return;
   }

   // =====================================================================
   // 2. PREPARAÇÃO VISUAL: Limpa a tabela
   // =====================================================================
   GtkTreeView *tree_view = GTK_TREE_VIEW( ui_diario->treeview_frequencia );
   GtkListStore *store_view = GTK_LIST_STORE( gtk_tree_view_get_model( tree_view ) );
   gtk_list_store_clear( store_view );

   // =====================================================================
   // 3. RENDERIZAÇÃO UNIFICADA E FILTRO DE PARADA (O(N))
   // =====================================================================
   int linhas_renderizadas = 0;

   for ( int i = 0; i < ctx->dados.qtd_alunos_total; i++ ) {
      FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, i );
      int status_atual = diario->chamada[i].status;

      // 1. REGRA DE PARADA: Só interrompe a varredura se encontrar um aluno ATIVO e PENDENTE
      if ( ficha->ativo && status_atual == SEM_STATUS ) {
         break;
      }

      // 2. PROCESSAMENTO AUTOMÁTICO DE INATIVOS:
      // Se o aluno for inativo, garante que o código dele esteja salvo e atribui SEM_STATUS (0) na RAM
      if ( !ficha->ativo ) {
         // diario->chamada[i].cod_aluno = ficha->cod_aluno;
         diario->chamada[i].status = SEM_STATUS;
         status_atual = SEM_STATUS;
      }

      // 3. RENDERIZAÇÃO NA TREEVIEW:
      // Adiciona o aluno à lista (ativos com seus status e inativos com SEM_STATUS e riscados)
      GtkTreeIter iter;
      gtk_list_store_append( store_view, &iter );

      GdkRGBA cor_texto;
      int tem_cor = _cor_texto_linha_frequencia( status_atual, ctx->dados.interface_style, &cor_texto );

      g_autofree gchar *nasc = _formatar_data_extenso( ficha->nasc );

      gtk_list_store_set( store_view, &iter,
                        0, i + 1,
                        1, ficha->aluno,
                        2, nasc,
                        3, ctx->listas.status_assiduidade[status_atual].str,
                        4, !ficha->ativo, // TRUE para aplicar o risco do GtkCellRendererText
                        5, ( tem_cor == 0 ) ? NULL : &cor_texto,
                        -1 );

      linhas_renderizadas++;
   }

   if ( style_changed ) return;

   // =====================================================================
   // 4. AJUSTES FINAIS UI (Controle de navegação e Rolagem)
   // =====================================================================
   if ( linhas_renderizadas < ctx->dados.qtd_alunos_total ) {
      ui_diario->limite_combo_alunos = linhas_renderizadas + 1;
   } else {
      ui_diario->limite_combo_alunos = ctx->dados.qtd_alunos_total;
   }

   ui_diario->foco_combo_alunos = ui_diario->limite_combo_alunos - 1;
   gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ), ui_diario->foco_combo_alunos );
}


void renderizar_frequencia_modo_por_aluno( AppContext *ctx, gboolean style_changed ) {
   g_return_if_fail( ctx && ctx->diarios );

   int idx_aluno = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ) );
   if ( idx_aluno < 0 || idx_aluno >= ctx->dados.qtd_alunos_total ) {
      return;
   }

   GtkTreeView *tree_view = GTK_TREE_VIEW( ctx->ui_diario.treeview_frequencia );
   GtkListStore *store_view = GTK_LIST_STORE( gtk_tree_view_get_model( tree_view ) );
   gtk_list_store_clear( store_view );

   FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, idx_aluno );

   int proxima_data_pendente = 0;
   gboolean achou_pendente = FALSE;

   for ( guint i = 0; i < ctx->diarios->len; i++ ) {
      RegistroDiario *diario = &g_array_index( ctx->diarios, RegistroDiario, i );

      gboolean tipo_feriado    = ( diario->tipo_registro == TIPO_REGISTRO_FERIADO );
      gboolean tipo_pedagogico = ( diario->tipo_registro == TIPO_REGISTRO_PEDAGOGICO );
      if ( tipo_feriado || tipo_pedagogico ) {
         if ( (guint)proxima_data_pendente == i ) {
            proxima_data_pendente = i + 1;
         }
         continue;
      }

      int idx_st = diario->chamada[idx_aluno].status;

      // Localiza a primeira aula sem chamada para este aluno
      if ( idx_st == SEM_STATUS && !achou_pendente ) {
         proxima_data_pendente = i;
         achou_pendente = TRUE;
      }

      const char *str_status = ctx->listas.status_assiduidade[idx_st].str;
      GdkRGBA cor_texto;
      int r = _cor_texto_linha_frequencia( idx_st, ctx->dados.interface_style, &cor_texto );

      GtkTreeIter iter;
      gtk_list_store_append( store_view, &iter );
      gtk_list_store_set( store_view, &iter,
                          0, idx_aluno + 1,
                          1, ficha->aluno,
                          2, diario->data,       // Coluna 2 agora recebe a DATA
                          3, str_status,
                          4, !ficha->ativo,   // Riscar
                          5, ( r == 0 ) ? NULL : &cor_texto, -1 );
   }

   if ( style_changed ) return;

   // Posiciona o combo_data e a rolagem visual na data pendente
   if ( ctx->diarios->len > 0 ) {
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_data ), proxima_data_pendente );
   }
}



void treeview_frequencia_navegar_modo_por_aluno( const AppContext *ctx, int indice_linha ) {
   g_return_if_fail( ctx );

   int linha_atual = 0;
   int indice_real_diario = -1;
   guint total_diarios = ctx->diarios->len;

   for ( guint i = 0; i < total_diarios; i++ ) {
      RegistroDiario *diario = &g_array_index( ctx->diarios, RegistroDiario, i );

      gboolean tipo_feriado    = ( diario->tipo_registro == TIPO_REGISTRO_FERIADO );
      gboolean tipo_pedagogico = ( diario->tipo_registro == TIPO_REGISTRO_PEDAGOGICO );
      if ( tipo_feriado || tipo_pedagogico ) continue;

      if ( linha_atual == indice_linha ) {
         indice_real_diario = ( int )i;
         break;
      }
      linha_atual++;
   }

   if ( indice_real_diario >= 0 && ctx->ui_diario.combo_data ) {
      g_object_set_data( G_OBJECT( ctx->ui_diario.combo_data ), "programatico", GINT_TO_POINTER( TRUE ) );
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_data ), indice_real_diario );
      g_object_set_data( G_OBJECT( ctx->ui_diario.combo_data ), "programatico", GINT_TO_POINTER( FALSE ) );
   }
}

void treeview_frequencia_navegar_modo_normal( const AppContext *ctx, GtkTreeView *treeview, int indice_linha ) {
   g_return_if_fail( ctx && treeview );

   if ( indice_linha < 0 && indice_linha >= ctx->dados.qtd_alunos_total ) {
      return;
   }

   FichaAluno *ficha = NULL;
   ficha = &g_array_index( ctx->fichas, FichaAluno, indice_linha );

   if ( ficha->ativo ) {
      g_object_set_data( G_OBJECT( ctx->ui_diario.combo_alunos ), "programatico", GINT_TO_POINTER( TRUE ) );
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ), indice_linha );
      g_object_set_data( G_OBJECT( ctx->ui_diario.combo_alunos ), "programatico", GINT_TO_POINTER( FALSE ) );
      return;
   }

   // ALUNO INATIVO: Lógica de repulsão magnética (Pulo Automático)
   int foco_anterior = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ) );
   int target = -1;

   if ( indice_linha > foco_anterior ) {
      // Descendo (seta para baixo ou clique abaixo)
      for ( int i = indice_linha + 1; i < ctx->dados.qtd_alunos_total; i++ ) {
         ficha = &g_array_index( ctx->fichas, FichaAluno, i );
         if ( ficha->ativo ) { target = i; break; }
      }
   } else if ( indice_linha < foco_anterior ) {
      // Subindo (seta para cima ou clique acima)
      for ( int i = indice_linha - 1; i >= 0; i-- ) {
         ficha = &g_array_index( ctx->fichas, FichaAluno, i );
         if ( ficha->ativo ) { target = i; break; }
      }
   }

   // Se não encontrou ninguém na direção (ex: chegou no fim da lista e os últimos são inativos)
   if ( target == -1 ) {
      target = foco_anterior; // Volta para o porto seguro
   }

   // Força a TreeView a pular o inativo e focar no alvo válido.
   // NOTA: Isso dispara 'cursor-changed' novamente de forma limpa,
   // mas como 'target' é ativo, cairá no primeiro IF encerrando a recursão instantaneamente.
   g_autoptr( GtkTreePath ) novo_path = gtk_tree_path_new_from_indices( target, -1 );
   gtk_tree_view_set_cursor( treeview, novo_path, NULL, FALSE );
}



void rolagem_automatica_treeview_frequencia( const AppContext *ctx ) {
   g_return_if_fail( ctx );

   GtkTreeView *treeview = GTK_TREE_VIEW( ctx->ui_diario.treeview_frequencia );
   GtkTreeModel *model = gtk_tree_view_get_model( treeview );

   // Prevenção caso a árvore ainda não tenha sido populada
   if ( !model ) return;

   gboolean modo_por_aluno = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( ctx->ui_diario.check_por_aluno ) );

   // Captura o índice ativo original dos combos
   int ativo = modo_por_aluno ?
               gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_data ) ) :
               gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ) );

   if ( ativo < 0 ) return;

   // === TRADUÇÃO DE ÍNDICE ===
   // Converte o índice absoluto de memória (combo_data) para o índice visual (ListStore filtrado)
   if ( modo_por_aluno ) {
      ativo = _indexar_liststore_modo_por_aluno( ctx->diarios, ativo );
   }

   int linhas = gtk_tree_model_iter_n_children( model, NULL );
   if ( linhas == 0 ) return;

   int foco;

   // Lógica de foco:
   // Se o índice ativo atingiu ou ultrapassou a quantidade de linhas (fim da lista)
   if ( ativo >= linhas ) {
      foco = linhas - 1;
   } else {
      foco = ativo;
   }

   // Proteção extra contra underflow
   if ( foco < 0 ) foco = 0;

   float row_align = 1.0;

   // O g_autoptr cuida da liberação automática de memória do GtkTreePath
   g_autoptr( GtkTreePath ) path = gtk_tree_path_new_from_indices( foco, -1 );

   // TRUE ativa o alinhamento.
   // Penúltimo parâmetro (row_align) controla eixo Y.
   // Último parâmetro (col_align) mantido em 0.0 para evitar pulos horizontais.
   gtk_tree_view_scroll_to_cell( treeview, path, NULL, TRUE, row_align, 0.0 );
}



void selecionar_combo_status( const AppContext *ctx ) {
   g_return_if_fail( ctx );

   int ativo = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ) );
   if ( ativo < 0 ) return;

   int idx_aula = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_data ) );

   if ( idx_aula >= 0 && ( guint )idx_aula < ctx->diarios->len ) {
      const RegistroDiario *diario = &g_array_index( ctx->diarios, RegistroDiario, idx_aula );
      int status_atual = diario->chamada[ativo].status;

      // O índice do combo de status corresponde exatamente ao valor do enum (StatusAssiduidade)
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_status ), status_atual );
   }
}




void carregar_diario( AppContext *ctx ) {
   g_return_if_fail( ctx );

   // 1. Se já existir um diário de outra turma na RAM, liberamos a memória
   if ( ctx->diarios ) {
      g_array_unref( ctx->diarios );
   }

   // 2. Inicializa o array dinâmico vazio
   ctx->diarios = g_array_new( FALSE, FALSE, sizeof( RegistroDiario ) );

   if ( !ctx->path_save ) return;

   // 3. Lê o arquivo de forma atômica direto para a memória
   g_autofree gchar *conteudo = NULL;
   gsize tamanho = 0;

   if ( g_file_get_contents( ctx->path_save, &conteudo, &tamanho, NULL ) ) {
      int total_registros = tamanho / sizeof( RegistroDiario );
      if ( total_registros > 0 ) {
         // Injeta o bloco bruto de memória dentro do GArray de uma só vez (muito rápido)
         g_array_append_vals( ctx->diarios, conteudo, total_registros );

         _marcar_diario_modificado( ctx ); // Para salvamento automático
      }
   }
}


void salvar_diario( AppContext *ctx, gboolean final_save ) {
   g_return_if_fail( ctx );

   // 1. DESCARREGA OS DADOS PENDENTES
   // Se havia um arquivo aberto e dados na RAM, salva antes de mudar a rota
   if ( ctx->path_save && ctx->diarios ) {

      // Opcional, mas de mestre: Garante que os registros sejam salvos no disco
      // ordenados cronologicamente, independente de como o professor inseriu.
      // GG, o ordenamento já está acontendendo a cada novo registro de aula
      // g_array_sort( ctx->diarios, _comparar_datas_diario );

      gsize bytes_para_gravar = ctx->diarios->len * sizeof( RegistroDiario );
      GError *erro = NULL;

      // Escrita atômica segura no disco
      if ( !g_file_set_contents( ctx->path_save, (const gchar *)ctx->diarios->data, bytes_para_gravar, &erro ) ) {
         g_printerr( "Aviso: Falha ao salvar %s: %s\n", ctx->path_save, erro->message );
         g_clear_error( &erro );
      }
   }

   // 2. ATUALIZA O CAMINHO DE SALVAMENTO PARA O PRÓXIMO USO
   if ( !final_save ) {
      g_free( ctx->path_save );
      ctx->path_save = g_build_filename( ctx->caminho.dados, "diario.bin", NULL );
   }
}


// AVALIAÇÕES

void popover_adicionar_avaliacao( AppContext *ctx, const char *texto ) {
   g_return_if_fail( ctx != NULL );
   g_return_if_fail( texto != NULL );

   GtkComboBox *combo = GTK_COMBO_BOX( ctx->ui_diario.combo_avaliacoes );
   GtkTreeModel *model_view = gtk_combo_box_get_model( combo );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );

   // 1. Regra de Limite
   int total = gtk_tree_model_iter_n_children( model_view, NULL );
   if ( total >= 5 ) {
      g_print( "⚠ Limite de 5 avaliações atingido.\n" );
      return;
   }

   gboolean riscar = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( ctx->ui_diario.check_desativar_avaliacao ) );

   MetaAvaliacao meta = {0};
   g_strlcpy( meta.nome_av, texto, sizeof( meta.nome_av ) );
   meta.ativa = !riscar;

   // 2. Manipulação de Binário (Anexar ao final)
   g_autofree char *arquivo = g_build_filename( ctx->caminho.dados, "avaliacoes.bin", NULL );
   gsize tamanho_atual = 0;
   gchar *conteudo_atual = NULL;
   GError *erro = NULL;

   g_file_get_contents( arquivo, &conteudo_atual, &tamanho_atual, NULL );

   // Expande o buffer para caber o novo registro
   gsize novo_tamanho = tamanho_atual + sizeof( MetaAvaliacao );
   gchar *novo_conteudo = g_malloc0( novo_tamanho );

   if ( conteudo_atual ) {
      memcpy( novo_conteudo, conteudo_atual, tamanho_atual );
   }

   memcpy( novo_conteudo + tamanho_atual, &meta, sizeof( MetaAvaliacao ) );

   if ( !g_file_set_contents( arquivo, novo_conteudo, novo_tamanho, &erro ) ) {
      g_print( "Erro ao salvar o binário: %s\n", erro->message );
      g_clear_error( &erro );
   }

   g_free( conteudo_atual );
   g_free( novo_conteudo );

   // 3. Atualiza a interface gráfica
   GtkTreeIter iter;
   gtk_list_store_append( store_view, &iter );
   gtk_list_store_set( store_view, &iter, 0, texto, 1, riscar, -1 );

   gtk_combo_box_set_active_iter( combo, &iter );
   _marcar_diario_modificado( ctx );
}


void popover_editar_avaliacao( AppContext *ctx, const char *texto ) {
   g_return_if_fail( ctx != NULL );
   g_return_if_fail( texto != NULL );

   GtkComboBox *combo = GTK_COMBO_BOX( ctx->ui_diario.combo_avaliacoes );
   int ativo = gtk_combo_box_get_active( combo );

   if ( ativo < 0 ) {
      g_print( "⚠ Nenhuma avaliação selecionada para edição.\n" );
      return;
   }

   GtkTreeModel *model_view = gtk_combo_box_get_model( combo );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );

   gboolean riscar = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( ctx->ui_diario.check_desativar_avaliacao ) );

   MetaAvaliacao meta = {0};
   g_strlcpy( meta.nome_av, texto, sizeof( meta.nome_av ) );
   meta.ativa = !riscar;

   // 2. Manipulação de Binário (Substituição in-place)
   g_autofree char *arquivo = g_build_filename( ctx->caminho.dados, "avaliacoes.bin", NULL );
   gsize tamanho_atual = 0;
   gchar *conteudo_atual = NULL;
   GError *erro = NULL;

   if ( g_file_get_contents( arquivo, &conteudo_atual, &tamanho_atual, NULL ) ) {

      // Proteção contra corrupção: garante que o bloco que vamos editar existe no arquivo
      if ( ( ativo + 1 ) * sizeof( MetaAvaliacao ) <= tamanho_atual ) {

         // Editamos diretamente no buffer carregado (sem alocar um segundo buffer)
         memcpy( conteudo_atual + ( ativo * sizeof( MetaAvaliacao ) ), &meta, sizeof( MetaAvaliacao ) );

         if ( !g_file_set_contents( arquivo, conteudo_atual, tamanho_atual, &erro ) ) {
            g_print( "Erro ao salvar o binário: %s\n", erro->message );
            g_clear_error( &erro );
         }
      } else {
         g_print( "⚠ Arquivo corrompido: índice de edição fora dos limites do arquivo.\n" );
      }
      g_free( conteudo_atual );
   }

   // 3. Atualiza a interface gráfica
   GtkTreeIter iter;
   if ( gtk_tree_model_iter_nth_child( model_view, &iter, NULL, ativo ) ) {
      gtk_list_store_set( store_view, &iter, 0, texto, 1, riscar, -1 );
      gtk_combo_box_set_active_iter( combo, &iter );
   }

   _marcar_diario_modificado( ctx );
}
