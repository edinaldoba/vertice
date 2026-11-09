/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>

#include "comum.h"
#include "interface.h"
#include "basicas.h"
#include "callbacks.h"
#include "mensagens.h"
#include "dinamica.h"
#include "diario.h"

// Esta é a única inclusão deste arquivo em todo o projeto
#include "auxiliar.h"






void interface_style( AppContext *ctx ) {
   if ( !ctx ) return;

   // Capturamos a screen global uma única vez no topo do escopo da função
   GdkScreen *screen = gdk_screen_get_default();

   /* ==========================================================================
      🚀 1. BLINDAGEM DA MEMÓRIA E EXPURGO DO CSS ANTIGO
      ========================================================================== */
   if ( ctx->provider != NULL ) {
      // Remove o vínculo do provedor de estilo da tela ativa
      gtk_style_context_remove_provider_for_screen( screen, GTK_STYLE_PROVIDER( ctx->provider ) );

      // Decrementa o contador de referências do objeto GLib e zera o ponteiro
      g_object_unref( ctx->provider );
      ctx->provider = NULL;
   }

   /* ==========================================================================
      🏛️ 2. CRIAÇÃO DO PROVEDOR E MAPEAMENTO DO ARQUIVO ALVO
      ========================================================================== */
   ctx->provider = gtk_css_provider_new();

   // Alocação segura via GLib com liberação automática (sem limite fixo de 256 bytes)
   g_autofree gchar *arquivo_css = NULL;

   switch ( ctx->cabecalho.foco_estilo ) {
   case STYLE_DARK_GREEN:
      arquivo_css = g_strdup_printf( "%s/css/style_dark_green.css", ctx->caminho.recursos_prefix );
      break;
   case STYLE_DEEP_BLUE:
      arquivo_css = g_strdup_printf( "%s/css/style_deep_blue.css", ctx->caminho.recursos_prefix );
      break;
   case STYLE_LIGHT:
      arquivo_css = g_strdup_printf( "%s/css/style_light.css", ctx->caminho.recursos_prefix );
      break;
   default:
      arquivo_css = g_strdup_printf( "%s/css/style_dark_green.css", ctx->caminho.recursos_prefix );
      break;
   }

   /* ==========================================================================
      📥 3. CARREGAMENTO E INJEÇÃO NA MÁQUINA DE RENDERIZAÇÃO DO GTK
      ========================================================================== */
   gtk_css_provider_load_from_resource( ctx->provider, arquivo_css );

   // Aplica as novas diretrizes visuais na tela global
   gtk_style_context_add_provider_for_screen(
      screen,
      GTK_STYLE_PROVIDER( ctx->provider ),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
   );
}









//---------------------------------------------------------------------------------------------------------------
void atualizar_booleanos_interface( const gboolean estado, const int categoria, AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );

   InterfaceDados  *dados     = &ctx->dados;
   const InterfaceDiario *diario = &ctx->diario;
   InterfaceProva  *prova  = &ctx->prova;

   switch ( categoria ) {
   case CATEGORIA_UI_VALIDAR_CICLOS:
      dados->validar_ciclos = estado;
      break;

   case CATEGORIA_UI_NAO_PRESENCIAL:
      dados->nao_presencial = estado;

      // Regra: Se a prova não é presencial (TRUE), os dados PRECISAM ser expostos (TRUE).
      if ( dados->nao_presencial && !dados->expor_dados ) {
         // Força o botão 'Expor Dados' a ligar. O GTK emitirá um "toggled" para ele.
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( diario->check_expor_dados ), TRUE );
      }
      break;

   case CATEGORIA_UI_EXPOR_DADOS:
      dados->expor_dados = estado;

      // Regra: Se os dados forem ocultados (FALSE), a prova não pode ser 'não presencial' (FALSE).
      if ( !dados->expor_dados && dados->nao_presencial ) {
         // Força o botão 'Não Presencial' a desligar. O GTK emitirá um "toggled" para ele.
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( prova->check_nao_presencial ), FALSE );
      }
      break;

   default:
      g_warning( "Categoria desconhecida no despacho de gbooleaneanos da interface: %d", categoria );
      break;
   }
}




//---------------------------------------------------------------------------------------------------------------
void atualizar_generic_interface( AppContext *ctx, const int categoria, const int valor ) {
   g_return_if_fail( ctx != NULL );

   InterfaceDados  *dados  = &ctx->dados;

   switch ( categoria ) {

   case CATEGORIA_UI_COLUNAS:
      dados->qtd_colunas = valor; // 2 ou 3
      // Restrição de Layout: Se a prova tiver 2 colunas, o cabeçalho não pode ser lateral (tipo 2).
      // Dispara o rádio tipo 1 (Acima) automaticamente.
      if ( dados->qtd_colunas == 2 && dados->cabecalho_tipo == 2 ) {
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->latex.radio_cabecalho_tipo[0] ), TRUE );
      }
      break;

   case CATEGORIA_UI_SEPARADORES:
      dados->separadores = valor; // 1 (pontilhado) ou 2 (contínuo)
      break;

   case CATEGORIA_UI_FONTE_LATEX:
      dados->fonte_latex = valor; // 1 (CMB Right) ou 2 (CMU Serif)
      break;

   case CATEGORIA_UI_PAGINAS:
      dados->qtd_paginas = valor; // 1 ou 2
      break;

   case CATEGORIA_UI_CABECALHO:
      dados->cabecalho_tipo = valor; // 1 (acima) ou 2 (à esquerda)
      // Restrição de Layout: Se forçarem o cabeçalho lateral (tipo 2), a prova precisa ter 3 colunas.
      if ( dados->cabecalho_tipo == 2 && dados->qtd_colunas == 2 ) {
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ctx->latex.radio_qtd_colunas[1] ), TRUE );
      }
      break;

   default:
      g_warning( "Categoria desconhecida no despacho da interface: %d", categoria );
      break;
   }
}






/**
 * @brief Aplica espaçamento uniforme intercalando Hair Spaces (\u200A)
 *        diretamente no texto de um GtkLabel.
 */
void ui_label_aplicar_espacamento_manual( GtkWidget *label_widget, gint quantidade_espacos ) {
   g_return_if_fail( GTK_IS_LABEL( label_widget ) );

   const gchar *texto_original = gtk_label_get_text( GTK_LABEL( label_widget ) );
   if ( !texto_original || *texto_original == '\0' ) return;

   GString *resultado = g_string_new( "" );
   const gchar *p = texto_original;

   while ( *p != '\0' ) {
      gunichar c = g_utf8_get_char( p );
      const gchar *proximo = g_utf8_next_char( p );

      if ( c == 0x200A ) {
         p = proximo;
         continue;
      }

      g_string_append_unichar( resultado, c );
      gunichar proximo_c = g_utf8_get_char( proximo );

      if ( *proximo != '\0' && proximo_c != 0x200A ) {
         for ( gint i = 0; i < quantidade_espacos; i++ ) {
            g_string_append( resultado, "\u200A" );
         }
      }
      p = proximo;
   }

   gtk_label_set_text( GTK_LABEL( label_widget ), resultado->str );
   g_string_free( resultado, TRUE );
}







