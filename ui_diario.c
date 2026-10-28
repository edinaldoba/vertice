/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include "ui_diario.h"

#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <inttypes.h>

#include "comum.h"
#include "interface.h"
#include "basicas.h"
#include "callbacks.h"
#include "mensagens.h"
#include "dinamica.h"


static gboolean salvar_garray_binario( GArray *array, gsize tamanho_elemento, const gchar *caminho_arquivo );

// Callback executada periodicamente em segundo plano pelo loop principal da GLib
static gboolean _autosave_dados_binarios_cb( gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;

   // Proteção contra ponteiros inválidos
   if ( !ctx ) return G_SOURCE_REMOVE;

   // Se nada foi alterado, apenas mantém o timer ativo sem gastar I/O
   if ( !ctx->dados_modificados ) {
      return G_SOURCE_CONTINUE;
   }

   // 1. Grava o diário se houver dados e caminho válido
   gboolean salvou_algo = salvar_garray_binario( ctx->diarios, sizeof( RegistroDiario ), ctx->path_save_diario );

   // 2. Grava as avaliações se houver dados e caminho válido
   salvou_algo = salvar_garray_binario( ctx->avaliacoes, sizeof( MetaAvaliacao ), ctx->path_save_avaliacao );

   salvou_algo = salvar_fichas( ctx, TRUE );

   // 3. Reseta a flag global APENAS se alguma gravação foi realizada
   if ( salvou_algo ) {
      ctx->dados_modificados = FALSE;
      g_print( "[Autosave] Dados sincronizados no disco com sucesso.\n" );
   }

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
   ctx->autosave_timer_id = g_timeout_add( intervalo_ms, _autosave_dados_binarios_cb, ctx );
   ctx->dados_modificados = FALSE;
}

// Função centralizada para marcar a RAM como alterada
static void _marcar_dados_modificado( AppContext *ctx ) {
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

   gboolean eh_aula_normal = ( diario->tipo_registro == TIPO_REGISTRO_AULA_NORMAL );
   gboolean eh_aula_extra  = ( diario->tipo_registro == TIPO_REGISTRO_AULA_EXTRA );

   gboolean tem_chamada = eh_aula_normal || eh_aula_extra;

   gtk_list_store_set( store, iter,
                       0, diario->data,
                       1, ( guint )diario->qtd_aulas,
                       2, eh_aula_extra,
                       3, !tem_chamada,   // FALSE (risca nome/desativa) se for Feriado/Pedagógico
                       4, tem_chamada,  // TRUE (trava seleção visual) se for Feriado/Pedagógico
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
   const RegistroDiario *d1 = ( const RegistroDiario * )a;
   const RegistroDiario *d2 = ( const RegistroDiario * )b;

   int dia1, mes1, ano1, dia2, mes2, ano2;
   // Converte a string "27/08/2026" para inteiros separadamente
   sscanf( d1->data, "%d/%d/%d", &dia1, &mes1, &ano1 );
   sscanf( d2->data, "%d/%d/%d", &dia2, &mes2, &ano2 );

   // Ordenação: Ano -> Mês -> Dia
   if ( ano1 != ano2 ) return ano1 - ano2;
   if ( mes1 != mes2 ) return mes1 - mes2;
   return dia1 - dia2;
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




/**
 * @brief Remove o registro de aula selecionado com trava de segurança para chamadas já realizadas.
 *
 * @return gboolean TRUE se a remoção foi confirmada e executada; FALSE se foi cancelada ou falhou.
 */
gboolean remover_registro_diario_selecionado( AppContext *ctx, int indice_remocao, GtkTreeModel *model, GtkTreeIter *iter ) {
   g_return_val_if_fail( ctx && ctx->diarios && model && iter, FALSE );

   InterfaceConteudo *conteudo = &ctx->ui_diario.conteudo;

   // 1. Proteção de limites da memória
   if ( indice_remocao < 0 || ( guint )indice_remocao >= ctx->diarios->len ) return FALSE;

   RegistroDiario *d = &g_array_index( ctx->diarios, RegistroDiario, indice_remocao );

   // =========================================================================
   // 2. TRAVA DE SEGURANÇA: Verifica se já existe chamada lançada nesta aula
   // =========================================================================
   gboolean possui_chamada = FALSE;
   for ( int i = 0; i < ctx->dados.qtd_alunos_total && i < 64; i++ ) {
      if ( d->chamada[i].status != SEM_STATUS ) {
         possui_chamada = TRUE;
         break;
      }
   }

   // Se houver presença/ausência gravada, exige confirmação explícita do professor
   if ( possui_chamada ) {
      GtkWidget *treeview = GTK_WIDGET( conteudo->treeview );
      GtkWindow *janela_principal = GTK_WINDOW( gtk_widget_get_toplevel( treeview ) );

      const char *str_detalhe = ( d->tema[0] != '\0' ) ? d->tema :
                                ( ( d->descricao[0] != '\0' ) ? d->descricao : "Registro sem Tema" );

      g_autofree gchar *mensagem = meu_gerador_variadico(
                                      "Você está prestes a remover um registro de aula com frequência já lançada.\n\n"
                                      "<b>DATA:</b> %s\n"
                                      "<b>CONTEÚDO/REGISTRO:</b> %s\n\n"
                                      "⚠️ <b>Atenção:</b> Este registro já possui dados de presença/ausência gravados para a turma. "
                                      "Ao removê-lo, todo o histórico de assiduidade desta aula será permanentemente excluído.\n\n"
                                      "Deseja realmente confirmar a exclusão deste registro?",
                                      d->data, str_detalhe
                                   );

      gboolean confirmar = mostrar_popup_confirmacao( janela_principal, "Confirmação de Exclusão de Aula", mensagem );

      // Se o professor cancelar, aborta a remoção mantendo a RAM e a UI intactas
      if ( !confirmar ) return FALSE;
   }

   // =========================================================================
   // 3. CANCELAMENTO DO MODO DE EDIÇÃO (Caso esteja editando a linha a apagar)
   // =========================================================================
   if ( conteudo->editando ) {
      g_autoptr( GtkTreePath ) path_remocao = gtk_tree_path_new_from_indices( indice_remocao, -1 );
      g_autoptr( GtkTreePath ) path_edicao  = gtk_tree_model_get_path( model, &conteudo->iter_em_edicao );

      if ( path_edicao && gtk_tree_path_compare( path_remocao, path_edicao ) == 0 ) {
         conteudo->editando = FALSE;
         gboolean tipo_feriado    = ( d->tipo_registro == TIPO_REGISTRO_FERIADO );
         gboolean tipo_pedagogico = ( d->tipo_registro == TIPO_REGISTRO_PEDAGOGICO );

         if ( tipo_feriado || tipo_pedagogico ) {
            gtk_entry_set_text( GTK_ENTRY( conteudo->entry_tema ), "" );
         }
         gtk_entry_set_text( GTK_ENTRY( conteudo->entry_descricao ), "" );
      }
   }

   // =========================================================================
   // 4. REMOÇÃO ATÔMICA: RAM E AUTOSAVE
   // =========================================================================
   g_array_remove_index( ctx->diarios, indice_remocao );
   _marcar_dados_modificado( ctx );

   // =========================================================================
   // 5. ATUALIZAÇÃO VISUAL NO GTK
   // =========================================================================
   gtk_list_store_remove( GTK_LIST_STORE( model ), iter );
   GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( conteudo->treeview ) );
   gtk_tree_selection_unselect_all( selection );

   return TRUE;
}






//===================================================================================================
// CONSTRUTOR: Centraliza a inserção no GtkListStore de Conteúdos
//===================================================================================================
static void _inserir_linha_conteudo_ui( GtkListStore *store, GtkTreeIter *iter, int indice,
                                        const RegistroDiario *reg ) {
   if ( indice < 0 ) gtk_list_store_append( store, iter );
   else gtk_list_store_insert( store, iter, indice );

   gtk_list_store_set( store, iter,
                       0, reg->data,
                       1, reg->qtd_aulas,
                       2, reg->tema,
                       3, reg->descricao,
                       4, reg->tipo_registro,
                       -1 );
}

//===================================================================================================
// EXTRATOR: Lê os dados recém-injetados no ListStore e transfere com segurança para a RAM
//===================================================================================================
static void _extrair_linha_liststore_para_ram( GtkListStore *store, GtkTreeIter *iter, RegistroDiario *reg ) {
   gchar *data = NULL, *tema = NULL, *desc = NULL;
   int qtd = 0, tipo = 0;

   // Busca os dados puros diretamente da interface GTK
   gtk_tree_model_get( GTK_TREE_MODEL( store ), iter,
                       0, &data, 1, &qtd, 2, &tema, 3, &desc, 4, &tipo, -1 );

   // Transfere para a struct e auto-libera a memória do GTK
   if ( data ) {
      g_strlcpy( reg->data, data, sizeof( reg->data ) );
      g_free( data );
   }
   if ( tema ) {
      g_strlcpy( reg->tema, tema, sizeof( reg->tema ) );
      g_free( tema );
   }
   if ( desc ) {
      g_strlcpy( reg->descricao, desc, sizeof( reg->descricao ) );
      g_free( desc );
   }

   reg->qtd_aulas = qtd;
   reg->tipo_registro = tipo;
}

//===================================================================================================
// RE-RENDERIZADOR GLOBAL: Redesenha toda a UI com múltiplas chamadas à função de linha
//===================================================================================================
static void _reconstruir_ui_conteudos_da_ram( AppContext *ctx ) {
   InterfaceConteudo *conteudo = &ctx->ui_diario.conteudo;

   GtkTreeView *tree_view = GTK_TREE_VIEW( conteudo->treeview );
   GtkListStore *store = GTK_LIST_STORE( gtk_tree_view_get_model( tree_view ) );

   // Otimização O(1): Desanexa a UI para redesenho em massa instantâneo
   g_object_ref( store );
   gtk_tree_view_set_model( tree_view, NULL );
   gtk_list_store_clear( store );

   GtkTreeIter iter;
   for ( guint i = 0; i < ctx->diarios->len; i++ ) {
      RegistroDiario *reg = &g_array_index( ctx->diarios, RegistroDiario, i );
      _inserir_linha_conteudo_ui( store, &iter, -1, reg );
   }

   gtk_tree_view_set_model( tree_view, GTK_TREE_MODEL( store ) );
   g_object_unref( store );
}



void registrar_aula( AppContext *ctx ) {
   g_return_if_fail( ctx && ctx->diarios );
   InterfaceConteudo *conteudo = &ctx->ui_diario.conteudo;

   const gchar *tema = gtk_entry_get_text( GTK_ENTRY( conteudo->entry_tema ) );
   const gchar *descricao = gtk_entry_get_text( GTK_ENTRY( conteudo->entry_descricao ) );

   if ( g_strcmp0( tema, "" ) == 0 && g_strcmp0( descricao, "" ) == 0 ) return;

   GtkTreeView *tree_view = GTK_TREE_VIEW( conteudo->treeview );
   GtkListStore *store_view = GTK_LIST_STORE( gtk_tree_view_get_model( tree_view ) );
   GtkTreeIter iter_temp;

   int temp_qtd = 0;
   const gchar *str_ch = gtk_label_get_text( GTK_LABEL( conteudo->qtd_aulas ) );
   if ( str_ch ) sscanf( str_ch, "%d", &temp_qtd );

   // =====================================================================
   // 1. INSERE NA UI PRIMEIRO (ListStore como Buffer Primário)
   // =====================================================================
   gtk_list_store_append( store_view, &iter_temp );
   gtk_list_store_set( store_view, &iter_temp,
                       0, gtk_entry_get_text( GTK_ENTRY( conteudo->entry_data ) ),
                       1, temp_qtd,
                       2, tema,
                       3, descricao,
                       4, gtk_combo_box_get_active( GTK_COMBO_BOX( conteudo->tipo_registro ) ),
                       -1 );

   // =====================================================================
   // 2. EXTRAI DA UI PARA A RAM E ORDENA
   // =====================================================================
   RegistroDiario nova_aula = {0};
   _extrair_linha_liststore_para_ram( store_view, &iter_temp, &nova_aula );

   // Popula a frequência dos alunos antes de salvar
   for ( int i = 0; i < ctx->dados.qtd_alunos_total; i++ ) {
      const FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, i );
      nova_aula.chamada[i].cod_aluno = ficha->cod_aluno;
   }

   g_array_append_val( ctx->diarios, nova_aula );
   g_array_sort( ctx->diarios, _comparar_datas_diario );
   _marcar_dados_modificado( ctx );

   // =====================================================================
   // 3. RE-RENDERIZA TODA A UI PERFEITAMENTE ORDENADA
   // =====================================================================
   _reconstruir_ui_conteudos_da_ram( ctx );

   // Acha o índice final da aula para dar o scroll
   int novo_indice = ctx->diarios->len - 1;
   for ( guint i = 0; i < ctx->diarios->len; i++ ) {
      RegistroDiario *r = &g_array_index( ctx->diarios, RegistroDiario, i );
      if ( g_strcmp0( r->data, nova_aula.data ) == 0 && g_strcmp0( r->tema, nova_aula.tema ) == 0 ) {
         novo_indice = ( int )i;
         break;
      }
   }

   // Limpezas de UI e Scroll...
   g_autoptr( GtkTreePath ) path_novo = gtk_tree_path_new_from_indices( novo_indice, -1 );
   if ( path_novo ) {
      gtk_tree_view_scroll_to_cell( tree_view, path_novo, NULL, FALSE, 0.0, 0.0 );
   }
   gtk_tree_selection_unselect_all( gtk_tree_view_get_selection( tree_view ) );

   RegistroDiario *d = &g_array_index( ctx->diarios, RegistroDiario, novo_indice );
   if ( d->tipo_registro == TIPO_REGISTRO_FERIADO || d->tipo_registro == TIPO_REGISTRO_PEDAGOGICO ) {
      gtk_entry_set_text( GTK_ENTRY( conteudo->entry_tema ), "" );
      gtk_widget_grab_focus( conteudo->entry_tema );
   } else {
      gtk_widget_grab_focus( conteudo->entry_descricao );
   }
   gtk_entry_set_text( GTK_ENTRY( conteudo->entry_descricao ), "" );
}


