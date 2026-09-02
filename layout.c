/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include "layout.h"
#include "comum.h"
#include "basicas.h"
#include "interface.h"
#include <stdio.h>




// Essa função será chamada automaticamente pelo GTK para cada linha
static void formatar_cor_aluno( GtkCellLayout *layout, GtkCellRenderer *cell,
                                GtkTreeModel *model, GtkTreeIter *iter, gpointer data ) {
   ( void )layout;
   ( void )data;
   gboolean ativo;
   // Lê a coluna 1 (booleana) criada lá no seu liststore1
   gtk_tree_model_get( model, iter, 1, &ativo, -1 );

   if ( !ativo ) {
      g_object_set( cell, "sensitive", FALSE, "strikethrough", TRUE, NULL );
   } else {
      g_object_set( cell, "sensitive", TRUE, "strikethrough", FALSE, NULL );
   }
}

static void aplicar_estilo_cores_combo( GtkComboBox *combo ) {
   if ( !combo ) return;

   GList *cells = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( combo ) );
   if ( cells != NULL ) {
      // Pega o primeiro renderer de célula encontrado (o GtkCellRendererText do Glade)
      GtkCellRenderer *renderer = GTK_CELL_RENDERER( cells->data );

      // Conecta a função que decide a cor dinamicamente
      gtk_cell_layout_set_cell_data_func( GTK_CELL_LAYOUT( combo ), renderer,
                                          formatar_cor_aluno, NULL, NULL );

      g_list_free( cells ); // Boa prática: libera a lista gerada pelo GTK
   }
}


static void treeview_alinhar_coluna_renderizada( GtkWidget *widget, int coluna, float alinhamento ) {
   GtkTreeViewColumn *col_ch = gtk_tree_view_get_column( GTK_TREE_VIEW( widget ), coluna );
   gtk_tree_view_column_set_alignment( col_ch, alinhamento ); // Centraliza o título "CH"
   GList *renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( col_ch ) );
   if ( renderers ) {
      g_object_set( G_OBJECT( renderers->data ), "xalign", alinhamento, NULL ); // Centraliza o número 1
      g_list_free( renderers );
   }
}