void atualizar_tema( AppContext *ctx, char *tema ) {
   if ( !ctx ) return;

   InterfaceDados   *dados   = &ctx->dados;
   CaminhoDiretorio *caminho = &ctx->caminho;
   InterfaceProva  *prova  = &ctx->prova;
   InterfacePainel  *painel  = &ctx->painel;

   // 1. Libera com segurança a cópia do tema anterior mantida em dados
   g_clear_pointer( &dados->tema, g_free );

   /* =========================================================================
    * 🛡️ REGRA DE POSSE DE MEMÓRIA (OWNERSHIP):
    * NÃO substituir g_strdup por g_steal_pointer ou atribuição direta!
    *
    * O ponteiro 'tema' pertence ao vetor 'prova->temas'. Roubar sua posse ou
    * compartilhar seu endereço causará um Dangling Pointer quando o vetor for
    * destruído, resultando em um crash por Double Free em 'dados->tema'.
    * 'dados->tema' DEVE obrigatoriamente possuir sua própria cópia no Heap.
    * ========================================================================= */
   dados->tema = g_strdup( tema );

   g_clear_pointer( &prova->subtemas, g_strfreev );

   g_autofree gchar *diretorio = g_build_filename( caminho->banco_questoes, dados->tema, NULL );
   prova->limite_subtemas = quantidade_diretorios( diretorio );

   if ( prova->limite_subtemas > 0 ) {
      prova->subtemas = carregar_diretorios_temas( prova->limite_subtemas, diretorio, NULL );

   } else {
      const char *nome_widget = gtk_widget_get_name( ctx->prova.combo_tema );
      if ( g_strcmp0( nome_widget, "evento_via_codigo" ) != 0 ) {

         // Alimenta os buffers variádicos e delega o estilo para o motor central (AVISO)
         painel->format_titulo    = meu_gerador_variadico( "⚠ Aviso:" );
         painel->format_subtitulo = meu_gerador_variadico( "O tema '%s' está vazio!", dados->tema );
         painel->format_instrucao = meu_gerador_variadico( "Escolha outro ou adicione novos subtemas ao tema selecionado." );

         criar_mensagem_painel( AVISO, painel );

         fprintf( stderr, "[AVISO] Tema Principal VAZIO! Escolha outro ou adicione novos Subtemas.\n" );
      }
   }
   atualizar_listbox_subtemas( ctx );

}







/* =================================================================================================================
   GERENCIAMENTO DE FLUXO DE GABARITOS - Versão Otimizada (Painel de Feedback)
   ================================================================================================================= */
void gerenciar_fluxo_gabaritos( GtkWidget *widget, InterfacePainel *painel, const AppContext *ctx ) {
   if ( !widget || !painel || !ctx ) return;

   const InterfaceDados     *dados = &ctx->dados;
   const InterfaceCabecalho *cabecalho  = &ctx->cabecalho;

   g_autofree char *destino = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano, dados->escola, "gabaritos", NULL );

   if ( g_mkdir_with_parents( destino, 0755 ) != 0 ) {
      g_printerr( "Erro crítico: Não foi possível criar os diretórios de destino: %s\n", destino );
      return;
   }

   char nome[32] = {0};
   nome_base_gabaritos_bin( nome, sizeof( nome ), cabecalho->foco_turma, cabecalho->foco_disciplina, cabecalho->foco_periodo, dados->seq_prova );

   g_autofree char *arquivo = g_build_filename( destino, nome, NULL );

   // Cenário A: O arquivo não existe (Primeira compilação da prova)
   if ( !g_file_test( arquivo, G_FILE_TEST_EXISTS ) ) {
      gerar_gabaritos( arquivo, dados->qtd_alunos_ativos, dados, "wb" );
      return;
   }

   int contador = contar_registros_binarios( arquivo, sizeof( ItemTextoCurto ) );

   // Sub-cenário B1: Alunos adicionados (Seguro - Não sobrescreve, apenas anexa)
   if ( contador < dados->qtd_alunos_ativos ) {
      int adicionados = dados->qtd_alunos_ativos - contador;

      gerar_gabaritos( arquivo, adicionados, dados, "ab" );

      painel->format_titulo    = meu_gerador_variadico( "⚠ Lista de Alunos Expandida" );
      painel->format_subtitulo = meu_gerador_variadico( "%d novos alunos ativos foram adicionados desde a última compilação.", adicionados );
      painel->format_instrucao = meu_gerador_variadico( "%d novos gabaritos foram anexados mantendo os antigos intactos.", adicionados );
      criar_mensagem_painel( AVISO, painel );

      gchar *format_titulo    = meu_gerador_variadico( "⚙️ Gerando Provas e Gabaritos" );
      gchar *format_subtitulo = meu_gerador_variadico( "O processo leva aproximadamente 9 segundos." );
      gchar *format_instrucao = meu_gerador_variadico( "Compilando lote para <b>%s</b> (%s)... Por favor, aguarde.", ctx->dados.escola, ctx->dados.ano );
      agendar_ou_exibir_mensagem_painel( INFO, painel, format_titulo, format_subtitulo, format_instrucao );
   }
   // Sub-cenário B2: Risco de Perda de Dados (Alunos removidos ou quantidade idêntica -> Exige "wb")
   else {
      GtkWindow *janela_principal = GTK_WINDOW( gtk_widget_get_toplevel( widget ) );
      char *msg_gabarito = NULL;

      if ( contador > dados->qtd_alunos_ativos ) {
         int removidos = contador - dados->qtd_alunos_ativos;
         msg_gabarito = meu_gerador_variadico(
                           "<b>ESCOLA:</b> %s\n"
                           "<b>TURMA:</b> %s\n\n"
                           "A turma atual possui <b>%d aluno(s) a menos</b> que o arquivo de gabaritos existente.\n\n"
                           "Deseja sobrescrever o arquivo para refletir a nova quantidade exata da turma?",
                           dados->escola, dados->turma, removidos
                        );
      } else {
         msg_gabarito = meu_gerador_variadico(
                           "<b>ESCOLA:</b> %s\n"
                           "<b>TURMA:</b> %s\n\n"
                           "O arquivo de <b>Gabaritos</b> da <b>%s Prova</b> do <b>%s</b> já existe.\n\n"
                           "Deseja criar um novo arquivo e sobrescrever o anterior?",
                           dados->escola, dados->turma, dados->prova_sequencia, dados->periodo
                        );
      }

      gboolean emitir_gabarito = mostrar_popup_confirmacao( janela_principal, "Aviso de Sobrescrita", msg_gabarito );
      g_free( msg_gabarito );

      if ( !emitir_gabarito ) {
         painel->format_titulo    = meu_gerador_variadico( "✔ Reaproveitando Histórico" );
         painel->format_subtitulo = meu_gerador_variadico( "Gerando provas com o uso de gabaritos gerados em compilação anterior." );
         painel->format_instrucao = meu_gerador_variadico( "Os dados originais dos alunos foram preservados com sucesso." );
      } else {
         gerar_gabaritos( arquivo, dados->qtd_alunos_ativos, dados, "wb" );
         painel->format_titulo    = meu_gerador_variadico( "✔ Novos Gabaritos Alocados" );
         painel->format_subtitulo = meu_gerador_variadico( "Gerando provas com o uso de uma nova sequência de gabaritos." );
         painel->format_instrucao = meu_gerador_variadico( "O arquivo antigo foi sobrescrito conforme solicitado." );
      }

      criar_mensagem_painel( SUCESSO, painel );

      gchar *format_titulo    = meu_gerador_variadico( "⚙️ Gerando Provas e Gabaritos" );
      gchar *format_subtitulo = meu_gerador_variadico( "O processo leva aproximadamente 9 segundos." );
      gchar *format_instrucao = meu_gerador_variadico( "Compilando lote para <b>%s</b> (%s)... Por favor, aguarde.", ctx->dados.escola, ctx->dados.ano );
      agendar_ou_exibir_mensagem_painel( INFO, painel, format_titulo, format_subtitulo, format_instrucao );
   }
}




