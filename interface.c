/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

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

// Esta é a única inclusão deste arquivo em todo o projeto
#include "auxiliar.h"





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
static void _iniciar_autosave_diario( AppContext *ctx, guint intervalo_minutos ) {
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





// void interface_style( AppContext *ctx ) {
//    if ( !ctx ) return;
//
//    // Capturamos a screen global uma única vez no topo do escopo da função
//    GdkScreen *screen = gdk_screen_get_default();
//
//    /* ==========================================================================
//       🚀 1. BLINDAGEM DA MEMÓRIA E EXPURGO DO CSS ANTIGO
//       ========================================================================== */
//    if ( ctx->provider != NULL ) {
//       // Remove o vínculo do provedor de estilo da tela activa
//       gtk_style_context_remove_provider_for_screen( screen, GTK_STYLE_PROVIDER( ctx->provider ) );
//
//       // Decrementa o contador de referências do objeto GLib e zera o ponteiro
//       g_object_unref( ctx->provider );
//       ctx->provider = NULL;
//    }
//
//    /* ==========================================================================
//       🏛️ 2. CRIAÇÃO DO PROVEDOR E MAPEAMENTO DO ARQUIVO ALVO
//       ========================================================================== */
//    ctx->provider = gtk_css_provider_new();
//
//    char arquivo_css[256];
//    switch ( ctx->dados.interface_style ) {
//    case 0 :
//       snprintf( arquivo_css, sizeof( arquivo_css ), "%s/css/style_dark_green.css", ctx->caminho.recursos_prefix );
//       break;
//    case 1 :
//       snprintf( arquivo_css, sizeof( arquivo_css ), "%s/css/style_deep_blue.css", ctx->caminho.recursos_prefix );
//       break;
//    case 2 :
//    default:
//       snprintf( arquivo_css, sizeof( arquivo_css ), "%s/css/style_light.css", ctx->caminho.recursos_prefix );
//       break;
//    }
//
//    /* ==========================================================================
//       📥 3. CARREGAMENTO E INJEÇÃO NA MÁQUINA DE RENDERIZAÇÃO DO GTK
//       ========================================================================== */
//
//    // Como você está usando a função _load_from_resource, agora o caminho purista vai casar perfeitamente!
//    gtk_css_provider_load_from_resource( ctx->provider, arquivo_css );
//
//    // Aplica as novas diretrizes visuais na tela global
//    gtk_style_context_add_provider_for_screen(
//       screen,
//       GTK_STYLE_PROVIDER( ctx->provider ),
//       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
//    );
// }




void interface_style( AppContext *ctx ) {
   if ( !ctx ) return;

   // Capturamos a screen global uma única vez no topo do escopo da função
   GdkScreen *screen = gdk_screen_get_default();

   /* ==========================================================================
      🚀 1. BLINDAGEM DA MEMÓRIA E EXPURGO DO CSS ANTIGO
      --------------------------------------------------------------------------
      Antes de alocar novos estilos, removemos o provedor anterior da tela e
      liberamos sua referência no Heap. Isso evita sobreposição de regras de
      estilo e vazamento de memória (Memory Leaks) nas trocas dinâmicas.
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
      --------------------------------------------------------------------------
      Os arquivos CSS controlam 100% da identidade visual do sistema (incluindo
      as janelas, caixas de diálogo e as barras superiores). Dispensamos
      qualquer alteração manual em GtkSettings para garantir a soberania do CSS.
      ========================================================================== */
   ctx->provider = gtk_css_provider_new();

   const char *arquivo_css;
   switch ( ctx->dados.interface_style ) {
   case 0 :
      arquivo_css = "./recursos/css/style_dark_green.css";
      break;
   case 1 :
      arquivo_css = "./recursos/css/style_deep_blue.css" ;
      break;
   case 2 :
   default:
      arquivo_css = "./recursos/css/style_light.css"     ;
      break;
   }

   /* ==========================================================================
      📥 3. CARREGAMENTO E INJEÇÃO NA MÁQUINA DE RENDERIZAÇÃO DO GTK
      --------------------------------------------------------------------------
      Injetamos as regras com prioridade APPLICATION, o que força o motor do GTK
      a ignorar os temas padrões do sistema operacional (Debian/Ubuntu) e adotar
      as especificidades declaradas nas nossas folhas de estilo.
      ========================================================================== */
   GError *error = NULL;
   gtk_css_provider_load_from_path( ctx->provider, arquivo_css, &error );

   if ( error != NULL ) {
      g_printerr( "🚨 Erro ao carregar o arquivo CSS (%s): %s\n", arquivo_css, error->message );
      g_clear_error( &error );
   }

   // Aplica as novas diretrizes visuais na tela global
   gtk_style_context_add_provider_for_screen(
      screen,
      GTK_STYLE_PROVIDER( ctx->provider ),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
   );

}






void atualizar_booleanos_interface( const bool estado, const int categoria, AppContext *ctx ) {
   if ( !ctx ) return;
   InterfaceDados          *dados    = &ctx->dados;
   const InterfaceCheck    *check    = &ctx->check;
   const InterfaceHandlers *handlers = &ctx->handlers;

   switch ( categoria ) {
   case 1:
      dados->cruz = estado;
      break; // Números Cruzados

   case 2: // Não Presencial
      dados->naopresencial = estado;

      if ( dados->naopresencial != dados->expor ) {

         dados->expor = dados->naopresencial;

         g_signal_handler_block( check->expor_dados, handlers->expor_dados );

         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check->expor_dados ), dados->expor );

         g_signal_handler_unblock( check->expor_dados, handlers->expor_dados );
      }

      break;

   case 3: // Expôr dados
      dados->expor = estado;

      if ( !dados->expor && dados->naopresencial ) {

         dados->naopresencial = dados->expor;

         g_signal_handler_block( check->nao_presencial, handlers->nao_presencial );

         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check->nao_presencial ), dados->naopresencial );

         g_signal_handler_unblock( check->nao_presencial, handlers->nao_presencial );
      }

      break;

   default:
      g_print( "Categoria desconhecida: %d\n", categoria );
      break;
   }
}


//---------------------------------------------------------------------------------------------------------------
static void _ui_restaurar_registros_de_aula( const char *caminho_arquivo, InterfaceRegistroDiario *ui_diario,
                                          const int foco_estilo, gboolean rolagem );
//---------------------------------------------------------------------------------------------------------------
void atualizar_generic_interface( AppContext *ctx, const int categoria, const int valor ) {
   InterfaceDados *dados = ( InterfaceDados * ) & ( ctx->dados );
   InterfaceListas *listas = ( InterfaceListas * ) & ( ctx->listas );

   switch ( categoria ) {
   case 1: // Colunas
      dados->qtd_colunas = valor; // valor 2 ou 3
      break;
   case 2: // Separadores
      dados->separadores = valor; // valor 1 (pontinhado) ou 2 (contínuo)
      break;
   case 3: // Fonte
      dados->fonte_latex = valor; // valor 1 (CMB Right) ou 2 (CMU Serif)
      break;
   case 4: // Quantidade de páginas
      dados->qtd_paginas = valor; // valor 1 ou 2
      break;
   case 5: // Cabeçalho (tipo)
      dados->cabecalho_tipo = valor; // valor 1 (acima) ou 2 (à esquerda)
      break;
   case 6: // Prova
      dados->iprova = valor; // valor 1 (Primeira), 2 (Segunda) ou 3 (Terceira)
      snprintf( dados->prova_sequencia, sizeof( dados->prova_sequencia ), "%s", listas->provas_sequencia[valor - 1].str );
      break;
   case 7: // Interface Style
      dados->interface_style = valor;
      interface_style( ctx );
      char *caminho_arquivo = g_build_filename( ctx->caminho.dados, "diario.bin", NULL );
      _ui_restaurar_registros_de_aula( caminho_arquivo, &ctx->ui_diario, dados->interface_style, FALSE );
      g_free(caminho_arquivo);
      renderizar_frequencia_por_data( ctx );
      break;
   default:
      g_print( "Categoria desconhecida: %d\n", categoria );
      break;
   }

}