void modificar_registro_aula( AppContext *ctx ) {
   g_return_if_fail( ctx && ctx->diarios );
   InterfaceConteudo *conteudo = &ctx->ui_diario.conteudo;

   GtkTreeView *tree_view = GTK_TREE_VIEW( conteudo->treeview );
   GtkTreeModel *model_view = gtk_tree_view_get_model( tree_view );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );

   g_autoptr( GtkTreePath ) path_antigo = gtk_tree_model_get_path( model_view, &conteudo->iter_em_edicao );
   if ( !path_antigo ) return;

   int idx = gtk_tree_path_get_indices( path_antigo )[0];
   if ( idx < 0 || ( guint )idx >= ctx->diarios->len ) return;

   RegistroDiario *reg = &g_array_index( ctx->diarios, RegistroDiario, idx );

   const gchar *tema = gtk_entry_get_text( GTK_ENTRY( conteudo->entry_tema ) );
   const gchar *desc = gtk_entry_get_text( GTK_ENTRY( conteudo->entry_descricao ) );
   if ( !tema[0] && !desc[0] ) return;

   int temp_qtd = 0;
   const gchar *str_ch = gtk_label_get_text( GTK_LABEL( conteudo->qtd_aulas ) );
   if ( str_ch ) sscanf( str_ch, "%d", &temp_qtd );

   // 1. INJETA AS ALTERAÇÕES DIRETAMENTE NA LINHA DO LISTSTORE
   gtk_list_store_set( store_view, &conteudo->iter_em_edicao,
                       0, gtk_entry_get_text( GTK_ENTRY( conteudo->entry_data ) ),
                       1, temp_qtd,
                       2, tema,
                       3, desc,
                       4, gtk_combo_box_get_active( GTK_COMBO_BOX( conteudo->tipo_registro ) ),
                       -1 );

   // 2. EXTRAI DO LISTSTORE PARA O CLONE DA RAM (Mantém a lista de frequência intacta)
   RegistroDiario reg_clone = *reg;
   _extrair_linha_liststore_para_ram( store_view, &conteudo->iter_em_edicao, &reg_clone );

   // 3. ATUALIZA A RAM GLOBAL E ORDENA
   *reg = reg_clone;
   g_array_sort( ctx->diarios, _comparar_datas_diario );
   _marcar_dados_modificado( ctx );

   // 4. RE-RENDERIZA TUDO (Garante que se a data mudou, ele reposicione visualmente a linha)
   _reconstruir_ui_conteudos_da_ram( ctx );

   // Encontra o novo índice pós-ordenação para o scroll
   int novo_indice = 0;
   for ( guint i = 0; i < ctx->diarios->len; i++ ) {
      RegistroDiario *r = &g_array_index( ctx->diarios, RegistroDiario, i );
      if ( g_strcmp0( r->data, reg_clone.data ) == 0 && g_strcmp0( r->tema, reg_clone.tema ) == 0 ) {
         novo_indice = ( int )i;
         break;
      }
   }

   g_autoptr( GtkTreePath ) path_novo = gtk_tree_path_new_from_indices( novo_indice, -1 );
   if ( path_novo ) {
      gtk_tree_view_scroll_to_cell( tree_view, path_novo, NULL, FALSE, 0.0, 0.0 );
   }

   conteudo->editando = FALSE;
   gtk_tree_selection_unselect_all( gtk_tree_view_get_selection( tree_view ) );
   gtk_entry_set_text( GTK_ENTRY( conteudo->entry_descricao ), "" );
}




void ui_restaurar_registros_de_aula( AppContext *ctx, gboolean rolagem ) {
   g_return_if_fail( ctx );
   InterfaceConteudo *conteudo = &ctx->ui_diario.conteudo;

   g_autofree char *caminho_arquivo = g_build_filename( ctx->caminho.dados, "diario.bin", NULL );


   GtkTreeView *tree_view = GTK_TREE_VIEW( conteudo->treeview );
   GtkTreeModel *model_view = gtk_tree_view_get_model( tree_view );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );

   GtkTreePath *path_topo = NULL;
   if ( !rolagem ) {
      gtk_tree_view_get_visible_range( tree_view, &path_topo, NULL );
   }

   gtk_list_store_clear( store_view );

   EstadoArquivo estado = verificar_arquivo( caminho_arquivo );
   if ( estado & ( ARQUIVO_INEXISTENTE | ARQUIVO_VAZIO ) ) {
      if ( path_topo ) gtk_tree_path_free( path_topo );
      return;
   }

   g_autofree gchar *cont_bin = NULL;
   gsize tamanho = 0;

   if ( !g_file_get_contents( caminho_arquivo, &cont_bin, &tamanho, NULL ) ) {
      if ( path_topo ) gtk_tree_path_free( path_topo );
      return;
   }

   int total_registros = tamanho / sizeof( RegistroDiario );
   RegistroDiario *registros = ( RegistroDiario * )cont_bin;
   GtkTreeIter iter;

   // =====================================================================
   // 3. RENDERIZAÇÃO LIMPA E MODULARIZADA
   // =====================================================================
   for ( int i = 0; i < total_registros; i++ ) {
      // Passa -1 para indicar "append" em lote na inicialização
      _inserir_linha_conteudo_ui( store_view, &iter, -1, &registros[i] );
   }

   if ( rolagem && total_registros > 0 ) {
      g_autoptr( GtkTreePath ) path_fim = gtk_tree_model_get_path( model_view, &iter );
      if ( path_fim ) {
         gtk_tree_view_scroll_to_cell( tree_view, path_fim, NULL, FALSE, 0.0, 0.0 );
      }
   } else if ( !rolagem && path_topo ) {
      gtk_tree_view_scroll_to_cell( tree_view, path_topo, NULL, TRUE, 0.0, 0.0 );
      gtk_tree_path_free( path_topo );
   }
}



// 1. A FUNÇÃO MODULAR (Pode ir para um arquivo .c separado, como diario_ui.c)
void carregar_registro_para_edicao( AppContext *ctx, GtkTreeIter *iter ) {
   g_return_if_fail( ctx && ctx->diarios && iter );

   InterfaceConteudo *conteudo = &ctx->ui_diario.conteudo;

   GtkTreeView *tree_view = GTK_TREE_VIEW( conteudo->treeview );
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
   gtk_entry_set_text( GTK_ENTRY( conteudo->entry_data ), diario_edicao->data );

   g_autofree gchar *str_ch = g_strdup_printf( "%d h", diario_edicao->qtd_aulas );
   gtk_label_set_text( GTK_LABEL( conteudo->qtd_aulas ), str_ch );

   gtk_combo_box_set_active( GTK_COMBO_BOX( conteudo->tipo_registro ), diario_edicao->tipo_registro );
   gtk_entry_set_text( GTK_ENTRY( conteudo->entry_tema ), diario_edicao->tema );
   gtk_entry_set_text( GTK_ENTRY( conteudo->entry_descricao ), diario_edicao->descricao );

   // 4. Atualiza o estado da aplicação
   conteudo->iter_em_edicao = *iter;
   conteudo->editando = TRUE;
}







// FREQUÊNCIA