//=== NO MOMENTO SÓ É USADA NA FUNÇÃO ACIMA, MAS É UMA FUNÇÃO DE POP-UP GERAL IMPORTANTÍSSIMA
gboolean mostrar_popup_confirmacao( GtkWindow *parent, const char *titulo, const char *mensagem ) {
   if ( !mensagem ) return FALSE;

   GtkWidget *dialog;
   gint resposta;
   gboolean prosseguir = FALSE;

   // 💡 COMPORTAMENTO DINÂMICO: Se o título for NULL, vira um aviso informativo com botão OK
   GtkMessageType tipo_mensagem = ( titulo == NULL ) ? GTK_MESSAGE_INFO : GTK_MESSAGE_QUESTION;
   GtkButtonsType tipo_botoes   = ( titulo == NULL ) ? GTK_BUTTONS_OK   : GTK_BUTTONS_YES_NO;

   // 1. Cria o Pop-up nativo configurado dinamicamente
   dialog = gtk_message_dialog_new(
               parent,
               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
               tipo_mensagem,
               tipo_botoes,
               NULL
            );

   // 2. Define o título da janela (Se for NULL, assume um padrão discreto para o gerenciador de janelas)
   gtk_window_set_title( GTK_WINDOW( dialog ), titulo ? titulo : "Aviso" );

   // 3. Injeta a mensagem formatada (Pango Markup)
   gtk_message_dialog_format_secondary_markup( GTK_MESSAGE_DIALOG( dialog ), "%s", mensagem );

   // 5. Roda o loop síncrono do diálogo nativo
   resposta = gtk_dialog_run( GTK_DIALOG( dialog ) );

   // Se for YES (no caso de confirmação) ou OK (no caso de informativo), retorna TRUE
   if ( resposta == GTK_RESPONSE_YES || resposta == GTK_RESPONSE_OK ) {
      prosseguir = TRUE;
   }

   // 6. Destrói o widget com segurança
   gtk_widget_destroy( dialog );

   return prosseguir;
}






void salvar_estado_aplicativo( const InterfaceDados *dados, const CaminhoDiretorio *caminho ) {
   if ( !dados ) return;

   g_autofree gchar *cache = g_strdup_printf( "cache%d.bin", dados->seq_prova );
   g_autofree gchar *caminho_arquivo = g_build_filename( caminho->dados, cache, NULL );

   FILE *arquivo = fopen( caminho_arquivo, "wb" );
   if ( !arquivo ) return;

   // 1. A matemática do tamanho exato em bytes
   int n = dados->total_quest_prova; // A quantidade escolhida pelo usuário (ex: 5, 8, 15)
   size_t tamanho_total = sizeof( DadosRascunho ) + ( n * sizeof( QuestaoRascunho ) );

   // 2. Aloca a struct com o tamanho da "bagagem" extra usando o calloc (para zerar a memória)
   DadosRascunho *rascunho = ( DadosRascunho * ) g_malloc0( tamanho_total );

   // 3. Popula os dados estáticos
   rascunho->foco_cor_serie = dados->foco_cor_serie;
   rascunho->foco_decoracao = dados->foco_decoracao;
   rascunho->qtd_paginas    = dados->qtd_paginas;
   rascunho->qtd_colunas    = dados->qtd_colunas;
   rascunho->separadores    = dados->separadores;
   rascunho->cabecalho_tipo = dados->cabecalho_tipo;
   rascunho->fonte_latex    = dados->fonte_latex;
   rascunho->expor_dados    = dados->expor_dados;
   rascunho->nao_presencial = dados->nao_presencial;
   rascunho->total_quest_prova = n;

   // 4. Popula o Flexible Array Member
   for ( int i = 0; i < n; i++ ) {
      rascunho->questoes[i].qtd = dados->qtd_quest_subtema[i]; // ou dados->qtd_questoes[i]

      if ( dados->selecao_quest_prova[i] != NULL ) {
         snprintf( rascunho->questoes[i].str, 128, "%s", dados->selecao_quest_prova[i] );
      }
   }

   // 5. Despeja o pacote de tamanho cirúrgico no disco
   fwrite( rascunho, tamanho_total, 1, arquivo );

   fclose( arquivo );
   g_free( rascunho ); // Limpa a RAM usada para a montagem do pacote
}







/* =================================================================================================================
   VALIDAÇÃO DE DADOS DA INTERFACE - Versão Modular (CSS + Painel de Feedback)
   ================================================================================================================= */
