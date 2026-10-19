/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include "signals.h"
#include "acervo.h"
#include "comum.h"
#include "interface.h"
#include "callbacks.h"
#include "imagens.h"
#include "relatorios.h"
#include "acervo.h"





/**
 * @brief Atribui metadados GObject (categoria e valor) aos radio buttons e nomeia widgets para o CSS.
 */
static void _configurar_nomes_dos_widgets( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );

   // 1. Nomes específicos para seletores do CSS
   gtk_widget_set_name( ctx->entry.tema, "tema" );
   gtk_widget_set_name( ctx->check.validar_ciclos, "Números Cruzados" );
   gtk_widget_set_name( ctx->check.nao_presencial, "Não Presencial" );
   gtk_widget_set_name( ctx->check.expor_dados, "Expôr Dados" );

   g_object_set_data( G_OBJECT( ctx->check.validar_ciclos ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_NUM_CRUZADOS ) );
   g_object_set_data( G_OBJECT( ctx->check.nao_presencial ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_NAO_PRESENCIAL ) );
   g_object_set_data( G_OBJECT( ctx->check.expor_dados ),    "categoria", GINT_TO_POINTER( CATEGORIA_UI_EXPOR_DADOS ) );

   // 2. Categoria 1: Colunas (Valor 2 ou 3)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( ctx->radio.qtd_colunas[i], "Colunas" );
      g_object_set_data( G_OBJECT( ctx->radio.qtd_colunas[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_COLUNAS ) );
      g_object_set_data( G_OBJECT( ctx->radio.qtd_colunas[i] ), "valor",     GINT_TO_POINTER( i + 2 ) ); // 2 ou 3
   }

   // 3. Categoria 2: Separadores (Valor 1 = Pontilhado, Valor 2 = Contínuo)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( ctx->radio.separadores[i], "Separadores" );
      g_object_set_data( G_OBJECT( ctx->radio.separadores[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_SEPARADORES ) );
      g_object_set_data( G_OBJECT( ctx->radio.separadores[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1 ou 2
   }

   // 4. Categoria 3: Fonte TeX (Valor 1 = CMB Right, Valor 2 = CMU Serif)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( ctx->radio.fonte_latex[i], "Fonte TeX" );
      g_object_set_data( G_OBJECT( ctx->radio.fonte_latex[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_FONTE_LATEX ) );
      g_object_set_data( G_OBJECT( ctx->radio.fonte_latex[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1 ou 2
   }

   // 5. Categoria 4: Quantidade de Páginas (Valor 1 ou 2)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( ctx->radio.qtd_paginas[i], "Qtd Páginas" );
      g_object_set_data( G_OBJECT( ctx->radio.qtd_paginas[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_PAGINAS ) );
      g_object_set_data( G_OBJECT( ctx->radio.qtd_paginas[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1 ou 2
   }

   // 6. Categoria 5: Tipo de Cabeçalho (Valor 1 = Acima, Valor 2 = À esquerda)
   for ( int i = 0; i < 2; i++ ) {
      gtk_widget_set_name( ctx->radio.cabecalho_tipo[i], "Cabeçalho" );
      g_object_set_data( G_OBJECT( ctx->radio.cabecalho_tipo[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_CABECALHO ) );
      g_object_set_data( G_OBJECT( ctx->radio.cabecalho_tipo[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1 ou 2
   }

   // 7. Categoria 6: Sequência Prova (Valor 1 = 1ª, Valor 2 = 2ª, Valor 3 = 3ª)
   for ( int i = 0; i < 3; i++ ) {
      gtk_widget_set_name( ctx->radio.avaliacao[i], "Seq Prova" );
      g_object_set_data( G_OBJECT( ctx->radio.avaliacao[i] ), "categoria", GINT_TO_POINTER( CATEGORIA_UI_PROVA ) );
      g_object_set_data( G_OBJECT( ctx->radio.avaliacao[i] ), "valor",     GINT_TO_POINTER( i + 1 ) ); // 1, 2 ou 3
   }
}




/**
 * @brief Função auxiliar universal para configurar renderizadores de texto editáveis em um intervalo de colunas.
 *
 * @param ctx Contexto global da aplicação.
 * @param treeview Ponteiro para o GtkTreeView alvo.
 * @param col_inicio Índice numérico da primeira coluna editável.
 * @param col_fim Índice numérico da última coluna editável.
 */
static void _configurar_colunas_editaveis( AppContext *ctx, GtkTreeView *treeview, int col_inicio, int col_fim ) {
   g_return_if_fail( GTK_IS_TREE_VIEW( treeview ) );

   for ( int col_idx = col_inicio; col_idx <= col_fim; col_idx++ ) {
      GtkTreeViewColumn *col = gtk_tree_view_get_column( treeview, col_idx );
      if ( !col ) continue;

      GList *renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( col ) );
      if ( renderers && GTK_IS_CELL_RENDERER_TEXT( renderers->data ) ) {
         GtkCellRendererText *r = GTK_CELL_RENDERER_TEXT( renderers->data );

         // 1. Garante que o renderizador permite edição
         g_object_set( G_OBJECT( r ), "editable", TRUE, NULL );

         // 2. Salva os metadados (índice da coluna e ponteiro do TreeView)
         g_object_set_data( G_OBJECT( r ), "col_model_idx", GINT_TO_POINTER( col_idx ) );
         g_object_set_data( G_OBJECT( r ), "ptr_treeview", treeview );

         // 3. Conecta os callbacks UNIVERSAIS
         g_signal_connect( r, "editing-started", G_CALLBACK( on_cell_nota_editing_started ), ctx );
         g_signal_connect( r, "edited",          G_CALLBACK( on_cell_nota_edited ),          ctx );

         g_list_free( renderers );
      }
   }
}











static int _cor_texto_tipo_registro( int tipo_registro, int tema_ativo, GdkRGBA *cor_out ) {
   if ( !cor_out ) return 0;

   // =====================================================================
   // 1. TIPO: Atividade Pedagógica
   // =====================================================================
   if ( tipo_registro == TIPO_REGISTRO_PEDAGOGICO ) {
      if ( tema_ativo == 1 ) { // Deep Blue
         *cor_out = ( GdkRGBA ) { 0.50, 0.88, 1.00, 1.0 };
      } else if ( tema_ativo == 2 ) { // Light
         *cor_out = ( GdkRGBA ) { 0.00, 0.00, 1.00, 1.0 };
      } else { // Dark Green (Padrão)
         *cor_out = ( GdkRGBA ) { 0.39, 0.71, 0.96, 1.0 };
      }
      return 1;
   }

   // =====================================================================
   // 2. TIPO: Feriado
   // =====================================================================
   if ( tipo_registro == TIPO_REGISTRO_FERIADO ) {
      if ( tema_ativo == 1 ) { // Deep Blue
         *cor_out = ( GdkRGBA ) { 0.96, 0.40, 0.50, 1.0 };
      } else if ( tema_ativo == 2 ) { // Light
         *cor_out = ( GdkRGBA ) { 0.85, 0.00, 0.00, 1.0 };
      } else { // Dark Green (Padrão)
         *cor_out = ( GdkRGBA ) { 0.90, 0.45, 0.45, 1.0 };
      }
      return 1;
   }

   // =====================================================================
   // 3. TIPO: Aula Extra
   // =====================================================================
   if ( tipo_registro == TIPO_REGISTRO_AULA_EXTRA ) {
      if ( tema_ativo == 1 ) { // Deep Blue
         *cor_out = ( GdkRGBA ) { 1.00, 0.79, 0.16, 1.0 }; // #FFCA28
      } else if ( tema_ativo == 2 ) { // Light
         *cor_out = ( GdkRGBA ) { 0.62, 0.49, 0.10, 1.0 }; // ##9d7c19
      } else { // Dark Green (Padrão)
         *cor_out = ( GdkRGBA ) { 1.00, 0.84, 0.31, 1.0 }; // #FFD54F
      }
      return 1;
   }

   return 0; // Fallback de segurança
}

//===================================================================================================
// FUNÇÃO DE APLICAÇÃO VISUAL DA COR
//===================================================================================================
static void _aplicar_cor_condicional_conteudo( GtkCellRenderer *cell, int tipo_registro, int interface_style ) {
   GdkRGBA cor;

   // Se for Feriado ou Pedagógico, a função preenche a cor correta do tema e retorna 1
   if ( _cor_texto_tipo_registro( tipo_registro, interface_style, &cor ) ) {
      g_object_set( cell, "foreground-rgba", &cor, "foreground-set", TRUE, NULL );
   } else {
      // Fallback: Aulas normais usam a cor padrão do GTK para o tema
      g_object_set( cell, "foreground-set", FALSE, NULL );
   }
}

//===================================================================================================
// CALLBACK DO GTK (Cell Data Func)
//===================================================================================================
static void _on_renderizar_cores_conteudo( GtkTreeViewColumn *col, GtkCellRenderer *cell,
                                          GtkTreeModel *model, GtkTreeIter *iter, gpointer data ) {
   ( void )col;
   AppContext *ctx = ( AppContext * )data;

   int tipo_registro = 0;

   // Captura o tipo de registro da coluna 4 silenciosamente
   gtk_tree_model_get( model, iter, 4, &tipo_registro, -1 );

   // Aplica a cor na célula atual
   _aplicar_cor_condicional_conteudo( cell, tipo_registro, ctx->dados.interface_style );
}

static void _inicializar_renderizadores_conteudo( AppContext *ctx ) {
   GtkTreeView *tree_view = GTK_TREE_VIEW( ctx->ui_diario.treeview_conteudo );

   // Aplica para as colunas visíveis que precisam da cor (ex: Data, Tema, Descrição...)
   // Supondo que sejam as colunas 0, 1, 2 e 3 do TreeView
   for ( int i = 0; i <= 3; i++ ) {
      GtkTreeViewColumn *col = gtk_tree_view_get_column( tree_view, i );
      if ( !col ) continue;

      GList *renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( col ) );
      if ( renderers ) {
         GtkCellRenderer *renderer = GTK_CELL_RENDERER( renderers->data );
         gtk_tree_view_column_set_cell_data_func( col, renderer, _on_renderizar_cores_conteudo, ctx, NULL );
         g_list_free( renderers );
      }
   }
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
         *cor_out = ( GdkRGBA ) {
            0.40, 0.45, 0.55, 1.0
         };
      } else if ( tema_ativo == 2 ) { // Light (Cinza padrão)
         *cor_out = ( GdkRGBA ) {
            0.60, 0.60, 0.60, 1.0
         };
      } else { // Dark Green - Padrão (Cinza esverdeado escuro)
         *cor_out = ( GdkRGBA ) {
            0.45, 0.50, 0.45, 1.0
         };
      }
      return 1;
   }

   // =====================================================================
   // 3. ALERTAS AVERMELHADOS: Ausente e Suspenso
   // =====================================================================
   if ( status == AUSENTE || status == SUSPENSO || status == FOI_EMBORA ) {
      if ( tema_ativo == 1 ) { // Deep Blue
         *cor_out = ( GdkRGBA ) {
            0.96, 0.40, 0.50, 1.0
         };
      } else if ( tema_ativo == 2 ) { // Light
         *cor_out = ( GdkRGBA ) {
            0.85, 0.00, 0.00, 1.0
         };
      } else { // Dark Green (Padrão)
         *cor_out = ( GdkRGBA ) {
            0.90, 0.45, 0.45, 1.0
         };
      }
      return 1;
   }

   // =====================================================================
   // 4. AVISOS AMARELADOS: Fora de sala e Foi embora
   // =====================================================================
   if ( status == FORA_DE_SALA || status == DISPENSADO ) {
      if ( tema_ativo == 1 ) { // Deep Blue
         *cor_out = ( GdkRGBA ) {
            1.00, 0.79, 0.16, 1.0
         }; // #FFCA28
      } else if ( tema_ativo == 2 ) { // Light
         *cor_out = ( GdkRGBA ) {
            0.62, 0.49, 0.10, 1.0
         }; // #9D7C19
      } else { // Dark Green (Padrão)
         *cor_out = ( GdkRGBA ) {
            1.00, 0.84, 0.31, 1.0
         }; // #FFD54F
      }
      return 1;
   }

   // =====================================================================
   // 5. DESTAQUE (ROXO/PÚRPURA): Atividade Domiciliar (Requer ação extra)
   // =====================================================================
   if ( status == ATIVIDADE_DOMICILIAR ) {
      if ( tema_ativo == 1 ) { // Deep Blue (Lilás vibrante)
         *cor_out = ( GdkRGBA ) {
            0.80, 0.60, 0.95, 1.0
         };
      } else if ( tema_ativo == 2 ) { // Light (Roxo escuro)
         *cor_out = ( GdkRGBA ) {
            0.45, 0.15, 0.60, 1.0
         };
      } else { // Dark Green - Padrão (Púrpura suave)
         *cor_out = ( GdkRGBA ) {
            0.75, 0.55, 0.85, 1.0
         };
      }
      return 1;
   }

   // =====================================================================
   // 5. DESTAQUE (ROXO/PÚRPURA): Atividade Domiciliar (Requer ação extra)
   // =====================================================================
   if ( status == FALTA_JUSTIFICADA ) {
      if ( tema_ativo == 1 ) { // Deep Blue (Verde-azulado / Turquesa vibrante)
         *cor_out = ( GdkRGBA ) {
            0.25, 0.85, 0.75, 1.0
         };
      } else if ( tema_ativo == 2 ) { // Light (Verde escuro / Floresta - alto contraste em fundo claro)
         *cor_out = ( GdkRGBA ) {
            0.15, 0.55, 0.25, 1.0
         };
      } else { // Dark Green - Padrão (Verde suave / Menta claro - legível em fundo escuro)
         *cor_out = ( GdkRGBA ) {
            0.50, 0.85, 0.55, 1.0
         };
      }
      return 1;
   }

   return 0; // Fallback de segurança
}



//===================================================================================================
// FUNÇÃO DE APLICAÇÃO VISUAL DA COR (FREQUÊNCIA)
//===================================================================================================
static void _aplicar_cor_condicional_frequencia( GtkCellRenderer *cell, int status, int interface_style ) {
   GdkRGBA cor;

   // A sua função original que calcula as cores de frequência já retorna 1 se houver cor!
   if ( _cor_texto_linha_frequencia( status, interface_style, &cor ) ) {
      g_object_set( cell, "foreground-rgba", &cor, "foreground-set", TRUE, NULL );
   } else {
      g_object_set( cell, "foreground-set", FALSE, NULL );
   }
}

//===================================================================================================
// CALLBACK DO GTK (Cell Data Func)
//===================================================================================================
static void _on_renderizar_cores_frequencia( GtkTreeViewColumn *col, GtkCellRenderer *cell,
                                            GtkTreeModel *model, GtkTreeIter *iter, gpointer data ) {
   ( void )col;
   AppContext *ctx = ( AppContext * )data;

   int status = 0;

   // Captura o status puro da coluna 5
   gtk_tree_model_get( model, iter, 5, &status, -1 );

   // Delega a pintura
   _aplicar_cor_condicional_frequencia( cell, status, ctx->dados.interface_style );
}

//===================================================================================================
// INICIALIZADOR (Chame UMA VEZ no seu signals.c ou na inicialização da interface)
//===================================================================================================
static void _inicializar_renderizadores_frequencia( AppContext *ctx ) {
   GtkTreeView *tree_view = GTK_TREE_VIEW( ctx->ui_diario.treeview_frequencia );

   // Aplica nas colunas: Nome (1), Data/Nascimento (2) e Status Textual (3)
   for ( int i = 1; i <= 3; i++ ) {
      GtkTreeViewColumn *col = gtk_tree_view_get_column( tree_view, i );
      if ( !col ) continue;

      GList *renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( col ) );
      if ( renderers ) {
         GtkCellRenderer *renderer = GTK_CELL_RENDERER( renderers->data );
         gtk_tree_view_column_set_cell_data_func( col, renderer, _on_renderizar_cores_frequencia, ctx, NULL );
         g_list_free( renderers );
      }
   }
}







typedef enum {
   TOM_CINZA = 0,
   TOM_VERMELHO = 1
} TomCor;

static int _cor_cinza_inativo_e_nota_vermelha( TomCor tom, int tema_ativo, GdkRGBA *cor_out ) {
   if ( !cor_out ) return 0;

   if ( tom == TOM_CINZA ) {
      if ( tema_ativo == 1 ) { // Deep Blue (Cinza azulado escuro)
         *cor_out = ( GdkRGBA ) { 0.40, 0.45, 0.55, 1.0 };
      } else if ( tema_ativo == 2 ) { // Light (Cinza padrão)
         *cor_out = ( GdkRGBA ) { 0.60, 0.60, 0.60, 1.0 };
      } else { // Dark Green - Padrão (Cinza esverdeado escuro)
         *cor_out = ( GdkRGBA ) { 0.45, 0.50, 0.45, 1.0 };
      }
      return 1;
   }

   if ( tom == TOM_VERMELHO ) {
      if ( tema_ativo == 1 ) { // Deep Blue
         *cor_out = ( GdkRGBA ) { 0.96, 0.40, 0.50, 1.0 };
      } else if ( tema_ativo == 2 ) { // Light
         *cor_out = ( GdkRGBA ) { 0.85, 0.00, 0.00, 1.0 };
      } else { // Dark Green (Padrão)
         *cor_out = ( GdkRGBA ) { 0.90, 0.45, 0.45, 1.0 };
      }
      return 1;
   }

   return 0; // Fallback de segurança
}


//===================================================================================================
// FUNÇÃO AUXILIAR: Carrega as notas da Memória (GArray) para a Interface Visual (GtkListStore)
//===================================================================================================
static void _aplicar_cor_condicional_nota( GtkCellRenderer *cell, gboolean inativo,
                                           const gchar *texto_nota, int interface_style ) {
   // 1. Tratamento para alunos inativos (Transferidos, etc.)
   if ( inativo ) {
      GdkRGBA cor;
      _cor_cinza_inativo_e_nota_vermelha( TOM_CINZA, interface_style, &cor ); // Cor cinza/opaca
      g_object_set( cell, "foreground-rgba", &cor, "foreground-set", TRUE, NULL );
      return;
   }

   // 2. Tratamento da nota digitada (Vermelho para < 6.0, Padrão para >= 6.0)
   if ( texto_nota && texto_nota[0] != '\0' && g_strcmp0( texto_nota, "-" ) != 0 ) {
      const gchar *ponteiro_leitura = texto_nota;

      // Avança o ponteiro caso o caractere de alinhamento tabular (Figure Space) esteja presente
      if ( g_str_has_prefix( ponteiro_leitura, "\u2007" ) ) {
         ponteiro_leitura += 3;
      }

      // Buffer local limpo para garantir a leitura correta do float, trocando ',' por '.' em RAM O(1)
      char buffer_float[16];
      g_strlcpy( buffer_float, ponteiro_leitura, sizeof( buffer_float ) );
      g_strdelimit( buffer_float, ",", '.' );

      double nota = g_ascii_strtod( buffer_float, NULL );

      if ( nota >= 0.0 && nota < 6.0 ) {
         GdkRGBA cor;
         _cor_cinza_inativo_e_nota_vermelha( TOM_VERMELHO, interface_style, &cor ); // Cor vermelha de reprovação
         g_object_set( cell, "foreground-rgba", &cor, "foreground-set", TRUE, NULL );
         return;
      }
   }

   // 3. Fallback: Desliga a sobreposição de cor, usando a cor padrão do GTK
   g_object_set( cell, "foreground-set", FALSE, NULL );
}
//============================================================================================================





//============================================================================================================
static void _on_renderizar_cores_notas( GtkTreeViewColumn *col, GtkCellRenderer *cell, GtkTreeModel *model,
                                       GtkTreeIter *iter, gpointer data ) {
   ( void )col;
   AppContext *ctx = ( AppContext * )data;
   gboolean inativo = FALSE;
   gtk_tree_model_get( model, iter, 12, &inativo, -1 ); // Coluna 12 para Avaliações

   gint col_idx = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( cell ), "col_model_idx" ) );
   gchar *texto = NULL;
   if ( col_idx >= 2 && col_idx <= 11 ) g_object_get( cell, "text", &texto, NULL );

   _aplicar_cor_condicional_nota( cell, inativo, texto, ctx->dados.interface_style );
   g_free( texto );
}
//----------------------------------------------------------------------------------------------------------------
static void _inicializar_renderizadores_avaliacoes( AppContext *ctx ) {
   g_return_if_fail( ctx );

   // =====================================================================
   // 1. CONECTA RENDERIZADOR DA ABA DE AVALIAÇÕES (Colunas 1 a 11)
   // =====================================================================
   GtkTreeView *tree_avaliacoes = GTK_TREE_VIEW( ctx->ui_diario.treeview_avaliacoes );
   if ( tree_avaliacoes ) {
      for ( int i = 1; i <= 11; i++ ) {
         GtkTreeViewColumn *col = gtk_tree_view_get_column( tree_avaliacoes, i );
         if ( !col ) continue;

         GList *renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( col ) );
         if ( renderers ) {
            GtkCellRenderer *renderer = GTK_CELL_RENDERER( renderers->data );
            g_object_set_data( G_OBJECT( renderer ), "col_model_idx", GINT_TO_POINTER( i ) );
            gtk_tree_view_column_set_cell_data_func( col, renderer, _on_renderizar_cores_notas, ctx, NULL );
            g_list_free( renderers );
         }
      }
   }
}
//============================================================================================================




//============================================================================================================
//===================================================================================================
// FUNÇÃO DEDICADA: Cores de fundo suaves para células que requerem ação (ex: Matrícula Tardia)
//===================================================================================================
static int _cor_fundo_celula_pendente( int tema_ativo, GdkRGBA *cor_out ) {
   if ( !cor_out ) return 0;

   if ( tema_ativo == 1 ) {
      // Deep Blue (Tema Escuro Azulado)
      // Fundo: Um azul/roxo escuro levemente destacado do fundo normal, sem ofuscar a visão
      *cor_out = ( GdkRGBA ) { 0.18, 0.25, 0.38, 1.0 };
   }
   else if ( tema_ativo == 2 ) {
      // Light (Tema Claro)
      // Fundo: Um amarelo/creme bem pastel (pêssego claro). Destaca sem brigar com a nota vermelha.
      *cor_out = ( GdkRGBA ) { 1.00, 0.96, 0.82, 1.0 };
   }
   else {
      // Dark Green - Padrão (Tema Escuro Esverdeado)
      // Fundo: Um tom oliva/âmbar mutado e discreto.
      *cor_out = ( GdkRGBA ) { 0.20, 0.30, 0.20, 1.0 };
   }

   return 1;
}
//--------------------------------------------------------------------------------------------------------
static void _on_renderizar_cores_relatorio( GtkTreeViewColumn *col, GtkCellRenderer *cell,
                                           GtkTreeModel *model, GtkTreeIter *iter, gpointer data ) {
   ( void )col;
   AppContext *ctx = ( AppContext * )data;

   gboolean inativo = FALSE;
   int mascara_pendencias = 0;

   // (Ajuste o índice para ler a coluna 13 onde você guardou a máscara)
   gtk_tree_model_get( model, iter, 11, &inativo, 12, &mascara_pendencias, -1 );

   gint col_idx = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( cell ), "col_model_idx" ) );

   // Verifica o estado da "chave mestra" de edição
   gboolean modo_pendencias = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( ctx->ui_diario.check_pendencias ) );

   gboolean editavel = FALSE;
   GdkRGBA cor_fundo;
   gboolean usar_cor_fundo = FALSE;

   // Se for coluna de Período Letivo (bits 0 a 3)
   if ( modo_pendencias && !inativo && col_idx >= 2 && col_idx <= 4 ) {
      int p = col_idx - 2;
      if ( mascara_pendencias & ( 1 << p ) ) {
         editavel = TRUE;
         if ( _cor_fundo_celula_pendente( ctx->dados.interface_style, &cor_fundo ) ) usar_cor_fundo = TRUE;
      }
   }
   // Se for coluna de Recuperação Final (Coluna 8 -> Bit 4)
   else if ( modo_pendencias && !inativo && col_idx == 8 ) {
      if ( mascara_pendencias & ( 1 << 4 ) ) {
         editavel = TRUE;
         if ( _cor_fundo_celula_pendente( ctx->dados.interface_style, &cor_fundo ) ) usar_cor_fundo = TRUE;
      }
   }
   // Se for coluna de Conselho (Coluna 9 -> Bit 5)
   else if ( modo_pendencias && !inativo && col_idx == 9 ) {
      if ( mascara_pendencias & ( 1 << 5 ) ) {
         editavel = TRUE;
         if ( _cor_fundo_celula_pendente( ctx->dados.interface_style, &cor_fundo ) ) usar_cor_fundo = TRUE;
      }
   }

   // Aplica as propriedades
   if ( editavel ) {
      g_object_set( cell, "editable", TRUE, NULL );
      if ( usar_cor_fundo ) {
         g_object_set( cell, "cell-background-rgba", &cor_fundo, "cell-background-set", TRUE, NULL );
      }
   } else {
      g_object_set( cell, "editable", FALSE, "cell-background-set", FALSE, NULL );
   }

   // =======================================================================
   // LÓGICA DO TEXTO E COR DA FONTE
   // =======================================================================
   gchar *texto = NULL;
   if ( ( col_idx >= 2 && col_idx <= 5 ) || ( col_idx >= 7 && col_idx <= 9 ) ) {
      g_object_get( cell, "text", &texto, NULL );
   }

   _aplicar_cor_condicional_nota( cell, inativo, texto, ctx->dados.interface_style );
   g_free( texto );
}
//------------------------------------------------------------------------------------------------------------
 static void _inicializar_renderizadores_relatorio( AppContext *ctx ) {
   g_return_if_fail( ctx );

   GtkTreeView *tree_relatorio = GTK_TREE_VIEW( ctx->ui_diario.treeview_relatorio );
   if ( tree_relatorio ) {
      for ( int i = 1; i <= 10; i++ ) {
         GtkTreeViewColumn *col = gtk_tree_view_get_column( tree_relatorio, i );
         if ( !col ) continue;

         GList *renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( col ) );
         if ( renderers ) {
            GtkCellRenderer *renderer = GTK_CELL_RENDERER( renderers->data );
            g_object_set_data( G_OBJECT( renderer ), "col_model_idx", GINT_TO_POINTER( i ) );
            gtk_tree_view_column_set_cell_data_func( col, renderer, _on_renderizar_cores_relatorio, ctx, NULL );
            g_list_free( renderers );
         }
      }
   }
}
//============================================================================================================