void atualizar_tema( AppContext *ctx, const char *tema ) {
   if ( !ctx ) return;

   InterfaceDados   *dados   = &ctx->dados;
   CaminhoDiretorio *caminho = &ctx->caminho;
   InterfaceListas  *listas  = &ctx->listas;
   LimitesFiltro    *limite  = &ctx->cascata.limite;
   InterfaceEntry   *entry   = &ctx->entry;
   InterfacePainel  *painel  = &ctx->painel;

   snprintf( dados->tema, sizeof( dados->tema ), "%s", tema );

   free( listas->subtemas );
   listas->subtemas = NULL;

   char diretorio[512];
   snprintf( diretorio, sizeof( diretorio ), "%s/%s", caminho->banco_questoes, dados->tema );

   limite->subtemas = quantidade_diretorios( diretorio );

   if ( limite->subtemas > 0 ) {
      listas->subtemas = carregar_diretorios_temas( limite->subtemas, diretorio, NULL );
   } else {
      const char *nome_widget = gtk_widget_get_name( entry->tema );
      if ( strcmp( nome_widget, "evento_via_codigo" ) != 0 ) {

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

   const InterfaceDados  *dados = &ctx->dados;
   const FocoCoordenadas *foco  = &ctx->cascata.foco;

   // 2. g_autofree: Libera o destino automaticamente não importa como a função acabe
   g_autofree
   char *destino = g_build_filename( ".", "dados", "gabaritos", dados->ano, dados->escola, "gabaritos", NULL );

   if ( g_mkdir_with_parents( destino, 0755 ) != 0 ) {
      g_printerr( "Erro crítico: Não foi possível criar os diretórios de destino: %s\n", destino );
      return; // Destino é liberado pelo g_autofree aqui
   }

   // Zera o buffer inicialmente para segurança
   char nome[32];
   nome_base_gabaritos_bin( nome, sizeof( nome ), foco->turma, foco->disciplina, foco->periodo, dados->iprova );

   // 3. g_autofree no arquivo resolve definitivamente o Memory Leak
   g_autofree char *arquivo = g_build_filename( destino, nome, NULL );

   // Cenário A: O arquivo não existe (Primeira compilação da prova)
   if ( !g_file_test( arquivo, G_FILE_TEST_EXISTS ) ) {
      gerar_gabaritos( arquivo, dados->qtd_alunos_ativos, dados->total_questoes, "wb" );
      return; // "arquivo" e "destino" são liberados silenciosamente pelo compilador aqui!
   }


   int contador = contar_registros_binarios( arquivo, sizeof( ItemTextoCurto ) );
   char sub_texto[256] = {0};
   char instrucao[256] = {0};

   // Sub-cenário B1: Novos alunos entraram (Aviso e Anexação com "a")
   if ( contador < dados->qtd_alunos_ativos ) {
      int adicionados = dados->qtd_alunos_ativos - contador;

      snprintf( sub_texto, sizeof( sub_texto ),
                "%d novos alunos ativos foram adicionados desde a última compilação.", adicionados );

      snprintf( instrucao, sizeof( instrucao ),
                "%d novos gabaritos serão anexados mantendo os antigos intactos.", adicionados );

      // Alimenta os buffers variádicos e delega o estilo para o motor central (AVISO)
      painel->format_titulo    = meu_gerador_variadico( "⚠ Lista de Alunos Modificada" );
      painel->format_subtitulo = meu_gerador_variadico( "%s", sub_texto );
      painel->format_instrucao = meu_gerador_variadico( "%s", instrucao );

      criar_mensagem_painel( AVISO, painel );

      gerar_gabaritos( arquivo, adicionados, dados->total_questoes, "ab" );
   }

   // Sub-cenário B2: Quantidade de alunos idêntica (Usa Pop-up)
   else if ( contador == dados->qtd_alunos_ativos ) {
      GtkWindow *janela_principal = GTK_WINDOW( gtk_widget_get_toplevel( widget ) );

      // Exemplo de uso na rotina de verificação do Diário/Gabarito:
      char *msg_gabarito = meu_gerador_variadico(
                              "<b>ESCOLA:</b> %s\n"  "<b>TURMA:</b> %s\n\n"
                              "O arquivo de <b>Gabaritos</b> da <b>%s Prova</b> do <b>%s</b> já existe.\n\n"
                              "Deseja criar um novo arquivo e sobrescrever o anterior?",
                              dados->escola, dados->turma, dados->prova_sequencia, dados->periodo
                           );
      gboolean emitir_gabarito = mostrar_popup_confirmacao( janela_principal, "Aviso de Alteração de Diário", msg_gabarito );
      g_free( msg_gabarito );

      if ( !emitir_gabarito ) {
         painel->format_titulo    = meu_gerador_variadico( "✔ Reaproveitando Histórico" );
         painel->format_subtitulo = meu_gerador_variadico( "Gerando provas com o uso de gabaritos gerados em compilação anterior." );
         painel->format_instrucao = meu_gerador_variadico( "Os dados originais dos alunos foram preservados com sucesso." );
      } else {
         gerar_gabaritos( arquivo, dados->qtd_alunos_ativos, dados->total_questoes, "wb" );

         painel->format_titulo    = meu_gerador_variadico( "✔ Novos Gabaritos Alocados" );
         painel->format_subtitulo = meu_gerador_variadico( "Gerando provas com o uso de uma nova sequência de gabaritos." );
         painel->format_instrucao = meu_gerador_variadico( "O arquivo antigo foi sobrescrito conforme solicitado." );
      }

      // Dispara o motor central com estado de SUCESSO
      criar_mensagem_painel( SUCESSO, painel );
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
   gtk_window_set_title( GTK_WINDOW( dialog ), titulo ? titulo : "Aviso do Sistema" );

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






void salvar_estado_aplicativo( const InterfaceDados *dados, const FocoCoordenadas *foco, const CaminhoDiretorio *caminho ) {
   if ( !dados || !foco ) return;

   char caminho_arquivo[1024];
   snprintf( caminho_arquivo, sizeof( caminho_arquivo ), "%s/cache%d.bin", caminho->dados, dados->iprova );

   FILE *arquivo = fopen( caminho_arquivo, "wb" );
   if ( !arquivo ) return;

   PacotePersistencia pacote;

   // 1. Copia apenas o "recheio" estético e pedagógico
   snprintf( pacote.rascunho.cor_destaque, sizeof( pacote.rascunho.cor_destaque ), "%s", dados->cor_destaque );
   snprintf( pacote.rascunho.decoracao_estilo, sizeof( pacote.rascunho.decoracao_estilo ), "%s", dados->decoracao_estilo );

   pacote.rascunho.qtd_paginas    = dados->qtd_paginas;
   pacote.rascunho.qtd_colunas    = dados->qtd_colunas;
   pacote.rascunho.separadores    = dados->separadores;
   pacote.rascunho.cabecalho_tipo = dados->cabecalho_tipo;
   pacote.rascunho.fonte_latex    = dados->fonte_latex;
   pacote.rascunho.expor          = dados->expor;
   pacote.rascunho.naopresencial  = dados->naopresencial;

   // Copia as matrizes de questões (0 a nti)
   for ( int i = 0; i < NTI; i++ ) {
      pacote.rascunho.qtd_questoes[i] = dados->qtd_questoes[i];
      size_t tam = sizeof( pacote.rascunho.temas_prova_sequencia[i].str );
      snprintf( pacote.rascunho.temas_prova_sequencia[i].str, tam, "%s", dados->temas_prova_sequencia[i].str );
   }

   // 2. Copia apenas os focos que importam (os estéticos)
   pacote.foco.cor_destaque     = foco->cor_destaque;
   pacote.foco.decoracao_estilo = foco->decoracao_estilo;

   // 3. Despeja no disco o pacote compacto
   fwrite( &pacote, sizeof( PacotePersistencia ), 1, arquivo );
   fclose( arquivo );
}







/* =================================================================================================================
   VALIDAÇÃO DE DADOS DA INTERFACE - Versão Modular (CSS + Painel de Feedback)
   ================================================================================================================= */
bool verificar_dados_da_interface( InterfacePainel *painel, const InterfaceDados *dados ) {

   // Garantimos que os buffers locais nasçam zerados para evitar lixo de memória
   char str_corpo[256] = {0};
   char str_meta[256]  = {0};

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

      return true;
   }

   // =========================================================================
   // VALIDAÇÃO 2: QUANTIDADE DE QUESTÕES (META EXATA DE 10 QUESTÕES)
   // =========================================================================
   int cnq = 0;
   for ( int i = 0; i < NTI; i++ ) {
      cnq += dados->qtd_questoes[i];
   }

   if ( cnq == 0 ) {
      painel->format_titulo    = meu_gerador_variadico( "✘ Falha na Distribuição:" );
      painel->format_subtitulo = meu_gerador_variadico( "Não foi possível \"Gerar Prova\"." );
      painel->format_instrucao = meu_gerador_variadico( "Selecione ao menos um tema no listbox à esquerda." );
      criar_mensagem_painel( ERRO, painel );
      return true;

   } else if ( cnq != 10 ) {
      snprintf( str_corpo, sizeof( str_corpo ), "%s %d quest%s para atingir a meta.",
                ( cnq > dados->total_questoes ) ? "Retire" : "Adicione",
                abs( dados->total_questoes - cnq ),
                ( abs( dados->total_questoes - cnq ) == 1 ) ? "ão" : "ões" );

      snprintf( str_meta, sizeof( str_meta ), "A prova deve conter exatamente 10 questões (Atual: %d).", cnq );

      painel->format_titulo    = meu_gerador_variadico( "⚠ Ajuste de Estrutura:" );
      painel->format_subtitulo = meu_gerador_variadico( "%s", str_corpo );
      painel->format_instrucao = meu_gerador_variadico( "%s", str_meta );
      criar_mensagem_painel( AVISO, painel );
      return true;
   }

   // Se passou por todas as validações, o painel fica livre para receber mensagens de sucesso
   return false;
}



















//=====================================================================================================//
//                           PERSISTÊNCIA: CARREGAMENTO INDIRETO DO DISCO                              //
//=====================================================================================================//
bool carregar_estado_aplicativo( AppContext *ctx ) {
   if ( !ctx ) return false;

   CaminhoDiretorio *caminho = &ctx->caminho;
   InterfacePainel  *painel  = &ctx->painel; // Ponteiro centralizado do painel de feedback
   char instrucao[256];

   // 1. Montagem do caminho baseado no índice da prova
   char caminho_arquivo[1024];
   snprintf( caminho_arquivo, sizeof( caminho_arquivo ), "%s/cache%d.bin", caminho->dados, ctx->dados.iprova );

   // 2. Tenta abrir o arquivo em modo de leitura binária ("rb")
   FILE *arquivo = fopen( caminho_arquivo, "rb" );
   if ( !arquivo ) {
      snprintf( instrucao, sizeof( instrucao ), "Aguardando novas entradas no %s.", ctx->dados.periodo );

      // Alimenta os buffers variádicos para o estado de AVISO
      painel->format_titulo    = meu_gerador_variadico( "⚠ Sem Dados na Memória:" );
      painel->format_subtitulo = meu_gerador_variadico( "Não há um rascunho anterior salvo para esta avaliação." );
      painel->format_instrucao = meu_gerador_variadico( "%s", instrucao );

      // Renderiza as cores dinâmicas via CSS e limpa o Heap
      criar_mensagem_painel( AVISO, painel );
      return false;
   }

   // 3. Aloca o pacote receptor temporário na pilha
   PacotePersistencia pacote_temp;

   // 4. Suga o bloco binário do disco
   size_t lidos = fread( &pacote_temp, sizeof( PacotePersistencia ), 1, arquivo );
   fclose( arquivo );

   if ( lidos != 1 ) {
      // Alimenta os buffers variádicos para o estado de ERRO crítico
      painel->format_titulo    = meu_gerador_variadico( "✘ Erro de Leitura:" );
      painel->format_subtitulo = meu_gerador_variadico( "O arquivo de cache está corrompido ou ilegível." );
      painel->format_instrucao = meu_gerador_variadico( "Tente reconfigurar a estrutura e salvar novamente." );

      // Renderiza o estilo do tema ativo (ex: tons carmim no Dark Green ou coral no Deep Blue)
      criar_mensagem_painel( ERRO, painel );

      fprintf( stderr, "Erro crítico: Arquivo de cache '%s' inválido.\n", caminho_arquivo );
      return false;
   }

   // Nota: O passo 5 original foi totalmente extinto! A quantidade de alunos permanece intacta no sistema.

   // 5. Mensagem de Sucesso usando o ecossistema unificado
   snprintf( instrucao, sizeof( instrucao ), "Sessão ativa para o %s.", ctx->dados.periodo );

   painel->format_titulo    = meu_gerador_variadico( "✔ Cache Carregado!" );
   painel->format_subtitulo = meu_gerador_variadico( "Parâmetros de temas, colunas e questões restaurados." );
   painel->format_instrucao = meu_gerador_variadico( "%s", instrucao );

   // Renderiza com sucesso e aplica as classes estáticas estáveis do CSS (.sucesso-titulo, etc.)
   criar_mensagem_painel( SUCESSO, painel );

   // 6. SOLUÇÃO MESTRE: Alimenta a interface usando os dados temporários
   alimentar_interface_temporaria( ctx, &pacote_temp.rascunho, &pacote_temp.foco );

   return true;
}
//=====================================================================================================//






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
         ctx->diario = NULL; // Protege o ponteiro de trabalho

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
      nova_aula.chamada[i].cod_aluno = ctx->ficha[i].cod_aluno;
   }

   // =====================================================================
   // 2. INSERE NA RAM E ORDENA
   // =====================================================================
   g_array_append_val( ctx->diarios, nova_aula );
   g_array_sort( ctx->diarios, comparar_datas_diario );

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
   GtkListStore *liststore = ui_diario->liststore_conteudo;
   GtkTreeIter iter;

   gtk_list_store_insert( liststore, &iter, novo_indice );

   GdkRGBA cor_texto;
   int r = cor_texto_linha_liststore( &nova_aula, foco_estilo, &cor_texto );

   gtk_list_store_set( liststore, &iter,
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
   GtkTreeModel *model = GTK_TREE_MODEL( ui_diario->liststore_conteudo );

   // 1. Descobre o índice da linha clicada na TreeView
   GtkTreePath *path = gtk_tree_model_get_path( model, iter );
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

   // Sincroniza o ponteiro de trabalho do app para essa aula específica
   ctx->diario = diario_edicao;
}



void modificar_registro_aula( AppContext *ctx ) {
   g_return_if_fail( ctx && ctx->diarios );

   InterfaceRegistroDiario *ui = &ctx->ui_diario;
   GtkListStore *liststore = ui->liststore_conteudo;

   g_autoptr(GtkTreePath) path_antigo = gtk_tree_model_get_path( GTK_TREE_MODEL( liststore ), &ui->iter_em_edicao );
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

   gtk_list_store_remove( liststore, &ui->iter_em_edicao );
   g_array_sort( ctx->diarios, comparar_datas_diario );

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
   gtk_list_store_insert( liststore, &iter, novo_indice );

   GdkRGBA cor_texto;
   int r_estilo = cor_texto_linha_liststore( &reg_clone, ctx->dados.interface_style, &cor_texto );

   gtk_list_store_set( liststore, &iter,
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
   gtk_entry_set_text( GTK_ENTRY( ui->tema ), "" );
}




static void _ui_restaurar_registros_de_aula( const char *caminho_arquivo, InterfaceRegistroDiario *ui_diario,
                                          const int foco_estilo, gboolean rolagem ) {
   g_return_if_fail( caminho_arquivo && ui_diario );

   GtkTreeView *treeview = GTK_TREE_VIEW( ui_diario->treeview_conteudo );
   GtkListStore *liststore = ui_diario->liststore_conteudo;

   // =====================================================================
   // 1. SALVA A POSIÇÃO EXATA DA TELA (Antes do clear)
   // =====================================================================
   GtkTreePath *path_topo = NULL;
   if ( !rolagem ) {
      // Captura o path da linha que está perfeitamente no topo visível no momento
      gtk_tree_view_get_visible_range( treeview, &path_topo, NULL );
   }

   gtk_list_store_clear( liststore );

   EstadoArquivo estado = verificar_arquivo( caminho_arquivo );
   if ( estado & ( ARQUIVO_INEXISTENTE | ARQUIVO_VAZIO ) ) {
      if ( path_topo ) gtk_tree_path_free( path_topo );
      return;
   }

   // =====================================================================
   // 2. LEITURA ATÔMICA DA GLIB (Evita engasgos na interface gráfica)
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
      gtk_list_store_append( liststore, &iter );

      GdkRGBA cor_texto;
      int r = cor_texto_linha_liststore( &registros[i], foco_estilo, &cor_texto );

      gtk_list_store_set( liststore, &iter,
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
      // Rola para a última linha (Novo registro inserido)
      g_autoptr( GtkTreePath ) path_fim = gtk_tree_model_get_path( GTK_TREE_MODEL( liststore ), &iter );
      if ( path_fim ) {
         gtk_tree_view_scroll_to_cell( treeview, path_fim, NULL, FALSE, 0.0, 0.0 );
      }
   } else if ( !rolagem && path_topo ) {
      // GG! Devolve a tela exatamente para a linha que estava no topo.
      // O TRUE e os zeros (0.0) garantem que a célula seja alinhada ao topo da TreeView.
      gtk_tree_view_scroll_to_cell( treeview, path_topo, NULL, TRUE, 0.0, 0.0 );
      gtk_tree_path_free( path_topo );
   }
}





void popular_datas( AppContext *ctx ) {
   g_return_if_fail( ctx && ctx->diarios );

   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;

   // Puxa a quantidade de itens e os dados brutos diretamente da memória (GArray)
   int qtd_itens = ( int )ctx->diarios->len;
   ctx->diario = ( RegistroDiario * )ctx->diarios->data;

   int foco = qtd_itens;

   // Encontra a última aula que não seja feriado ou atividade pedagógica
   if ( ctx->diario && qtd_itens > 0 ) {
      do {
         foco--;
         if ( foco < 0 ) break;
      } while ( ctx->diario[foco].tipo_registro == TIPO_REGISTRO_FERIADO ||
                ctx->diario[foco].tipo_registro == TIPO_REGISTRO_PEDAGOGICO );
   }

   if ( ctx->diario && qtd_itens > 0 && foco >= 0 ) {

      popular_combo_box_generico( ui_diario->combo_data, ctx->diario, qtd_itens, foco,
                                  ui_diario->handler_combo_data, mapear_datas_frequencia );
      GtkTreeIter iter;
      GtkComboBox *combo = GTK_COMBO_BOX( ctx->ui_diario.combo_data );
      if ( gtk_combo_box_get_active_iter( combo, &iter ) ) {
         GtkTreeModel *model = gtk_combo_box_get_model( combo );
         guint qtd_aulas = 0;
         gtk_tree_model_get( model, &iter, 1, &qtd_aulas, -1 );
         g_autofree gchar *str_qtd_aulas = meu_gerador_variadico( "<b>%u h</b>", qtd_aulas );
         gtk_label_set_markup( GTK_LABEL( ctx->ui_diario.label_ch ), str_qtd_aulas );
      }

   } else {
      // Limpa visualmente se a turma estiver vazia ou só tiver feriados
      popular_combo_box_generico( ui_diario->combo_data, NULL, 0, 0,
                                  ui_diario->handler_combo_data, mapear_datas_frequencia );

      gtk_label_set_text( GTK_LABEL( ui_diario->label_ch ), "0 h" );

      // Limpa a visualização da TreeView
      GtkTreeView *tree_view = GTK_TREE_VIEW( ui_diario->treeview_frequencia );
      GtkListStore *store_view = GTK_LIST_STORE( gtk_tree_view_get_model( tree_view ) );
      gtk_list_store_clear( store_view );
   }
}




void registrar_status_assiduidade_frequencia( InterfacePainel *painel, AppContext *ctx, StatusAssiduidade status ) {
   g_return_if_fail( ctx && painel );

   if ( ctx->diario == NULL ) return; // Retornar em silêncio


   // 1. Validação de segurança
   if ( status == SEM_STATUS ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Aviso:" );
      painel->format_subtitulo = meu_gerador_variadico( "Status de assiduidade inválido" );
      painel->format_instrucao = meu_gerador_variadico( "Selecione uma justificativa válida no menu antes de registrar a frequência." );
      criar_mensagem_painel( AVISO, painel );
      return;
   }

   // 2. Extrai índice do aluno
   int idx_aluno = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ) );
   if ( idx_aluno < 0 || idx_aluno >= ctx->dados.qtd_alunos_total ) return;

   // O ponteiro de trabalho já sabe exatamente em qual aula estamos
   RegistroDiario *diario = ctx->diario;

   // 3. Atualiza a RAM e aciona o gatilho do Autosave
   // O código do aluno agora é inserido para toda a turma durante o registro de uma nova aula
   // diario->chamada[idx_aluno].cod_aluno = ctx->ficha[idx_aluno].cod_aluno;
   diario->chamada[idx_aluno].status = status;
   _marcar_diario_modificado( ctx );

   GtkTreeView *tree_view = GTK_TREE_VIEW( ctx->ui_diario.treeview_frequencia );
   GtkListStore *store_view = GTK_LIST_STORE( gtk_tree_view_get_model( tree_view ) );
   GtkTreeModel *model_view = GTK_TREE_MODEL( store_view );
   const char *str_status = ctx->listas.status_assiduidade[status].str;

   GdkRGBA cor_texto;
   int r = cor_texto_linha_frequencia( status, ctx->dados.interface_style, &cor_texto );

   gboolean modo_por_aluno = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( ctx->ui_diario.check_por_aluno ) );

   if ( modo_por_aluno ) {
      // === NOVO COMPORTAMENTO: Atualiza a linha da DATA e avança o combo de DATAS ===
      int foco_data = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_data ) );
      g_autoptr( GtkTreePath ) path_edicao = gtk_tree_path_new_from_indices( foco_data, -1 );
      GtkTreeIter iter;

      // Altera apenas o status daquela aula na interface
      if ( gtk_tree_model_get_iter( model_view, &iter, path_edicao ) ) {
         gtk_list_store_set( store_view, &iter, 3, str_status, 5, (r==0) ? NULL : &cor_texto, -1 );
      }

      // Avança para o próximo dia (isso disparará o combo_data changed automaticamente)
      if ( foco_data + 1 < (int)ctx->diarios->len ) {
         gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_data ), foco_data + 1 );
      }
      return; // Encerra aqui, pulando a lógica de avançar alunos
   }

   // 4. VERIFICA SE O ALUNO JÁ ESTÁ NA TELA
   gboolean modo_edicao = FALSE;
   GtkTreeIter iter_view;
   int linha_alvo = 0;

   if ( gtk_tree_model_get_iter_first( model_view, &iter_view ) ) {
      do {
         int num_lista = 0;
         gtk_tree_model_get( model_view, &iter_view, 0, &num_lista, -1 );

         if ( num_lista == idx_aluno + 1 ) {
            modo_edicao = TRUE;
            break;
         }
         linha_alvo++;
      } while ( gtk_tree_model_iter_next( model_view, &iter_view ) );
   }

   // 5. ATUALIZA A INTERFACE VISUAL E PROCESSA INATIVOS
   if ( modo_edicao ) {
      gtk_list_store_set( store_view, &iter_view, 3, str_status, 5, (r==0) ? NULL : &cor_texto, -1 );
      idx_aluno++;
      while ( idx_aluno < ctx->dados.qtd_alunos_total && !ctx->ficha[idx_aluno].ativo ) {
         idx_aluno++;
      }
   } else {
      g_autofree gchar *nasc = formatar_data_extenso( ctx->ficha[idx_aluno].nasc );
      gboolean riscar = !ctx->ficha[idx_aluno].ativo;

      gtk_list_store_append( store_view, &iter_view );
      gtk_list_store_set( store_view, &iter_view,
                          0, idx_aluno + 1,
                          1, ctx->ficha[idx_aluno].aluno,
                          2, nasc,
                          3, str_status,
                          4, riscar,
                          5, (r==0) ? NULL : &cor_texto, -1 );
      idx_aluno++;

      // Processa inativos residuais na sequência e os espelha na RAM
      while ( idx_aluno < ctx->dados.qtd_alunos_total && !ctx->ficha[idx_aluno].ativo ) {
         // O código do aluno agora é inserido para toda a turma durante o registro de uma nova aula
         // diario->chamada[idx_aluno].cod_aluno = ctx->ficha[idx_aluno].cod_aluno;
         diario->chamada[idx_aluno].status = SEM_STATUS;

         g_autofree gchar *nasc_inativo = formatar_data_extenso( ctx->ficha[idx_aluno].nasc );
         gboolean riscar_inativo = !ctx->ficha[idx_aluno].ativo;
         int r_inativo = cor_texto_linha_frequencia( 0, ctx->dados.interface_style, &cor_texto );

         GtkTreeIter iter_inativo;
         gtk_list_store_append( store_view, &iter_inativo );
         gtk_list_store_set( store_view, &iter_inativo,
                             0, idx_aluno + 1,
                             1, ctx->ficha[idx_aluno].aluno,
                             2, nasc_inativo,
                             3, ctx->listas.status_assiduidade[0].str,
                             4, riscar_inativo,
                             5, (r_inativo==0) ? NULL : &cor_texto, -1 );
         idx_aluno++;
      }

      linha_alvo = gtk_tree_model_iter_n_children( model_view, NULL ) - 1;
   }

   // 6. ATUALIZA O LIMITE E AVANÇA O COMBO
   ctx->ui_diario.limite_combo_alunos = gtk_tree_model_iter_n_children( model_view, NULL ) + 1;

   if ( idx_aluno < ctx->dados.qtd_alunos_total ) {
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ), idx_aluno );
   }

   // 7. Rola a tela (g_autoptr faz o gtk_tree_path_free nos bastidores)
   if ( linha_alvo >= 0 ) {
      g_autoptr( GtkTreePath ) path_novo = gtk_tree_path_new_from_indices( linha_alvo, -1 );
      if ( path_novo ) {
         gtk_tree_view_scroll_to_cell( tree_view, path_novo, NULL, FALSE, 0.0, 0.0 );
      }
   }
}