gboolean verificar_dados_da_interface( InterfacePainel *painel, const InterfaceDados *dados ) {

   // =========================================================================
   // VALIDAÇÃO 1: VERIFICAÇÃO DE ALUNOS HABILITADOS (ERROS E AVISOS)
   // =========================================================================
   if ( dados->qtd_alunos_ativos == 0 ) {
      if ( dados->periodo[0] == 'R' ) {
         painel->format_titulo    = meu_gerador_variadico( "✘ Erro de Listagem:" );
         painel->format_subtitulo = meu_gerador_variadico( "Sem alunos habilitados para Recuperação Final." );
         painel->format_instrucao = meu_gerador_variadico( "Por favor, execute o botão \"Relatório Final\" !" );
         criar_mensagem_painel( ERRO, painel );

      } else if ( dados->periodo[0] == 'C' ) {
         painel->format_titulo    = meu_gerador_variadico( "⚠ Restrição de Diário:" );
         painel->format_subtitulo = meu_gerador_variadico( "Não se aplica a produção de avaliações neste período." );
         painel->format_instrucao = meu_gerador_variadico( "Ambiente restrito ao \"Conselho de Classe\"." );
         criar_mensagem_painel( AVISO, painel );

      } else {
         painel->format_titulo    = meu_gerador_variadico( "✘ Dados Incompletos:" );
         painel->format_subtitulo = meu_gerador_variadico( "A lista de alunos do \"%s\" está vazia.", dados->periodo );
         painel->format_instrucao = meu_gerador_variadico( "Execute o botão \"Abrir\" e faça o preenchimento." );
         criar_mensagem_painel( ERRO, painel );
      }

      return TRUE;
   }

   // =========================================================================
   // VALIDAÇÃO 2: QUANTIDADE DE QUESTÕES (META DINÂMICA)
   // =========================================================================
   int cnq = 0;

   // Varre iterando sobre o limite dinâmico de questões escolhido para a prova
   for ( int i = 0; i < dados->total_quest_prova; i++ ) {
      cnq += dados->qtd_quest_subtema[i];
   }

   if ( cnq == 0 ) {
      painel->format_titulo    = meu_gerador_variadico( "✘ Falha na Distribuição:" );
      painel->format_subtitulo = meu_gerador_variadico( "Não foi possível \"Gerar Prova\"." );
      painel->format_instrucao = meu_gerador_variadico( "Selecione ao menos um tema no listbox à esquerda." );
      criar_mensagem_painel( ERRO, painel );
      return TRUE;

   } else if ( cnq != dados->total_quest_prova ) {

      // Extraímos a lógica ternária para ponteiros locais leves e rápidos.
      // Isso deixa a chamada da função variádica limpa e extremamente legível.
      int diferenca = abs( dados->total_quest_prova - cnq );
      const char *acao = ( cnq > dados->total_quest_prova ) ? "Retire" : "Adicione";
      const char *plural = ( diferenca == 1 ) ? "ão" : "ões";

      painel->format_titulo    = meu_gerador_variadico( "⚠ Ajuste de Estrutura:" );

      // O poder do variádico: repassamos os parâmetros diretamente, abolindo sprintf estáticos
      painel->format_subtitulo = meu_gerador_variadico( "%s %d quest%s para atingir a meta.", acao, diferenca, plural );
      painel->format_instrucao = meu_gerador_variadico( "A prova deve conter exatamente %d questões (Atual: %d).", dados->total_quest_prova, cnq );

      criar_mensagem_painel( AVISO, painel );
      return TRUE;
   }

   // Se passou por todas as validações, o painel fica livre para receber mensagens de sucesso
   return FALSE;
}







//=====================================================================================================//
//                           PERSISTÊNCIA: CARREGAMENTO INDIRETO DO DISCO                              //
//=====================================================================================================//
gboolean carregar_estado_aplicativo( AppContext *ctx ) {
   if ( !ctx ) return FALSE;

   CaminhoDiretorio *caminho = &ctx->caminho;
   InterfacePainel  *painel  = &ctx->painel;

   // 1. Montagem do caminho baseado no índice da prova
   g_autofree gchar *cache = g_strdup_printf( "cache%d.bin", ctx->dados.seq_prova );
   g_autofree gchar *caminho_arquivo = g_build_filename( caminho->dados, cache, NULL );

   // 2. Tenta abrir o arquivo em modo de leitura binária ("rb")
   FILE *arquivo = fopen( caminho_arquivo, "rb" );
   if ( !arquivo ) {

      painel->format_titulo    = meu_gerador_variadico( "⚠ Sem Dados na Memória:" );
      painel->format_subtitulo = meu_gerador_variadico( "Não há um rascunho anterior salvo para esta avaliação." );
      painel->format_instrucao = meu_gerador_variadico( "Aguardando novas entradas no %s.", ctx->dados.periodo );

      criar_mensagem_painel( AVISO, painel );
      return FALSE;
   }

   // =========================================================================
   // 3. ENGENHARIA REVERSA DO TAMANHO (FAM - Flexible Array Member)
   // =========================================================================
   fseek( arquivo, 0, SEEK_END );
   size_t tamanho_arquivo = ftell( arquivo );
   rewind( arquivo );

   // Trava de segurança: O arquivo precisa ter no mínimo o tamanho do cabeçalho
   if ( tamanho_arquivo < sizeof( DadosRascunho ) ) {
      fclose( arquivo );

      painel->format_titulo    = meu_gerador_variadico( "✘ Erro de Leitura:" );
      painel->format_subtitulo = meu_gerador_variadico( "O arquivo de cache está corrompido (tamanho inválido)." );
      painel->format_instrucao = meu_gerador_variadico( "Tente reconfigurar a estrutura e salvar novamente." );

      criar_mensagem_painel( ERRO, painel );
      return FALSE;
   }

   // 4. Aloca a struct local com o tamanho matematicamente exato
   DadosRascunho *rascunho = ( DadosRascunho * ) g_malloc0( tamanho_arquivo );

   // 5. Suga o bloco binário inteiro do disco de uma só vez
   size_t lidos = fread( rascunho, 1, tamanho_arquivo, arquivo );
   fclose( arquivo );

   if ( lidos != tamanho_arquivo ) {
      g_free( rascunho ); // Libera a memória para não vazar no erro

      painel->format_titulo    = meu_gerador_variadico( "✘ Erro de Leitura:" );
      painel->format_subtitulo = meu_gerador_variadico( "O arquivo de cache está corrompido ou ilegível." );
      painel->format_instrucao = meu_gerador_variadico( "Tente reconfigurar a estrutura e salvar novamente." );

      criar_mensagem_painel( ERRO, painel );
      fprintf( stderr, "Erro crítico: Arquivo de cache '%s' inválido.\n", caminho_arquivo );
      return FALSE;
   }

   // =========================================================================
   // 6. SUCESSO E ALIMENTAÇÃO DA INTERFACE
   // =========================================================================
   painel->format_titulo    = meu_gerador_variadico( "✔ Cache Carregado!" );
   painel->format_subtitulo = meu_gerador_variadico( "Parâmetros de temas, colunas e questões restaurados." );
   painel->format_instrucao = meu_gerador_variadico( "Sessão ativa para o %s.", ctx->dados.periodo );

   criar_mensagem_painel( SUCESSO, painel );

   // SOLUÇÃO MESTRE: Como rascunho agora é um ponteiro alocado, passamos direto sem o '&'
   alimentar_interface_temporaria( ctx, rascunho );

   // 7. Libera o pacote lido após a interface e a estrutura de dados interna já terem consumido o que precisavam
   g_free( rascunho );

   return TRUE;
}
//=====================================================================================================//










