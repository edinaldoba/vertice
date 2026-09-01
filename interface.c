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


static void carregar_diario_na_interface( const char *caminho_arquivo, InterfaceRegistroDiario *ui_diario,
                                          const int foco_estilo, gboolean rolagem );
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
      char *caminho_arquivo = g_build_filename( ctx->caminho.dados, "conteudo.bin", NULL );
      carregar_diario_na_interface( caminho_arquivo, &ctx->ui_diario, dados->interface_style, FALSE );// Renderizar liststore
      g_free(caminho_arquivo);
      on_combo_data_frequencia_changed_restore(ctx); // Renderizar liststore
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



// O Núcleo da Operação: Limpo e idêntico, usando a macro g_build_filename corretamente
void excluir_registro_diario( AppContext *ctx, int indice, const char *data, TipoRegistroDiario tipo ) {
   g_autofree char *arquivo_conteudo = g_build_filename( ctx->caminho.dados, "conteudo.bin", NULL );
   remover_conteudo_por_indice( arquivo_conteudo, indice );

   if ( tipo == TIPO_REGISTRO_AULA_NORMAL || tipo == TIPO_REGISTRO_AULA_EXTRA ) {
      g_autofree char *arquivo_frequencia = g_build_filename( ctx->caminho.dados, "frequencia.bin", NULL );
      remover_frequencia_por_data( arquivo_frequencia, data );
      popular_datas( &ctx->ui_diario, arquivo_conteudo );
   }
}



static gboolean tratar_registro_em_edicao( GtkWidget *widget, AppContext *ctx, GtkListStore *liststore, int *indice_antigo ) {
   g_return_val_if_fail( ctx, FALSE );

   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;
   const CaminhoDiretorio *caminho = &ctx->caminho;
   RegistroConteudo *diario = &ctx->diario;

   GtkTreePath *path_antigo = gtk_tree_model_get_path( GTK_TREE_MODEL( liststore ), &ui_diario->iter_em_edicao );
   *indice_antigo = gtk_tree_path_get_indices( path_antigo )[0];
   gtk_tree_path_free( path_antigo );

   g_autofree gchar *data_antiga = NULL;
   int n_horarios_antigo = 0;
   int tipo_registro_antigo = 0;

   gtk_tree_model_get( GTK_TREE_MODEL( liststore ), &ui_diario->iter_em_edicao,
                        0, &data_antiga, 1, &n_horarios_antigo, 4, &tipo_registro_antigo, -1 );

   g_autofree gchar *arquivo_freq = g_build_filename( caminho->dados, "frequencia.bin", NULL );

   // =====================================================================
   // REGRA DE NEGÓCIO: Feriados e Atividades Pedagógicas não têm chamada!
   // =====================================================================
   if ( diario->tipo_registro == TIPO_REGISTRO_PEDAGOGICO || diario->tipo_registro == TIPO_REGISTRO_FERIADO ) {
      if ( data_antiga ) {
         // Só avisa se estivermos mudando de uma aula com chamada para uma sem chamada
         if ( tipo_registro_antigo == TIPO_REGISTRO_AULA_NORMAL || tipo_registro_antigo == TIPO_REGISTRO_AULA_EXTRA ) {

            GtkWidget *widget_salvar = (widget == ui_diario->descricao) ? ui_diario->descricao : ui_diario->salvar_conteudo;
            GtkWindow *janela_principal = GTK_WINDOW( gtk_widget_get_toplevel( widget_salvar ) );

            // Mensagem refinada e específica para o contexto da frequência
            g_autofree gchar *msg_alerta = meu_gerador_variadico(
               "<b>Aviso de Remoção:</b>\n\n"
               "Você está alterando uma aula que possui chamada para um dia sem registro (Feriado/Pedagógico).\n\n"
               "Isso <b>excluirá permanentemente</b> a frequência dos alunos no dia <b>%s</b>.\n"
               "Deseja continuar?", data_antiga
            );

            gboolean confirmar = mostrar_popup_confirmacao( janela_principal, "Aviso do Diário", msg_alerta );

            // Se o professor clicar em "Não", a função retorna FALSE e cancela a edição
            if ( !confirmar ) return FALSE;

            remover_frequencia_por_data( arquivo_freq, data_antiga );
         }
      }
      memset( &ctx->chamada, 0, sizeof( RegistroFrequencia ) );
   }
   // =====================================================================
   // MOTOR DE SINCRONIZAÇÃO: Aula Normal ou Extra (Transfere os dados)
   // =====================================================================
   else if ( data_antiga && g_strcmp0( ctx->chamada.data, data_antiga ) == 0 ) {

      gboolean mudou_data = ( g_strcmp0( data_antiga, diario->data ) != 0 );
      gboolean mudou_horario = ( n_horarios_antigo != diario->n_horarios );

      if ( mudou_data || mudou_horario ) {
         // OBRIGATÓRIO: Apaga a frequência da data antiga para evitar que o registro se duplique
         // e crie uma "frequência órfã" no sistema, garantindo que ela apenas "se mova".
         if ( mudou_data ) {
            remover_frequencia_por_data( arquivo_freq, data_antiga );
         }

         g_strlcpy( ctx->chamada.data, diario->data, sizeof( ctx->chamada.data ) );
         ctx->chamada.n_horarios = diario->n_horarios;
         salvar_frequencia( &ctx->chamada, arquivo_freq );
      }
   }

   return TRUE; // Sucesso, permite prosseguir com o salvamento
}