void renderizar_frequencia_por_data( AppContext *ctx ) {
   g_return_if_fail( ctx );

   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;

   int foco = gtk_combo_box_get_active( GTK_COMBO_BOX( ui_diario->combo_data ) );

   // =====================================================================
   // 1. SINCRONIA: Aborta em segurança se o combo for inválido
   // =====================================================================
   if ( foco < 0 || ( guint )foco >= ctx->diarios->len ) {
      ctx->diario = NULL;
      gtk_list_store_clear( GTK_LIST_STORE( gtk_tree_view_get_model( GTK_TREE_VIEW( ui_diario->treeview_frequencia ) ) ) );
      ctx->ui_diario.foco_combo_alunos = -1;
      ctx->ui_diario.limite_combo_alunos = 0;
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ), ctx->ui_diario.foco_combo_alunos );
      return;
   }

   ctx->diario = &g_array_index( ctx->diarios, RegistroDiario, foco );
   RegistroDiario *diario = ctx->diario;

   // NOVA TRAVA: Se estiver no modo Aluno, atualizamos o ponteiro e rolamos a tela, mas abortamos a renderização geral.
   if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( ui_diario->check_por_aluno ) ) ) {
      g_autoptr( GtkTreePath ) path = gtk_tree_path_new_from_indices( foco, -1 );
      if ( path ) {
         gtk_tree_view_scroll_to_cell( GTK_TREE_VIEW( ui_diario->treeview_frequencia ), path, NULL, FALSE, 0.0, 0.0 );
      }
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
      gboolean ativo = ctx->ficha[i].ativo;
      int status_atual = diario->chamada[i].status;

      // 1. REGRA DE PARADA: Só interrompe a varredura se encontrar um aluno ATIVO e PENDENTE
      if ( ativo && status_atual == SEM_STATUS ) {
         break;
      }

      // 2. PROCESSAMENTO AUTOMÁTICO DE INATIVOS:
      // Se o aluno for inativo, garante que o código dele esteja salvo e atribui SEM_STATUS (0) na RAM
      if ( !ativo ) {
         diario->chamada[i].cod_aluno = ctx->ficha[i].cod_aluno;
         diario->chamada[i].status = SEM_STATUS;
         status_atual = SEM_STATUS;
      }

      // 3. RENDERIZAÇÃO NA TREEVIEW:
      // Adiciona o aluno à lista (ativos com seus status e inativos com SEM_STATUS e riscados)
      GtkTreeIter iter;
      gtk_list_store_append( store_view, &iter );

      GdkRGBA cor_texto;
      int tem_cor = cor_texto_linha_frequencia( status_atual, ctx->dados.interface_style, &cor_texto );

      g_autofree gchar *nasc = formatar_data_extenso( ctx->ficha[i].nasc );

      gtk_list_store_set( store_view, &iter,
                        0, i + 1,
                        1, ctx->ficha[i].aluno,
                        2, nasc,
                        3, ctx->listas.status_assiduidade[status_atual].str,
                        4, !ativo, // TRUE para aplicar o risco do GtkCellRendererText
                        5, ( tem_cor == 0 ) ? NULL : &cor_texto,
                        -1 );

      linhas_renderizadas++;
   }

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

   if ( linhas_renderizadas > 0 ) {
      // Rola a exibição da TreeView para sempre mostrar o último aluno avaliado
      GtkTreePath *path = gtk_tree_path_new_from_indices( linhas_renderizadas - 1, -1 );
      if ( path ) {
         gtk_tree_view_scroll_to_cell( tree_view, path, NULL, FALSE, 0.0, 0.0 );
         gtk_tree_path_free( path );
      }
   }
}