void popular_combo_box_generico( GtkWidget *combo, const void *dados_array, int limite, int foco,
                                 gulong handler_id, ComboMapperFunc mapper ) {
   g_return_if_fail( combo && mapper );

   if ( handler_id > 0 ) g_signal_handler_block( combo, handler_id );

   GtkTreeModel *model = gtk_combo_box_get_model( GTK_COMBO_BOX( combo ) );
   GtkListStore *store = GTK_LIST_STORE( model );
   gtk_list_store_clear( store );

   if ( limite > 0 && dados_array != NULL ) {
      GtkTreeIter iter;
      for ( int i = 0; i < limite; i++ ) {
         gtk_list_store_append( store, &iter );
         mapper( store, &iter, dados_array, i );
      }
   }

   if ( foco >= 0 ) gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), foco );

   if ( handler_id > 0 ) g_signal_handler_unblock( combo, handler_id );
}




//=====================================================================================================//
// FUNÇÃO AUXILIAR: Alimenta qualquer GtkComboBoxText de forma limpa e automática                      //
//=====================================================================================================//
void popular_combo_box_text( GtkWidget *combo, gchar **lista, int foco, int limite, gulong handler_id ) {
   // Validações rigorosas de tipo para evitar crashes (Melhor prática GTK/GLib)
   g_return_if_fail( GTK_IS_COMBO_BOX_TEXT( combo ) );
   g_return_if_fail( lista != NULL );

   // 1. Bloqueia o sinal para evitar disparos acidentais durante a alteração
   if ( handler_id > 0 ) {
      g_signal_handler_block( combo, handler_id );
   }

   // 2. O remove_all roda em absoluto silêncio
   gtk_combo_box_text_remove_all( GTK_COMBO_BOX_TEXT( combo ) );

   // 3. Preenchimento seguro baseado no padrão GStrv (terminado em NULL).
   // O limite < 0 atua como um "coringa" para ler o array todo, se você quiser.
   for ( int i = 0; lista[i] != NULL && ( limite < 0 || i < limite ); i++ ) {
      gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT( combo ), lista[i] );
   }

   gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), foco );

   // 4. Desbloqueia o sinal
   if ( handler_id > 0 ) {
      g_signal_handler_unblock( combo, handler_id );
   }

   // 5. Ajuste visual (Truncamento de texto grande)
   int qtd_caracteres = 22;
   renderizar_combo_box_ellipsize( combo, qtd_caracteres );
}
//=====================================================================================================//



/**
 * @brief Obtém o texto de um GtkComboBoxText em um índice específico.
 *
 * @param combo O GtkComboBoxText alvo.
 * @param index O índice (base 0) do item desejado.
 * @return gchar* alocado dinamicamente com o texto, ou NULL em caso de falha.
 *         IMPORTANTE: O chamador deve liberar a memória (use g_autofree).
 */
gchar *combo_box_text_get_text_at( GtkComboBox *combo, gint index ) {
   // 1. Blindagem de ponteiro padrão GLib
   g_return_val_if_fail( GTK_IS_COMBO_BOX( combo ), NULL );

   // Índice negativo geralmente ocorre ao resgatar um 'active_index' sem nada selecionado
   if ( index < 0 ) return NULL;

   // 2. Obtém o modelo de dados interno (GtkListStore)
   GtkTreeModel *model = gtk_combo_box_get_model( combo );
   if ( !model ) return NULL;

   // 3. Aponta o iterador para o índice informado
   GtkTreeIter iter;
   if ( !gtk_tree_model_iter_nth_child( model, &iter, NULL, index ) ) {
      return NULL; // Índice fora dos limites do combo
   }

   // 4. Extrai a string da coluna 0 (O GTK aloca memória aqui internamente)
   gchar *texto = NULL;
   gtk_tree_model_get( model, &iter, 0, &texto, -1 );

   return texto;
}







static void _atualizar_acervo_questoes_e_temas( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );

   InterfaceDados    *dados    = &ctx->dados;
   CaminhoDiretorio  *caminho  = &ctx->caminho;
   InterfaceDinamica *montagem = &ctx->prova.montagem;
   InterfaceLatex    *latex = &ctx->latex;
   InterfaceProva    *prova = &ctx->prova;

   // 1. Limpeza de interface anterior (se o combo já estiver ativo)
   if ( prova->handler_combo_tema > 0 ) {
      limpar_container( prova->listbox_subtemas );
      limpar_container( latex->listbox_subtemas );
      limpar_container( prova->flowbox_selecionados );

      for ( int i = 0; i < dados->total_quest_prova; i++ ) {
         g_clear_pointer( &dados->selecao_quest_prova[i], g_free );
         dados->qtd_quest_subtema[i] = 0;
      }
      montagem->cont_add = 0;
   }

   // Usa a nova função auxiliar de gerenciamento seguro de strings
   g_clear_pointer( &prova->subtemas, g_strfreev );
   g_clear_pointer( &prova->temas, g_strfreev );

   prova->foco_tema = 0;
   prova->limite_temas = 0;
   prova->limite_subtemas = 0; // Garantia extra de redefinição de estado

   // 3. Define o caminho físico do Acervo para a disciplina atual
   g_clear_pointer( &caminho->banco_questoes, g_free );
   caminho->banco_questoes = g_build_filename( dados->raiz, "acervo", dados->disciplina, NULL );
   g_mkdir_with_parents( caminho->banco_questoes, 0755 );

   // 4. Verifica a existência e processa o diretório
   if ( diretorio_existe( caminho->banco_questoes ) ) {
      prova->limite_temas = quantidade_diretorios( caminho->banco_questoes );
   }

   // 5. Bifurcação de Estado (Carga ou Fallback)
   if ( prova->limite_temas > 0 ) {

      prova->temas = carregar_diretorios_temas( prova->limite_temas, caminho->banco_questoes, NULL );

      popular_combo_box_text( ctx->prova.combo_tema, prova->temas, 0,
                              prova->limite_temas, prova->handler_combo_tema );

      popular_combo_box_text( ctx->latex.combo_tema_espelho, prova->temas, 0,
                              prova->limite_temas, latex->handler_combo_tema_espelho );

      atualizar_tema( ctx, prova->temas[0] );

   } else {
      // Pasta ausente ou vazia
      g_clear_pointer( &dados->tema, g_free );
      g_printerr( "AVISO: Acervo ausente ou vazio em: %s\n", caminho->banco_questoes );

      // Solução Elegante para as 6 linhas: Agrupamos por contexto de execução.
      // Primeiro blindamos a interface, depois alteramos os dados, e então liberamos a interface.
      if ( prova->handler_combo_tema > 0 ) {
         g_signal_handler_block( ctx->prova.combo_tema, prova->handler_combo_tema );
      }
      if ( latex->handler_combo_tema_espelho > 0 ) {
         g_signal_handler_block( ctx->latex.combo_tema_espelho, latex->handler_combo_tema_espelho );
      }

      gtk_combo_box_text_remove_all( GTK_COMBO_BOX_TEXT( ctx->prova.combo_tema ) );
      gtk_combo_box_text_remove_all( GTK_COMBO_BOX_TEXT( ctx->latex.combo_tema_espelho ) );

      if ( prova->handler_combo_tema > 0 ) {
         g_signal_handler_unblock( ctx->prova.combo_tema, prova->handler_combo_tema );
      }
      if ( latex->handler_combo_tema_espelho > 0 ) {
         g_signal_handler_unblock( ctx->latex.combo_tema_espelho, latex->handler_combo_tema_espelho );
      }
   }
}