void popular_datas( AppContext *ctx ) {
   g_return_if_fail( ctx && ctx->diarios );
   InterfaceFrequencia *frequencia = &ctx->ui_diario.frequencia;

   int qtd_itens = ( int )ctx->diarios->len;
   int foco = -1; // -1 indica que nenhuma data válida foi encontrada ainda

   // Busca retroativa (do último para o primeiro) usando um laço 'for' limpo e seguro
   RegistroDiario *diario = NULL;
   for ( int i = qtd_itens - 1; i >= 0; i-- ) {
      diario = &g_array_index( ctx->diarios, RegistroDiario, i );

      if ( diario->tipo_registro != TIPO_REGISTRO_FERIADO &&
            diario->tipo_registro != TIPO_REGISTRO_PEDAGOGICO ) {
         foco = i;
         break;
      }
   }

   if ( qtd_itens > 0 && foco >= 0 ) {
      popular_combo_box_generico( frequencia->combo_data, ctx->diarios, qtd_itens, foco,
                                  frequencia->handler_combo_data, _mapear_datas_frequencia );

      GtkTreeIter iter;
      GtkComboBox *combo = GTK_COMBO_BOX( frequencia->combo_data );

      if ( gtk_combo_box_get_active_iter( combo, &iter ) ) {
         GtkTreeModel *model_view = gtk_combo_box_get_model( combo );
         guint qtd_aulas = 0;
         gboolean eh_extra = FALSE;
         gtk_tree_model_get( model_view, &iter, 1, &qtd_aulas, 2, &eh_extra, -1 );

         g_autofree gchar *str_qtd_aulas = meu_gerador_variadico( "<b>%u h</b>%s", qtd_aulas, eh_extra ? " - extra" : "" );
         gtk_label_set_markup( GTK_LABEL( frequencia->label_ch ), str_qtd_aulas );
      }

   } else {
      // Limpa visualmente se a turma estiver vazia ou só tiver feriados
      popular_combo_box_generico( frequencia->combo_data, NULL, 0, 0,
                                  frequencia->handler_combo_data, _mapear_datas_frequencia );

      gtk_label_set_text( GTK_LABEL( frequencia->label_ch ), "0 h" );

      // Limpa a visualização da TreeView de forma segura
      GtkTreeView *tree_view = GTK_TREE_VIEW( frequencia->treeview );
      GtkTreeModel *model_view = gtk_tree_view_get_model( tree_view );
      if ( model_view ) {
         gtk_list_store_clear( GTK_LIST_STORE( model_view ) );
      }

      // SE NÃO HÁ AULAS REGISTRADAS, ENTÃO O COMBO DOS ALUNOS DEVE SER OCULTADO
      gtk_list_store_clear( GTK_LIST_STORE( gtk_tree_view_get_model( GTK_TREE_VIEW( frequencia->treeview ) ) ) );
      frequencia->foco_combo_alunos = -1;
      frequencia->limite_combo_alunos = 0;
      gtk_combo_box_set_active( GTK_COMBO_BOX( frequencia->combo_alunos ), frequencia->foco_combo_alunos );
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

static void _processar_modo_por_aluno( AppContext *ctx, int idx_aula, const char *str_status, int status_num ) {
   InterfaceFrequencia *frequencia = &ctx->ui_diario.frequencia;

   GtkTreeView *tree_view = GTK_TREE_VIEW( frequencia->treeview );
   GtkTreeModel *model_view = gtk_tree_view_get_model( tree_view );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );

   int idx_linha_liststore = _indexar_liststore_modo_por_aluno( ctx->diarios, idx_aula );
   g_autoptr( GtkTreePath ) path = gtk_tree_path_new_from_indices( idx_linha_liststore, -1 );
   GtkTreeIter iter;

   if ( gtk_tree_model_get_iter( model_view, &iter, path ) ) {
      // Injeção MVC pura! Coluna 5 recebe o int.
      gtk_list_store_set( store_view, &iter,
                          3, str_status,
                          5, status_num,
                          -1 );
   }

   for ( idx_aula = idx_aula + 1; ( guint )idx_aula < ctx->diarios->len; idx_aula++ ) {
      const RegistroDiario *diario = &g_array_index( ctx->diarios, RegistroDiario, idx_aula );
      if ( diario->tipo_registro == TIPO_REGISTRO_AULA_NORMAL ||
            diario->tipo_registro == TIPO_REGISTRO_AULA_EXTRA ) {
         break;
      }
   }

   if ( ( guint )idx_aula < ctx->diarios->len ) {
      gtk_combo_box_set_active( GTK_COMBO_BOX( frequencia->combo_data ), idx_aula );
   }
}

static void _processar_modo_normal( AppContext *ctx, RegistroDiario *diario, int idx_aluno,
                                    const char *str_status, int status_num ) {
   InterfaceFrequencia *frequencia = &ctx->ui_diario.frequencia;

   GtkTreeView *tree_view = GTK_TREE_VIEW( frequencia->treeview );
   GtkTreeModel *model_view = gtk_tree_view_get_model( tree_view );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );

   gboolean modo_edicao = FALSE;
   GtkTreeIter iter_view;

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

   if ( modo_edicao ) {
      gtk_list_store_set( store_view, &iter_view,
                          3, str_status,
                          5, status_num,
                          -1 );

      for ( idx_aluno = idx_aluno + 1; idx_aluno < ctx->dados.qtd_alunos_total; idx_aluno++ ) {
         FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, idx_aluno );
         if ( ficha->ativo ) break;
      }

   } else {
      FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, idx_aluno );
      g_autofree gchar *nasc = _formatar_data_extenso( ficha->nasc );
      const char *fonte_numerica = "Monospace 19px";

      gtk_list_store_append( store_view, &iter_view );
      gtk_list_store_set( store_view, &iter_view,
                          0, idx_aluno + 1,
                          1, ficha->aluno,
                          2, nasc,
                          3, str_status,
                          4, !ficha->ativo,
                          5, status_num, // Status numérico puro
                          6, fonte_numerica,
                          -1 );

      for ( idx_aluno = idx_aluno + 1; idx_aluno < ctx->dados.qtd_alunos_total; idx_aluno++ ) {
         ficha = &g_array_index( ctx->fichas, FichaAluno, idx_aluno );
         if ( ficha->ativo ) break;

         diario->chamada[idx_aluno].status = SEM_STATUS;

         GtkComboBox *combo_status = GTK_COMBO_BOX( frequencia->combo_status );
         g_autofree gchar *texto_status = combo_box_text_get_text_at( combo_status, 0 );

         g_autofree gchar *nasc_inativo = _formatar_data_extenso( ficha->nasc );

         GtkTreeIter iter_inativo;
         gtk_list_store_append( store_view, &iter_inativo );
         gtk_list_store_set( store_view, &iter_inativo,
                             0, idx_aluno + 1,
                             1, ficha->aluno,
                             2, nasc_inativo,
                             3, texto_status,
                             4, !ficha->ativo,
                             5, SEM_STATUS, // Status numérico puro para inativos
                             6, fonte_numerica,
                             -1 );
      }
   }

   frequencia->limite_combo_alunos = gtk_tree_model_iter_n_children( model_view, NULL ) + 1;

   if ( idx_aluno < ctx->dados.qtd_alunos_total ) {
      gtk_combo_box_set_active( GTK_COMBO_BOX( frequencia->combo_alunos ), idx_aluno );
   } else if ( idx_aluno == ctx->dados.qtd_alunos_total ) {
      rolagem_automatica_treeview_frequencia( ctx );
   }
}

void registrar_status_assiduidade_frequencia( InterfacePainel *painel, AppContext *ctx, StatusAssiduidade status ) {
   g_return_if_fail( ctx && ctx->diarios && painel );
   InterfaceFrequencia *frequencia = &ctx->ui_diario.frequencia;

   int idx_aluno = gtk_combo_box_get_active( GTK_COMBO_BOX( frequencia->combo_alunos ) );
   if ( idx_aluno < 0 || idx_aluno >= ctx->dados.qtd_alunos_total ) return;

   int idx_aula = gtk_combo_box_get_active( GTK_COMBO_BOX( frequencia->combo_data ) );
   if ( idx_aula < 0 || ( guint )idx_aula >= ctx->diarios->len ) return;

   RegistroDiario *diario = &g_array_index( ctx->diarios, RegistroDiario, idx_aula );
   diario->chamada[idx_aluno].status = status;
   _marcar_dados_modificado( ctx );

   GtkComboBox *combo_status = GTK_COMBO_BOX( frequencia->combo_status );
   g_autofree gchar *texto_status = combo_box_text_get_text_at( combo_status, status );

   // O roteamento agora passa o número do status, eliminando os GdkRGBA intermediários!
   gboolean modo_por_aluno = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( frequencia->check_por_aluno ) );
   if ( modo_por_aluno ) {
      _processar_modo_por_aluno( ctx, idx_aula, texto_status, status );
   } else {
      _processar_modo_normal( ctx, diario, idx_aluno, texto_status, status );
   }
}

void renderizar_frequencia_modo_normal( AppContext *ctx ) {
   g_return_if_fail( ctx );
   InterfaceFrequencia *frequencia = &ctx->ui_diario.frequencia;

   int foco = gtk_combo_box_get_active( GTK_COMBO_BOX( frequencia->combo_data ) );
   if ( foco < 0 || ( guint )foco >= ctx->diarios->len ) return;

   RegistroDiario *diario = &g_array_index( ctx->diarios, RegistroDiario, foco );

   if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( frequencia->check_por_aluno ) ) ) return;

   GtkTreeView *tree_view = GTK_TREE_VIEW( frequencia->treeview );
   GtkTreeModel *model_view = gtk_tree_view_get_model( tree_view );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );

   // Trava de segurança O(1)
   g_object_ref( store_view );
   gtk_tree_view_set_model( tree_view, NULL );
   gtk_list_store_clear( store_view );

   int linhas_renderizadas = 0;

   for ( int i = 0; i < ctx->dados.qtd_alunos_total; i++ ) {
      FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, i );
      int status_atual = diario->chamada[i].status;

      if ( ficha->ativo && status_atual == SEM_STATUS ) break;

      if ( !ficha->ativo ) {
         diario->chamada[i].status = SEM_STATUS;
         status_atual = SEM_STATUS;
      }

      GtkTreeIter iter;
      gtk_list_store_append( store_view, &iter );

      g_autofree gchar *nasc = _formatar_data_extenso( ficha->nasc );
      const char *fonte_numerica = "Monospace 19px";

      GtkComboBox *combo_status = GTK_COMBO_BOX( frequencia->combo_status );
      g_autofree gchar *texto_status = combo_box_text_get_text_at( combo_status, status_atual );

      gtk_list_store_set( store_view, &iter,
                          0, i + 1,
                          1, ficha->aluno,
                          2, nasc,
                          3, texto_status,
                          4, !ficha->ativo,
                          5, status_atual, // Injeta apenas o inteiro!
                          6, fonte_numerica,
                          -1 );

      linhas_renderizadas++;
   }

   gtk_tree_view_set_model( tree_view, model_view );
   g_object_unref( store_view );

   if ( linhas_renderizadas < ctx->dados.qtd_alunos_total ) {
      frequencia->limite_combo_alunos = linhas_renderizadas + 1;
   } else {
      frequencia->limite_combo_alunos = ctx->dados.qtd_alunos_total;
   }

   for ( int i = frequencia->limite_combo_alunos - 1; i >= 0; i-- ) {
      FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, i );
      if ( ficha->ativo ) {
         frequencia->foco_combo_alunos = i;
         gtk_combo_box_set_active( GTK_COMBO_BOX( frequencia->combo_alunos ), i );
         break;
      }
   }
}

void renderizar_frequencia_modo_por_aluno( AppContext *ctx ) {
   g_return_if_fail( ctx && ctx->diarios );
   InterfaceFrequencia *frequencia = &ctx->ui_diario.frequencia;

   int idx_aluno = gtk_combo_box_get_active( GTK_COMBO_BOX( frequencia->combo_alunos ) );
   if ( idx_aluno < 0 || idx_aluno >= ctx->dados.qtd_alunos_total ) return;

   GtkTreeView *tree_view = GTK_TREE_VIEW( frequencia->treeview );
   GtkTreeModel *model_view = gtk_tree_view_get_model( tree_view );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );

   g_object_ref( store_view );
   gtk_tree_view_set_model( tree_view, NULL );
   gtk_list_store_clear( store_view );

   FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, idx_aluno );
   int proxima_data_pendente = 0;
   gboolean achou_pendente = FALSE;

   for ( guint i = 0; i < ctx->diarios->len; i++ ) {
      RegistroDiario *diario = &g_array_index( ctx->diarios, RegistroDiario, i );

      gboolean tipo_feriado    = ( diario->tipo_registro == TIPO_REGISTRO_FERIADO );
      gboolean tipo_pedagogico = ( diario->tipo_registro == TIPO_REGISTRO_PEDAGOGICO );
      if ( tipo_feriado || tipo_pedagogico ) {
         if ( ( guint )proxima_data_pendente == i ) proxima_data_pendente = i + 1;
         continue;
      }

      int idx_st = diario->chamada[idx_aluno].status;

      if ( idx_st == SEM_STATUS && !achou_pendente ) {
         proxima_data_pendente = i;
         achou_pendente = TRUE;
      }

      GtkComboBox *combo_status = GTK_COMBO_BOX( frequencia->combo_status );
      g_autofree gchar *texto_status = combo_box_text_get_text_at( combo_status, idx_st );

      GtkTreeIter iter;
      gtk_list_store_append( store_view, &iter );
      gtk_list_store_set( store_view, &iter,
                          0, idx_aluno + 1,
                          1, ficha->aluno,
                          2, diario->data,
                          3, texto_status,
                          4, !ficha->ativo,
                          5, idx_st, // Injeta apenas o inteiro!
                          -1 );
   }

   gtk_tree_view_set_model( tree_view, model_view );
   g_object_unref( store_view );

   if ( ctx->diarios->len > 0 ) {
      gtk_combo_box_set_active( GTK_COMBO_BOX( frequencia->combo_data ), proxima_data_pendente );
   }
}