void renderizar_frequencia_por_aluno( AppContext *ctx ) {
   g_return_if_fail( ctx && ctx->diarios );

   int idx_aluno = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ) );
   if ( idx_aluno < 0 || idx_aluno >= ctx->dados.qtd_alunos_total ) return;

   GtkTreeView *tree_view = GTK_TREE_VIEW( ctx->ui_diario.treeview_frequencia );
   GtkListStore *store_view = GTK_LIST_STORE( gtk_tree_view_get_model( tree_view ) );
   gtk_list_store_clear( store_view );

   gboolean riscar = !ctx->ficha[idx_aluno].ativo;
   const char *nome_aluno = ctx->ficha[idx_aluno].aluno;
   int proxima_data_pendente = 0;
   gboolean achou_pendente = FALSE;

   for ( guint i = 0; i < ctx->diarios->len; i++ ) {
      RegistroDiario *diario = &g_array_index( ctx->diarios, RegistroDiario, i );

      gboolean tipo_feriado    = ( diario->tipo_registro == TIPO_REGISTRO_FERIADO );
      gboolean tipo_pedagogico = ( diario->tipo_registro == TIPO_REGISTRO_PEDAGOGICO );
      if ( tipo_feriado || tipo_pedagogico ) continue;

      int idx_st = diario->chamada[idx_aluno].status;

      // Localiza a primeira aula sem chamada para este aluno
      if ( idx_st == SEM_STATUS && !achou_pendente ) {
         proxima_data_pendente = i;
         achou_pendente = TRUE;
      }

      const char *str_status = ctx->listas.status_assiduidade[idx_st].str;
      GdkRGBA cor_texto;
      int r = cor_texto_linha_frequencia( idx_st, ctx->dados.interface_style, &cor_texto );

      GtkTreeIter iter;
      gtk_list_store_append( store_view, &iter );
      gtk_list_store_set( store_view, &iter,
                          0, idx_aluno + 1,
                          1, nome_aluno,
                          2, diario->data,       // Coluna 2 agora recebe a DATA
                          3, str_status,
                          4, riscar,
                          5, ( r == 0 ) ? NULL : &cor_texto, -1 );
   }

   // Posiciona o combo_data e a rolagem visual na data pendente
   if ( ctx->diarios->len > 0 ) {
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_data ), proxima_data_pendente );
   }
}