//===============================================================================================
void atualizar_dados_e_alunos_ativos( AppContext *ctx ) {
   if ( !ctx ) return;

   const InterfaceDiario *diario = &ctx->diario;
   InterfaceDados      *dados      = &ctx->dados;
   InterfaceConteudo   *conteudo   = &ctx->diario.conteudo;
   InterfaceFrequencia *frequencia = &ctx->diario.frequencia;
   InterfaceAvaliacao  *avaliacao = &ctx->diario.avaliacao;

   ui_restaurar_registros_de_aula( ctx, TRUE );
   conteudo->editando = FALSE; // GG, acabei retornando para bancada por esse pequeno detalhe, ele me permite carregar um registro de uma turma e salvar em outra (muito útil no dia a dia). Essa função é executada quando a turma ou o período muda. A ausência dessa linha estava causando falha de segmentação quando eu tentava executar o referido procedimento. Agora vou dormir de verdade, rsrs. Boa noite.

   salvar_fichas( ctx, FALSE );

   acessar_e_carregar_ficha_dos_alunos_da_turma( ctx );

   carregar_relatorio_ui( ctx );

   if ( ctx->cabecalho.foco_periodo != RECUPERACAO_FINAL ) {
      carregar_notas_ui_por_periodo( ctx );
   }

   int limite = ( dados->qtd_alunos_total < 0 ) ? 0 : dados->qtd_alunos_total;

   int foco = obter_foco_inicial( limite, ctx );

   popular_combo_box_generico( frequencia->combo_alunos, diario->fichas, limite, foco,
                               frequencia->handler_combo_alunos, mapear_alunos );

   // Precisa que a lista de alunos já esteja populada
   salvar_diario( ctx, FALSE );
   carregar_diario( ctx );

   popular_datas( ctx );

   if ( ctx->cabecalho.foco_periodo != RECUPERACAO_FINAL ) {
      salvar_avaliacoes( ctx, FALSE );
      carregar_avaliacoes( ctx );
      obter_avaliacoes_validas( avaliacao );
      sincronizar_registro_aula_com_turma_siaep( ctx );
   }

   gboolean modo_por_aluno = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( frequencia->check_por_aluno ) );

   if ( modo_por_aluno ) {
      renderizar_frequencia_modo_por_aluno( ctx );
   } else {
      renderizar_frequencia_modo_normal( ctx );
   }

   if ( frequencia->handler_combo_data == 0 || frequencia->handler_combo_alunos == 0 ) {
      rolagem_automatica_treeview_frequencia( ctx );
      selecionar_combo_status( ctx );
   }

   iniciar_autosave_diario( ctx, 5 );

   g_autofree gchar *texto_ativos = g_strdup_printf( "%d ativos", dados->qtd_alunos_ativos );
   gtk_label_set_text( GTK_LABEL( ctx->prova.label_qtd_ativos ), texto_ativos );
}
//==================================================================================================





//==================================================================================================
// Função auxiliar para criar arquivos ocultos vazios
static void _criar_arquivo_oculto( const gchar *diretorio, const gchar *nome ) {
   if ( !diretorio || !nome || nome[0] == '\0' ) return;

   g_autofree gchar *nome_oculto = g_strdup_printf( ".%s", nome );
   g_autofree gchar *caminho_arquivo = g_build_filename( diretorio, nome_oculto, NULL );

   if ( !g_file_test( caminho_arquivo, G_FILE_TEST_EXISTS ) ) {
      g_file_set_contents( caminho_arquivo, "", -1, NULL ); // Cria o arquivo invisível vazio
   }
}