void salvar_conteudo( GtkWidget *widget, AppContext *ctx ) {
   g_return_if_fail( ctx );

   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;
   const CaminhoDiretorio *caminho = &ctx->caminho;
   RegistroConteudo *diario = &ctx->diario;
   const int foco_estilo = ctx->dados.interface_style;

   g_return_if_fail( diario && ui_diario && caminho );

   const gchar *tema = gtk_entry_get_text( GTK_ENTRY( ui_diario->tema ) );
   const gchar *descricao = gtk_entry_get_text( GTK_ENTRY( ui_diario->descricao ) );

   if ( g_strcmp0( tema, "" ) == 0 && g_strcmp0( descricao, "" ) == 0 ) return;

   g_strlcpy( diario->tema, tema, sizeof( diario->tema ) );
   g_strlcpy( diario->descricao, descricao, sizeof( diario->descricao ) );
   diario->tipo_registro = gtk_combo_box_get_active( GTK_COMBO_BOX( ui_diario->tipo_registro ) );

   GtkListStore *liststore = ui_diario->liststore_conteudo;
   GtkTreeIter iter;

   // 1. OBTÉM ÍNDICE DE EDIÇÃO E APLICA REGRAS (Agora respeita o botão Cancelar)
   int indice_antigo = -1;
   if ( ui_diario->editando ) {
      // Se o usuário clicar em "Não" no popup, a função retorna silenciosamente
      if ( !tratar_registro_em_edicao( widget, ctx, liststore, &indice_antigo ) ) {
         return;
      }
   }

   // 2. SALVA NO BINÁRIO PRIMEIRO E DESCOBRE A POSIÇÃO CORRETA DA DATA
   g_autofree char *arquivo_turma = g_build_filename( caminho->dados, "conteudo.bin", NULL );
   int novo_indice = gravar_diario_binario( arquivo_turma, diario, indice_antigo );

   popular_datas( ui_diario, arquivo_turma );

   // 3. ATUALIZA A INTERFACE VISUAL
   if ( ui_diario->editando ) {
      gtk_list_store_remove( liststore, &ui_diario->iter_em_edicao );
   }

   gtk_list_store_insert( liststore, &iter, novo_indice );

   GdkRGBA cor_texto;
   int r = cor_texto_linha_liststore( diario, foco_estilo, &cor_texto );

   gtk_list_store_set( liststore, &iter, 0, diario->data,      1, diario->n_horarios,    2, diario->tema,
                                         3, diario->descricao, 4, diario->tipo_registro, 5, (r==0) ? NULL : &cor_texto, -1 );

   // 4. ROLA A TELA PARA ONDE O REGISTRO CAIU APÓS A ORDENAÇÃO
   GtkTreePath *path_novo = gtk_tree_path_new_from_indices( novo_indice, -1 );
   gtk_tree_view_scroll_to_cell( GTK_TREE_VIEW( ui_diario->treeview_conteudo ), path_novo, NULL, FALSE, 0.0, 0.0 );
   gtk_tree_path_free( path_novo );

   ui_diario->editando = FALSE;
   gtk_tree_selection_unselect_all( gtk_tree_view_get_selection( GTK_TREE_VIEW( ui_diario->treeview_conteudo ) ) );
   gtk_entry_set_text( GTK_ENTRY( ui_diario->descricao ), "" );
}