void treeview_frequencia_navegar_modo_por_aluno( const AppContext *ctx, int indice_linha ) {
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
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_data ), indice_real_diario );
   }
}

void treeview_frequencia_navegar_modo_normal( const AppContext *ctx, GtkTreeView *treeview, int indice_linha ) {
   if ( indice_linha < 0 && indice_linha >= ctx->dados.qtd_alunos_total ) {
      return;
   }

   if ( ctx->ficha[indice_linha].ativo ) {
      // Aluno Ativo: sincroniza o combo normalmente
      if ( ctx->ui_diario.combo_alunos ) {
         gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ), indice_linha );
      }
   } else {
      // ALUNO INATIVO: Lógica de repulsão magnética (Pulo Automático)
      int foco_anterior = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->ui_diario.combo_alunos ) );
      int target = -1;

      if ( indice_linha > foco_anterior ) {
         // Descendo (seta para baixo ou clique abaixo)
         for ( int i = indice_linha + 1; i < ctx->dados.qtd_alunos_total; i++ ) {
            if ( ctx->ficha[i].ativo ) { target = i; break; }
         }
      } else if ( indice_linha < foco_anterior ) {
         // Subindo (seta para cima ou clique acima)
         for ( int i = indice_linha - 1; i >= 0; i-- ) {
            if ( ctx->ficha[i].ativo ) { target = i; break; }
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
}




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
void popular_combo_box_text( GtkWidget *combo, const ItemCombo *lista, int foco, int limite, gulong handler_id ) {
   if ( !combo || !lista ) return;

   /* ==========================================================================
   ⚠️ EVITA LOOP DE FEEDBACK INFINITO (RECURSÃO MÚTUA DE SINAIS)
   --------------------------------------------------------------------------
   Por que este teste é vital?
   Quando alteramos ou populamos os ComboBoxes em cascata via código (ex: mudar
   a Escola força a limpeza e reinserção das Turmas), o GTK dispara o sinal
   "changed" AUTOMATICAMENTE, mesmo sem a intervenção física do usuário.

   A Variável 'handler_id':
   - Se o sinal estiver conectado com sucesso, o GLib retorna um ID > 0.
   - O valor 0 é reservado exclusivamente para indicar sinal ausente/desconectado.

   A Estratégia:
   Bloqueamos temporariamente o callback antes da mutação programática dos dados
   e o desbloqueamos imediatamente após, garantindo que o motor lógico só
   reaja quando o clique partir genuinamente do usuário no layout.
   ========================================================================== */
   if ( handler_id > 0 ) {
      g_signal_handler_block( combo, handler_id );
   }

   // 2. Agora o remove_all pode rodar em silêncio absoluto
   gtk_combo_box_text_remove_all( GTK_COMBO_BOX_TEXT( combo ) );

   if ( limite > 0 && lista != NULL ) {
      for ( int i = 0; i < limite; i++ ) {
         gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT( combo ), lista[i].str );
      }
      gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), foco );
   }

   // 3. Desbloqueia o sinal
   if ( handler_id > 0 ) {
      g_signal_handler_unblock( combo, handler_id );
   }

   int qtd_caracteres;
   if ( g_strcmp0( gtk_widget_get_name( combo ), "momento" ) == 0 ) {
      qtd_caracteres = 15;
   } else {
      qtd_caracteres = 22;
   }

   renderizar_combo_box_ellipsize( combo, qtd_caracteres );

}
//=====================================================================================================//