//--------------------------------------------------------------------------------------------------
static void _carregar_escolas_e_turmas( const char *raiz, GtkComboBox *combo_periodo ) {
   g_return_if_fail( raiz != NULL && combo_periodo != NULL );

   // =========================================================================
   // 1. EXTRAÇÃO DOS PERÍODOS DO GLADE
   // =========================================================================
   GtkTreeModel *model = gtk_combo_box_get_model( GTK_COMBO_BOX( combo_periodo ) );
   GtkTreeIter iter;
   gboolean valid = gtk_tree_model_get_iter_first( model, &iter );

   g_autoptr( GPtrArray ) lista_periodos = g_ptr_array_new_with_free_func( g_free );

   // Varre o modelo interno do ComboBox e salva as strings ("1º Período", "2º Período"...)
   while ( valid ) {
      gchar *periodo = NULL;
      // Em um GtkComboBoxText, o texto sempre fica armazenado na coluna 0
      gtk_tree_model_get( model, &iter, 0, &periodo, -1 );

      if ( periodo ) {
         g_ptr_array_add( lista_periodos, periodo );
      }
      valid = gtk_tree_model_iter_next( model, &iter );
   }

   // =========================================================================
   // 2. LEITURA DA CONFIGURAÇÃO INICIAL (vertice.ini)
   // =========================================================================
   g_autofree gchar *vertice_ini = g_build_filename( raiz, "config", "vertice.ini", NULL );

   g_autoptr( GKeyFile ) keyfile = g_key_file_new();
   g_autoptr( GError ) error = NULL;

   if ( !g_key_file_load_from_file( keyfile, vertice_ini, G_KEY_FILE_NONE, &error ) ) {
      g_printerr( "[Aviso] Arquivo vertice.ini não encontrado ou inválido: %s\n", error->message );
      return;
   }

   g_autofree gchar *ano_letivo = g_key_file_get_string( keyfile, "Geral", "ano_letivo", NULL );
   if ( !ano_letivo ) ano_letivo = g_strdup( "2027" ); // Fallback

   gsize n_escolas = 0;
   g_auto( GStrv ) escolas = g_key_file_get_string_list( keyfile, "Geral", "escolas", &n_escolas, NULL );
   if ( !escolas ) return;

   g_autofree gchar *base_informados = g_build_filename( raiz, "dados", "informados", ano_letivo, NULL );

   // =========================================================================
   // 3. CONSTRUÇÃO DA HIERARQUIA COMPLETA
   // =========================================================================
   for ( gsize e = 0; e < n_escolas; e++ ) {
      const gchar *escola = escolas[e];
      g_autofree gchar *dir_escola = g_build_filename( base_informados, escola, NULL );
      g_mkdir_with_parents( dir_escola, 0755 );

      // Arquivo oculto do GESTOR (Raiz da escola)
      g_autofree gchar *gestor = g_key_file_get_string( keyfile, escola, "gestor", NULL );
      if ( gestor ) _criar_arquivo_oculto( dir_escola, gestor );

      gsize n_turmas = 0;
      g_auto( GStrv ) turmas = g_key_file_get_string_list( keyfile, escola, "turmas", &n_turmas, NULL );
      if ( !turmas ) continue;

      for ( gsize i = 0; i < n_turmas; i++ ) {
         const gchar *turma = turmas[i];
         g_autofree gchar *dir_turma = g_build_filename( dir_escola, turma, NULL );
         g_mkdir_with_parents( dir_turma, 0755 );

         g_autofree gchar *secao_turma = g_strdup_printf( "%s:%s", escola, turma );

         gsize n_disc = 0;
         g_auto( GStrv ) disciplinas = g_key_file_get_string_list( keyfile, secao_turma, "disciplinas", &n_disc, NULL );
         if ( !disciplinas ) continue;

         for ( gsize j = 0; j < n_disc; j++ ) {
            const gchar *disc = disciplinas[j];
            g_autofree gchar *dir_disc = g_build_filename( dir_turma, disc, NULL );
            g_mkdir_with_parents( dir_disc, 0755 );

            // Arquivo oculto do PROFESSOR (Raiz da disciplina)
            g_autofree gchar *chave_prof = g_strdup_printf( "prof_%s", disc );
            g_autofree gchar *prof = g_key_file_get_string( keyfile, secao_turma, chave_prof, NULL );
            if ( prof ) _criar_arquivo_oculto( dir_disc, prof );

            // ---------------------------------------------------------------
            // O GRAN FINALE: Injeção dos Períodos lidos do GTK na Disciplina
            // ---------------------------------------------------------------
            for ( guint p = 0; p < lista_periodos->len; p++ ) {
               const gchar *periodo = g_ptr_array_index( lista_periodos, p );

               g_autofree gchar *dir_periodo = g_build_filename( dir_disc, periodo, NULL );
               g_mkdir_with_parents( dir_periodo, 0755 );
            }
         }
      }
   }
}
//--------------------------------------------------------------------------------------------------
void inicializar_estado_do_aplicativo( AppContext *ctx ) {
   if ( !ctx ) return;

   InterfaceDados     *dados    = &ctx->dados;
   InterfaceConteudo  *conteudo = &ctx->diario.conteudo;
   InterfaceAvaliacao *avaliacao = &ctx->diario.avaliacao;
   InterfaceCabecalho *cabecalho = &ctx->cabecalho;

   dados->max_quest_prova = 20;
   dados->total_quest_prova = 10; // Esse valor deverá ser escolhido na interface em breve.
   dados->selecao_quest_prova = g_new0( gchar*, dados->total_quest_prova + 1 );
   dados->qtd_quest_subtema = g_new0( gint, dados->total_quest_prova );
   avaliacao->validas = g_new0( gboolean, MAX_AVALIACOES );

   gtk_widget_set_name( ctx->cabecalho.combo_turma, "turma" );

   obter_data_hoje( &ctx->hoje );

   g_autofree char *data_formatada = g_strdup_printf( "%02d/%02d/%04d", ctx->hoje.dia, ctx->hoje.mes, ctx->hoje.ano );
   gtk_entry_set_text( GTK_ENTRY( conteudo->entry_data ), data_formatada );

   g_autofree char *str_qtd_aulas = g_strdup_printf( "%d h", 1 );
   gtk_label_set_text( GTK_LABEL( conteudo->label_ch ), str_qtd_aulas );

   int qtd_caracteres = 10;
   renderizar_combo_box_ellipsize( cabecalho->combo_periodo, qtd_caracteres );

   long int escalar_hoje = mapear_data_para_id( ctx->hoje.dia, ctx->hoje.mes, ctx->hoje.ano );
   cabecalho->foco_periodo_vigente = foco_periodo_corrente( escalar_hoje );
   cabecalho->foco_periodo = cabecalho->foco_periodo_vigente;
   GtkComboBox *combo_periodo = GTK_COMBO_BOX( cabecalho->combo_periodo );

   gtk_combo_box_set_active( combo_periodo, cabecalho->foco_periodo );
   dados->periodo = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( cabecalho->combo_periodo ) );

   dados->foco_cor_serie = COR_SERIE_PRIMEIRO_ANO;
   gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->latex.combo_cor_serie ), dados->foco_cor_serie );

   dados->foco_decoracao = DECORACAO_TRAPEZIOS;
   gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->latex.combo_decoracao ), dados->foco_decoracao );

   dados->prova_sequencia = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( ctx->prova.combo_seq_prova ) );

   const gchar *home = g_get_home_dir();
   // ctx->caminho.raiz = g_build_filename( home ? home : ".", "Documentos", "LATEX", "SEDUC", NULL );
   ctx->caminho.raiz = g_build_filename( home ? home : ".", "Documentos", "Vértice", NULL );
   dados->raiz = ctx->caminho.raiz;

   _carregar_escolas_e_turmas( dados->raiz, combo_periodo );

   g_autofree gchar *diretorio = g_build_filename( dados->raiz, "dados", "informados", NULL );
   cabecalho->limite_anos = quantidade_diretorios( diretorio );
   cabecalho->anos = carregar_diretorios_temas( cabecalho->limite_anos, diretorio, NULL );

   popular_combo_box_text( cabecalho->combo_ano, cabecalho->anos, cabecalho->limite_anos - 1, cabecalho->limite_anos, cabecalho->handler_ano );

   on_entry_atualizar_ano_interface_changed( NULL, ctx );
}
//======================================================================================================================//






gboolean atualizar_ano_interface( AppContext *ctx, gchar *novo_ano, gboolean forcar_atualizacao ) {
   if ( !ctx || !novo_ano ) return FALSE;

   InterfaceDados  *dados   = &( ctx->dados );
   InterfaceCabecalho *cabecalho = &ctx->cabecalho;

   // 1. Agora SIM! TRUE significa que mudou, FALSE significa que é igual.
   gboolean ano_mudou = ( g_strcmp0( novo_ano, dados->ano ) != 0 );

   // 2. Barreira de Identidade: Se o ano NÃO mudou E NÃO for um disparo forçado (inicialização), aborta!
   if ( !ano_mudou && !forcar_atualizacao ) {
      return FALSE;
   }

   // 3. Se passou da barreira e o ano mudou de fato, atualiza o dado estrutural
   if ( ano_mudou ) {
      g_clear_pointer( &dados->ano, g_free );
      dados->ano = g_strdup( novo_ano );
   }

   // 4. Montagem de diretórios e carga lógica (Ocorre na inicialização ou se o ano mudou de verdade)
   g_autofree gchar *diretorio = g_build_filename( dados->raiz, "dados", "informados", dados->ano, NULL );

   // Gerenciamento seguro da RAM
   g_clear_pointer( &cabecalho->escolas, g_strfreev );

   ctx->cabecalho.limite_escolas = quantidade_diretorios( diretorio );
   if ( ctx->cabecalho.limite_escolas > 0 ) {
      cabecalho->escolas = carregar_diretorios_temas( ctx->cabecalho.limite_escolas, diretorio, NULL );
   }

   return TRUE; // Retorna TRUE indicando que uma carga real (ou semente) foi processada
}





