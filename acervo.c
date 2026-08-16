/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include "acervo.h"
#include "comum.h"
#include "basicas.h"
#include "glibrary.h"
#include "mensagens.h"
#include "interface.h"
#include "assincrono.h"

#include <stdbool.h> // Caso ainda não tenha no seu header
#include <stdio.h>




static bool executar_gerador_c_nativamente( const char *pasta_tema ) {
   g_return_val_if_fail( pasta_tema != NULL, false );

   if ( quantidade_arquivos_por_extensao( pasta_tema, ".c" ) == 1 ) {

      // O 'sh -c' gerencia o '&&' e garante que a execução ocorra estritamente dentro da pasta
      char *argv[] = { ( char * )"sh", ( char * )"-c",
                       ( char * )"gcc Q.c -lm -O3 -march=native -o Q && ./Q", NULL
                     };
      GError *erro = NULL;

      if ( !g_spawn_sync( pasta_tema, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL, NULL, &erro ) ) {
         g_printerr( "[ERRO FATAL] Falha ao compilar gerador C em '%s': %s\n", pasta_tema, erro->message );
         g_clear_error( &erro );
         return false;
      }

      return true; // Compilou e executou o gerador com sucesso
   }

   return false; // Não encontrou o arquivo .c neste subtema
}



static void motor_gerador_latex( const char *pasta_tema, const char *subtema, const InterfaceDados *dados ) {
   g_return_if_fail( pasta_tema != NULL );
   g_return_if_fail( subtema != NULL );
   g_return_if_fail( dados != NULL );

   g_autofree char *tema_pdf = g_strdup_printf( "%s.tex", pasta_tema );

   FILE *pp = fopen( tema_pdf, "w+" );
   if ( !pp ) {
      g_printerr( "[ERRO] Não foi possível criar o arquivo de destino em: %s\n", pasta_tema );
      return;
   }

   // =========================================================================
   // 1. CABEÇALHO (O Template inteiro agora é ejetado diretamente da RAM)
   // =========================================================================
   // fputs é mais rápido que fprintf, pois não processa formatadores (%)
   fputs(
      "\\documentclass[11pt,a4paper]{report}\n"
      "\\usepackage[utf8]{inputenc}\n"
      "\\usepackage[T1]{fontenc}\n", pp );

   // Lógica injetada cirurgicamente
   if ( dados->fonte_latex == 1 ) {
      fputs( "\\usepackage{cmbright}\n", pp );
   }

   fputs(
      "\\usepackage[brazil]{babel}\n"
      "\\usepackage[bottom=1cm,top=1cm,left=1cm,right=1cm]{geometry}\n"
      "\\usepackage[dvipsnames,table]{xcolor}\n"
      "\\usepackage{multicol}\n"
      "\\usepackage{enumerate}\n"
      "\\usepackage[nointegrals]{wasysym}\n"
      "\\usepackage{array,multirow,graphicx}\n"
      "\\usepackage{amsmath,amssymb}\n"
      "\\usepackage{ifthen}\n"
      "\\usepackage{setspace}\n"
      "\\usepackage{ulem}\n"
      "\\onehalfspacing\n"
      "\\pagestyle{empty}\n\n", pp );

   fprintf( pp, "\\usepackage[%s]{professor}\n\n", dados->cor_destaque );

   fputs(
      "\\newcommand*{\\vtext}[2]{\\parbox[t]{9pt}{\\multirow{#1}{*}{\\rotatebox[origin=c]{90}{#2}}}}\n\n"
      "\\newcommand{\\sen}{\\mathrm{sen}\\hspace{2pt}}\n"
      "\\newcommand{\\cossec}{\\mathrm{cossec}\\hspace{2pt}}\n"
      "\\newcommand{\\tg}{\\mathrm{tg}\\hspace{2pt}}\n"
      "\\newcommand{\\cotg}{\\mathrm{cotg}\\hspace{2pt}}\n\n"
      "\\newcolumntype{L}[1]{>{\\raggedright\\arraybackslash}p{#1}}\n"
      "\\newcolumntype{C}[1]{>{\\centering\\arraybackslash}p{#1}}\n"
      "\\newcolumntype{R}[1]{>{\\raggedleft\\arraybackslash}p{#1}}\n\n"
      "\\begin{document}\n"
      "\\noindent\\tikz{\n", pp );

   // Injeção do estilo e do título
   fprintf( pp, "\\tema%sColorida{CorSerie}{1}\n", dados->decoracao_estilo );
   fprintf( pp, "\\node[inner sep=0pt, right] at (0,-0.65) {\\LARGE\\bf %s: %s};\n}\n\n", dados->tema, subtema );

   fprintf( pp, "\\setlength{\\columnsep}{%dmm}\n", 12 - 2 * dados->qtd_colunas );
   fputs( "\\setlength{\\columnseprule}{0.8pt}\n\n", pp );

   fprintf( pp, "\\begin{multicols}{%d}\n\n", dados->qtd_colunas );
   fputs( "\\begin{enumerate}[\\hspace{-1.8mm}]\n\n", pp );


   // =========================================================================
   // 2. GERAÇÃO C E INJEÇÃO DAS QUESTÕES (Lógica de montagem preservada)
   // =========================================================================
   executar_gerador_c_nativamente( pasta_tema );

   int narq = quantidade_arquivos_por_extensao( pasta_tema, ".tex" );
   char str[2048];

   for ( int q = 0; q < narq; q++ ) {
      g_autofree char *questao = g_strdup_printf( "%s/Q%d.tex", pasta_tema, q + 1 );
      FILE *pb = fopen( questao, "r" );

      if ( !pb ) continue;

      while ( fgets( str, sizeof str, pb ) != NULL ) {
         if ( strcmp( str, "% QUESTAO\n" ) == 0 ) {
            fputs( str, pp );

            if ( strcmp( dados->decoracao_estilo, "Quadrados" ) == 0 )
               fprintf( pp, "\\item{$\\questao%sColorida{CorSerie}{black}{%d}{%.2d}$}\\\\\n", dados->decoracao_estilo, 3 - dados->qtd_colunas, q + 1 );
            else if ( strcmp( dados->decoracao_estilo, "Ondas" ) == 0 )
               fprintf( pp, "\\item{$\\questao%sColorida{CorSerie}{%d}{%.2d}$}\\\\\n", dados->decoracao_estilo, 3 - dados->qtd_colunas, q + 1 );
            else
               fprintf( pp, "\\item{$\\questao%s{%d}{%d}{%.2d}$}\\\\\n", dados->decoracao_estilo, 0, 3 - dados->qtd_colunas, q + 1 );

            if ( fgets( str, sizeof str, pb ) ) {
               while ( str[0] != '\n' ) {
                  fputs( str, pp );
                  if ( !fgets( str, sizeof str, pb ) ) break;
               }
            }
         } else if ( strncmp( str, "% ALTERNATIVAS", 14 ) == 0 ) {
            fputs( str, pp );
            fputs( "\\vspace{-2mm}\n\\begin{enumerate}[\\quad]\n", pp );

            for ( int letra = 65; letra < 70; letra++ ) {
               if ( fgets( str, sizeof str, pb ) ) {
                  fprintf( pp, "\\item[$\\circledColorida{CorSerie}{20}{%c}$] %s", letra, str );
               }
            }
            fputs( "\\end{enumerate}\n\n", pp );
         } else {
            fputs( "\n", pp );
            while ( str[0] != '\n' ) {
               fputs( str, pp );
               if ( !fgets( str, sizeof str, pb ) ) break;
            }
         }
      }
      fclose( pb );
   }


   // =========================================================================
   // 3. RODAPÉ
   // =========================================================================
   fputs(
      "\\end{enumerate}\n\n"
      "\\end{multicols}\n\n"
      "\\end{document}\n", pp );

   fclose( pp );
}


