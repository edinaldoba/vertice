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




/**
 * @brief Aplica espaçamento uniforme intercalando Hair Spaces (\u200A)
 *        diretamente no texto de um GtkLabel. O espaçamento é aplicado
 *        entre TODOS os caracteres, incluindo espaços em branco, para
 *        manter a proporção visual simétrica entre letras e palavras.
 *
 * @param label_widget Ponteiro para o GtkWidget (GtkLabel).
 * @param quantidade_espacos Número de \u200A a serem inseridos (1 a 4).
 */
static void _ui_label_aplicar_espacamento_manual( GtkWidget *label_widget, gint quantidade_espacos ) {
   g_return_if_fail( GTK_IS_LABEL( label_widget ) );

   // 1. Extrai o texto do label
   const gchar *texto_original = gtk_label_get_text( GTK_LABEL( label_widget ) );
   if ( !texto_original || *texto_original == '\0' ) return;

   GString *resultado = g_string_new( "" );
   const gchar *p = texto_original;

   while ( *p != '\0' ) {
      gunichar c = g_utf8_get_char( p );
      const gchar *proximo = g_utf8_next_char( p );

      // Blindagem: ignora Hair Spaces (\u200A) pré-existentes para evitar duplicação
      if ( c == 0x200A ) {
         p = proximo;
         continue;
      }

      // Copia o caractere atual para a nova string
      g_string_append_unichar( resultado, c );

      gunichar proximo_c = g_utf8_get_char( proximo );

      // Injeta a quantidade desejada de Hair Spaces (\u200A) entre TODOS os caracteres,
      // desde que não seja o fim da string e o próximo não seja um Hair Space acidental.
      if ( *proximo != '\0' && proximo_c != 0x200A ) {
         for ( gint i = 0; i < quantidade_espacos; i++ ) {
            g_string_append( resultado, "\u200A" );
         }
      }

      p = proximo;
   }

   // 2. Aplica o texto formatado de volta no label
   gtk_label_set_text( GTK_LABEL( label_widget ), resultado->str );

   // 3. Libera a memória da estrutura GString
   g_string_free( resultado, TRUE );
}


/**
 * @brief Extrai o GtkLabel interno de um botão configurado pelo Glade e aplica
 *        o espaçamento manual usando Hair Spaces.
 *
 * @param button_widget Ponteiro para o GtkWidget do botão (GtkButton).
 * @param quantidade_espacos Número de \u200A a serem inseridos entre cada letra (1 a 4).
 */