/**
 * @brief Conecta todos os sinais de eventos (cliques e alternâncias)do aplicativo.
 * @param bot      Ponteiro para a estrutura persistente de botões de controle.
 * @param opt      Ponteiro para a estrutura persistente de configurações LaTeX da prova.
 * @param alt      Ponteiro para a estrutura persistente de seletores booleanos (toggles).
 * @param provider Ponteiro para o gerenciador de estilo CSS do GTK.
 */
void app_signals_connect( gpointer user_data ) {

   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   _configurar_nomes_dos_widgets( ctx );

   int i;

   // =========================================================================
   // 2. VERIFICADORES DE ESTADO DA INTERFACE (BOOLEANOS)
   // =========================================================================
   // Conecta os seletores booleanos do diário/sistema de forma clara e explícita
   g_signal_connect( ctx->check.validar_ciclos, "toggled",
                     G_CALLBACK( on_check_atualizar_booleanos_interface_toggled ), ctx );
   ctx->handlers.nao_presencial = g_signal_connect( ctx->check.nao_presencial, "toggled",
                                  G_CALLBACK( on_check_atualizar_booleanos_interface_toggled ), ctx );
   ctx->handlers.expor_dados = g_signal_connect( ctx->check.expor_dados,    "toggled",
                               G_CALLBACK( on_check_atualizar_booleanos_interface_toggled ), ctx );

   // =========================================================================
   // 3. SELETORES DE CONFIGURAÇÃO DO DOCUMENTO LATEX (OPÇÕES COMPILADAS)
   // =========================================================================
   for ( i = 0; i < 2 ; i++ ) {
      g_signal_connect( ctx->radio.qtd_colunas[i],    "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
      g_signal_connect( ctx->radio.separadores[i],    "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
      g_signal_connect( ctx->radio.fonte_latex[i],    "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
      g_signal_connect( ctx->radio.qtd_paginas[i],    "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
      g_signal_connect( ctx->radio.cabecalho_tipo[i], "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
   }
   for ( i = 0; i < 3; i++ ) {
      g_signal_connect( ctx->radio.avaliacao[i],       "toggled",
                        G_CALLBACK( on_radio_atualizar_generic_interface_toggled ), ctx );
   }

   // =========================================================================
   // 4. CONEXÃO DE SINAIS: CICLO PEDAGÓGICO E PRODUÇÃO DE DOCUMENTOS
   // =========================================================================
   g_signal_connect( ctx->button.frequencia,        "clicked", G_CALLBACK( on_button_relatorio_de_frequencia_clicked ), ctx );
   g_signal_connect( ctx->button.conteudos,         "clicked", G_CALLBACK( on_button_relatorio_de_conteudos_clicked ), ctx );
   g_signal_connect( ctx->button.avaliacoes,        "clicked", G_CALLBACK( on_button_relatorio_de_avaliacoes_clicked ), ctx );
   g_signal_connect( ctx->button.relatorio_final,   "clicked", G_CALLBACK( on_button_relatorio_final_clicked ), ctx );
   g_signal_connect( ctx->button.atualizar_alunos,  "clicked", G_CALLBACK( on_button_siaep_atualizar_alunos_clicked ), ctx );

   g_signal_connect( ctx->ui_diario.calendario_data, "next-month", G_CALLBACK( on_calendar_navigation ), ctx );
   g_signal_connect( ctx->ui_diario.calendario_data, "prev-month", G_CALLBACK( on_calendar_navigation ), ctx );
   g_signal_connect( ctx->ui_diario.calendario_data, "next-year", G_CALLBACK( on_calendar_navigation ), ctx );
   g_signal_connect( ctx->ui_diario.calendario_data, "prev-year", G_CALLBACK( on_calendar_navigation ), ctx );

   g_signal_connect( ctx->ui_diario.calendario_data, "day-selected", G_CALLBACK( on_calendar_day_selected ), ctx );
   g_signal_connect( ctx->ui_diario.entry_data, "button-press-event", G_CALLBACK( on_entry_data_button_press ), ctx );
   g_signal_connect( ctx->ui_diario.entry_data, "focus-out-event", G_CALLBACK( on_entry_validar_data_focus_out ), ctx );

   g_signal_connect( ctx->ui_diario.stepper_menos, "clicked", G_CALLBACK( on_button_stepper_menos_num_horarios_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.stepper_mais,  "clicked", G_CALLBACK( on_button_stepper_mais_num_horarios_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.remover_registro, "clicked",
                     G_CALLBACK( on_button_remover_registro_diario_selecionado_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.treeview_conteudo, "key-press-event",
                     G_CALLBACK( on_treeview_remover_registro_diario_selecionado_key_press_event ), ctx );
   // Obtém o controle de seleção do TreeView
   GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( ctx->ui_diario.treeview_conteudo ) );
   g_signal_connect( selection, "changed", G_CALLBACK( on_diario_selection_changed ), ctx );

   g_signal_connect( ctx->ui_diario.descricao, "activate", G_CALLBACK( on_entry_registrar_aula_activate ), ctx );
   g_signal_connect( ctx->ui_diario.btn_salvar_conteudo, "clicked", G_CALLBACK( on_button_registrar_aula_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.treeview_conteudo, "row-activated",
                     G_CALLBACK( on_treeview_carregar_registro_para_edicao_row_activated ), ctx );
   _inicializar_renderizadores_conteudo( ctx );


   ctx->ui_diario.handler_combo_data = g_signal_connect( ctx->ui_diario.combo_data, "changed",
                                       G_CALLBACK( on_combo_data_frequencia_changed ), ctx );
   g_signal_connect( ctx->ui_diario.check_por_aluno, "toggled", G_CALLBACK( on_check_por_aluno_toggled ), ctx );
   g_signal_connect( ctx->ui_diario.btn_presente, "clicked", G_CALLBACK( on_button_presente_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.btn_ausente, "clicked", G_CALLBACK( on_button_ausente_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.btn_justificada, "clicked", G_CALLBACK( on_button_justificada_clicked ), ctx );
   g_signal_connect( ctx->window, "key-press-event",
                     G_CALLBACK( on_key_presente_ou_ausente_key_press_event ), ctx );
   g_signal_connect( ctx->ui_diario.btn_salvar_frequencia, "clicked", G_CALLBACK( on_button_salvar_frequencia_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.treeview_frequencia, "cursor-changed",
                     G_CALLBACK( on_treeview_frequencia_cursor_changed ), ctx );
   _inicializar_renderizadores_frequencia( ctx );


   // AVALIAÇÕES
   ctx->ui_diario.handler_combo_avaliacoes = g_signal_connect( ctx->ui_diario.combo_avaliacoes, "changed",
         G_CALLBACK( on_combo_selecionar_avaliacao_changed ), ctx );
   g_signal_connect( ctx->ui_diario.btn_nova_avaliacao, "clicked", G_CALLBACK( on_button_nova_avaliacao_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.btn_editar_avaliacao, "clicked", G_CALLBACK( on_button_editar_avaliacao_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.btn_popover_nomear, "clicked",
                     G_CALLBACK( on_button_popover_nomear_avaliacao_clicked ), ctx );
   g_signal_connect( ctx->ui_diario.entry_popover_nomear, "activate",
                     G_CALLBACK( on_entry_popover_nomear_avaliacao_activate ), ctx );
   ctx->ui_diario.handler_check_desativar = g_signal_connect( ctx->ui_diario.check_desativar_avaliacao, "toggled",
         G_CALLBACK( on_check_desativar_avaliacao_toggled ), ctx );

   // Aba Avaliações (Colunas 2 a 11 contíguas)
   _configurar_colunas_editaveis( ctx, GTK_TREE_VIEW( ctx->ui_diario.treeview_avaliacoes ), 2, 11 );

   g_signal_connect( ctx->ui_diario.treeview_avaliacoes, "cursor-changed",
                     G_CALLBACK( on_treeview_notas_cursor_changed ), ctx );
   g_signal_connect( ctx->ui_diario.btn_salvar_avaliacoes, "clicked",
                     G_CALLBACK( on_button_salvar_avaliacoes_clicked ), ctx );

   _inicializar_renderizadores_avaliacoes( ctx );


   // RELATÓRIO FINAL
   g_signal_connect( ctx->ui_diario.check_pendencias, "toggled", G_CALLBACK( on_check_pendencias_relatorio_toggled ), ctx );
   g_signal_connect( ctx->ui_diario.btn_consolidar, "clicked", G_CALLBACK( on_button_consolidar_relatorio_clicked ), ctx );

   // Aba Relatório Final (Colunas 2, 3 e 4: 1º, 2º e 3º Períodos) (Colunas 8 e 9: Rec Final e Conselho)
   _configurar_colunas_editaveis( ctx, GTK_TREE_VIEW( ctx->ui_diario.treeview_relatorio ), 2, 4 );
   _configurar_colunas_editaveis( ctx, GTK_TREE_VIEW( ctx->ui_diario.treeview_relatorio ), 8, 9 );

   _inicializar_renderizadores_relatorio( ctx );



   g_signal_connect( ctx->button.frequencia, "enter-notify-event", G_CALLBACK( on_button_frequencia_enter_notify_event ), ctx );
   g_signal_connect( ctx->button.conteudos,  "enter-notify-event", G_CALLBACK( on_button_conteudos_enter_notify_event ), ctx );
   g_signal_connect( ctx->button.avaliacoes, "enter-notify-event", G_CALLBACK( on_button_avaliacoes_enter_notify_event ), ctx );
   g_signal_connect( ctx->button.relatorio_final, "enter-notify-event",
                     G_CALLBACK( on_button_relatorio_enter_notify_event ), ctx );

   g_signal_connect( ctx->ui_diario.check_modo_edicao, "toggled", G_CALLBACK( on_check_modo_edicao_toggled ), ctx );
   g_signal_connect( ctx->notebook, "switch-page", G_CALLBACK( on_notebook_principal_switch_page ), ctx );



   // Operação Crítica: Passa o ponteiro 'opt' para leitura e aplicação estável do cache físico (.dat)
   g_signal_connect( ctx->button.importar_dados, "clicked", G_CALLBACK( on_button_carregar_estado_aplicativo_clicked ), ctx );
   g_signal_connect( ctx->button.gerar_prova,       "clicked", G_CALLBACK( on_button_gerar_prova_clicked ), ctx );
   g_signal_connect( ctx->button.processar_imagens, "clicked", G_CALLBACK( on_button_processar_imagens_clicked ), ctx );
   g_signal_connect( ctx->button.corrigir_prova,    "clicked", G_CALLBACK( on_button_corrigir_prova_clicked ), ctx );

   g_signal_connect( ctx->button.abrir_pdf_acervo,      "clicked", G_CALLBACK( on_button_abrir_tema_clicked ), ctx );
   g_signal_connect( ctx->button.compilar_latex_acervo, "clicked", G_CALLBACK( on_button_compilar_questoes_clicked ), ctx );
   g_signal_connect( ctx->button.executar_gcc_acervo,   "clicked", G_CALLBACK( on_button_atualizar_questoes_clicked ), ctx );


   ctx->handlers.ano = g_signal_connect( ctx->entry.ano, "changed",
                                         G_CALLBACK( on_entry_atualizar_ano_interface_changed ), ctx );

   ctx->handlers.escola = g_signal_connect( ctx->entry.escola, "changed",
                          G_CALLBACK( on_entry_atualizar_escola_interface_changed ), ctx );

   ctx->handlers.turma = g_signal_connect( ctx->entry.turma, "changed",
                                           G_CALLBACK( on_entry_atualizar_turma_interface_changed ), ctx );

   ctx->handlers.disciplina = g_signal_connect( ctx->entry.disciplina, "changed",
                              G_CALLBACK( on_entry_atualizar_disciplina_interface_changed ), ctx );

   ctx->handlers.periodo = g_signal_connect( ctx->entry.periodo, "changed",
                           G_CALLBACK( on_entry_periodo_interface_changed ), ctx );

   ctx->handlers.cor_destaque = g_signal_connect( ctx->entry.cor_destaque, "changed",
                                G_CALLBACK( on_entry_cor_destaque_interface_changed ), ctx );

   ctx->handlers.decoracao_estilo = g_signal_connect( ctx->entry.decoracao_estilo, "changed",
                                    G_CALLBACK( on_entry_decoracao_estilo_interface_changed ), ctx );

   g_signal_connect( ctx->entry.estilo, "changed", G_CALLBACK( on_entry_estilo_interface_changed ), ctx );

   // =========================================================================
   // 6. RASTREAMENTO AUTOMÁTICO DINÂMICO (INPUTS DE TEMAS E QUESTÕES)
   // =========================================================================
   ctx->handlers.tema = g_signal_connect( ctx->entry.tema, "changed",
                                          G_CALLBACK( on_entry_atualizar_tema_changed ), ctx );

   ctx->handlers.tema_espelho = g_signal_connect( ctx->entry.tema_espelho, "changed",
                                G_CALLBACK( on_entry_atualizar_tema_changed ), ctx );



   ctx->provas.handler_scrolled = g_signal_connect( ctx->provas.scrolled_window, "size-allocate",
                                  G_CALLBACK( on_scrolled_rolar_para_o_fim_sizeallocate ), ctx );
   g_signal_handler_block( ctx->provas.scrolled_window, ctx->provas.handler_scrolled );
   ctx->provas.importar = false;


   ctx->ui_diario.handler_combo_alunos = g_signal_connect( G_OBJECT( ctx->ui_diario.combo_alunos ), "changed",
                                         G_CALLBACK( on_combo_alunos_changed ), ctx );


   g_signal_connect( G_OBJECT( ctx->painel.eventbox_painel ), "enter-notify-event",
                     G_CALLBACK( on_painel_feedback_enter_notify_event ), ctx );

   g_signal_connect( G_OBJECT( ctx->painel.eventbox_painel ), "leave-notify-event",
                     G_CALLBACK( on_painel_feedback_leave_notify_event ), ctx );

   g_signal_connect( G_OBJECT( ctx->painel.eventbox_painel ), "button-press-event",
                     G_CALLBACK( on_painel_feedback_button_press_event ), ctx );

   g_signal_connect( G_OBJECT( ctx->painel.eventbox_orelhinha ), "button-press-event",
                     G_CALLBACK( on_orelhinha_button_press_event ), ctx );

   g_signal_connect( ctx->painel.revealer_painel, "notify::child-revealed",
                     G_CALLBACK( on_revealer_child_revealed_notify ), ctx );


}