void compilar_questoes( GtkWidget *widget, InterfacePainel *painel, const AppContext *ctx ) {
   g_return_if_fail( painel != NULL );
   g_return_if_fail( ctx != NULL );

   const InterfaceDados *dados = &( ctx->dados );
   const LimitesFiltro *limite = &( ctx->cascata.limite );
   const InterfaceListas *listas = &( ctx->listas );
   const CaminhoDiretorio *caminho = &( ctx->caminho );

   if ( listas->subtemas == NULL ) return;

   // 1. Delega a construção dos arquivos .tex para o Motor LaTeX
   for ( int i = 0; i < limite->subtemas; i++ ) {
      g_autofree char *pasta_tema = g_build_filename( caminho->banco_questoes,
                                    dados->tema,
                                    listas->subtemas[i].str,
                                    NULL );

      motor_gerador_latex( pasta_tema, listas->subtemas[i].str, dados );
   }


   // 2. Dispara o processamento paralelo e assíncrono para gerar os PDFs!
   g_autofree char *pasta_raiz_tema = g_build_filename( caminho->banco_questoes, dados->tema, NULL );
   g_pdflatex_parallel_async( widget, pasta_raiz_tema, painel, ctx );

   // char comando[4096];
   //
   //
   // if ( verificar_pdfs_latex_acervo_questoes( pasta_tema, listas->subtemas, limite->subtemas,
   //       painel, dados->tema ) ) {
   //
   //    snprintf( comando, sizeof( comando ), "pdfunite '%s'/*.pdf '%s/%s'.pdf",
   //              pasta_tema, caminho->banco_questoes, dados->tema );
   //
   //    if ( system( comando ) != 0 ) {
   //       fprintf( stderr, "[ERRO] Falha ao concatenar PDFs: %s\n", dados->tema );
   //    }
   //
   //    for ( i = 0; i < limite->subtemas; i ++ ) {
   //       apagar_arquivos_temporarios_latex_nativamente( pasta_tema, listas->subtemas[i].str, 5 );
   //    }
   // }

}






