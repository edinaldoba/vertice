/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "signals.h"
#include "callbacks.h"
#include "comum.h"
#include "imagens.h"
#include "interface.h"
#include "relatorios.h"
#include "provas.h"
#include "basicas.h"
#include "acervo.h"
#include "gabaritos.h"
#include "mensagens.h"
#include "dinamica.h"
#include "assincrono.h"
#include "seduc-ma.h"





void on_entry_atualizar_ano_interface_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   ctx->cascata.foco.ano = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.ano ) );
   if ( ctx->cascata.foco.ano < 0 || ctx->listas.anos == NULL ) return;

   gchar *ano_selecionado = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.ano ) );
   if ( !ano_selecionado ) return;

   gboolean clique_real = ( widget != NULL );
   gboolean ano_mudou = atualizar_ano_interface( ctx, ano_selecionado, !clique_real );
   g_free( ano_selecionado );

   if ( !ano_mudou ) return;

   popular_combo_box_text( ctx->entry.escola, ctx->listas.escolas, 0, ctx->cascata.limite.escolas, ctx->handlers.escola );
   on_entry_atualizar_escola_interface_changed( NULL, ctx );
}




void on_entry_atualizar_escola_interface_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   ctx->cascata.foco.escola = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.escola ) );
   if ( ctx->cascata.foco.escola < 0 || ctx->listas.escolas == NULL ) return;

   gchar *escola_selecionada = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.escola ) );
   if ( !escola_selecionada ) return;

   gboolean clique_real = ( widget != NULL );
   gboolean escola_mudou = atualizar_escola_interface( ctx, escola_selecionada, !clique_real );
   g_free( escola_selecionada );

   if ( !escola_mudou ) return;

   popular_combo_box_text( ctx->entry.turma, ctx->listas.turmas, 0, ctx->cascata.limite.turmas, ctx->handlers.turma );
   on_entry_atualizar_turma_interface_changed( NULL, ctx ); // Ou o novo nome da callback de turma!
}




void on_entry_atualizar_turma_interface_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   ctx->cascata.foco.turma = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.turma ) );
   if ( ctx->cascata.foco.turma < 0 || ctx->listas.turmas == NULL ) return;

   gchar *turma_selecionada = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.turma ) );
   if ( !turma_selecionada ) return;

   // 3. Passa a responsabilidade para a função embrulhada
   gboolean clique_real = ( widget != NULL );
   gboolean turma_mudou = atualizar_turma_interface( ctx, turma_selecionada, !clique_real );
   g_free( turma_selecionada );

   if ( !turma_mudou ) return;

   popular_combo_box_text( ctx->entry.disciplina, ctx->listas.disciplinas, 0,
                           ctx->cascata.limite.disciplinas, ctx->handlers.disciplina );
   on_entry_atualizar_disciplina_interface_changed( NULL, ctx );
}




void on_entry_atualizar_disciplina_interface_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;

   ctx->cascata.foco.disciplina = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.disciplina ) );
   if ( ctx->cascata.foco.disciplina < 0 || ctx->listas.disciplinas == NULL ) return;

   gchar *disciplina_selecionada = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.disciplina ) );
   if ( !disciplina_selecionada ) return;

   gboolean clique_real = ( widget != NULL );
   atualizar_disciplina_interface( ctx, disciplina_selecionada, !clique_real );
   g_free( disciplina_selecionada );
}




void on_entry_periodo_interface_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   ( void )widget;

   ctx->cascata.foco.periodo = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.periodo ) );
   if ( ctx->cascata.foco.periodo < 0 || ctx->listas.periodos == NULL ) return;

   gchar *periodo_selecionado = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.periodo ) );
   if ( !periodo_selecionado ) return;

   atualizar_periodo_interface( ctx, periodo_selecionado );
   g_free( periodo_selecionado ); // Limpa a memória
}




void on_entry_cor_destaque_interface_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;
   ( void )widget; // Impede o alerta de variável 'widget' não utilizada

   ctx->cascata.foco.cor_destaque = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.cor_destaque ) );
   if ( ctx->cascata.foco.cor_destaque < 0 || ctx->listas.cores_destaque == NULL ) return;

   gchar *cor_destaque_selecionada = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.cor_destaque ) );
   if ( !cor_destaque_selecionada ) return;

   snprintf( ctx->dados.cor_destaque, sizeof( ctx->dados.cor_destaque ), "%s", cor_destaque_selecionada );
   g_free( cor_destaque_selecionada ); // Limpa a memória

}


void on_entry_decoracao_estilo_interface_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;
   ( void )widget; // Impede o alerta de variável 'widget' não utilizada

   ctx->cascata.foco.decoracao_estilo = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.decoracao_estilo ) );
   if ( ctx->cascata.foco.decoracao_estilo < 0 || ctx->listas.decoracoes_estilo == NULL ) return;

   gchar *decoracao_estilo_selecionado = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->entry.decoracao_estilo ) );
   if ( !decoracao_estilo_selecionado ) return;

   snprintf( ctx->dados.decoracao_estilo, sizeof( ctx->dados.decoracao_estilo ), "%s", decoracao_estilo_selecionado );
   g_free( decoracao_estilo_selecionado ); // Limpa a memória
}