// 1. A FUNÇÃO MODULAR (Pode ir para um arquivo .c separado, como diario_ui.c)
void carregar_registro_para_edicao( AppContext *ctx, GtkTreeIter *iter ) {
   g_return_if_fail( ctx && iter );

   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;
   RegistroConteudo *diario = &ctx->diario;
   GtkTreeModel *model = GTK_TREE_MODEL( ui_diario->liststore_conteudo );

   g_autofree gchar *data = NULL;
   guint ch = 0;
   g_autofree gchar *tema = NULL;
   g_autofree gchar *descricao = NULL;
   guint tipo = 0;

   // Puxa os dados da linha selecionada
   gtk_tree_model_get( model, iter, 0, &data, 1, &ch, 2, &tema, 3, &descricao, 4, &tipo, -1 );

   // Zera a chamada na memória para garantir que não haja lixo de edições anteriores
   memset( &ctx->chamada, 0, sizeof( RegistroFrequencia ) );

   if ( data ) {
      gtk_entry_set_text( GTK_ENTRY( ui_diario->entry_data ), data );
      g_strlcpy( diario->data, data, sizeof( diario->data ) );

      // =====================================================================
      // INTEGRAÇÃO GLIB: Busca se existe frequência para essa data
      // =====================================================================
      g_autofree gchar *arquivo_freq = g_build_filename( ctx->caminho.dados, "frequencia.bin", NULL );
      g_autofree gchar *conteudo = NULL;
      gsize tamanho = 0;

      if ( g_file_get_contents( arquivo_freq, &conteudo, &tamanho, NULL ) ) {
         int total = tamanho / sizeof( RegistroFrequencia );
         RegistroFrequencia *buffer = ( RegistroFrequencia * )conteudo;

         for ( int i = 0; i < total; i++ ) {
            if ( g_strcmp0( buffer[i].data, data ) == 0 ) {
               // Achou! Carrega na memória RAM do app
               ctx->chamada = buffer[i];
               break;
            }
         }
      }
   }

   if ( ch != 0 ) {
      diario->n_horarios = ( int )ch;
      g_autofree char *str_ch = g_strdup_printf( "%d h", diario->n_horarios );
      gtk_label_set_text( GTK_LABEL( ui_diario->n_horarios ), str_ch );
   }

   gtk_combo_box_set_active( GTK_COMBO_BOX( ui_diario->tipo_registro ), ( gint )tipo );
   gtk_entry_set_text( GTK_ENTRY( ui_diario->tema ), tema ? tema : "" );
   gtk_entry_set_text( GTK_ENTRY( ui_diario->descricao ), descricao ? descricao : "" );

   ui_diario->iter_em_edicao = *iter;
   ui_diario->editando = TRUE;
}