gboolean atualizar_escola_interface( AppContext *ctx, gchar *nova_escola, gboolean forcar_atualizacao ) {
   if ( !ctx || !nova_escola ) return FALSE;

   InterfaceDados     *dados     = &ctx->dados;
   InterfaceResponsavel *responsavel = &ctx->diario.responsavel;
   InterfaceCabecalho *cabecalho = &ctx->cabecalho;

   gboolean escola_mudou = ( g_strcmp0( nova_escola, dados->escola ) != 0 );

   if ( escola_mudou ) {
      g_clear_pointer( &dados->escola, g_free );
      dados->escola = g_strdup( nova_escola );

   } else if ( !forcar_atualizacao ) {
      return FALSE;
   }

   g_autofree gchar *diretorio = g_build_filename( dados->raiz, "dados", "informados", dados->ano, dados->escola, NULL );

   ctx->cabecalho.limite_turmas = quantidade_diretorios( diretorio );

   g_clear_pointer( &cabecalho->turmas, g_strfreev );

   if ( ctx->cabecalho.limite_turmas > 0 ) {
      cabecalho->turmas = carregar_diretorios_temas( ctx->cabecalho.limite_turmas, diretorio, ordenar_turmas_novo_em );
   }

   g_clear_pointer( &dados->gestor, g_free );
   dados->gestor = gestor_da_escola( diretorio, responsavel->gestor );

   return TRUE;
}





gboolean atualizar_turma_interface( AppContext *ctx, gchar *nova_turma, gboolean forcar_atualizacao ) {
   if ( !ctx || !nova_turma ) return FALSE;

   InterfaceDados  *dados   = &( ctx->dados );
   InterfaceCabecalho *cabecalho = &ctx->cabecalho;

   // Verifica se a turma mudou de fato na memória
   gboolean turma_mudou = ( g_strcmp0( nova_turma, dados->turma ) != 0 );

   if ( turma_mudou ) {
      g_clear_pointer( &dados->turma, g_free );
      dados->turma = g_strdup( nova_turma );

   } else if ( !forcar_atualizacao ) {
      // Se a turma for idêntica e não for um disparo em cadeia/forçado, aborta aqui!
      return FALSE;
   }

   // Montagem do caminho físico: ./dados/informados/ANO/ESCOLA/TURMA
   g_autofree gchar *diretorio = g_build_filename( dados->raiz, "dados", "informados", dados->ano, dados->escola, dados->turma, NULL );

   // Varre o diretório para contar e carregar as disciplinas desta turma
   ctx->cabecalho.limite_disciplinas = quantidade_diretorios( diretorio );

   g_clear_pointer( &cabecalho->disciplinas, g_strfreev );

   if ( ctx->cabecalho.limite_disciplinas > 0 ) {
      cabecalho->disciplinas = carregar_diretorios_temas( ctx->cabecalho.limite_disciplinas, diretorio, NULL );
   }

   int foco = obter_foco_inicial_disciplina( ctx, ctx->cabecalho.limite_disciplinas );
   popular_combo_box_generico( ctx->cabecalho.combo_disciplina, ctx, ctx->cabecalho.limite_disciplinas, foco,
                               ctx->cabecalho.handler_disciplina, mapear_disciplinas );

   return TRUE; // Retorna TRUE indicando que a carga lógica foi realizada com sucesso
}








void atualizar_disciplina_interface( AppContext *ctx, gchar *nova_disciplina, gboolean forcar_atualizacao ) {
   if ( !ctx || !nova_disciplina ) return;

   InterfaceDados     *dados     = &ctx->dados;
   CaminhoDiretorio   *caminho   = &ctx->caminho;
   InterfaceResponsavel *responsavel = &ctx->diario.responsavel;

   // Agora SIM! TRUE significa que mudou, FALSE significa que é igual.
   gboolean disciplina_mudou = ( g_strcmp0( nova_disciplina, dados->disciplina ) != 0 );

   // 1. Atualiza o dado estrutural se houver mudança real
   if ( disciplina_mudou ) {
      g_clear_pointer( &dados->disciplina, g_free );
      dados->disciplina = g_strdup( nova_disciplina );
   }

   // 2. Lógica de atualização dos caminhos e alunos da pauta
   if ( disciplina_mudou || forcar_atualizacao ) {
      caminhos_uteis_de_diretorios( dados, caminho );
      atualizar_dados_e_alunos_ativos( ctx );

   } else if ( !forcar_atualizacao ) {
      return;
   }

   if ( disciplina_mudou ) {
      g_autofree gchar *diretorio = g_build_filename( dados->raiz, "dados", "informados", dados->ano, dados->escola, dados->turma, dados->disciplina, NULL );

      g_clear_pointer( &dados->professor, g_free );
      dados->professor = professor_da_disciplina( diretorio, responsavel->professor );

      _atualizar_acervo_questoes_e_temas( ctx );

   }
}



void atualizar_periodo_interface( AppContext *ctx, gchar *novo_periodo ) {
   if ( !ctx || !novo_periodo ) return;

   InterfaceDados   *dados   = &( ctx->dados );
   CaminhoDiretorio *caminho = &( ctx->caminho );

   g_clear_pointer( &dados->periodo, g_free );
   dados->periodo = g_strdup( novo_periodo );
   caminhos_uteis_de_diretorios( dados, caminho );
   atualizar_dados_e_alunos_ativos( ctx );
}






// // PARA O FUTURO ... (função feita pela GG)
// #include <sys/stat.h>
// #include <sys/types.h>
//
// void criar_pastas_atividades(void) {
//     char CaminhoDiretorio[1100];
//
//     // Primeiro cria a pasta raiz da turma/disciplina/periodo
//     // Usamos caminhos_uteis_de_diretorios() para garantir que caminho->dados está atualizado
//     snprintf(CaminhoDiretorio, sizeof(CaminhoDiretorio), "%s/Atividades Entregues", caminho->dados);
//
//     // Cria a pasta principal (ignora se já existir)
//     mkdir(CaminhoDiretorio, 0777);
//
//     for (int i = 0; i < dados->qtd_alunos_total; i++) {
//         char pasta_aluno[1200];
//         // Formata: "01 - NOME DO ALUNO"
//         snprintf(pasta_aluno, sizeof(pasta_aluno), "%s/%.2d - %s",
//                  CaminhoDiretorio, i + 1, ficha[i].aluno);
//
//         // Cria a pasta do aluno de forma nativa e ultra-rápida
//         if (mkdir(pasta_aluno, 0777) == 0) {
//             // Sucesso!
//         }
//     }
// }