void treeview_frequencia_navegar_modo_por_aluno( const AppContext *ctx, int indice_linha ) {
   g_return_if_fail( ctx );
   const InterfaceFrequencia *frequencia = &ctx->ui_diario.frequencia;

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

   if ( indice_real_diario >= 0 && frequencia->combo_data ) {
      g_object_set_data( G_OBJECT( frequencia->combo_data ), "programatico", GINT_TO_POINTER( TRUE ) );
      gtk_combo_box_set_active( GTK_COMBO_BOX( frequencia->combo_data ), indice_real_diario );
      g_object_set_data( G_OBJECT( frequencia->combo_data ), "programatico", GINT_TO_POINTER( FALSE ) );
   }
}

void treeview_frequencia_navegar_modo_normal( const AppContext *ctx, GtkTreeView *treeview, int indice_linha ) {
   g_return_if_fail( ctx && treeview );
   const InterfaceFrequencia *frequencia = &ctx->ui_diario.frequencia;

   if ( indice_linha < 0 && indice_linha >= ctx->dados.qtd_alunos_total ) {
      return;
   }

   FichaAluno *ficha = NULL;
   ficha = &g_array_index( ctx->fichas, FichaAluno, indice_linha );

   if ( ficha->ativo ) {
      g_object_set_data( G_OBJECT( frequencia->combo_alunos ), "programatico", GINT_TO_POINTER( TRUE ) );
      gtk_combo_box_set_active( GTK_COMBO_BOX( frequencia->combo_alunos ), indice_linha );
      g_object_set_data( G_OBJECT( frequencia->combo_alunos ), "programatico", GINT_TO_POINTER( FALSE ) );
      return;
   }

   // ALUNO INATIVO: Lógica de repulsão magnética (Pulo Automático)
   int foco_anterior = gtk_combo_box_get_active( GTK_COMBO_BOX( frequencia->combo_alunos ) );
   int target = -1;

   if ( indice_linha > foco_anterior ) {
      // Descendo (seta para baixo ou clique abaixo)
      for ( int i = indice_linha + 1; i < ctx->dados.qtd_alunos_total; i++ ) {
         ficha = &g_array_index( ctx->fichas, FichaAluno, i );
         if ( ficha->ativo ) {
            target = i;
            break;
         }
      }
   } else if ( indice_linha < foco_anterior ) {
      // Subindo (seta para cima ou clique acima)
      for ( int i = indice_linha - 1; i >= 0; i-- ) {
         ficha = &g_array_index( ctx->fichas, FichaAluno, i );
         if ( ficha->ativo ) {
            target = i;
            break;
         }
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
   const InterfaceFrequencia *frequencia = &ctx->ui_diario.frequencia;

   GtkTreeView *treeview = GTK_TREE_VIEW( frequencia->treeview );
   GtkTreeModel *model = gtk_tree_view_get_model( treeview );

   // Prevenção caso a árvore ainda não tenha sido populada
   if ( !model ) return;

   gboolean modo_por_aluno = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( frequencia->check_por_aluno ) );

   // Captura o índice ativo original dos combos
   int ativo = modo_por_aluno ?
               gtk_combo_box_get_active( GTK_COMBO_BOX( frequencia->combo_data ) ) :
               gtk_combo_box_get_active( GTK_COMBO_BOX( frequencia->combo_alunos ) );

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


/**
 * @brief Sincroniza o estado do `combo_status` com o registro de assiduidade.
 *
 * Atualiza o seletor de status de presença (combo_status) na aba de frequência
 * ("page_frequencia") com base na combinação do aluno selecionado (`combo_alunos`)
 * e da data da aula (`combo_data`).
 *
 * @param[in] ctx Ponteiro para o contexto global da aplicação (AppContext).
 */
void selecionar_combo_status( const AppContext *ctx ) {
   g_return_if_fail( ctx );
   const InterfaceFrequencia *frequencia = &ctx->ui_diario.frequencia;

   int ativo = gtk_combo_box_get_active( GTK_COMBO_BOX( frequencia->combo_alunos ) );
   if ( ativo < 0 ) return;

   int idx_aula = gtk_combo_box_get_active( GTK_COMBO_BOX( frequencia->combo_data ) );

   if ( idx_aula >= 0 && ( guint )idx_aula < ctx->diarios->len ) {
      const RegistroDiario *diario = &g_array_index( ctx->diarios, RegistroDiario, idx_aula );
      int status_atual = diario->chamada[ativo].status;

      // O índice do combo de status corresponde exatamente ao valor do enum (StatusAssiduidade)
      gtk_combo_box_set_active( GTK_COMBO_BOX( frequencia->combo_status ), status_atual );
   }
}




// Retorna TRUE se conseguiu ler e injetar dados na memória
static gboolean carregar_garray_binario( GArray **array_ptr, gsize tamanho_elemento, const gchar *caminho_arquivo ) {
   if ( *array_ptr ) {
      g_array_unref( *array_ptr );
   }

   *array_ptr = g_array_new( FALSE, FALSE, tamanho_elemento );

   if ( !caminho_arquivo ) return FALSE;

   g_autofree gchar *conteudo = NULL;
   gsize tamanho = 0;

   if ( g_file_get_contents( caminho_arquivo, &conteudo, &tamanho, NULL ) ) {
      int total_registros = tamanho / tamanho_elemento;
      if ( total_registros > 0 ) {
         g_array_append_vals( *array_ptr, conteudo, total_registros );
         return TRUE;
      }
   }

   return FALSE;
}

static gboolean salvar_garray_binario( GArray *array, gsize tamanho_elemento, const gchar *caminho_arquivo ) {
   if ( !array || !caminho_arquivo ) return FALSE;

   gsize bytes_para_gravar = array->len * tamanho_elemento;
   GError *erro = NULL;

   if ( !g_file_set_contents( caminho_arquivo, ( const gchar * )array->data, bytes_para_gravar, &erro ) ) {
      g_printerr( "Aviso: Falha ao salvar %s: %s\n", caminho_arquivo, erro->message );
      g_clear_error( &erro );
      return FALSE;
   }
   return TRUE;
}


// =====================================================================
// DIÁRIO
// =====================================================================
void carregar_diario( AppContext *ctx ) {
   g_return_if_fail( ctx );
   if ( carregar_garray_binario( &ctx->diarios, sizeof( RegistroDiario ), ctx->path_save_diario ) ) {
      _marcar_dados_modificado( ctx );
   }
}

void salvar_diario( AppContext *ctx, gboolean final_save ) {
   g_return_if_fail( ctx );

   salvar_garray_binario( ctx->diarios, sizeof( RegistroDiario ), ctx->path_save_diario );

   if ( !final_save ) {
      g_free( ctx->path_save_diario );
      ctx->path_save_diario = g_build_filename( ctx->caminho.dados, "diario.bin", NULL );
   }
}

// =====================================================================
// AVALIAÇÕES
// =====================================================================
static void mapper_avaliacao_diario( GtkListStore *store, GtkTreeIter *iter, const void *dados_array, int index ) {
   g_return_if_fail( store != NULL );
   g_return_if_fail( iter != NULL );
   g_return_if_fail( dados_array != NULL );

   // Cast do bloco contíguo de dados para a struct MetaAvaliacao
   const MetaAvaliacao *avaliacoes = ( const MetaAvaliacao * )dados_array;
   const MetaAvaliacao *meta = &avaliacoes[index];

   // Inverte a lógica: se ativa == FALSE, então riscar (strikethrough) == TRUE
   gboolean riscar = !meta->ativa;

   // Preenche a linha do ListStore
   gtk_list_store_set( store, iter,
                       0, meta->nome_av,
                       1, riscar,
                       -1 );
}


static void _visibilidade_avaliacoes( const AppContext *ctx, GtkTreeView *treeview ) {
   // 3. Atualiza a visibilidade dos 5 pares de colunas (Av/Rec)
   for ( int i = 0; i < 5; i++ ) {
      gboolean visivel = FALSE;

      // Verifica se o índice existe no GArray e se a avaliação está ativa
      if ( ctx->avaliacoes && ( guint )i < ctx->avaliacoes->len ) {
         MetaAvaliacao *meta = &g_array_index( ctx->avaliacoes, MetaAvaliacao, i );
         visivel = meta->ativa;
      }

      // Calcula os índices das colunas no TreeView (ex: i=0 -> 2,3 | i=1 -> 4,5)
      int idx_col_av = 2 + ( i * 2 );
      int idx_col_rec = 3 + ( i * 2 );

      GtkTreeViewColumn *col_av = gtk_tree_view_get_column( treeview, idx_col_av );
      GtkTreeViewColumn *col_rec = gtk_tree_view_get_column( treeview, idx_col_rec );

      if ( col_av ) gtk_tree_view_column_set_visible( col_av, visivel );
      if ( col_rec ) gtk_tree_view_column_set_visible( col_rec, visivel );
   }
}



void selecionar_avaliacao( AppContext *ctx, GtkComboBox *combo, int item_ativo ) {
   InterfaceAvaliacao *avaliacao = &ctx->ui_diario.avaliacao;
   // -----------------------------------------------------------------
   // REGRA 2: Mostra a ordem da avaliação selecionada no label
   // -----------------------------------------------------------------
   avaliacao->foco_avaliacao = item_ativo;

   GtkTreeModel *model = gtk_combo_box_get_model( combo );
   GtkTreeIter iter;

   if ( gtk_combo_box_get_active_iter( combo, &iter ) ) {
      gboolean riscado = FALSE;

      // 1. Lê a coluna 1 do ListStore (TRUE se estiver desativada/riscada)
      gtk_tree_model_get( model, &iter, 1, &riscado, -1 );

      GtkWidget *check = avaliacao->check_desativar_avaliacao;
      gulong handler = avaliacao->handler_check_desativar;

      // 2. Bloqueia o sinal temporariamente para evitar disparo falso do autosave
      if ( handler > 0 ) {
         g_signal_handler_block( check, handler );
      }

      // 3. Atualiza a interface da caixa de seleção
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check ), riscado );

      // 4. Desbloqueia o sinal para que cliques reais do professor voltem a funcionar
      if ( handler > 0 ) {
         g_signal_handler_unblock( check, handler );
      }
   }
}