//=====================================================================================================//
//                                   ORQUESTRADOR MESTRE DA INTERFACE                                  //
//=====================================================================================================//
void construir_interface( GtkApplication *app, AppContext *ctx ) {
   GtkBuilder *builder = gtk_builder_new();
   GError *error = NULL;

   // Carrega a interface diretamente da memória interna (GResource)
   // g_autofree gchar *interface_glade = g_build_filename( ctx->caminho.recursos_prefix, "interface.glade", NULL );
   // gtk_builder_add_from_resource( builder, interface_glade, &error );
   gtk_builder_add_from_file( builder, "./recursos/interface.glade", &error );

   if ( error != NULL ) {
      g_printerr( "🚨 Erro ao carregar a interface embutida do Glade: %s\n", error->message );
      g_clear_error( &error );
      return;
   }

   // Captura a janela principal definida lá dentro do Glade
   ctx->window = GTK_WIDGET( gtk_builder_get_object( builder, "janela_principal" ) );

   gtk_window_set_application( GTK_WINDOW( ctx->window ), app );

   // Posicionamento geográfico idêntico ao seu motor original
   if ( detectar_ubuntu() ) {
      gtk_window_set_position( GTK_WINDOW( ctx->window ), GTK_WIN_POS_CENTER );
   } else {
      gtk_window_move( GTK_WINDOW( ctx->window ), 1019, 141 );
   }

   // =========================================================================
   // 🏛️ ATIVAÇÃO DA HEADERBAR INTERNA (Para o CSS poder controlar)
   // =========================================================================
   GtkWidget *header = gtk_header_bar_new();
   gtk_header_bar_set_show_close_button( GTK_HEADER_BAR( header ), TRUE );

   // Definimos o título aqui na barra interna (pode remover o gtk_window_set_title antigo)
   gtk_header_bar_set_title( GTK_HEADER_BAR( header ),
                             "V É R T I C E   -   S I S T E M A   D E   G E S T Ã O   E D U C A C I O N A L" );

   // A MÁGICA: Diz ao GTK para usar essa barra como a barra de título oficial da janela
   gtk_window_set_titlebar( GTK_WINDOW( ctx->window ), header );
   // =========================================================================

   // =========================================================================
   // 🔗 PONTE DE ENGENHARIA: REAPEAMENTO DO APPCONTEXT (Mapeamento Simétrico)
   // =========================================================================

   // --- [ COLUNA 2 ORIGINAL / ABA RELATÓRIOS ] ---
   ctx->entry.ano        = GTK_WIDGET( gtk_builder_get_object( builder, "combo_ano" ) );
   ctx->entry.escola     = GTK_WIDGET( gtk_builder_get_object( builder, "combo_escola" ) );
   ctx->entry.turma      = GTK_WIDGET( gtk_builder_get_object( builder, "combo_turma" ) );
   ctx->entry.disciplina = GTK_WIDGET( gtk_builder_get_object( builder, "combo_disciplina" ) );
   ctx->entry.periodo    = GTK_WIDGET( gtk_builder_get_object( builder, "combo_momento" ) );
   ctx->ui_diario.combo_alunos     = GTK_WIDGET( gtk_builder_get_object( builder, "combo_alunos" ) );
   aplicar_estilo_cores_combo( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ) );

   ctx->cabecalho.gestor    = GTK_WIDGET( gtk_builder_get_object( builder, "box_gestor" ) );
   ctx->cabecalho.professor = GTK_WIDGET( gtk_builder_get_object( builder, "box_professor" ) );

   ctx->ui_diario.stack_pages        = GTK_WIDGET( gtk_builder_get_object( builder, "stack_pages" ) );

   //-- CONTEÚDOS
   ctx->ui_diario.entry_data         = GTK_WIDGET( gtk_builder_get_object( builder, "entry_data" ) );
   ctx->ui_diario.popover_calendario = GTK_WIDGET( gtk_builder_get_object( builder, "popover_calendario" ) );
   ctx->ui_diario.calendario_data    = GTK_WIDGET( gtk_builder_get_object( builder, "calendar_data" ) );
   g_object_ref( ctx->ui_diario.popover_calendario );

   ctx->ui_diario.stepper_menos = GTK_WIDGET( gtk_builder_get_object( builder, "button_stepper_menos" ) );
   ctx->ui_diario.qtd_aulas    = GTK_WIDGET( gtk_builder_get_object( builder, "label_qtd_aulas" ) );
   ctx->ui_diario.stepper_mais  = GTK_WIDGET( gtk_builder_get_object( builder, "button_stepper_mais" ) );

   ctx->ui_diario.tipo_registro = GTK_WIDGET( gtk_builder_get_object( builder, "combo_tipo_registro" ) );

   ctx->ui_diario.tema             = GTK_WIDGET( gtk_builder_get_object( builder, "entry_tema" ) );
   ctx->ui_diario.descricao        = GTK_WIDGET( gtk_builder_get_object( builder, "entry_descricao" ) );
   ctx->ui_diario.salvar_conteudo  = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_conteudo" ) );
   ctx->ui_diario.remover_registro = GTK_WIDGET( gtk_builder_get_object( builder, "button_remover_registro" ) );

   ctx->ui_diario.liststore_conteudo = GTK_LIST_STORE( gtk_builder_get_object( builder, "liststore_conteudo" ) );
   ctx->ui_diario.scrolled_window_conteudo    = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_window_conteudo" ) );
   ctx->ui_diario.treeview_conteudo  = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_conteudo" ) );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_conteudo, 1, 0.5 );

   //-- FREQUÊNCIA
   ctx->ui_diario.scrolled_window_frequencia = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_window_frequencia" ) );
   ctx->ui_diario.treeview_frequencia  = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_frequencia" ) );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_frequencia, 0, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_frequencia, 2, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_frequencia, 3, 0.5 );

   ctx->ui_diario.combo_data        = GTK_WIDGET( gtk_builder_get_object( builder, "combo_data" ) );
   ctx->ui_diario.label_ch          = GTK_WIDGET( gtk_builder_get_object( builder, "label_ch_freq" ) );
   ctx->ui_diario.salvar_frequencia = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_frequencia" ) );
   ctx->ui_diario.presente          = GTK_WIDGET( gtk_builder_get_object( builder, "button_presente" ) );
   ctx->ui_diario.ausente           = GTK_WIDGET( gtk_builder_get_object( builder, "button_ausente" ) );
   ctx->ui_diario.combo_status  = GTK_WIDGET( gtk_builder_get_object( builder, "combo_status" ) );



   ctx->entry.cor_destaque     = GTK_WIDGET( gtk_builder_get_object( builder, "combo_cor_serie" ) );
   ctx->entry.decoracao_estilo = GTK_WIDGET( gtk_builder_get_object( builder, "combo_decoracao" ) );

   // --- [ CHECKS / VALIDAÇÕES GLOBAL ] ---
   ctx->check.validar_ciclos = GTK_WIDGET( gtk_builder_get_object( builder, "check_cruz" ) );
   ctx->check.nao_presencial = GTK_WIDGET( gtk_builder_get_object( builder, "check_naopresencial" ) );
   ctx->check.expor_dados    = GTK_WIDGET( gtk_builder_get_object( builder, "check_expor" ) );

   // --- [ COLUNA 3 ORIGINAL / ABA Acervo / PROVAS ] ---
   // Se você optou por deixar o combo na aba Provas, garanta que o ID no Glade seja combo_tema_provas
   ctx->entry.tema         = GTK_WIDGET( gtk_builder_get_object( builder, "combo_tema" ) );
   ctx->entry.tema_espelho = GTK_WIDGET( gtk_builder_get_object( builder, "combo_tema_acervo" ) );

   ctx->button.abrir_pdf_acervo      = GTK_WIDGET( gtk_builder_get_object( builder, "button_pdf_latex" ) );
   ctx->button.compilar_latex_acervo = GTK_WIDGET( gtk_builder_get_object( builder, "button_compilar" ) );
   ctx->button.executar_gcc_acervo   = GTK_WIDGET( gtk_builder_get_object( builder, "button_gcc" ) );

   // --- [ COLUNA 1 ORIGINAL / ABA CONFIGURAÇÕES LATEX (BOTÕES DE RÁDIO) ] ---
   char id_string[64];
   for ( int i = 0; i < 2; i++ ) {
      snprintf( id_string, sizeof( id_string ), "radio_colunas_%d", i + 1 );
      ctx->radio.qtd_colunas[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      snprintf( id_string, sizeof( id_string ), "radio_separadores_%d", i + 1 );
      ctx->radio.separadores[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      snprintf( id_string, sizeof( id_string ), "radio_fonte_%d", i + 1 );
      ctx->radio.fonte_latex[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      snprintf( id_string, sizeof( id_string ), "radio_paginas_%d", i + 1 );
      ctx->radio.qtd_paginas[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      snprintf( id_string, sizeof( id_string ), "radio_cabecalho_%d", i + 1 );
      ctx->radio.cabecalho_tipo[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );
   }

   // Vetores de 3 opções (Avaliações e Estilos de Interface)
   for ( int i = 0; i < 3; i++ ) {
      snprintf( id_string, sizeof( id_string ), "radio_prova_%d", i + 1 );
      ctx->radio.avaliacao[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );

      snprintf( id_string, sizeof( id_string ), "radio_style_%d", i + 1 );
      ctx->radio.interface_style[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );
   }

   // --- [ COLUNA 4 ORIGINAL / DIÁRIO E MOTORES DE AÇÃO ] ---
   ctx->button.carregar_dados   = GTK_WIDGET( gtk_builder_get_object( builder, "button_carregar_dados" ) );
   ctx->button.frequencia       = GTK_WIDGET( gtk_builder_get_object( builder, "button_frequencia" ) );
   ctx->button.conteudos        = GTK_WIDGET( gtk_builder_get_object( builder, "button_conteudos" ) );
   ctx->button.avaliacoes       = GTK_WIDGET( gtk_builder_get_object( builder, "button_avaliacoes" ) );
   ctx->button.abrir            = GTK_WIDGET( gtk_builder_get_object( builder, "button_abrir" ) );
   ctx->button.relatorio_final  = GTK_WIDGET( gtk_builder_get_object( builder, "button_relatorio_final" ) );
   ctx->button.atualizar_alunos = GTK_WIDGET( gtk_builder_get_object( builder, "button_atualizar_alunos" ) );

   ctx->button.gerar_prova       = GTK_WIDGET( gtk_builder_get_object( builder, "button_gerar_prova" ) );
   ctx->button.corrigir_prova    = GTK_WIDGET( gtk_builder_get_object( builder, "button_corrigir_prova" ) );
   ctx->button.processamento_img = GTK_WIDGET( gtk_builder_get_object( builder, "button_processar_imagens" ) );

   ctx->latex.listbox_subtemas      = GTK_WIDGET( gtk_builder_get_object( builder, "listbox_subtemas_acervo" ) );
   ctx->provas.listbox_subtemas     = GTK_WIDGET( gtk_builder_get_object( builder, "listbox_subtemas" ) );
   ctx->provas.flowbox_selecionados = GTK_WIDGET( gtk_builder_get_object( builder, "flowbox_selecionados" ) );

   ctx->provas.scrolled_window = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_subtemas_selecionados" ) );


   // --- [ CONSOLE / TERMINAL DE FEEDBACK INFERIOR ] ---
   ctx->painel.container = GTK_WIDGET( gtk_builder_get_object( builder, "painel_feedback" ) );

   // int base_size = tamanho_fonte_px( ctx->window );
   // if ( base_size <= 0 ) base_size = 15; // Fallback de segurança caso Pango falhe
   // gtk_widget_set_size_request( ctx->painel.container, -1, 15*base_size );

   ctx->painel.cabecalho = GTK_WIDGET( gtk_builder_get_object( builder, "label_cabecalho" ) );

   ctx->painel.titulo    = GTK_WIDGET( gtk_builder_get_object( builder, "label_titulo" ) );
   ctx->painel.subtitulo = GTK_WIDGET( gtk_builder_get_object( builder, "label_subtitulo" ) );
   ctx->painel.instrucao = GTK_WIDGET( gtk_builder_get_object( builder, "label_instrucao" ) );

   gtk_label_set_line_wrap( GTK_LABEL( ctx->painel.titulo ),    TRUE );
   gtk_label_set_line_wrap( GTK_LABEL( ctx->painel.subtitulo ), TRUE );
   gtk_label_set_line_wrap( GTK_LABEL( ctx->painel.instrucao ), TRUE );

   // 3. Libera o objeto builder da memória, pois os ponteiros já foram guardados de forma segura
   g_object_unref( builder );
}
//=====================================================================================================//