static void carregar_diario_na_interface( const char *caminho_arquivo, InterfaceRegistroDiario *ui_diario,
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

   int total_registros = tamanho / sizeof( RegistroConteudo );
   RegistroConteudo *registros = ( RegistroConteudo * )conteudo;
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
                          1, registros[i].n_horarios,
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





void popular_datas( InterfaceRegistroDiario *ui_diario, const char *arquivo_turma ) {
   g_return_if_fail( ui_diario && arquivo_turma );

   int qtd_itens = 0;
   RegistroConteudo *registros = carregar_datas_dos_registros_de_aula( arquivo_turma, &qtd_itens );

   if ( registros && qtd_itens > 0 ) {
      popular_combo_box_generico( ui_diario->combo_data, registros, qtd_itens, qtd_itens - 1,
                                  ui_diario->handler_combo_data, mapear_datas_frequencia );

   } else {
      // Limpa visualmente se a turma estiver vazia
      popular_combo_box_generico( ui_diario->combo_data, NULL, 0, 0,
                                  ui_diario->handler_combo_data, mapear_datas_frequencia );
      gtk_label_set_text( GTK_LABEL( ui_diario->label_ch ), "0 h" );

      // Limpa a visualização anterior
      GtkTreeView *tree_view = GTK_TREE_VIEW( ui_diario->treeview_frequencia );
      GtkListStore *store_view = GTK_LIST_STORE( gtk_tree_view_get_model( tree_view ) );
      gtk_list_store_clear( store_view );
   }
}


void salvar_frequencia( const RegistroFrequencia *nova_chamada, const gchar *path_save ) {
   g_return_if_fail( nova_chamada );
   g_return_if_fail( path_save != NULL );

   g_autofree gchar *conteudo = NULL;
   gsize tamanho = 0;
   GError *erro = NULL;

   // Cria o array gerenciado automaticamente (não precisa de g_array_free no final)
   g_autoptr( GArray ) registros = g_array_new( FALSE, FALSE, sizeof( RegistroFrequencia ) );

   // 1. CARREGA TUDO DO DISCO (Leitura atômica, caso o arquivo já exista)
   if ( g_file_test( path_save, G_FILE_TEST_EXISTS ) ) {
      if ( g_file_get_contents( path_save, &conteudo, &tamanho, &erro ) ) {
         int total_registros = tamanho / sizeof( RegistroFrequencia );
         if ( total_registros > 0 ) {
            // Injeta todo o bloco de memória lido no GArray de uma só vez (muito mais rápido que o fread em loop)
            g_array_append_vals( registros, conteudo, total_registros );
         }
      } else {
         g_warning( "Aviso: Nao foi possivel ler %s: %s", path_save, erro->message );
         g_clear_error( &erro );
      }
   }

   // 2. BUSCA A DATA PARA ATUALIZAR (Edição) OU ADICIONAR (Novo)
   gboolean encontrou_data = FALSE;
   for ( guint i = 0; i < registros->len; i++ ) {
      RegistroFrequencia *reg = &g_array_index( registros, RegistroFrequencia, i );

      // Verifica se a data que estamos tentando salvar já existe no arquivo
      if ( g_strcmp0( reg->data, nova_chamada->data ) == 0 ) {
         // Sobrescreve o registro da chamada INTEIRO de uma única vez em O(1)
         *reg = *nova_chamada;
         encontrou_data = TRUE;
         break;
      }
   }

   if ( !encontrou_data ) {
      // Se a data não existia, anexa a estrutura completa recém-criada
      g_array_append_val( registros, *nova_chamada );
   }

   // 3. MÁGICA GLIB: Ordena cronologicamente todo o array
   g_array_sort( registros, comparar_datas );

   // 4. SOBRESCREVE ORDENADO NO DISCO (Escrita atômica segura em arquivo temporário + rename)
   gsize bytes_para_gravar = registros->len * sizeof( RegistroFrequencia );

   if ( !g_file_set_contents( path_save, ( const gchar * )registros->data, bytes_para_gravar, &erro ) ) {
      g_warning( "Erro: Nao foi possivel salvar %s: %s", path_save, erro->message );
      g_clear_error( &erro );
   }
}


void registrar_status_assiduidade_frequencia( InterfacePainel *painel, AppContext *ctx, StatusAssiduidade status ) {
   g_return_if_fail( ctx && painel );

   // 1. Validação de segurança
   if ( status == SEM_STATUS ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Aviso:" );
      painel->format_subtitulo = meu_gerador_variadico( "Status de assiduidade inválido" );
      painel->format_instrucao = meu_gerador_variadico( "Selecione uma justificativa válida no menu antes de registrar a frequência." );
      criar_mensagem_painel( AVISO, painel );
      return;
   }

   // 2. Extrai índices e dados
   int idx_aluno = gtk_combo_box_get_active( GTK_COMBO_BOX( ctx->entry.alunos ) );
   if ( idx_aluno < 0 ) return;

   GtkTreeIter iter_data;
   g_autofree gchar *str_data = NULL;
   if ( gtk_combo_box_get_active_iter( GTK_COMBO_BOX( ctx->ui_diario.combo_data ), &iter_data ) ) {
      GtkTreeModel *model_data = gtk_combo_box_get_model( GTK_COMBO_BOX( ctx->ui_diario.combo_data ) );
      gtk_tree_model_get( model_data, &iter_data, 0, &str_data, -1 );
   }
   if ( !str_data ) return;

   // 3. Atualiza a RAM
   g_strlcpy( ctx->chamada.data, str_data, sizeof( ctx->chamada.data ) );
   ctx->chamada.freq[idx_aluno].cod_aluno = ctx->ficha[idx_aluno].cod_aluno;
   ctx->chamada.freq[idx_aluno].status = status;

   GtkTreeView *tree_view = GTK_TREE_VIEW( ctx->ui_diario.treeview_frequencia );
   GtkListStore *store_view = GTK_LIST_STORE( gtk_tree_view_get_model( tree_view ) );
   GtkTreeModel *model_view = GTK_TREE_MODEL( store_view );
   const char *str_status = ctx->listas.status_assiduidade[status].str;

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

   GdkRGBA cor_texto;
   int r = cor_texto_linha_frequencia( status, ctx->dados.interface_style, &cor_texto );

   // 5. ATUALIZA A INTERFACE VISUAL E PROCESSA INATIVOS
   if ( modo_edicao ) {
      gtk_list_store_set( store_view, &iter_view, 3, str_status, 5, (r==0) ? NULL : &cor_texto, -1 );
      idx_aluno++;
      while ( idx_aluno < ctx->dados.qtd_alunos_total && !ctx->ficha[idx_aluno].ativo ) {
         idx_aluno++;
      }
   } else {
      g_autofree gchar *nasc = formatar_data_extenso( ctx->ficha[idx_aluno].nasc );
      gboolean riscar = !ctx->ficha[idx_aluno].ativo; // TRUE se inativo (Sem Status)

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
         ctx->chamada.freq[idx_aluno].cod_aluno = ctx->ficha[idx_aluno].cod_aluno;
         ctx->chamada.freq[idx_aluno].status = SEM_STATUS;

         nasc = formatar_data_extenso( ctx->ficha[idx_aluno].nasc );
         riscar = !ctx->ficha[idx_aluno].ativo; // TRUE se inativo (Sem Status)

         GtkTreeIter iter_inativo;
         gtk_list_store_append( store_view, &iter_inativo );
         gtk_list_store_set( store_view, &iter_inativo,
                             0, idx_aluno + 1,
                             1, ctx->ficha[idx_aluno].aluno,
                             2, nasc,
                             3, ctx->listas.status_assiduidade[0].str,
                             4, riscar,
                             5, (r==0) ? NULL : &cor_texto, -1 );
         idx_aluno++;
      }

      linha_alvo = gtk_tree_model_iter_n_children( model_view, NULL ) - 1;
   }

   // 6. ATUALIZA O LIMITE E AVANÇA O COMBO
   // O limite exato é sempre a quantidade de itens já renderizados
   ctx->ui_diario.limite_combo_alunos = gtk_tree_model_iter_n_children( model_view, NULL );

   if ( idx_aluno < ctx->dados.qtd_alunos_total ) {
      // Como o limite foi expandido na linha acima, essa mudança passará pela validação do changed sem bloqueios
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->entry.alunos ), idx_aluno );
   }

   // 7. Rola a tela (g_autoptr faz o gtk_tree_path_free nos bastidores)
   if ( linha_alvo >= 0 ) {
      g_autoptr( GtkTreePath ) path_novo = gtk_tree_path_new_from_indices( linha_alvo, -1 );
      if ( path_novo ) {
         gtk_tree_view_scroll_to_cell( tree_view, path_novo, NULL, FALSE, 0.0, 0.0 );
      }
   }
}