void carregar_avaliacoes( AppContext *ctx ) {
   g_return_if_fail( ctx );
   InterfaceAvaliacao *avaliacao = &ctx->ui_diario.avaliacao;

   GtkWidget *check = avaliacao->check_desativar_avaliacao;
   gulong handler = avaliacao->handler_check_desativar;

   if ( handler > 0 ) {
      g_signal_handler_block( check, handler );
   }

   // 1. Carrega o binário para a memória RAM (GArray)
   if ( carregar_garray_binario( &ctx->avaliacoes, sizeof( MetaAvaliacao ), ctx->path_save_avaliacao ) ) {

      // 2. Se houver dados, repassa o ponteiro interno do GArray para popular o UI
      if ( ctx->avaliacoes && ctx->avaliacoes->len > 0 ) {
         popular_combo_box_generico(
            avaliacao->combo_avaliacao,
            ctx->avaliacoes->data,                    // Bloco contíguo das structs
            ctx->avaliacoes->len,                     // Quantidade exata de itens
            0,                                        // Foco padrão
            avaliacao->handler_combo_avaliacao,  // Silencia o sinal durante o processo
            mapper_avaliacao_diario
         );

         MetaAvaliacao *meta = &g_array_index( ctx->avaliacoes, MetaAvaliacao, 0 );
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check ), !meta->ativa );
      }
      _marcar_dados_modificado( ctx );

   } else {
      popular_combo_box_generico( avaliacao->combo_avaliacao, ctx->avaliacoes->data, 0, 0,
                                  avaliacao->handler_combo_avaliacao, mapper_avaliacao_diario );

      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check ), FALSE );
   }

   if ( handler > 0 ) {
      g_signal_handler_unblock( check, handler );
   }

   // Assumindo que o ponteiro do seu TreeView está no contexto
   GtkTreeView *treeview = GTK_TREE_VIEW( avaliacao->treeview );
   _visibilidade_avaliacoes( ctx, treeview );

}


void salvar_avaliacoes( AppContext *ctx, gboolean final_save ) {
   g_return_if_fail( ctx );

   salvar_garray_binario( ctx->avaliacoes, sizeof( MetaAvaliacao ), ctx->path_save_avaliacao );

   if ( !final_save ) {
      g_free( ctx->path_save_avaliacao );
      ctx->path_save_avaliacao = g_build_filename( ctx->caminho.dados, "avaliacoes.bin", NULL );
   }
}


void desativar_avaliacao( AppContext *ctx, gboolean estado ) {
   InterfaceAvaliacao *avaliacao = &ctx->ui_diario.avaliacao;

   GtkComboBox *combo = GTK_COMBO_BOX( avaliacao->combo_avaliacao );
   int ativo = gtk_combo_box_get_active( combo );

   // 1. Aborta se não houver avaliação selecionada no combo
   if ( ativo < 0 ) return;

   // 2. Atualiza o estado lógico na memória RAM (GArray)
   if ( ctx->avaliacoes && ativo < ( int )ctx->avaliacoes->len ) {
      // Se 'estado' (riscar) for TRUE, a avaliação fica INATIVA (ativa = FALSE)
      g_array_index( ctx->avaliacoes, MetaAvaliacao, ativo ).ativa = !estado;
   } else {
      g_print( "⚠ Erro: Índice selecionado está fora dos limites do GArray.\n" );
      return;
   }

   // 3. Atualiza o estado visual no GtkComboBox (ListStore)
   GtkTreeModel *model = gtk_combo_box_get_model( combo );
   GtkListStore *store = GTK_LIST_STORE( model );
   GtkTreeIter iter;

   if ( gtk_tree_model_iter_nth_child( model, &iter, NULL, ativo ) ) {
      // Seta a Coluna 1 (strikethrough) com o valor gbooleaneano do checkbox
      gtk_list_store_set( store, &iter, 1, estado, -1 );
   }

   // 4. Aciona a flag para que o _autosave_dados_binarios_cb grave no disco
   _marcar_dados_modificado( ctx );

   // Assumindo que o ponteiro do seu TreeView está no contexto
   GtkTreeView *treeview = GTK_TREE_VIEW( avaliacao->treeview );
   _visibilidade_avaliacoes( ctx, treeview );
}


void popover_adicionar_avaliacao( AppContext *ctx, const char *texto ) {
   g_return_if_fail( ctx != NULL );
   g_return_if_fail( texto != NULL );
   InterfaceAvaliacao *avaliacao = &ctx->ui_diario.avaliacao;

   GtkComboBox *combo = GTK_COMBO_BOX( avaliacao->combo_avaliacao );
   GtkTreeModel *model_view = gtk_combo_box_get_model( combo );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );

   // 1. Regra de Limite na Interface
   if ( gtk_tree_model_iter_n_children( model_view, NULL ) >= 5 ) {
      InterfacePainel *painel = &ctx->painel;

      painel->format_titulo    = meu_gerador_variadico( "⚠ Limite de Avaliações Atingido" );
      painel->format_subtitulo = meu_gerador_variadico( "Não é possível cadastrar mais de 5 avaliações por período." );
      painel->format_instrucao = meu_gerador_variadico( "Para adicionar uma nova, edite e renomeie uma das avaliações existentes." );

      criar_mensagem_painel( AVISO, painel );
      return;
   }

   // Garante que a estrutura na RAM esteja pronta
   if ( !ctx->avaliacoes ) {
      ctx->avaliacoes = g_array_new( FALSE, FALSE, sizeof( MetaAvaliacao ) );
   }

   MetaAvaliacao meta = {0};
   g_strlcpy( meta.nome_av, texto, sizeof( meta.nome_av ) );
   meta.ativa = TRUE;

   gboolean riscar = !meta.ativa;

   // 2. Manipulação Direta na Memória RAM
   g_array_append_val( ctx->avaliacoes, meta );

   // 3. Atualiza a Interface Gráfica
   GtkTreeIter iter;
   gtk_list_store_append( store_view, &iter );
   gtk_list_store_set( store_view, &iter, 0, texto, 1, riscar, -1 );

   gtk_combo_box_set_active_iter( combo, &iter );

   // 4. Sinaliza ao sistema (autosave) que há dados pendentes para gravação
   _marcar_dados_modificado( ctx );

   GtkTreeView *treeview = GTK_TREE_VIEW( avaliacao->treeview );
   _visibilidade_avaliacoes( ctx, treeview );
}


void popover_editar_avaliacao( AppContext *ctx, const char *texto ) {
   g_return_if_fail( ctx != NULL );
   g_return_if_fail( texto != NULL );
   InterfaceAvaliacao *avaliacao = &ctx->ui_diario.avaliacao;

   GtkComboBox *combo = GTK_COMBO_BOX( avaliacao->combo_avaliacao );
   int ativo = gtk_combo_box_get_active( combo );

   if ( ativo < 0 ) {
      g_print( "⚠ Nenhuma avaliação selecionada para edição.\n" );
      return;
   }

   if ( !ctx->avaliacoes ) return; // Prevenção de falha

   GtkTreeModel *model_view = gtk_combo_box_get_model( combo );
   GtkListStore *store_view = GTK_LIST_STORE( model_view );
   GtkTreeIter iter;

   // Captura o iterador da linha selecionada uma única vez
   if ( gtk_combo_box_get_active_iter( combo, &iter ) ) {
      gboolean riscado = FALSE;

      // 1. Lê a coluna 1 do ListStore (TRUE se estiver desativada/riscada)
      gtk_tree_model_get( model_view, &iter, 1, &riscado, -1 );

      MetaAvaliacao meta = {0};
      g_strlcpy( meta.nome_av, texto, sizeof( meta.nome_av ) );
      meta.ativa = !riscado;

      // 2. Manipulação Direta na Memória RAM (Substituição in-place)
      if ( ativo < ( int )ctx->avaliacoes->len ) {
         g_array_index( ctx->avaliacoes, MetaAvaliacao, ativo ) = meta;
      } else {
         g_print( "⚠ Erro de sincronia: índice de edição fora dos limites do array em memória.\n" );
         return;
      }

      // 3. Atualiza a Interface Gráfica usando o iter já carregado
      gtk_list_store_set( store_view, &iter, 0, texto, 1, riscado, -1 );
   }

   // 4. Sinaliza ao sistema (autosave) que há dados pendentes para gravação
   _marcar_dados_modificado( ctx );
}




//===================================================================================================
// FUNÇÃO AUXILIAR GENÉRICA: Salva qualquer bloco de memória (Struct única ou GArray)
//===================================================================================================
static gboolean _salvar_bloco_binario( gconstpointer dados, gsize tamanho_bytes, const gchar *caminho_arquivo ) {
   if ( !dados || !caminho_arquivo ) return FALSE;

   GError *erro = NULL;
   if ( !g_file_set_contents( caminho_arquivo, ( const gchar * )dados, tamanho_bytes, &erro ) ) {
      g_printerr( "Aviso: Falha ao salvar %s: %s\n", caminho_arquivo, erro->message );
      g_clear_error( &erro );
      return FALSE;
   }
   return TRUE;
}

//===================================================================================================
// FUNÇÃO PRINCIPAL
//===================================================================================================
gboolean salvar_fichas( AppContext *ctx, gboolean final_save ) {
   g_return_val_if_fail( ctx, FALSE );

   gboolean salvou_algo = FALSE;

   if ( ctx->fichas != NULL ) {
      for ( guint i = 0; i < ctx->fichas->len; i++ ) {
         // Aponta para a ficha específica do aluno na memória
         FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, i );

         if ( !ficha->ativo || !ficha->ficha_modificada ) {
            continue;
         }

         g_autofree char *ficha_bin = g_strdup_printf( "%" PRIu32 ".bin", ficha->cod_aluno );
         g_autofree char *path_ficha_bin = g_build_filename( ctx->path_save_fichas, ficha_bin, NULL );

         // Salva APENAS o bloco de memória desta ficha (tamanho exato da struct FichaAluno)
         salvou_algo = _salvar_bloco_binario( ficha, sizeof( FichaAluno ), path_ficha_bin );

         ficha->ficha_modificada = !salvou_algo;
      }
   }

   // Atualização do diretório de salvamento
   if ( !final_save ) {
      g_free( ctx->path_save_fichas );
      ctx->path_save_fichas = g_build_filename( ".", "dados", "informados",
                              ctx->dados.ano, ctx->dados.escola, "alunos", NULL );
   }
   return salvou_algo;
}

