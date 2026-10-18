/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include "acervo.h"
#include "comum.h"
#include "basicas.h"
#include "glib_gio.h"
#include "mensagens.h"
#include "interface.h"
#include "assincrono.h"
#include "latex.h"
#include "./acervo/estatistica.h"

#include <stdbool.h> // Caso ainda não tenha no seu header
#include <stdio.h>




static bool executar_gerador_c_nativamente( const char *pasta_tema ) {
   g_return_val_if_fail( pasta_tema != NULL, false );

   if ( quantidade_arquivos_por_extensao( pasta_tema, ".c" ) == 1 ) {

      if ( quantidade_arquivos_por_extensao( pasta_tema, ".h" ) == 1 ) {

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

      } else {
         ac_estatistica();
      }
   }

   return true; // Não encontrou o arquivo .c neste subtema
}



static void motor_gerador_latex( const gchar *pasta_tema, const gchar *subtema, const InterfaceDados *dados ) {
   g_return_if_fail( pasta_tema != NULL );
   g_return_if_fail( subtema != NULL );
   g_return_if_fail( dados != NULL );

   g_autofree gchar *tema_pdf = g_strdup_printf( "%s.tex", pasta_tema );

   // Pré-aloca 32KB na RAM para máxima performance (excelente prática!)
   g_autoptr( GString ) tex = g_string_sized_new( 32768 );

   // =========================================================================
   // 1. CABEÇALHO DO DOCUMENTO LATEX
   // =========================================================================
   g_string_append( tex,
                    "\\documentclass[11pt,a4paper]{report}\n"
                    "\\usepackage[utf8]{inputenc}\n"
                    "\\usepackage[T1]{fontenc}\n" );

   if ( dados->fonte_latex == 1 ) {
      g_string_append( tex, "\\usepackage{cmbright}\n" );
   }

   g_string_append( tex,
                    "\\usepackage[brazil]{babel}\n"
                    "\\usepackage[bottom=1cm,top=1cm,left=1cm,right=1cm]{geometry}\n"
                    "\\usepackage[dvipsnames,table]{xcolor}\n"
                    "\\usepackage{multicol}\n"
                    "\\usepackage{enumerate}\n"
                    "\\usepackage[nointegrals]{wasysym}\n"
                    "\\usepackage{bm}\n"
                    "\\usepackage{array,multirow,graphicx}\n"
                    "\\usepackage{amsmath,amssymb}\n"
                    "\\usepackage{ifthen}\n"
                    "\\usepackage{setspace}\n"
                    "\\usepackage{ulem}\n"
                    "\\onehalfspacing\n"
                    "\\pagestyle{empty}\n\n" );

   // O pacote de cores do professor recebe o parâmetro (ex: "sexto") e converte para "CorSerie"
   g_string_append_printf( tex, "\\usepackage[%s]{professor}\n\n", dados->cor_destaque );

   g_string_append( tex,
                    "\\newcommand*{\\vtext}[2]{\\parbox[t]{9pt}{\\multirow{#1}{*}{\\rotatebox[origin=c]{90}{#2}}}}\n\n"
                    "\\newcommand{\\sen}{\\mathrm{sen}\\hspace{2pt}}\n"
                    "\\newcommand{\\cossec}{\\mathrm{cossec}\\hspace{2pt}}\n"
                    "\\newcommand{\\tg}{\\mathrm{tg}\\hspace{2pt}}\n"
                    "\\newcommand{\\cotg}{\\mathrm{cotg}\\hspace{2pt}}\n\n"
                    "\\newcolumntype{L}[1]{>{\\raggedright\\arraybackslash}p{#1}}\n"
                    "\\newcolumntype{C}[1]{>{\\centering\\arraybackslash}p{#1}}\n"
                    "\\newcolumntype{R}[1]{>{\\raggedleft\\arraybackslash}p{#1}}\n\n"
                    "\\begin{document}\n"
                    "\\noindent\\tikz{\n" );

   // =========================================================================
   // INJEÇÃO NATIVA DO TEMA DE PÁGINA (Com g_strcmp0 NULL-safe)
   // =========================================================================
   gint k_tema = 1;

   // O uso nativo de "CorSerie" fará o elo direto com o professor.sty
   if ( g_strcmp0( dados->decoracao_estilo, "Trapezios" ) == 0 ) {
      anexar_tema_pagina_trapezios( tex, k_tema );
   } else if ( g_strcmp0( dados->decoracao_estilo, "Ondas" ) == 0 ) {
      anexar_tema_pagina_ondas( tex, k_tema );
   } else if ( g_strcmp0( dados->decoracao_estilo, "Quadrados" ) == 0 ) {
      anexar_tema_pagina_quadrados( tex, k_tema );
   } else if ( g_strcmp0( dados->decoracao_estilo, "Linhas" ) == 0 ) {
      anexar_tema_pagina_linhas( tex, k_tema );
   } else if ( g_strcmp0( dados->decoracao_estilo, "Senoides" ) == 0 ) {
      anexar_tema_pagina_senoides( tex, k_tema );
   } else if ( g_strcmp0( dados->decoracao_estilo, "Circulos" ) == 0 ) {
      anexar_tema_pagina_circulos( tex, k_tema );
   } else if ( g_strcmp0( dados->decoracao_estilo, "Hexagonos" ) == 0 ) {
      anexar_tema_pagina_hexagonos( tex, k_tema );
   }

   // Título da Página injetado logo abaixo do tema vetorial
   g_string_append_printf( tex, "\\node[inner sep=0pt, right] at (0,-0.65) {\\LARGE\\bf %s: %s};\n}\n\n", dados->tema, subtema );

   g_string_append_printf( tex, "\\setlength{\\columnsep}{%dmm}\n", 12 - 2 * dados->qtd_colunas );
   g_string_append( tex, "\\setlength{\\columnseprule}{0.6pt}\n\n" );

   // A MÁGICA ACONTECE AQUI: Define a cor da linha divisória usando o xcolor e CorSerie
   g_string_append( tex, "\\def\\columnseprulecolor{\\color{CorSerie!40}}\n\n" );

   g_string_append_printf( tex, "\\begin{multicols}{%d}\n\n", dados->qtd_colunas );
   g_string_append( tex, "\\begin{enumerate}[\\hspace{-1.8mm}]\n\n" );

   // =========================================================================
   // 2. GERAÇÃO C E INJEÇÃO DAS QUESTÕES (Livre de limite de buffer)
   // =========================================================================
   executar_gerador_c_nativamente( pasta_tema );

   int narq = quantidade_arquivos_por_extensao( pasta_tema, ".tex" );
   gint k_questao = 3 - dados->qtd_colunas;

   for ( int q = 0; q < narq; q++ ) {
      g_autofree gchar *questao = g_strdup_printf( "%s/Q%d.tex", pasta_tema, q + 1 );

      // g_autofree evita memory leak atômico no loop
      g_autofree gchar *conteudo_arquivo = NULL;
      if ( !g_file_get_contents( questao, &conteudo_arquivo, NULL, NULL ) ) continue;

      // Fatiamento nativo em matriz de strings baseada nas quebras de linha (\n)
      g_auto( GStrv ) linhas = g_strsplit( conteudo_arquivo, "\n", -1 );

      // Usar while e controle manual de índice impede Segmentation Fault!
      int i = 0;
      while ( linhas[i] != NULL ) {
         gchar *linha = linhas[i];

         if ( g_str_has_prefix( linha, "% QUESTAO" ) ) {
            g_string_append_printf( tex, "%s\n", linha );

            // Injeção do Cabeçalho
            if ( g_strcmp0( dados->decoracao_estilo, "Trapezios" ) == 0 ) {
               anexar_cabecalho_questao_trapezios( tex, k_questao, q + 1 );
            } else if ( g_strcmp0( dados->decoracao_estilo, "Ondas" ) == 0 ) {
               anexar_cabecalho_questao_ondas( tex, k_questao, q + 1 );
            } else if ( g_strcmp0( dados->decoracao_estilo, "Quadrados" ) == 0 ) {
               anexar_cabecalho_questao_quadrados( tex, "black", k_questao, q + 1 );
            } else if ( g_strcmp0( dados->decoracao_estilo, "Linhas" ) == 0 ) {
               anexar_cabecalho_questao_linhas( tex, "black", k_questao, q + 1 );
            } else if ( g_strcmp0( dados->decoracao_estilo, "Senoides" ) == 0 ) {
               anexar_cabecalho_questao_senoides( tex, "black", k_questao, q + 1 );
            } else if ( g_strcmp0( dados->decoracao_estilo, "Circulos" ) == 0 ) {
               anexar_cabecalho_questao_circulos( tex, k_questao, q + 1 );
            } else if ( g_strcmp0( dados->decoracao_estilo, "Hexagonos" ) == 0 ) {
               anexar_cabecalho_questao_hexagonos( tex, k_questao, q + 1 );
            }

            // Coleta o bloco da questão até a próxima linha em branco
            i++;
            while ( linhas[i] != NULL && linhas[i][0] != '\0' ) {
               g_string_append_printf( tex, "%s\n", linhas[i] );
               i++;
            }

         } else if ( g_str_has_prefix( linha, "% ALTERNATIVAS" ) ) {
            g_string_append_printf( tex, "%s\n", linha );
            g_string_append( tex, "\\vspace{-2mm}\n\\begin{enumerate}[\\quad]\n" );

            i++; // Move o cursor para a primeira alternativa
            for ( int letra = 65; letra < 70; letra++ ) {
               if ( linhas[i] == NULL ) break;

               gchar letra_str[2] = { ( gchar )letra, '\0' };

               // SOLUÇÃO: Abraçando o argumento do \item com { }
               g_string_append( tex, "\\item[{" );
               anexar_circulo_alternativa_colorida( tex, 15, letra_str );
               g_string_append_printf( tex, "}] %s\n", linhas[i] );
               i++;
            }
            g_string_append( tex, "\\end{enumerate}\n\n" );

         } else if ( linha[0] != '\0' ) {
            // Qualquer outro bloco extra de texto detectado no parser
            g_string_append( tex, "\n" );
            while ( linhas[i] != NULL && linhas[i][0] != '\0' ) {
               g_string_append_printf( tex, "%s\n", linhas[i] );
               i++;
            }
         } else {
            // Pula linhas vazias seguras que não pertencem a blocos ativos
            i++;
         }
      }
   }

   // =========================================================================
   // 3. RODAPÉ E GRAVAÇÃO ATÔMICA DO ARQUIVO .TEX
   // =========================================================================
   g_string_append( tex,
                    "\\end{enumerate}\n\n"
                    "\\end{multicols}\n\n"
                    "\\end{document}\n" );

   GError *error = NULL;
   if ( !g_file_set_contents( tema_pdf, tex->str, tex->len, &error ) ) {
      g_printerr( "[ERRO] Não foi possível criar o arquivo de destino em: %s (%s)\n", pasta_tema, error->message );
      g_error_free( error );
   }
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

   if ( widget == ctx->button.compilar_latex_acervo ) {
      painel->format_titulo    = meu_gerador_variadico( "⏳ Compilação em Andamento" );
      painel->format_subtitulo = meu_gerador_variadico( "O documento PDF deste acervo ainda não foi gerado." );
      painel->format_instrucao = meu_gerador_variadico( "Processando as questões de %s. Aguarde a conclusão para visualizar o material.", dados->tema );
      criar_mensagem_painel( INFO, painel );
   }

   // 2. Dispara o processamento paralelo e assíncrono para gerar os PDFs!
   g_autofree char *pasta_raiz_tema = g_build_filename( caminho->banco_questoes, dados->tema, NULL );
   g_pdflatex_parallel_async( widget, pasta_raiz_tema, painel, ctx );

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