void on_combo_data_frequencia_changed_restore( AppContext *ctx ) {
   g_return_if_fail( ctx );

   const gchar *path_save = ctx->path_save;
   RegistroFrequencia *chamada = &ctx->chamada;
   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;

   // 1. Obtém a data e horários selecionados na interface
   GtkTreeIter iter_data;
   g_autofree gchar *str_data = NULL;
   int n_hor = 0;
   GtkComboBox *combo_data = GTK_COMBO_BOX( ui_diario->combo_data );

   if ( gtk_combo_box_get_active_iter( combo_data, &iter_data ) ) {
      GtkTreeModel *model = gtk_combo_box_get_model( combo_data );
      gtk_tree_model_get( model, &iter_data, 0, &str_data, 1, &n_hor, -1 );
   }

   if ( !str_data ) return;

   // 2. PREPARAÇÃO IMEDIATA DA MEMÓRIA RAM (O Espelho)
   // Zera completamente o buffer antes de mais nada para não herdar lixo de outras datas
   memset( chamada, 0, sizeof( RegistroFrequencia ) );
   snprintf( chamada->data, sizeof( chamada->data ), "%s", str_data );
   chamada->n_horarios = n_hor;

   // 3. Limpa a visualização anterior
   GtkTreeView *tree_view = GTK_TREE_VIEW( ui_diario->treeview_frequencia );
   GtkListStore *store_view = GTK_LIST_STORE( gtk_tree_view_get_model( tree_view ) );
   gtk_list_store_clear( store_view );

   // 4. Busca histórico no disco sem retornar precocemente em caso de arquivo vazio
   gsize length = 0;
   g_autofree RegistroFrequencia *buffer = NULL;
   int idx_encontrado = -1;

   if ( g_file_get_contents( path_save, (gchar **)&buffer, &length, NULL ) ) {
      size_t qtd_reg = length / sizeof( RegistroFrequencia );
      if ( qtd_reg > 0 ) {
         RegistroFrequencia chave = {0};
         g_strlcpy( chave.data, str_data, sizeof( chave.data ) );
         idx_encontrado = buscar_indice_bsearch( &chave, buffer, qtd_reg, sizeof(RegistroFrequencia), comparar_datas );
      }
   }

   // 5. CÓPIA ATÔMICA EM O(1)
   // Se o registro existe, joga a struct INTEIRA de uma vez para a RAM
   if ( idx_encontrado >= 0 ) {
      *chamada = buffer[idx_encontrado];
   }

   // 6. Renderização Visual baseada 100% no *chamada
   int proximo_aluno_pendente = 0;
   int linhas_renderizadas = 0;

   for ( int i = 0; i < ctx->dados.qtd_alunos_total; i++ ) {
      int idx_st = chamada->freq[i].status;

      // PARADA VISUAL: Encerra a renderização caso ache um aluno ATIVO e com SEM_STATUS
      if ( ctx->ficha[i].ativo && idx_st == SEM_STATUS ) {
         proximo_aluno_pendente = i;
         break;
      }

      GtkTreeIter iter_view;
      gtk_list_store_append( store_view, &iter_view );

      const char *str_status = ctx->listas.status_assiduidade[idx_st].str;

      GdkRGBA cor_texto;
      int r = cor_texto_linha_frequencia( idx_st, ctx->dados.interface_style, &cor_texto );

      g_autofree gchar *nasc = formatar_data_extenso( ctx->ficha[i].nasc );
      gboolean riscar = !ctx->ficha[i].ativo; // TRUE se inativo (Sem Status)

      gtk_list_store_set( store_view, &iter_view,
                          0, i + 1,
                          1, ctx->ficha[i].aluno,
                          2, nasc,
                          3, str_status,
                          4, riscar,
                          5, (r==0) ? NULL : &cor_texto,
                          -1 );

      proximo_aluno_pendente = i + 1;
      linhas_renderizadas++;
   }

   // 7. Correção de Mira para Datas Novas
   // Se a data está em branco, precisamos garantir que o combo aponte para o 1º aluno ATIVO
   if ( idx_encontrado < 0 && linhas_renderizadas == 0 ) {
      for ( int i = 0; i < ctx->dados.qtd_alunos_total; i++ ) {
         if ( ctx->ficha[i].ativo ) {
            proximo_aluno_pendente = i;
            break;
         }
      }
   } else if ( proximo_aluno_pendente >= ctx->dados.qtd_alunos_total ) {
      proximo_aluno_pendente = ctx->dados.qtd_alunos_total - 1;
   }

   // 8. Ajustes Finais UI
   ui_diario->limite_combo_alunos = linhas_renderizadas;

   if ( proximo_aluno_pendente < ctx->dados.qtd_alunos_total ) {
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->entry.alunos ), proximo_aluno_pendente );
   }

   if ( linhas_renderizadas > 0 ) {
      GtkTreePath *path_novo = gtk_tree_path_new_from_indices( linhas_renderizadas - 1, -1 );
      if ( path_novo ) {
         gtk_tree_view_scroll_to_cell( tree_view, path_novo, NULL, FALSE, 0.0, 0.0 );
         gtk_tree_path_free( path_novo );
      }
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










static void atualizar_acervo_questoes_e_temas( AppContext *ctx ) {
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




//==================================================================================================
static void carregar_frequencia( AppContext *ctx ) {
   g_return_if_fail( ctx );

   CaminhoDiretorio        *caminho   = &ctx->caminho;
   RegistroFrequencia      *chamada   = &ctx->chamada;
   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;

   GtkTreeIter iter;

   GtkComboBox *combo = GTK_COMBO_BOX( ui_diario->combo_data );

   if ( gtk_combo_box_get_active_iter( combo, &iter ) ) {
      GtkTreeModel *model = gtk_combo_box_get_model( combo );
      guint carga_horaria = 0;

      // Lê o valor da coluna 1 (Configurada como guint no Glade)
      gtk_tree_model_get( model, &iter, 1, &carga_horaria, -1 );

      // Formata a string conforme o design e insere no Label
      g_autofree gchar *str_horas = meu_gerador_variadico( "<b>%u h</b>", carga_horaria );
      gtk_label_set_markup( GTK_LABEL( ui_diario->label_ch ), str_horas );

      if ( ctx->path_save != NULL ) {
         // Não entra aqui uma única vez, na cascata de inicialização do app
         salvar_frequencia( chamada, ctx->path_save );
      }
      g_free( ctx->path_save );
      ctx->path_save = g_build_filename( caminho->dados, "frequencia.bin", NULL ); // Comentário em interface.h

      on_combo_data_frequencia_changed_restore( ctx );
   }
}
//--------------------------------------------------------------------------------------------------
static void atualizar_dados_e_alunos_ativos( AppContext *ctx ) {
   if ( !ctx ) return;

   InterfaceDados    *dados    = &ctx->dados;
   CaminhoDiretorio  *caminho  = &ctx->caminho;
   InterfaceEntry    *entry    = &ctx->entry;
   LimitesFiltro     *limite   = &ctx->cascata.limite;
   InterfacePainel   *painel   = &ctx->painel;
   InterfaceHandlers *handlers = &ctx->handlers;

   InterfaceRegistroDiario *ui_diario = &ctx->ui_diario;

   g_autofree char *caminho_arquivo = g_build_filename( caminho->dados, "conteudo.bin", NULL );
   carregar_diario_na_interface( caminho_arquivo, ui_diario, dados->interface_style, TRUE );
   ui_diario->editando = FALSE; // GG, acabei retornando para bancada por esse pequeno detalhe, ele me permite carregar um registro de uma turma e salvar em outra (muito útil no dia a dia). Essa função é executada quando a turma ou o período muda. A ausência dessa linha estava causando falha de segmentação quando eu tentava executar o referido procedimento. Agora vou dormir de verdade, rsrs. Boa noite.

   acessar_e_carregar_ficha_dos_alunos_da_turma( ctx );

   limite->alunos = ( dados->qtd_alunos_total < 0 ) ? 0 : dados->qtd_alunos_total;

   int foco = obter_foco_inicial( limite->alunos, ctx->ficha );

   popular_combo_box_generico( entry->alunos, ctx->ficha, limite->alunos, foco, handlers->alunos, mapear_alunos );

    // Precisa que a lista de alunos já esteja populada
   popular_datas( ui_diario, caminho_arquivo );
   carregar_frequencia( ctx );


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
   snprintf( ctx->diario.data, sizeof( ctx->diario.data ), "%s", data_formatada );

   ctx->diario.n_horarios = 1;
   g_autofree char *str_n_horarios = g_strdup_printf( "%d h", ctx->diario.n_horarios );
   gtk_label_set_text( GTK_LABEL( ctx->ui_diario.n_horarios ), str_n_horarios );

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

      atualizar_acervo_questoes_e_temas( ctx );

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