// EDINALDO! NÃO APAGUE ESTA FUNÇÃO COMENTADA!
// gboolean salvar_fichas( AppContext *ctx, gboolean final_save ) {
//    g_return_val_if_fail( ctx, FALSE );
//
//    gboolean salvou_algo = FALSE;
//
//    if ( ctx->fichas != NULL ) {
//       for ( guint i = 0; i < ctx->fichas->len; i++ ) {
//          // Aponta para a ficha antiga (arquitetura velha) na memória
//          FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, i );
//
//          // Zera completamente a nova estrutura garantindo que não haja lixo de memória
//          FichaAlunoAux ficha_aux = {0};
//
//          // =====================================================================
//          // 1. CÓPIA DOS DADOS BÁSICOS (Mapeamento Direto 1:1)
//          // =====================================================================
//          ficha_aux.cod_aluno = ficha->cod_aluno;
//          g_strlcpy( ficha_aux.aluno, ficha->aluno, sizeof( ficha_aux.aluno ) );
//          g_strlcpy( ficha_aux.sexo, ficha->sexo, sizeof( ficha_aux.sexo ) );
//          g_strlcpy( ficha_aux.nasc, ficha->nasc, sizeof( ficha_aux.nasc ) );
//          ficha_aux.atipico = ficha->atipico; // Seguro, pois FichaAtipicidade manteve os 8 bytes
//          ficha_aux.sit = ficha->sit;
//          ficha_aux.ativo = ficha->ativo;
//          ficha_aux.limite_corte = ficha->limite_corte;
//          ficha_aux.idx_siaep = ficha->idx;
//
//          // =====================================================================
//          // 2. MIGRAÇÃO DA ARQUITETURA DE DADOS (O fim do memcpy perigoso)
//          // =====================================================================
//          for ( int d = 0; d < QTD_DISC; d++ ) {
//
//             // Migra os 4 Períodos Letivos
//             for ( int p = 0; p < 4; p++ ) {
//                // Frequência
//                ficha_aux.disciplina[d].periodo[p].presencas = ficha->presencas[d][p];
//                ficha_aux.disciplina[d].periodo[p].ausencias = ficha->ausencias[d][p];
//
//                // Avaliações (AV e REC)
//                for ( int k = 0; k < 5; k++ ) {
//                   ficha_aux.disciplina[d].periodo[p].avaliacoes[k].av  = ficha->nota[d][p][k].av;
//                   ficha_aux.disciplina[d].periodo[p].avaliacoes[k].rec = ficha->nota[d][p][k].rec;
//                }
//
//                // Média do período (Antigo relatorio[0 a 3])
//                ficha_aux.disciplina[d].relatorio.notas_periodos[p] = ficha->relatorio[d][p];
//             }
//
//             // Migra o Consolidado Anual
//             ficha_aux.disciplina[d].relatorio.soma = ficha->soma[d];
//             ficha_aux.disciplina[d].relatorio.media_anual = ficha->media[d];
//
//             // Notas finais (Antigo relatorio[4] e relatorio[5])
//             // *Nota: Se no seu código antigo rec_final e conselho eram vetores separados
//             // (ex: ficha->rec_final[d]), basta trocar a atribuição aqui!
//             ficha_aux.disciplina[d].relatorio.rec_final = ficha->relatorio[d][4];
//             ficha_aux.disciplina[d].relatorio.conselho  = ficha->relatorio[d][5];
//          }
//
//          // Reseta a flag para que o sistema considere que o disco está sincronizado
//          ficha_aux.ficha_modificada = FALSE;
//
//          // =====================================================================
//          // 3. GRAVAÇÃO DO NOVO BLOCO (Estrutura Nova)
//          // =====================================================================
//          g_autofree char *ficha_bin = g_strdup_printf( "%" PRIu32 ".bin", ficha->cod_aluno );
//          g_autofree char *path_ficha_bin = g_build_filename( ctx->path_save_fichas, ficha_bin, NULL );
//
//          // Salva a estrutura convertida FichaAlunoAux no arquivo .bin
//          _salvar_bloco_binario( &ficha_aux, sizeof( FichaAlunoAux ), path_ficha_bin );
//       }
//       salvou_algo = TRUE;
//    }
//
//    // Atualização do diretório de salvamento após a conclusão
//    if ( !final_save ) {
//       g_free( ctx->path_save_fichas );
//       ctx->path_save_fichas = g_build_filename( ".", "dados", "alunos", NULL );
//    }
//
//    return salvou_algo;
// }




//===================================================================================================
// NAVEGAÇÃO, RENDERIZAÇÃO e SICRONIZAÇÃO das NOTAS DIGITADAS no LISTSTORE
//===================================================================================================
static gboolean _celula_pode_receber_foco( AppContext *ctx, GtkTreeModel *model,
      GtkTreeIter *iter, GtkTreeViewColumn *col ) {
   gint col_idx = 0;
   GtkTreeView *tree_view = NULL;
   GList *renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( col ) );

   if ( renderers ) {
      if ( GTK_IS_CELL_RENDERER_TEXT( renderers->data ) ) {
         GObject *r = G_OBJECT( renderers->data );
         col_idx = GPOINTER_TO_INT( g_object_get_data( r, "col_model_idx" ) );
         // Resgata o treeview plantado previamente nos metadados da célula!
         tree_view = GTK_TREE_VIEW( g_object_get_data( r, "ptr_treeview" ) );
      }
      g_list_free( renderers );
   }

   // Se falhar na leitura ou não encontrar o tree_view, repulsa o cursor
   if ( col_idx == 0 || tree_view == NULL ) return FALSE;

   gboolean eh_inativo = FALSE;

   // ==========================================================
   // REGRAS DO TREEVIEW DE RELATÓRIO
   // ==========================================================
   if ( tree_view == GTK_TREE_VIEW( ctx->ui_diario.relatorio.treeview ) ) {
      gtk_tree_model_get( model, iter, 11, &eh_inativo, -1 );
      if ( eh_inativo ) return FALSE;

      GtkToggleButton *check_pendencias = GTK_TOGGLE_BUTTON( ctx->ui_diario.relatorio.check_pendencias );
      gboolean modo_pendencias = gtk_toggle_button_get_active( check_pendencias );
      if ( !modo_pendencias ) return FALSE;

      int mascara = 0;
      gtk_tree_model_get( model, iter, 12, &mascara, -1 );

      if ( col_idx >= 2 && col_idx <= 4 ) return ( mascara & ( 1 << ( col_idx - 2 ) ) ) != 0;
      if ( col_idx == 8 ) return ( mascara & ( 1 << 4 ) ) != 0;
      if ( col_idx == 9 ) return ( mascara & ( 1 << 5 ) ) != 0;

      return FALSE;
   }

   // ==========================================================
   // REGRAS DO TREEVIEW DE AVALIAÇÕES
   // ==========================================================
   else if ( tree_view == GTK_TREE_VIEW( ctx->ui_diario.avaliacao.treeview ) ) {
      gtk_tree_model_get( model, iter, 13, &eh_inativo, -1 );
      if ( eh_inativo ) return FALSE;

      // Coluna 12 (Nota calculada do período) é somente leitura
      if ( col_idx == 12 ) return FALSE;
      // As notas (2 a 11) são editáveis para alunos ativos
      if ( col_idx >= 2 && col_idx <= 11 ) return TRUE;

      return FALSE;
   }

   return FALSE;
}
//---------------------------------------------------------------------------------------------------
gboolean calcular_destino_cursor( AppContext *ctx, guint keyval, GtkTreeModel *model, GtkTreePath *path,
                                  GList *current_col_node, GtkTreeViewColumn **nova_coluna ) {
   gboolean mover_cursor = FALSE;
   GtkTreeIter iter;

   switch ( keyval ) {
   case GDK_KEY_KP_Enter:
   case GDK_KEY_Down:
   case GDK_KEY_KP_Down:
      gtk_tree_path_next( path );
      while ( gtk_tree_model_get_iter( model, &iter, path ) ) {
         if ( _celula_pode_receber_foco( ctx, model, &iter, *nova_coluna ) ) {
            mover_cursor = TRUE;
            break;
         }
         gtk_tree_path_next( path );
      }
      break;

   case GDK_KEY_Up:
   case GDK_KEY_KP_Up:
      while ( gtk_tree_path_prev( path ) ) {
         if ( gtk_tree_model_get_iter( model, &iter, path ) ) {
            if ( _celula_pode_receber_foco( ctx, model, &iter, *nova_coluna ) ) {
               mover_cursor = TRUE;
               break;
            }
         }
      }
      break;

   case GDK_KEY_Tab:
   case GDK_KEY_Right:
   case GDK_KEY_KP_Right:
      if ( current_col_node && gtk_tree_model_get_iter( model, &iter, path ) ) {
         for ( GList *node = current_col_node->next; node != NULL; node = node->next ) {
            GtkTreeViewColumn *c = GTK_TREE_VIEW_COLUMN( node->data );
            if ( gtk_tree_view_column_get_visible( c ) ) {
               if ( _celula_pode_receber_foco( ctx, model, &iter, c ) ) {
                  *nova_coluna = c;
                  mover_cursor = TRUE;
                  break;
               }
            }
         }
      }
      break;

   case GDK_KEY_Left:
   case GDK_KEY_KP_Left:
      if ( current_col_node && gtk_tree_model_get_iter( model, &iter, path ) ) {
         for ( GList *node = current_col_node->prev; node != NULL; node = node->prev ) {
            GtkTreeViewColumn *c = GTK_TREE_VIEW_COLUMN( node->data );
            if ( gtk_tree_view_column_get_visible( c ) ) {
               if ( _celula_pode_receber_foco( ctx, model, &iter, c ) ) {
                  *nova_coluna = c;
                  mover_cursor = TRUE;
                  break;
               }
            }
         }
      }
      break;
   }

   return mover_cursor;
}
//---------------------------------------------------------------------------------------------------
void confirmar_edicao_e_mover_cursor( GtkTreeView *tree_view, GtkTreePath *path_destino, GtkTreeViewColumn *col_destino ) {
   g_return_if_fail( tree_view != NULL );
   g_return_if_fail( path_destino != NULL );
   g_return_if_fail( col_destino != NULL );

   // Captura o editor ativo antes de mudarmos o foco
   GtkWidget *editor = gtk_container_get_focus_child( GTK_CONTAINER( tree_view ) );

   // Rola a visualização (se necessário) e reabre a edição na nova célula
   gtk_tree_view_scroll_to_cell( tree_view, path_destino, col_destino, TRUE, 0.5, 0.9 );

   // Força o disparo síncrono do sinal "edited" nativo do GTK
   if ( GTK_IS_CELL_EDITABLE( editor ) ) {
      gtk_cell_editable_editing_done( GTK_CELL_EDITABLE( editor ) );
   }

   gtk_tree_view_set_cursor_on_cell( tree_view, path_destino, col_destino, NULL, TRUE );
}
//-----------------------------------------------------------------------------------------------------------









//=============================================================================================================
//=============================================================================================================
/**
 * @brief Validador universal de notas (0 a 10). Substitui vírgulas, detecta células limpas e ignora espaços tabulares.
 */
static gboolean _validar_nota_digitada( const gchar *texto, float *nota_saida ) {
   g_return_val_if_fail( nota_saida != NULL, FALSE );

   // 1. Tratamento rápido de campo nulo ou strings perfeitamente vazias
   if ( !texto || texto[0] == '\0' || g_strcmp0( texto, "-" ) == 0 ) {
      *nota_saida = -1.0f; // Representa vazio/"não fez"
      return TRUE;
   }

   // 2. Cria uma cópia segura e dinâmica
   g_autofree gchar *buffer = g_strdup( texto );

   // 3. Limpa espaços acidentais (ASCII padrão) nas pontas
   g_strstrip( buffer );

   gchar *start_ptr = buffer;

   // 4. Ignora o Figure Space (\u2007) caso o professor não o tenha apagado
   // O UTF-8 de \u2007 ocupa exatamente 3 bytes. O laço remove múltiplos se houver.
   while ( g_str_has_prefix( start_ptr, "\u2007" ) ) {
      start_ptr += 3;
   }

   // 5. Se após a limpeza o texto ficar vazio ou for apenas um traço
   if ( start_ptr[0] == '\0' || g_strcmp0( start_ptr, "-" ) == 0 ) {
      *nota_saida = -1.0f;
      return TRUE;
   }

   // 6. Substitui vírgulas por pontos nativamente, a partir da nova posição válida
   g_strdelimit( start_ptr, ",", '.' );

   gchar *endptr = NULL;
   gdouble valor = g_ascii_strtod( start_ptr, &endptr );

   // 7. Rejeita falha de conversão, lixo alfabético solto (ex: "8.5a") ou notas fora da escala
   if ( endptr == start_ptr || *endptr != '\0' || valor < 0.0 || valor > 10.0 ) {
      return FALSE;
   }

   *nota_saida = ( float )valor;
   return TRUE;
}
//-------------------------------------------------------------------------------------------------------------
/**
 * @brief Formata a nota injetando o Figure Space (\u2007) para alinhar decimais no GTK3.
 */
static void _formatar_nota_com_espaco_tabular( float nota, char *buffer, size_t size ) {
   if ( nota >= 10.0f ) {
      snprintf( buffer, size, "%.2f", nota );
   } else if ( nota >= 0.0f ) {
      snprintf( buffer, size, "\u2007%.2f", nota );
   } /*else {
      snprintf( buffer, size, "" );
   }*/
}
//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
/**
 * @brief Salva a nota tanto na Struct Soberana quanto no GtkListStore que disparou o evento.
 */