void on_botao_relatorio_de_avalicoes_clicked( GtkWidget *widget, gpointer user_data ) {
   ( void )widget;
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   relatorio_de_avaliacoes( &ctx->painel, ctx );
}

void on_botao_relatorio_de_conteudos_clicked( GtkWidget *widget, gpointer user_data ) {
   ( void )widget;
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   relatorio_de_conteudos( &ctx->painel, ctx );
}

void on_botao_relatorio_de_frequencia_clicked( GtkWidget *widget, gpointer user_data ) {
   ( void )widget;
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   relatorio_de_frequencia( &ctx->painel, ctx );
}

void on_botao_relatorio_final_clicked( GtkWidget *widget, gpointer user_data ) {
   ( void )widget;
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   relatorio_final( &ctx->painel, ctx );
}

void on_botao_abrir_arquivos_de_dados_clicked( GtkWidget *widget, gpointer user_data ) {
   ( void )widget;
   AppContext *ctx = ( AppContext* ) user_data;
   if ( !ctx ) return;
   abrir_arquivos_de_dados( &ctx->painel, ctx );
}

void on_botao_siaep_atualizar_alunos_clicked( GtkWidget *widget, gpointer user_data ) {
   ( void )widget;
   AppContext *ctx = ( AppContext* ) user_data;
   if ( !ctx ) return;
   siaep_atualizar_alunos( &ctx->painel, ctx->diario );
}






void on_botao_abrir_tema_clicked( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   abrir_tema( widget, &ctx->painel, ctx );
}


void on_botao_compilar_questoes_clicked( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   compilar_questoes( widget, &ctx->painel, ctx );
}


void on_botao_atualizar_questoes_clicked( GtkWidget *widget, gpointer user_data ) {
   ( void )widget;
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   atualizar_questoes( &ctx->painel, ctx );
}






void on_botao_carregar_estado_aplicativo_clicked( GtkWidget *widget, gpointer user_data ) {
   // Evita o aviso de variável não utilizada para o widget do botão
   ( void )widget;

   // Recupera o ponteiro da estrutura AppContext enviada pelo sinal
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) {
      fprintf( stderr, "ERRO: Ponteiro AppContext inválido no callback de cache.\n" );
      return;
   }

   // Carrega o arquivo de forma indireta, alimentando a tela que alimenta o ctx->dados automaticamente
   if ( carregar_estado_aplicativo( ctx ) ) {
      g_print( "Restauração magnética: Interface alimentada e sincronizada via sinais nativos!\n" );
   } else {
      g_print( "Nenhum cache encontrado. Pronto para nova entrada.\n" );
   }
}






void on_botao_gerar_prova_clicked( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx || !widget ) return;

   // 1. 🛡️ VALIDAÇÃO DE ENTRADA (Alunos vazios ou Meta de 10 questões)
   if ( verificar_dados_da_interface( &ctx->painel, &ctx->dados ) ) {
      return;
   }

   // 2. 🛡️ GERENCIAMENTO HISTÓRICO DOS GABARITOS (Encapsulado!)
   // Passamos o widget para que a função interna consiga descobrir a janela pai se precisar de pop-up
   gerenciar_fluxo_gabaritos( widget, &ctx->painel, ctx );

   // 3. 🚀 DISPARO ASSÍNCRONO
   gtk_widget_set_sensitive( widget, FALSE );
   disparar_geracao_prova_assincrona( widget, ctx, thread_gerar_prova_background );
}





void on_botao_processar_imagens_clicked( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx || !widget ) return;

   // GtkWindow *janela_principal = GTK_WINDOW( gtk_widget_get_toplevel( widget ) );
   //
   // // Refatoração focada na clareza e prevenção de erros humanos
   // g_autofree gchar *mensagem = meu_gerador_variadico(
   //    "Você está prestes a iniciar a leitura óptica das provas.\n\n"
   //    "<b>ESCOLA:</b> %s\n"
   //    "<b>ANO LETIVO:</b> %s\n\n"
   //    "⚠️ <b>Atenção:</b> Como o cartão-resposta não identifica a escola automaticamente, "
   //    "todos os acertos deste lote de imagens serão vinculados ao destino acima.\n\n"
   //    "Confirma que as imagens na pasta pertencem a esta escola e ano?",
   //    ctx->dados.escola, ctx->dados.ano );
   //
   // // Sugestão: Mudar o título do pop-up para algo que exija mais atenção
   // gboolean continuar = mostrar_popup_confirmacao( janela_principal, "Confirmação de Vínculo das Provas", mensagem );

   // if ( continuar ) {
   disparar_processamento_imagens_assincrono( widget, ctx, thread_processar_imagens_background );
   // }
}




void on_botao_corrigir_prova_clicked( GtkWidget *widget, gpointer user_data ) {
   ( void )widget;
   AppContext *ctx = ( AppContext * )user_data; // Resgata o contexto
   if ( !ctx ) return;
   corrigir_prova( &ctx->painel, ctx );
}