static void _atualizar_acervo_questoes_e_temas( AppContext *ctx ) {
   if ( !ctx ) return;

   InterfaceDados    *dados    = &ctx->dados;
   CaminhoDiretorio  *caminho  = &ctx->caminho;
   InterfaceListas   *listas   = &ctx->listas;
   FocoCoordenadas   *foco     = &ctx->cascata.foco;
   LimitesFiltro     *limite   = &ctx->cascata.limite;
   InterfaceEntry    *entry    = &ctx->entry;
   InterfaceHandlers *handlers = &ctx->handlers;
   InterfaceDinamica *provas   = &ctx->provas;
   InterfaceLatex    *latex    = &ctx->latex;

   if ( ctx->handlers.tema > 0 ) {
      limpar_container( provas->listbox_subtemas );
      limpar_container( latex->listbox_subtemas );
      limpar_container( provas->flowbox_selecionados );
      for ( int i = 0; i < NTI; i++ ) {
         dados->temas_prova_sequencia[i].str[0] = '\0';
         dados->qtd_questoes[i] = 0;
      }
      provas->cont_add = 0;
   }

   for ( int i = 0; i < limite->temas; i++ ) {
      free( provas->handler[i] );
   }

   // 1. Limpa memórias antigas com segurança (Gerenciamento de RAM)
   free( provas->handler );
   free( listas->subtemas );
   free( listas->temas );
   free( listas->qtd_subtemas );

   provas->handler          = NULL;
   listas->subtemas         = NULL;
   listas->temas = NULL;
   listas->qtd_subtemas     = NULL;

   foco->tema = 0;
   limite->temas = 0;

   // 2. Define o caminho físico do Acervo para a disciplina atual
   snprintf( caminho->banco_questoes, sizeof( caminho->banco_questoes ), "./acervo/%s", dados->disciplina );

   // 3. Verifica a existência física do diretório
   if ( diretorio_existe( caminho->banco_questoes ) ) {
      limite->temas = quantidade_diretorios( caminho->banco_questoes );



      if ( limite->temas > 0 ) {
         provas->handler = ( gulong** ) calloc( limite->temas, sizeof( gulong* ) );
         for ( int i = 0; i < limite->temas; i++ ) {
            provas->handler[i] = NULL;
         }

         listas->temas = carregar_diretorios_temas( limite->temas, caminho->banco_questoes, NULL );

         popular_combo_box_text( entry->tema, listas->temas, 0,
                                 limite->temas, handlers->tema );

         popular_combo_box_text( entry->tema_espelho, listas->temas, 0,
                                 limite->temas, handlers->tema_espelho );

         atualizar_tema( ctx, listas->temas[0].str );

      } else {
         // Pasta existe, mas está vazia
         snprintf( dados->tema, sizeof( dados->tema ), "%s", "" );
         g_printerr( "AVISO: Nenhum \"tema principal\" foi encontrado em %s\n", caminho->banco_questoes );
      }

   } else {

      // Estas 6 linhas são uma solução rápida, preciso pensar mais sobre isso.
      if ( handlers->tema > 0 ) g_signal_handler_block( entry->tema, handlers->tema );
      gtk_combo_box_text_remove_all( GTK_COMBO_BOX_TEXT( entry->tema ) );
      if ( handlers->tema > 0 ) g_signal_handler_unblock( entry->tema, handlers->tema );

      if ( handlers->tema_espelho > 0 ) g_signal_handler_block( entry->tema_espelho, handlers->tema_espelho );
      gtk_combo_box_text_remove_all( GTK_COMBO_BOX_TEXT( entry->tema_espelho ) );
      if ( handlers->tema_espelho > 0 ) g_signal_handler_unblock( entry->tema_espelho, handlers->tema_espelho );


      // Pasta da disciplina não existe no Acervo
      snprintf( dados->tema, sizeof( dados->tema ), "%s", "" );
      g_printerr( "AVISO: Pasta ausente: %s\n", caminho->banco_questoes );
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
      // g_array_sort( ctx->diarios, comparar_datas_diario );

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



//==================================================================================================
static void _sincronizar_registro_diario_com_turma_siaep( AppContext *ctx ) {
   g_return_if_fail( ctx );

   // 1. Aborta se não houver registros carregados na memória
   if ( !ctx->diarios || ctx->diarios->len == 0 ) return;

   // 2. Acessa o primeiro registro diretamente do GArray em O(1)
   RegistroDiario *primeiro_registro = &g_array_index( ctx->diarios, RegistroDiario, 0 );

   // 3. Conta os alunos presentes no primeiro registro salvo na RAM
   int qtd_fichas = 0;
   while ( qtd_fichas < 64 && primeiro_registro->chamada[qtd_fichas].cod_aluno != 0 ) {
      qtd_fichas++;
   }

   // 4. Validação do Gatilho: Verifica mudanças na quantidade
   gboolean precisa_sincronizar = ( qtd_fichas != ctx->dados.qtd_alunos_total );

   // 5. Verificação profunda: A ordem alfabética ou os IDs mudaram?
   if ( !precisa_sincronizar ) {
      for ( int k = 0; k < qtd_fichas; k++ ) {
         if ( primeiro_registro->chamada[k].cod_aluno != ctx->ficha[k].cod_aluno ) {
            precisa_sincronizar = TRUE;
            break;
         }
      }
   }

   // Chegaremos até aqui durante todas as mudanças de período ou de turma
   // Sempre que houver novos alunos entrantes, segue-se o fluxo da função até o fim
   if ( !precisa_sincronizar ) return; // Tudo em sincronia, aborta precocemente.

   // 6. Sincronização em Lote diretamente no GArray
   for ( guint r = 0; r < ctx->diarios->len; r++ ) {

      RegistroDiario *registro = &g_array_index( ctx->diarios, RegistroDiario, r );
      RegistroChamada nova_chamada[64] = {0}; // Zera magicamente todos os bytes

      for ( int k = 0; k < ctx->dados.qtd_alunos_total && k < 64; k++ ) {
         uint32_t cod_alvo = ctx->ficha[k].cod_aluno;
         nova_chamada[k].cod_aluno = cod_alvo; // Espelha a ordem da ficha atualizada

         gboolean encontrou_aluno = FALSE;

         // Varre a chamada antiga deste dia em busca do aluno
         for ( int j = 0; j < 64 && registro->chamada[j].cod_aluno != 0; j++ ) {
            if ( registro->chamada[j].cod_aluno == cod_alvo ) {
               nova_chamada[k].status = registro->chamada[j].status; // Restaura o status
               encontrou_aluno = TRUE;
               break;
            }
         }

         // Aplica a regra de negócio para novos alunos injetados pelo SIAEP
         if ( !encontrou_aluno ) {
            nova_chamada[k].status = NOVO_ALUNO;
         }
      }

      // Substitui o bloco desatualizado pelo novo na memória do GArray
      memcpy( registro->chamada, nova_chamada, sizeof( nova_chamada ) );
   }
}
//--------------------------------------------------------------------------------------------------
void atualizar_dados_e_alunos_ativos( AppContext *ctx ) {
   if ( !ctx ) return;

   InterfaceDados    *dados    = &ctx->dados;
   CaminhoDiretorio  *caminho  = &ctx->caminho;
   InterfacePainel   *painel   = &ctx->painel;

   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;

   g_autofree char *caminho_arquivo = g_build_filename( caminho->dados, "diario.bin", NULL );
   _ui_restaurar_registros_de_aula( caminho_arquivo, ui_diario, dados->interface_style, TRUE );
   ui_diario->editando = FALSE; // GG, acabei retornando para bancada por esse pequeno detalhe, ele me permite carregar um registro de uma turma e salvar em outra (muito útil no dia a dia). Essa função é executada quando a turma ou o período muda. A ausência dessa linha estava causando falha de segmentação quando eu tentava executar o referido procedimento. Agora vou dormir de verdade, rsrs. Boa noite.

   acessar_e_carregar_ficha_dos_alunos_da_turma( ctx );

   int limite = ( dados->qtd_alunos_total < 0 ) ? 0 : dados->qtd_alunos_total;

   int foco = obter_foco_inicial( limite, ctx->ficha );

   popular_combo_box_generico( ui_diario->combo_alunos, ctx->ficha, limite, foco,
                               ui_diario->handler_combo_alunos, mapear_alunos );

    // Precisa que a lista de alunos já esteja populada
   salvar_diario( ctx, FALSE );
   carregar_diario( ctx );
   popular_datas( ctx );

   _sincronizar_registro_diario_com_turma_siaep( ctx );

   renderizar_frequencia_por_data( ctx );

   _iniciar_autosave_diario( ctx, 5 );


   painel->format_cabecalho = meu_gerador_variadico( "%s  -  <b>%s</b>  -  %s  -  <b>%s / %c</b>  -  %d ativos",
                              dados->escola, dados->turma, dados->disciplina, dados->ano, dados->periodo[0],
                              ( dados->qtd_alunos_ativos == -1 ) ? 0 : dados->qtd_alunos_ativos );

   gtk_label_set_markup( GTK_LABEL( painel->cabecalho ), painel->format_cabecalho );
   g_free( painel->format_cabecalho );

}
//==================================================================================================






//==================================================================================================
void inicializar_estado_do_aplicativo( AppContext *ctx ) {
   if ( !ctx ) return;

   LimitesFiltro     *limite   = &ctx->cascata.limite;
   FocoCoordenadas   *foco     = &ctx->cascata.foco;
   InterfaceEntry    *entry    = &ctx->entry;
   InterfaceListas   *listas   = &ctx->listas;
   InterfaceDados    *dados    = &ctx->dados;
   InterfaceHandlers *handlers = &ctx->handlers;
   CalendarioData          *data     = &ctx->data;



   gtk_widget_set_name( ctx->entry.turma, "turma" );

   popular_combo_box_text( ctx->ui_diario.combo_status, listas->status_assiduidade, 0, 9, 0 );

   *data = data_de_hoje();
   long int escalar_hoje = mapear_data_para_id( data->dia, data->mes, data->ano );

   g_autofree char *data_formatada = g_strdup_printf( "%02d/%02d/%04d", data->dia, data->mes, data->ano );
   gtk_entry_set_text( GTK_ENTRY( ctx->ui_diario.entry_data ), data_formatada );

   g_autofree char *str_qtd_aulas = g_strdup_printf( "%d h", 1 );
   gtk_label_set_text( GTK_LABEL( ctx->ui_diario.qtd_aulas ), str_qtd_aulas );

   gtk_widget_set_name( ctx->entry.periodo, "momento" );
   foco->periodo = foco_periodo_corrente( escalar_hoje );
   popular_combo_box_text( entry->periodo, listas->periodos, foco->periodo, limite->periodos, handlers->periodo );
   snprintf( dados->periodo, sizeof( dados->periodo ), "%s", listas->periodos[ foco->periodo ].str );

   foco->cor_destaque = 0;
   popular_combo_box_text( entry->cor_destaque, listas->cores_destaque, foco->cor_destaque,
                           limite->cores_destaque, handlers->cor_destaque );
   snprintf( dados->cor_destaque, sizeof( dados->cor_destaque ), "%s", listas->cores_destaque[ foco->cor_destaque ].str );

   foco->decoracao_estilo = 1;
   popular_combo_box_text( entry->decoracao_estilo, listas->decoracoes_estilo, foco->decoracao_estilo,
                           limite->decoracoes_estilo, handlers->decoracao_estilo );
   snprintf( dados->decoracao_estilo, sizeof( dados->decoracao_estilo ), "%s",
             listas->decoracoes_estilo[ foco->decoracao_estilo ].str );


   // Varre os diretórios de anos letivos
   limite->anos = quantidade_diretorios( "./dados/informados" );
   listas->anos = carregar_diretorios_temas( limite->anos, "./dados/informados", NULL );

   popular_combo_box_text( entry->ano, listas->anos, limite->anos - 1, limite->anos, handlers->ano );
   on_entry_atualizar_ano_interface_changed( NULL, ctx );

}
//======================================================================================================================//






gboolean atualizar_ano_interface( AppContext *ctx, const char *novo_ano, gboolean forcar_atualizacao ) {
   if ( !ctx || !novo_ano ) return FALSE;

   InterfaceListas *listas  = &( ctx->listas );
   CascataControle *cascata = &( ctx->cascata );
   InterfaceDados  *dados   = &( ctx->dados );

   // 1. Agora SIM! TRUE significa que mudou, FALSE significa que é igual.
   gboolean ano_mudou = ( strcmp( novo_ano, dados->ano ) != 0 );

   // 2. Barreira de Identidade: Se o ano NÃO mudou E NÃO for um disparo forçado (inicialização), aborta!
   if ( !ano_mudou && !forcar_atualizacao ) {
      return FALSE;
   }

   // 3. Se passou da barreira e o ano mudou de fato, atualiza o dado estrutural
   if ( ano_mudou ) {
      snprintf( dados->ano, sizeof( dados->ano ), "%s", novo_ano );
   }

   // 4. Montagem de diretórios e carga lógica (Ocorre na inicialização ou se o ano mudou de verdade)
   char diretorio[128];
   snprintf( diretorio, sizeof( diretorio ), "./dados/informados/%s", dados->ano );

   // Gerenciamento seguro da RAM
   if ( listas->escolas ) {
      free( listas->escolas );
      listas->escolas = NULL; // Higiene contra ponteiros órfãos
   }

   cascata->limite.escolas = quantidade_diretorios( diretorio );
   if ( cascata->limite.escolas > 0 ) {
      listas->escolas = carregar_diretorios_temas( cascata->limite.escolas, diretorio, NULL );
   }

   return TRUE; // Retorna TRUE indicando que uma carga real (ou semente) foi processada
}





gboolean atualizar_escola_interface( AppContext *ctx, const char *nova_escola, gboolean forcar_atualizacao ) {
   if ( !ctx || !nova_escola ) return FALSE;

   InterfaceListas    *listas    = &ctx->listas;
   CascataControle    *cascata   = &ctx->cascata;
   InterfaceDados     *dados     = &ctx->dados;
   InterfaceCabecalho *cabecalho = &ctx->cabecalho;

   gboolean escola_mudou = ( strcmp( nova_escola, dados->escola ) != 0 );

   if ( escola_mudou ) {
      snprintf( dados->escola, sizeof( dados->escola ), "%s", nova_escola );
   } else if ( !forcar_atualizacao ) {
      return FALSE;
   }

   char diretorio[256];
   snprintf( diretorio, sizeof( diretorio ), "./dados/informados/%s/%s", dados->ano, dados->escola );

   cascata->limite.turmas = quantidade_diretorios( diretorio );

   if ( listas->turmas ) {
      free( listas->turmas );
      listas->turmas = NULL;
   }

   if ( cascata->limite.turmas > 0 ) {
      listas->turmas = carregar_diretorios_temas( cascata->limite.turmas, diretorio, ordenar_turmas_novo_em );
   }

   gestor_da_escola( diretorio, cabecalho->gestor, dados->gestor );

   return TRUE;
}





gboolean atualizar_turma_interface( AppContext *ctx, const char *nova_turma, gboolean forcar_atualizacao ) {
   if ( !ctx || !nova_turma ) return FALSE;

   InterfaceListas *listas  = &( ctx->listas );
   CascataControle *cascata = &( ctx->cascata );
   InterfaceDados  *dados   = &( ctx->dados );

   // Verifica se a turma mudou de fato na memória
   gboolean turma_mudou = ( strcmp( nova_turma, dados->turma ) != 0 );

   if ( turma_mudou ) {
      snprintf( dados->turma, sizeof( dados->turma ), "%s", nova_turma );
   } else if ( !forcar_atualizacao ) {
      // Se a turma for idêntica e não for um disparo em cadeia/forçado, aborta aqui!
      return FALSE;
   }

   // Montagem do caminho físico: ./dados/informados/ANO/ESCOLA/TURMA
   char diretorio[256];
   snprintf( diretorio, sizeof( diretorio ), "./dados/informados/%s/%s/%s",
             dados->ano, dados->escola, dados->turma );

   // Varre o diretório para contar e carregar as disciplinas desta turma
   cascata->limite.disciplinas = quantidade_diretorios( diretorio );

   if ( listas->disciplinas ) {
      free( listas->disciplinas );
      listas->disciplinas = NULL; // Higiene mental contra ponteiros órfãos
   }

   if ( cascata->limite.disciplinas > 0 ) {
      listas->disciplinas = carregar_diretorios_temas( cascata->limite.disciplinas, diretorio, NULL );
   }

   return TRUE; // Retorna TRUE indicando que a carga lógica foi realizada com sucesso
}








void atualizar_disciplina_interface( AppContext *ctx, const char *nova_disciplina, gboolean forcar_atualizacao ) {
   if ( !ctx || !nova_disciplina ) return;

   InterfaceDados     *dados     = &ctx->dados;
   CaminhoDiretorio   *caminho   = &ctx->caminho;
   InterfaceCabecalho *cabecalho = &ctx->cabecalho;

   // Agora SIM! TRUE significa que mudou, FALSE significa que é igual.
   gboolean disciplina_mudou = ( strcmp( nova_disciplina, dados->disciplina ) != 0 );

   // 1. Atualiza o dado estrutural se houver mudança real
   if ( disciplina_mudou ) {
      snprintf( dados->disciplina, sizeof( dados->disciplina ), "%s", nova_disciplina );
   }

   // 2. Lógica de atualização dos caminhos e alunos da pauta
   if ( disciplina_mudou || forcar_atualizacao ) {
      caminhos_uteis_de_diretorios( dados, caminho );
      atualizar_dados_e_alunos_ativos( ctx );

   } else if ( !forcar_atualizacao ) {
      return;
   }

   if ( disciplina_mudou ) {
      char diretorio[512];

      snprintf( diretorio, sizeof( diretorio ), "./dados/informados/%s/%s/%s/%s",
                dados->ano, dados->escola, dados->turma, dados->disciplina );

      professor_da_disciplina( diretorio, cabecalho->professor, dados->professor );

      _atualizar_acervo_questoes_e_temas( ctx );

   }
}



void atualizar_periodo_interface( AppContext *ctx, const char *novo_periodo ) {
   if ( !ctx || !novo_periodo ) return;

   InterfaceDados   *dados   = &( ctx->dados );
   CaminhoDiretorio *caminho = &( ctx->caminho );

   snprintf( dados->periodo, sizeof( dados->periodo ), "%s", novo_periodo );
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