void processar_gravacao_nota( AppContext *ctx, GtkCellRenderer *renderer,
                              const gchar *path_string, const gchar *new_text ) {
   InterfaceAvaliacao *avaliacao = &ctx->ui_diario.avaliacao;
   InterfaceRelatorio *relatorio = &ctx->ui_diario.relatorio;

   float nova_nota;
   if ( !_validar_nota_digitada( new_text, &nova_nota ) ) return;

   // 1. Resgata o tree_view silenciosamente dos metadados da célula
   GtkTreeView *tree_view = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( renderer ), "ptr_treeview" ) );
   if ( !tree_view ) return;

   GtkTreeModel *model = gtk_tree_view_get_model( tree_view );
   GtkTreeIter iter;
   if ( !gtk_tree_model_get_iter_from_string( model, &iter, path_string ) ) return;

   int index_aluno = atoi( path_string );
   FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, index_aluno );
   gint col_idx = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( renderer ), "col_model_idx" ) );
   int foco = ctx->cabecalho.foco_disciplina;

   float *nota_alvo = NULL;

// Desativa temporariamente a checagem de ponteiro para membros empacotados apenas neste bloco
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

   // ==========================================================
   // 2. ROTEAMENTO DINÂMICO: Descobre o endereço de memória alvo
   // ==========================================================
   if ( tree_view == GTK_TREE_VIEW( avaliacao->treeview ) ) {
      int periodo = ctx->cabecalho.foco_periodo;
      int av_index = ( col_idx - 2 ) / 2;
      gboolean is_rec = ( col_idx % 2 != 0 );

      NotaAvaliacao *nota = &ficha->disciplina[foco].periodo[periodo].avaliacoes[av_index];
      nota_alvo = is_rec ? &nota->rec : &nota->av;

   } else if ( tree_view == GTK_TREE_VIEW( relatorio->treeview ) ) {
      RelatorioAnual *rel = &ficha->disciplina[foco].relatorio;

      // Rotas dos Períodos Letivos (Colunas 2 a 4)
      if ( col_idx >= 2 && col_idx <= 4 ) {
         int p = col_idx - 2;
         nota_alvo = &rel->notas_periodos[p];
      } else if ( col_idx == 8 ) {
         nota_alvo = &rel->rec_final;
      } else if ( col_idx == 9 ) {
         nota_alvo = &rel->conselho;
      }
   }

#pragma GCC diagnostic pop

   if ( !nota_alvo ) return;

   // ==========================================================
   // 3. RETORNO ANTECIPADO UNIVERSAL (Destrava a rolagem do TreeView)
   // ==========================================================
   if ( *nota_alvo == nova_nota ) {
      GtkTreePath *path_temp = gtk_tree_path_new_from_string( path_string );
      if ( path_temp ) {
         gtk_tree_model_row_changed( model, path_temp, &iter );
         gtk_tree_path_free( path_temp );
      }
      return;
   }

   // 4. Efetiva a gravação na struct de forma limpa
   *nota_alvo = nova_nota;
   ficha->ficha_modificada = TRUE;

   // 5. Atualização Visual
   gchar texto_formatado[16] = "";

   if ( nova_nota >= 10.0f ) {
      snprintf( texto_formatado, sizeof( texto_formatado ), "%.2f", nova_nota );
   } else if ( nova_nota >= 0.0f ) {
      snprintf( texto_formatado, sizeof( texto_formatado ), "\u2007%.2f", nova_nota );
   }

   g_strdelimit( texto_formatado, ".", ',' );

   gtk_list_store_set( GTK_LIST_STORE( model ), &iter, col_idx, texto_formatado, -1 );
}
//=============================================================================================================




//===================================================================================================
// FUNÇÃO AUXILIAR: Cola múltiplas notas na coluna a partir da célula atual
//===================================================================================================
void colar_notas_em_lote( AppContext *ctx, GtkWidget *widget, GtkTreePath *start_path, GtkTreeViewColumn *column ) {
   InterfaceAvaliacao *avaliacao = &ctx->ui_diario.avaliacao;

   GtkClipboard *clipboard = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
   gchar *text = gtk_clipboard_wait_for_text( clipboard );
   if ( !text ) return;

   GtkTreeView *tree_view = GTK_TREE_VIEW( avaliacao->treeview );
   GtkTreeModel *model = gtk_tree_view_get_model( tree_view );

   GtkTreePath *current_path = gtk_tree_path_copy( start_path );
   GtkTreeIter iter;

   if ( !gtk_tree_model_get_iter( model, &iter, current_path ) ) {
      gtk_tree_path_free( current_path );
      g_free( text );
      return;
   }

   GList *cells = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ) );
   if ( !cells ) {
      gtk_tree_path_free( current_path );
      g_free( text );
      return;
   }
   GtkCellRendererText *renderer = GTK_CELL_RENDERER_TEXT( cells->data );
   g_list_free( cells );

   gchar **linhas = g_strsplit( text, "\n", -1 );
   int i = 0;

   // 4. Bloco do GtkEntry: Aplica a nota se o aluno da linha atual for ativo
   if ( GTK_IS_ENTRY( widget ) && linhas[0] != NULL ) {
      gchar *nota_str = g_strstrip( linhas[0] );
      gchar *path_str = gtk_tree_path_to_string( current_path );

      // Captura a correspondência absoluta do aluno pela interface
      gint *indices = gtk_tree_path_get_indices( current_path );
      if ( indices && ( guint )indices[0] < ctx->fichas->len ) {
         FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, indices[0] );

         if ( ficha->ativo ) {
            gtk_entry_set_text( GTK_ENTRY( widget ), nota_str );
            processar_gravacao_nota( ctx, GTK_CELL_RENDERER( renderer ), path_str, nota_str );
         } else {
            // Limpa o entry visual se o aluno colado for inativo (proteção de interface)
            gtk_entry_set_text( GTK_ENTRY( widget ), "" );
         }
      }

      g_free( path_str );
      i = 1;
      gtk_tree_path_next( current_path );

      if ( !gtk_tree_model_get_iter( model, &iter, current_path ) ) {
         gtk_tree_path_free( current_path );
         g_strfreev( linhas );
         g_free( text );
         return;
      }
   }

   // 5. Preenche as linhas seguintes mapeando o Path ao GArray corretamente
   while ( linhas[i] != NULL ) {
      gchar *nota_str = g_strstrip( linhas[i] );

      if ( linhas[i + 1] == NULL && strlen( nota_str ) == 0 ) break;

      gchar *path_str = gtk_tree_path_to_string( current_path );
      gint *indices = gtk_tree_path_get_indices( current_path );

      // Verifica o aluno correspondente à linha do GTK iterada, e NÃO pela variável 'i'
      if ( indices && ( guint )indices[0] < ctx->fichas->len ) {
         FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, indices[0] );

         if ( ficha->ativo ) {
            processar_gravacao_nota( ctx, GTK_CELL_RENDERER( renderer ), path_str, nota_str );
         }
      }

      g_free( path_str );
      i++;
      gtk_tree_path_next( current_path );

      if ( !gtk_tree_model_get_iter( model, &iter, current_path ) ) break;
   }

   gtk_tree_path_free( current_path );
   g_strfreev( linhas );
   g_free( text );
}
//===================================================================================================







//--------------------------------------------------------------------------------------------------
void carregar_notas_ui_por_periodo( AppContext *ctx ) {
   g_return_if_fail( ctx && ctx->fichas );
   InterfaceAvaliacao *avaliacao = &ctx->ui_diario.avaliacao;

   GtkTreeView *tree_view = GTK_TREE_VIEW( avaliacao->treeview );
   GtkListStore *store = GTK_LIST_STORE( gtk_tree_view_get_model( tree_view ) );

   int foco_per = ctx->cabecalho.foco_periodo;
   g_return_if_fail( foco_per >= 0 && foco_per <= 3 );

   // 1. Limpa o modelo inteiro para receber os dados do novo período
   gtk_list_store_clear( store );

   int foco = ctx->cabecalho.foco_disciplina;

   // 2. Percorre o GArray reconstruindo as linhas do zero
   for ( guint i = 0; i < ctx->fichas->len; i++ ) {
      FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, i );
      GtkTreeIter iter;

      // Buffer estático na Stack: 10 strings de até 8 caracteres (Ex: "10.00\0")
      // Zera todo o bloco de memória automaticamente (todas strings vazias por padrão)
      char str_notas[11][8] = {{0}};

      // Constrói as strings para os 5 pares de notas
      for ( int av = 0; av < 5; av++ ) {
         const NotaAvaliacao *nota = &ficha->disciplina[foco].periodo[foco_per].avaliacoes[av];
         int base = av * 2;

         // Chamadas limpas para a função auxiliar
         _formatar_nota_com_espaco_tabular( nota->av,  str_notas[base],     sizeof( str_notas[base] ) );
         _formatar_nota_com_espaco_tabular( nota->rec, str_notas[base + 1], sizeof( str_notas[base + 1] ) );
      }

      const RelatorioAnual *relatorio = &ficha->disciplina[foco].relatorio;
      _formatar_nota_com_espaco_tabular( relatorio->notas_periodos[foco_per], str_notas[10], sizeof( str_notas[10] ) );

      for ( int j = 0; j < 11; j++ ) {
         g_strdelimit( str_notas[j], ".", ',' ); // Aceita vírgula do teclado numérico
      }

      gtk_list_store_append( store, &iter );

      // Injeta todas as colunas de uma vez só na linha recém-criada
      gtk_list_store_set( store, &iter,
                          0, i + 1,
                          1, ficha->aluno,
                          2, str_notas[0],   3, str_notas[1],
                          4, str_notas[2],   5, str_notas[3],
                          6, str_notas[4],   7, str_notas[5],
                          8, str_notas[6],   9, str_notas[7],
                          10, str_notas[8],  11, str_notas[9],
                          12, str_notas[10], // Nota do período
                          13, !ficha->ativo,
                          -1 );
   }

   GtkTreePath *path_novo = gtk_tree_path_new_from_indices( 0, -1 );
   if ( path_novo ) {
      gtk_tree_view_scroll_to_cell( GTK_TREE_VIEW( avaliacao->treeview ), path_novo, NULL, FALSE, 0.0, 0.0 );
      gtk_tree_path_free( path_novo );
   }
}


static int numero_de_periodos_sem_notas_registradas( GArray *fichas, int d ) {
   // =========================================================================
   // PRE-LOAD 1: Descobre quais períodos já possuem notas lançadas na turma
   // =========================================================================
   gboolean periodo_iniciado[4] = {FALSE, FALSE, FALSE, FALSE};

   for ( guint i = 0; i < fichas->len; i++ ) {
      FichaAluno *f = &g_array_index( fichas, FichaAluno, i );
      const RelatorioAnual *r = &f->disciplina[d].relatorio;
      for ( int j = 0; j < 4; j++ ) {
         if ( r->notas_periodos[j] >= 0.0f ) {
            periodo_iniciado[j] = TRUE;
         }
      }
   }

   // Calcula a quantidade de períodos (bimestres) verdadeiramente pendentes globais
   int faltantes_globais = 0;
   for ( int j = 0; j < 4; j++ ) {
      if ( !periodo_iniciado[j] ) faltantes_globais++;
   }

   return faltantes_globais;
}