//========================================================================================================//
void atualizar_questoes( InterfacePainel *painel, const AppContext *ctx ) {
   g_return_if_fail( painel != NULL );
   g_return_if_fail( ctx != NULL );

   const InterfaceDados *dados = &( ctx->dados );
   const LimitesFiltro *limite = &ctx->cascata.limite;
   const InterfaceListas *listas = &( ctx->listas );
   const CaminhoDiretorio *caminho = &( ctx->caminho );

   if ( listas->subtemas == NULL ) return;

   bool mecanismo_executado = false;

   // Declaração do iterador dentro do for (padrão moderno C99+)
   for ( int i = 0; i < limite->subtemas; i++ ) {

      // Criação dinâmica e 100% segura do caminho do subtema, unindo as pastas com '/'
      g_autofree char *pasta_tema = g_build_filename( caminho->banco_questoes,
                                    dados->tema,
                                    listas->subtemas[i].str,
                                    NULL );

      // A função retorna true se o arquivo Q.c existia e foi processado
      if ( executar_gerador_c_nativamente( pasta_tema ) ) {
         mecanismo_executado = true;
      }
   }

   // =========================================================================
   // ATUALIZAÇÃO DA INTERFACE GTK
   // =========================================================================
   if ( mecanismo_executado ) {
      painel->format_titulo    = meu_gerador_variadico( "✔ Sucesso:" );
      painel->format_subtitulo = meu_gerador_variadico( "O mecanismo de diversificação foi executado." );
      painel->format_instrucao = meu_gerador_variadico( "As questões latex de %s foram atualizadas com sucesso...",
                                 dados->tema );
      criar_mensagem_painel( SUCESSO, painel );

   } else {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Atenção:" );
      painel->format_subtitulo = meu_gerador_variadico( "Nenhum arquivo \".c\" foi encontrado." );
      painel->format_instrucao = meu_gerador_variadico( "Mecanismo de diversificação ainda não implementado para %s...",
                                 dados->tema );
      criar_mensagem_painel( AVISO, painel );
   }
}
//========================================================================================================//






//========================================================================================================//
void abrir_tema( GtkWidget *widget, InterfacePainel *painel, const AppContext *ctx ) {
   // Proteção padrão de ponteiros
   g_return_if_fail( ctx != NULL );
   g_return_if_fail( painel != NULL );

   const InterfaceDados *dados = &( ctx->dados );
   const CaminhoDiretorio *caminho = &( ctx->caminho );

   // 1. Criamos apenas o nome final do arquivo isoladamente (ex: "Equacoes.pdf")
   g_autofree char *nome_arquivo = g_strdup_printf( "%s.pdf", dados->tema );

   // 2. Adeus snprintf! O g_build_filename une a pasta e o arquivo de forma
   // inteligente, garantindo que não faltem nem sobrem barras (/) no caminho.
   g_autofree char *caminho_absoluto = g_build_filename( caminho->banco_questoes, nome_arquivo, NULL );

   // 3. Verificamos existência do arquivo
   if ( !g_file_test( caminho_absoluto, G_FILE_TEST_EXISTS ) ) {

      // O arquivo não existe. Disparamos a compilação assíncrona.
      compilar_questoes( widget, painel, ctx );

      // Como o motor agora é paralelo, o arquivo só estará pronto daqui a ~10 segundos.
      // Precisamos avisar o usuário e ABORTAR a função aqui mesmo para não gerar erro no leitor de PDF.
      painel->format_titulo    = meu_gerador_variadico( "⏳ Em Processamento:" );
      painel->format_subtitulo = meu_gerador_variadico( "O PDF deste tema ainda não existe." );
      painel->format_instrucao = meu_gerador_variadico( "A compilação de '%s' foi iniciada. Aguarde a conclusão para abrir.", dados->tema );
      criar_mensagem_painel( INFO, painel ); // Assumindo que você tenha um enumerador tipo INFO ou AVISO

      return; // Sai da função imediatamente
   }

   // 4. Se chegou aqui, o arquivo já existe e está pronto. Abre normalmente!
   g_xdg_open( caminho_absoluto );
}
//========================================================================================================//