/* * NOTA DE ESTUDO ACERCA DE ABORDAGENS ALTERNATIVAS:
 * Atualmente, esta callback utiliza um "ID Composto" numérico (ex: "radio_opcao_42"),
 * onde a dezena (4) representa a categoria e a unidade (2) representa o valor.
 * * POSSIBILIDADE DE ABORDAGEM COMPLEMENTAR (strcmp + ID simples):
 * Uma alternativa para aumentar a legibilidade textual do código seria nomear os widgets
 * com base no seu grupo semântico seguido de um ID simples (ex: "radio_fonte_1", "radio_fonte_2").
 * O fluxo de captura seria dividido em duas etapas:
 * 1. Identificação do Grupo via String:
 * if (strstr(nome_do_radio_toggle, "fonte")) categoria = CAT_FONTE;
 * else if (strstr(nome_do_radio_toggle, "colunas")) categoria = CAT_COLUNAS;
 * 2. Extração do Valor Numérico:
 * int valor = extrair_id_widget(nome_do_radio_toggle);
 * * Opção atual mantida por centralizar toda a lógica em aritmética pura de alta performance.
 */
void on_radio_atualizar_generic_interface_toggled( GtkWidget *widget, gpointer user_data ) {

   if ( !gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) ) return;

   AppContext *ctx = ( AppContext * ) user_data;
   if ( !ctx || !widget ) return;

   const char *nome_do_radio_toggle = gtk_widget_get_name( widget );
   if ( !nome_do_radio_toggle ) return;

   int id_composto = extrair_id_widget( nome_do_radio_toggle );
   if ( id_composto == -1 ) return;

   int categoria = id_composto / 10;  // Pega a dezena
   int valor = id_composto % 10;      // Pega a unidade

   // Passa o endereço direto da sub-struct dados
   atualizar_generic_interface( ctx, categoria, valor );

   // Opcional: feedback no terminal para depuração
   g_print( "Estado alterado com segurança: Widget '%s' (Cat %d) -> (Valor %d)\n", nome_do_radio_toggle, categoria, valor );

}







void on_check_atualizar_booleanos_interface_toggled( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx || !widget ) return;

   // 1. Descobre quem foi o "culpado" pelo disparo do evento
   const char *check_nome = gtk_widget_get_name( widget );
   if ( !check_nome ) return;

   // 3. Agora sim, conversão 100% segura
   int categoria = extrair_id_widget( check_nome );
   if ( categoria == -1 ) return;

   bool estado = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) );

   // Dentro de callbacks.c (Linha 381):
   atualizar_booleanos_interface( estado, categoria, ctx );

   // Feedback limpo no terminal do seu Debian
   g_print( "Estado alterado com segurança: Widget '%s' (Cat %d) -> %s\n",
            check_nome, categoria, estado ? "ATIVADO" : "DESATIVADO" );
}





void on_entry_atualizar_tema_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;

   if ( !widget || !ctx ) return;

   ctx->cascata.foco.tema = gtk_combo_box_get_active( GTK_COMBO_BOX( widget ) );

   if ( widget == ctx->entry.tema ) {

      g_signal_handler_block( ctx->entry.tema_espelho, ctx->handlers.tema_espelho );

      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->entry.tema_espelho ), ctx->cascata.foco.tema );

      g_signal_handler_unblock( ctx->entry.tema_espelho, ctx->handlers.tema_espelho );


   } else if ( widget == ctx->entry.tema_espelho ) {

      g_signal_handler_block( ctx->entry.tema, ctx->handlers.tema );

      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->entry.tema ), ctx->cascata.foco.tema );

      g_signal_handler_unblock( ctx->entry.tema, ctx->handlers.tema );
   }

   gchar *tema = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( widget ) );

   if ( tema != NULL ) {
      atualizar_tema( ctx, tema );
      g_free( tema ); // Limpa a memória com segurança
   }
}






// --- EMBRULHOS GTK ---

void on_subtema_check_toggled( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx || !widget ) return;

   logica_subtema_toggled( widget, ctx );
}



void on_stepper_mais_clicked( GtkWidget *button, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx || !button ) return;

   logica_stepper_mais( button, ctx );
}



void on_stepper_menos_clicked( GtkWidget *button, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx || !button ) return;

   logica_stepper_menos( button, ctx );
}



void on_scrolled_rolar_para_o_fim_sizeallocate( GtkWidget *widget, GdkRectangle *allocation, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;
   ( void )allocation;

   rolar_para_o_fim( GTK_SCROLLED_WINDOW( widget ) );

   if ( ctx->provas.handler_scrolled > 0  && ctx->provas.importar == false ) {
      g_signal_handler_block( widget, ctx->provas.handler_scrolled );
   }

}



void on_combo_alunos_changed( GtkWidget *widget, gpointer user_data ) {
   AppContext *ctx = ( AppContext * )user_data;
   if ( !ctx ) return;
   ctx->cascata.foco.aluno = gtk_combo_box_get_active( GTK_COMBO_BOX( widget ) );
}