//===================================================================================================
// FUNÇÃO PRINCIPAL: Carrega e calcula os dados no TreeView do Relatório
//===================================================================================================
void carregar_relatorio_ui( AppContext *ctx ) {
   g_return_if_fail( ctx && ctx->fichas );
   InterfaceRelatorio *relatorio = &ctx->ui_diario.relatorio;

   GtkTreeView *tree_view = GTK_TREE_VIEW( relatorio->treeview );
   GtkListStore *store = GTK_LIST_STORE( gtk_tree_view_get_model( tree_view ) );

   // 1. Otimização de Performance GTK
   g_object_ref( store );
   gtk_tree_view_set_model( tree_view, NULL );
   gtk_list_store_clear( store );

   int d = ctx->cabecalho.foco_disciplina;
   gboolean eh_rec = ( ctx->cabecalho.foco_periodo_vigente == RECUPERACAO_FINAL );
   int foco_periodo = eh_rec ? QUARTO_PERIODO : ctx->cabecalho.foco_periodo_vigente; // 0 (1ºP) a 3 (4ºP)

   int faltantes_globais = numero_de_periodos_sem_notas_registradas( ctx->fichas, d );

   // =========================================================================
   // PRE-LOAD 2: Carregando acesso.bin em Tabelas Hash O(1)
   // =========================================================================
   GHashTable *hash_acesso[4] = {NULL, NULL, NULL, NULL};

   for ( int p = 0; p <= foco_periodo; p++ ) {
      hash_acesso[p] = g_hash_table_new( g_direct_hash, g_direct_equal );

      GtkComboBox *combo_periodo = GTK_COMBO_BOX( ctx->cabecalho.combo_periodo );
      g_autofree gchar *texto_periodo = combo_box_text_get_text_at( combo_periodo, p );

      g_autofree gchar *acesso_bin = g_build_filename( ".", "dados", "informados", ctx->dados.ano,
                                                       ctx->dados.escola, ctx->dados.turma,
                                                       ctx->dados.disciplina, texto_periodo,
                                                       "acesso.bin", NULL );
      gchar *conteudo = NULL;
      gsize tamanho = 0;

      if ( g_file_get_contents( acesso_bin, &conteudo, &tamanho, NULL ) ) {
         int qtd_acessos = tamanho / sizeof( AcessoFicha );
         AcessoFicha *acessos = ( AcessoFicha * )conteudo;

         for ( int a = 0; a < qtd_acessos; a++ ) {
            // SACADA DE MESTRE: Salva o cod_aluno como CHAVE e o status ativo como VALOR
            g_hash_table_insert( hash_acesso[p],
                                 GINT_TO_POINTER( acessos[a].cod_aluno ),
                                 GINT_TO_POINTER( acessos[a].ativo ) );
         }

         g_free( conteudo );
      }
   }

   // =========================================================================
   // LAÇO PRINCIPAL DOS ALUNOS
   // =========================================================================
   for ( guint i = 0; i < ctx->fichas->len; i++ ) {
      FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, i );
      const RelatorioAnual *rel = &ficha->disciplina[d].relatorio;

      GtkTreeIter iter;
      char str_cols[8][16] = {{0}};
      char obs[160] = {0};

      // --- PASSO NOVO: Busca do status ativo do aluno no período atual em O(1) ---
      gpointer ptr_ativo = NULL;
      gboolean presente_no_foco = g_hash_table_lookup_extended( hash_acesso[foco_periodo],
                                                                GINT_TO_POINTER( ficha->cod_aluno ),
                                                                NULL, &ptr_ativo );

      // Resgata o booleano de dentro do ponteiro
      gboolean ativo_no_foco = presente_no_foco ? GPOINTER_TO_INT( ptr_ativo ) : FALSE;
      int mascara_pendencias = 0;

      if ( presente_no_foco ) {
         for ( int p = 0; p < foco_periodo; p++ ) {
            if ( !g_hash_table_contains( hash_acesso[p], GINT_TO_POINTER( ficha->cod_aluno ) ) ) {
               mascara_pendencias |= ( 1 << p ); // Bits 0 a 3: Matrícula Tardia
            }
         }
      }

      // --- PASSO A: Formata as notas dos 4 períodos ---
      for ( int j = 0; j < 4; j++ ) {
         if ( rel->notas_periodos[j] >= 0.0f ) {
            _formatar_nota_com_espaco_tabular( rel->notas_periodos[j], str_cols[j], sizeof( str_cols[j] ) );
         }
      }

      // --- PASSO B: Soma, Média, Rec Final e Conselho ---
      if ( rel->soma >= 0.0f ) {
         _formatar_nota_com_espaco_tabular( rel->soma, str_cols[4], sizeof( str_cols[4] ) );
         _formatar_nota_com_espaco_tabular( rel->media_anual, str_cols[5], sizeof( str_cols[5] ) );
      }

      if ( rel->rec_final >= 0.0f ) {
         _formatar_nota_com_espaco_tabular( rel->rec_final, str_cols[6], sizeof( str_cols[6] ) );
      }

      if ( rel->conselho >= 0.0f ) {
         _formatar_nota_com_espaco_tabular( rel->conselho, str_cols[7], sizeof( str_cols[7] ) );
      }

      // --- PASSO C: Lógica de Status & Destravamento de Edição Final ---
      if ( rel->soma >= 0.0f ) {
         float max_futuro = rel->soma + ( faltantes_globais * 10.0f );

         if ( rel->conselho >= 0.0f ) {
            g_strlcpy( obs, ( rel->conselho >= 6.0f ) ? "Aprov Cons" : "Reprovado", sizeof( obs ) );
            mascara_pendencias |= ( 1 << 4 );
            mascara_pendencias |= ( 1 << 5 );
         } else if ( rel->rec_final >= 0.0f ) {
            g_strlcpy( obs, ( rel->rec_final >= 6.0f ) ? "Aprov Final" : "Conselho", sizeof( obs ) );
            mascara_pendencias |= ( 1 << 4 );
            if ( rel->rec_final < 6.0f ) {
               mascara_pendencias |= ( 1 << 5 );
            }
         } else if ( rel->soma > 22.2f ) {
            g_strlcpy( obs, "Aprov Dir", sizeof( obs ) );
         } else if ( max_futuro <= 22.2f && faltantes_globais < 4 ) {
            g_strlcpy( obs, "Rec Final", sizeof( obs ) );
            mascara_pendencias |= ( 1 << 4 );
         } else {
            g_strlcpy( obs, "Cursando", sizeof( obs ) );
         }
      }

      // --- PASSO D: Filtro Visual para Alunos Inativos ---
      if ( !ficha->ativo ) {
         str_cols[6][0] = '\0';
         str_cols[7][0] = '\0';
         obs[0] = '\0';
      }

      // ----------------------------------------------------------------------
      // CORREÇÃO DO BUG: Sincronização O(1) pelo cod_aluno.
      // Se o aluno está inativo (ou sequer existia) no período focado, oculta a OBS.
      // ----------------------------------------------------------------------
      if ( !ativo_no_foco ) {
         obs[0] = '\0';
      }

      for ( int j = 0; j < 8; j++ ) {
         g_strdelimit( str_cols[j], ".", ',' );
      }

      // --- PASSO E: Inserção Limpa no GTK ---
      gtk_list_store_append( store, &iter );
      gtk_list_store_set( store, &iter,
                          0, i + 1,
                          1, ficha->aluno,
                          2, str_cols[0],
                          3, str_cols[1],
                          4, str_cols[2],
                          5, str_cols[3],
                          6, str_cols[4],
                          7, str_cols[5],
                          8, str_cols[6],
                          9, str_cols[7],
                          10, obs,
                          11, !ficha->ativo,
                          12, mascara_pendencias,
                          -1 );
   }

   // Limpeza O(1) da Memória das Hash Tables
   for ( int p = 0; p <= foco_periodo; p++ ) {
      if ( hash_acesso[p] ) g_hash_table_destroy( hash_acesso[p] );
   }

   // 3. Reanexa o modelo atualizado à TreeView
   gtk_tree_view_set_model( tree_view, GTK_TREE_MODEL( store ) );
   g_object_unref( store );

   // 5. Retorna o scroll para o topo da lista
   GtkTreePath *path_novo = gtk_tree_path_new_first();
   if ( path_novo ) {
      gtk_tree_view_scroll_to_cell( tree_view, path_novo, NULL, FALSE, 0.0, 0.0 );
      gtk_tree_path_free( path_novo );
   }
}



void gerar_acesso_recuperacao_final( AppContext *ctx ) {
   InterfaceDados *dados = &ctx->dados;
   GtkComboBox *combo = GTK_COMBO_BOX( ctx->cabecalho.combo_periodo );

   // 1. Extração segura da nomenclatura direto da UI
   g_autofree gchar *nome_p4  = combo_box_text_get_text_at( combo, QUARTO_PERIODO );
   g_autofree gchar *nome_rec = combo_box_text_get_text_at( combo, RECUPERACAO_FINAL );
   if ( !nome_p4 || !nome_rec ) return;

   // 2. Construção dos caminhos absolutos (adicionando "acesso.bin" ao final do diretório)
   g_autofree gchar *dir_p4 = g_build_filename( ".", "dados", "informados",
                                                dados->ano, dados->escola, dados->turma, dados->disciplina, nome_p4, NULL );
   g_autofree gchar *arq_p4 = g_build_filename( dir_p4, "acesso.bin", NULL );

   g_autofree gchar *dir_rec = g_build_filename( ".", "dados", "informados",
                                                 dados->ano, dados->escola, dados->turma, dados->disciplina, nome_rec, NULL );
   g_autofree gchar *arq_rec = g_build_filename( dir_rec, "acesso.bin", NULL );

   if ( g_mkdir_with_parents( dir_rec, 0755) != 0 ) {
       g_printerr("[ERRO] Nao foi possivel criar/acessar a pasta de destino: %s\n", dir_rec);
       return;
   }

   // 3. Carrega o arquivo do 4º Período integralmente para a RAM
   gsize tamanho = 0;
   gchar *conteudo = NULL;
   if ( !g_file_get_contents( arq_p4, &conteudo, &tamanho, NULL ) ) {
      g_printerr( "Falha ao ler o acesso base do 4º Período: %s\n", arq_p4 );
      return;
   }

   g_autofree AcessoFicha *acessos = ( AcessoFicha * )conteudo;
   int qtd_acessos = tamanho / sizeof( AcessoFicha );

   // Desativa todo mundo por precaução antes do filtro
   for ( int i = 0; i < qtd_acessos; i++ ) acessos[i].ativo = FALSE;

   // 4. Varredura diretamente no modelo da interface (a fonte da verdade visual)
   GtkTreeModel *model = gtk_tree_view_get_model( GTK_TREE_VIEW( ctx->ui_diario.relatorio.treeview ) );
   GtkTreeIter iter;
   gboolean valid = gtk_tree_model_get_iter_first( model, &iter );
   int idx_ficha = 0;

   while ( valid ) {
      gchar *obs = NULL;
      gtk_tree_model_get( model, &iter, 10, &obs, -1 ); // Lê a coluna 10 (OBS)

      // Se o aluno está de Recuperação Final
      if ( g_strcmp0( obs, "Rec Final" ) == 0 ) {
         // Resgata a ficha que espelha essa linha
         FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, idx_ficha );

         // Procura esse aluno no buffer binário e marca como ativo
         for ( int i = 0; i < qtd_acessos; i++ ) {
            if ( acessos[i].cod_aluno == ficha->cod_aluno ) {
               acessos[i].ativo = TRUE;
               break; // Achou, pode parar a busca interna
            }
         }
      }
      g_free( obs );
      valid = gtk_tree_model_iter_next( model, &iter );
      idx_ficha++;
   }

   // 5. Cria a pasta da Recuperação (se não existir) e injeta o novo binário
   g_mkdir_with_parents( dir_rec, 0755 );

   if ( g_file_set_contents( arq_rec, ( const gchar * )acessos, tamanho, NULL ) ) {
      g_print( "Acesso da Recuperação Final gerado com sucesso em: %s\n", dir_rec );
   } else {
      g_printerr( "Falha ao gravar acesso da Recuperação Final.\n" );
   }
}