static void ui_button_label_aplicar_espacamento_manual( GtkWidget *button_widget, gint quantidade_espacos ) {
   g_return_if_fail( GTK_IS_BUTTON( button_widget ) );

   // 1. Resgata o GtkLabel interno do botão
   GtkWidget *label_interno = gtk_bin_get_child( GTK_BIN( button_widget ) );

   // 2. Passa a responsabilidade para a função base
   if ( GTK_IS_LABEL( label_interno ) ) {
      _ui_label_aplicar_espacamento_manual( label_interno, quantidade_espacos );
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
   ctx->notebook = GTK_WIDGET( gtk_builder_get_object( builder, "notebook_principal" ) );

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

   ctx->entry.estilo    = GTK_WIDGET( gtk_builder_get_object( builder, "combo_estilo" ) );

   ctx->cabecalho.gestor    = GTK_WIDGET( gtk_builder_get_object( builder, "box_gestor" ) );
   ctx->cabecalho.professor = GTK_WIDGET( gtk_builder_get_object( builder, "box_professor" ) );

   ctx->stack_pages        = GTK_WIDGET( gtk_builder_get_object( builder, "stack_pages" ) );
   ctx->ui_diario.check_modo_edicao = GTK_WIDGET( gtk_builder_get_object( builder, "check_modo_edicao" ) );


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
   ctx->ui_diario.btn_salvar_conteudo  = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_conteudo" ) );
   ctx->ui_diario.remover_registro = GTK_WIDGET( gtk_builder_get_object( builder, "button_remover_registro" ) );

   ctx->ui_diario.scrolled_window_conteudo    = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_window_conteudo" ) );
   ctx->ui_diario.treeview_conteudo  = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_conteudo" ) );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_conteudo, 1, 0.5 );

   //-- FREQUÊNCIA
   ctx->ui_diario.scrolled_window_frequencia = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_window_frequencia" ) );
   ctx->ui_diario.treeview_frequencia  = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_frequencia" ) );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_frequencia, 0, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_frequencia, 2, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_frequencia, 3, 0.5 );

   ctx->ui_diario.combo_data            = GTK_WIDGET( gtk_builder_get_object( builder, "combo_data" ) );
   ctx->ui_diario.label_ch              = GTK_WIDGET( gtk_builder_get_object( builder, "label_ch_freq" ) );
   ctx->ui_diario.check_por_aluno       = GTK_WIDGET( gtk_builder_get_object( builder, "check_chamada_por_aluno" ) );
   ctx->ui_diario.combo_alunos          = GTK_WIDGET( gtk_builder_get_object( builder, "combo_alunos" ) );
   ctx->ui_diario.btn_salvar_frequencia = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_frequencia" ) );
   ctx->ui_diario.btn_presente          = GTK_WIDGET( gtk_builder_get_object( builder, "button_presente" ) );
   ctx->ui_diario.btn_ausente           = GTK_WIDGET( gtk_builder_get_object( builder, "button_ausente" ) );
   ctx->ui_diario.btn_justificada       = GTK_WIDGET( gtk_builder_get_object( builder, "button_justificada" ) );
   ctx->ui_diario.combo_status          = GTK_WIDGET( gtk_builder_get_object( builder, "combo_status" ) );

   // AVALIAÇÕES
   ctx->ui_diario.scrolled_window_avaliacoes = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_window_avaliacoes" ) );
   ctx->ui_diario.treeview_avaliacoes        = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_avaliacoes" ) );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_avaliacoes, 0, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_avaliacoes, 2, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_avaliacoes, 3, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_avaliacoes, 4, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_avaliacoes, 5, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_avaliacoes, 6, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_avaliacoes, 7, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_avaliacoes, 8, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_avaliacoes, 9, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_avaliacoes, 10, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_avaliacoes, 11, 0.5 );

   ctx->ui_diario.combo_avaliacoes          = GTK_WIDGET( gtk_builder_get_object( builder, "combo_avaliacoes" ) );
   ctx->ui_diario.btn_editar_avaliacao      = GTK_WIDGET( gtk_builder_get_object( builder, "button_editar_avaliacao" ) );
   ctx->ui_diario.btn_nova_avaliacao        = GTK_WIDGET( gtk_builder_get_object( builder, "button_nova_avaliacao" ) );
   ctx->ui_diario.check_desativar_avaliacao = GTK_WIDGET( gtk_builder_get_object( builder, "check_desativar_avaliacao" ) );

   ctx->ui_diario.popover_nomear_avaliacao = GTK_WIDGET( gtk_builder_get_object( builder, "popover_nomear_avaliacao" ) );
   ctx->ui_diario.label_popover_avaliacao  = GTK_WIDGET( gtk_builder_get_object( builder, "label_popover_avaliacao" ) );
   ctx->ui_diario.entry_popover_nomear  = GTK_WIDGET( gtk_builder_get_object( builder, "entry_popover_nomear" ) );
   ctx->ui_diario.btn_popover_nomear    = GTK_WIDGET( gtk_builder_get_object( builder, "button_popover_avaliacao" ) );

   ctx->ui_diario.btn_salvar_avaliacoes = GTK_WIDGET( gtk_builder_get_object( builder, "button_salvar_avaliacao" ) );


   // RELATÓRIO
   ctx->ui_diario.scrolled_window_relatorio = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_window_relatorio" ) );
   ctx->ui_diario.btn_consolidar        = GTK_WIDGET( gtk_builder_get_object( builder, "button_consolidar" ) );
   ctx->ui_diario.treeview_relatorio        = GTK_WIDGET( gtk_builder_get_object( builder, "treeview_relatorio" ) );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_relatorio, 0, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_relatorio, 2, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_relatorio, 3, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_relatorio, 4, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_relatorio, 5, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_relatorio, 6, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_relatorio, 7, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_relatorio, 8, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_relatorio, 9, 0.5 );
   treeview_alinhar_coluna_renderizada( ctx->ui_diario.treeview_relatorio, 10, 0.5 );



   ui_button_label_aplicar_espacamento_manual( ctx->ui_diario.btn_consolidar, 1 );
   ui_button_label_aplicar_espacamento_manual( ctx->ui_diario.btn_nova_avaliacao, 1 );
   ui_button_label_aplicar_espacamento_manual( ctx->ui_diario.btn_salvar_avaliacoes, 1 );
   ui_button_label_aplicar_espacamento_manual( ctx->ui_diario.btn_salvar_conteudo, 1 );
   ui_button_label_aplicar_espacamento_manual( ctx->ui_diario.btn_salvar_frequencia, 1 );
   ui_button_label_aplicar_espacamento_manual( ctx->ui_diario.btn_popover_nomear, 1 );


   GtkWidget *label = NULL;
   const gchar *labels[7] = {
      "label_conteudo"   , "label_frequencia", "label_avaliacoes", "label_relatorio",
      "label_tema_acervo", "label_tema_prova", "label_montagem_da_prova"
   };

   for ( int i = 0; i < 7; i++ ) {
      label = GTK_WIDGET( gtk_builder_get_object( builder, labels[i] ) );
      if ( label ) {
         _ui_label_aplicar_espacamento_manual( label, 1 );
      } else {
         g_warning( "Aviso: Widget '%s' não foi encontrado no GtkBuilder.", labels[i] );
      }
   }




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

   ctx->button.abrir_pdf_acervo = GTK_WIDGET( gtk_builder_get_object( builder, "button_pdf_latex" ) );
   ctx->button.compilar_latex_acervo = GTK_WIDGET( gtk_builder_get_object( builder, "button_compilar" ) );
   ctx->button.executar_gcc_acervo   = GTK_WIDGET( gtk_builder_get_object( builder, "button_gcc" ) );

   ui_button_label_aplicar_espacamento_manual( ctx->button.abrir_pdf_acervo, 1 );
   ui_button_label_aplicar_espacamento_manual( ctx->button.compilar_latex_acervo, 1 );
   ui_button_label_aplicar_espacamento_manual( ctx->button.executar_gcc_acervo, 1 );

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

   // Vetores de 3 opções (Sequência Avaliações)
   for ( int i = 0; i < 3; i++ ) {
      snprintf( id_string, sizeof( id_string ), "radio_prova_%d", i + 1 );
      ctx->radio.avaliacao[i] = GTK_WIDGET( gtk_builder_get_object( builder, id_string ) );
   }

   // --- [ COLUNA 4 ORIGINAL / DIÁRIO E MOTORES DE AÇÃO ] ---
   ctx->button.importar_dados   = GTK_WIDGET( gtk_builder_get_object( builder, "button_importar_dados" ) );
   ctx->button.frequencia       = GTK_WIDGET( gtk_builder_get_object( builder, "button_frequencia" ) );
   ctx->button.conteudos        = GTK_WIDGET( gtk_builder_get_object( builder, "button_conteudos" ) );
   ctx->button.avaliacoes       = GTK_WIDGET( gtk_builder_get_object( builder, "button_avaliacoes" ) );
   ctx->button.relatorio_final  = GTK_WIDGET( gtk_builder_get_object( builder, "button_relatorio_final" ) );
   ctx->button.atualizar_alunos = GTK_WIDGET( gtk_builder_get_object( builder, "button_atualizar_alunos" ) );

   ui_button_label_aplicar_espacamento_manual( ctx->button.importar_dados, 1 );
   ui_button_label_aplicar_espacamento_manual( ctx->button.frequencia, 1 );
   ui_button_label_aplicar_espacamento_manual( ctx->button.conteudos, 1 );
   ui_button_label_aplicar_espacamento_manual( ctx->button.avaliacoes, 1 );
   ui_button_label_aplicar_espacamento_manual( ctx->button.relatorio_final, 1 );

   ctx->button.gerar_prova       = GTK_WIDGET( gtk_builder_get_object( builder, "button_gerar_prova" ) );
   ctx->button.corrigir_prova    = GTK_WIDGET( gtk_builder_get_object( builder, "button_corrigir_prova" ) );
   ctx->button.processar_imagens = GTK_WIDGET( gtk_builder_get_object( builder, "button_processar_imagens" ) );

   ui_button_label_aplicar_espacamento_manual( ctx->button.gerar_prova, 1 );
   ui_button_label_aplicar_espacamento_manual( ctx->button.corrigir_prova, 1 );
   ui_button_label_aplicar_espacamento_manual( ctx->button.processar_imagens, 1 );

   ctx->latex.listbox_subtemas      = GTK_WIDGET( gtk_builder_get_object( builder, "listbox_subtemas_acervo" ) );
   ctx->provas.listbox_subtemas     = GTK_WIDGET( gtk_builder_get_object( builder, "listbox_subtemas" ) );
   ctx->provas.flowbox_selecionados = GTK_WIDGET( gtk_builder_get_object( builder, "flowbox_selecionados" ) );

   ctx->provas.scrolled_window = GTK_WIDGET( gtk_builder_get_object( builder, "scrolled_subtemas_selecionados" ) );


   // --- [ CONSOLE / TERMINAL DE FEEDBACK INFERIOR ] ---
   ctx->painel.eventbox_painel = GTK_WIDGET( gtk_builder_get_object( builder, "eventbox_painel" ) );
   if ( ctx->painel.eventbox_painel ) {
      // Registra os eventos de entrada e saída de ponteiro no EventBox
      gtk_widget_add_events( ctx->painel.eventbox_painel, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK );
   }
   ctx->painel.container = GTK_WIDGET( gtk_builder_get_object( builder, "painel_feedback" ) );
   ctx->painel.revealer_painel = GTK_WIDGET( gtk_builder_get_object( builder, "revealer_painel_feedback" ) );
   ctx->painel.cabecalho = GTK_WIDGET( gtk_builder_get_object( builder, "label_cabecalho" ) );
   ctx->painel.titulo    = GTK_WIDGET( gtk_builder_get_object( builder, "label_titulo" ) );
   ctx->painel.subtitulo = GTK_WIDGET( gtk_builder_get_object( builder, "label_subtitulo" ) );
   ctx->painel.instrucao = GTK_WIDGET( gtk_builder_get_object( builder, "label_instrucao" ) );

   gtk_label_set_line_wrap( GTK_LABEL( ctx->painel.titulo ),    TRUE );
   gtk_label_set_line_wrap( GTK_LABEL( ctx->painel.subtitulo ), TRUE );
   gtk_label_set_line_wrap( GTK_LABEL( ctx->painel.instrucao ), TRUE );

   ctx->painel.eventbox_orelhinha = GTK_WIDGET( gtk_builder_get_object( builder, "eventbox_orelhinha" ) );
   if ( ctx->painel.eventbox_orelhinha ) {
      gtk_widget_add_events( ctx->painel.eventbox_orelhinha, GDK_BUTTON_PRESS_MASK );
   }

   // 3. Libera o objeto builder da memória, pois os ponteiros já foram guardados de forma segura
   g_object_unref( builder );
}
//=====================================================================================================//
