/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <omp.h>

#include "glibrary.h"
#include "interface.h"
#include "provas.h"
#include "gabaritos.h"
#include "comum.h"
#include "basicas.h"
#include "latex.h"
#include "imgcore.h"



void definir_titulo_documento( char *titulo_prova, const InterfaceDados *dados ) {
   if ( dados->periodo[0] == 'R' ) {
      sprintf( titulo_prova, "%s de %s / %s",
               dados->periodo, dados->disciplina, dados->ano );
   } else {
      sprintf( titulo_prova, "%s Prova de %s do %s / %s",
               dados->prova_sequencia, dados->disciplina, dados->periodo, dados->ano );
   }
}





void gerar_tex_lista_frequencia( const char *caminho_saida, char *titulo_prova, const FichaAluno *diario,
                                 const InterfaceDados *dados, const CalendarioData *data ) {


   // Define o título da lista de frequência
   definir_titulo_documento( titulo_prova, dados );

   // 1. PREPARAÇÃO DOS DADOS (O "Cérebro")
   double p = 26.7 / ( dados->qtd_alunos_total + 2 );
   double ll = 0.6 * p;

   // 2. ABERTURA E ESCRITA (A "Mão")
   FILE *p1 = fopen( caminho_saida, "w" );
   if ( !p1 ) return;

   // Cabeçalho Estático (Escrito diretamente)
   fprintf( p1, "\\documentclass[11pt,a4paper]{report}\n"
            "\\usepackage[utf8]{inputenc}\n"
            "\\usepackage[T1]{fontenc}\n" );

   if ( dados->fonte_latex == 1 ) fprintf( p1, "\\usepackage{cmbright}\n" );

   fprintf( p1, "\\usepackage[brazil]{babel}\n"
            "\\usepackage[left=0cm,right=0cm,top=0cm,bottom=0cm]{geometry}\n"
            "\\usepackage{xcolor,tikz,ifthen,ulem}\n"
            "\\usetikzlibrary{calc}\n"
            "\\pagestyle{empty}\n\n"
            "\\begin{document}\n"
            "\\noindent\\begin{tikzpicture}\n\n" );

   // 3. Variáveis de Cálculo (Aqui o C faz a conta que o \pgfmathsetmacro fazia)
   char str[16];
   g_ascii_formatd( str, sizeof( str ), "%.4f", p );
   fprintf( p1, "%% Configurações dinâmicas\n"
            "\\pgfmathsetmacro{\\p}{%s}\n"
            "\\fill (0,0) circle (0pt);\n"
            "\\draw (1,-1) rectangle (20,-28.7);\n\n", str ); // Põe ponto decimal ao ínves de vírgula

   // 4. Moldura e Linhas Verticais
   fprintf( p1, "\\draw (1.6,{-2-\\p}) -- (1.6,{-28.7+\\p})\n"
            "(7,-1) -- (7,{-28.7+\\p})\n"
            "(19,-2) -- (19,{-28.7+\\p})\n"
            "(18,-2) -- (18,{-28.7+\\p})\n"
            "(7,-2) -- (20,-2);\n\n" );

   // 5. Cabeçalho do Documento (Nomes, Turma, Escola)
   fprintf( p1, "\\node[inner sep=0pt,right] at (1.2,-1.6) {\\bf\\resizebox{5.5cm}{0.44cm}{%s}};\n", dados->escola );
   fprintf( p1, "\\node[inner sep=0pt,right] at (1.2,{-1.9-0.5*\\p}) {\\large Turma: \\bf %s};\n", dados->turma );
   fprintf( p1, "\\node[inner sep=0pt,left] at (17.8,{-2-0.5*\\p}) {Data: \\underline{\\qquad}/\\underline{\\qquad}/\\underline{%d}};\n", data->ano );
   fprintf( p1, "\\node[inner sep=0pt,right] at (7.2,{-2-0.5*\\p}) {\\underline{Frequência}};\n" );
   fprintf( p1, "\\node[inner sep=0pt] at (13.5,-1.6) {\\bf\\resizebox{12.5cm}{0.6cm}{%s}};\n", titulo_prova );

   g_ascii_formatd( str, sizeof( str ), "%.4f", ll );
   fprintf( p1, "\\node[inner sep=0pt] at (18.5,{-2-0.5*\\p}) {\\resizebox{0.9cm}{%scm}{\\bf Nota}};\n", str );
   fprintf( p1, "\\node[inner sep=0pt] at (19.5,{-2-0.5*\\p}) {\\resizebox{0.75cm}{%scm}{\\bf Rec}};\n\n", str );

   // 6. O GRANDE TRUNFO: O C substitui o \foreach do TikZ
   // Isso é muito mais rápido para o LaTeX compilar!
   for ( int i = 0; i <= dados->qtd_alunos_total; i++ ) {
      fprintf( p1, "\\draw (1,{-2-\\p*(%d+1)}) -- (7,{-2-\\p*(%d+1)}) (18,{-2-\\p*(%d+1)}) -- (20,{-2-\\p*(%d+1)});\n", i, i, i, i );
   }

   for ( int i = 1; i <= dados->qtd_alunos_total; i++ ) {
      fprintf( p1, "\\draw (7.1,{-2-\\p*(%d+0.85)}) -- (17.9,{-2-\\p*(%d+0.85)});\n", i, i );
   }

   // 7. Lista de Alunos e Status (Substituindo os arrays \alunos e \freq)
   for ( int i = 1; i <= dados->qtd_alunos_total; i++ ) {
      int idx_aluno = i - 1;

      // Número
      fprintf( p1, "\\node[inner sep=0pt] at (1.3,{-2-\\p*(%d+0.5)}) {%02d};\n", i, i );

      // Nome e Status (C decide a cor e o texto aqui)
      if ( diario[idx_aluno].ativo ) {
         fprintf( p1, "\\node[inner sep=0pt,right] at (1.75,{-2-\\p*(%d+0.5)}) {%.*s};\n",
                  i, diario[idx_aluno].limite_corte, diario[idx_aluno].aluno );
      } else {
         const char *motivo = ( dados->periodo[0] == 'R' ) ? "Aprovado(a) na Média" : "Não Frequenta";
         fprintf( p1, "\\node[inner sep=0pt,right] at (1.75,{-2-\\p*(%d+0.5)}) {\\color{gray!50}%.*s};\n",
                  i, diario[idx_aluno].limite_corte, diario[idx_aluno].aluno );
         fprintf( p1, "\\node[inner sep=0pt,color=gray!80,right] at (7.1,{-2-\\p*(%d+0.5)}) {%s};\n", i, motivo );
      }
   }

   // 8. Rodapé
   fprintf( p1, "\n\\node[inner sep=0pt] at (10.5,{-2.07-\\p*(%d+1.5)}) {SEDUC $-$ São Luis, \\underline{\\qquad} de \\underline{\\hspace{1.2cm}} de %d \\,/\\, Professor: \\underline{\\hspace{8.8cm}}};\n", dados->qtd_alunos_total, data->ano );

   fprintf( p1, "\n\\end{tikzpicture}\n%s\\end{document}\n", dados->qtd_paginas == 2 ? "\\newpage\\," : "" );
   fclose( p1 );
}




//========================================================================================================//
void imagens_para_prova( const int i, int numero, const FichaAluno *diario,
                         const InterfaceDados *dados, const FocoCoordenadas *foco ) {
   char arquivo[256];
   FILE *p;

   sprintf( arquivo, "./dados/temporarios/img%.2d.tex", i );
   p = fopen( arquivo, "w+" );

   // 1. Usamos a classe standalone passando o pacote tikz nativamente
   fprintf( p, "\\documentclass[11pt,tikz]{standalone}\n" );
   fprintf( p, "\\usepackage[utf8]{inputenc}\n" );
   fprintf( p, "\\usepackage[T1]{fontenc}\n" );
   fprintf( p, "\\usepackage[brazil]{babel}\n" );

   if ( dados->fonte_latex == 1 ) {
      fprintf( p, "\\usepackage{cmbright}\n" );
   }

   fprintf( p, "\\usepackage[%s]{professor}\n", dados->cor_destaque );

   // O pacote geometry e os cálculos de paperheight foram removidos,
   // pois o standalone já faz o crop perfeito automaticamente.
   char direcao = ( dados->qtd_colunas == 2 ) ? 'h' : 'v';

   fprintf( p, "\\begin{document}\n" );

   // \pagebreak, \hspace e \vfill foram removidos.
   // Eles geravam "enchimento" (padding) invisível.
   quadro_de_respostas( p, diario[numero - 1].aluno, numero, i, direcao, true, dados, foco );

   fprintf( p, "\\end{document}\n" );

   fclose( p );
}
//========================================================================================================//





//========================================================================================================//
void provinha( FILE *pm, FILE **pb, const int i, char *titulo_prova, const InterfaceDados *dados, const FocoCoordenadas *foco,
               const FichaAluno *diario, const CalendarioData *data, const ItemTextoCurto *G ) {

   int letra, j = 0, jj, k, q;

   uint8_t id;

   k = 0;
   for ( jj = 0; jj < dados->qtd_alunos_total; jj++ ) {
      if ( diario[jj].ativo ) {
         k++;
      }
      if ( i + 1 == k ) {
         break;
      }
   }

   if ( dados->naopresencial ) {
      imagens_para_prova( i, jj + 1, diario, dados, foco );
   }

   char strg[128];

   char str[1024], alternativas[5][1024];

   snprintf( str, sizeof( str ), "./dados/temporarios/prova%.2d.tex", i );
   FILE *pp = fopen( str, "w+" );


   while ( fgets( str, sizeof str, pm ) != NULL ) {
      if ( strcmp( str, "% FONTE\n" ) == 0 ) {
         if ( dados->fonte_latex == 1 )
            fputs( "\\usepackage{cmbright}\n", pp );
         continue;
      } else if ( strcmp( str, "\\usepackage[%s]{professor}\n" ) == 0 ) {
         fprintf( pp, str, dados->cor_destaque );
         continue;
      } else if ( strcmp( str, "% TURMA\n" ) == 0 ) {
         id = foco->turma;
         sprintf( str, "\\def\\turma{{\"%d\"", ( id >> 5 & 1 ) * 255 );
         for ( j = 4; j >= 0; j-- ) {
            sprintf( strg, ",\"%d\"", ( id >> j & 1 ) * 255 );
            snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", strg );
         }
         snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", "}}\n" );
         fputs( str, pp );
         continue;
      } else if ( strcmp( str, "% IDENTIFICADOR\n" ) == 0 ) {
         id = i;
         sprintf( str, "\\def\\id{{\"%d\"", ( id >> 5 & 1 ) * 255 );
         for ( j = 4; j >= 0; j-- ) {
            sprintf( strg, ",\"%d\"", ( id >> j & 1 ) * 255 );
            snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", strg );
         }
         snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", "}}\n" );
         fputs( str, pp );
         continue;
      } else if ( strcmp( str, "\\tema%s{%d}{%d}\n" ) == 0 ) {
         fprintf( pp, str, dados->decoracao_estilo, 0, dados->cabecalho_tipo == 1 );
         continue;
      } else if ( strcmp( str, "\\tema%sColorida{CorSerie}{%d}\n" ) == 0 ) {
         fprintf( pp, str, dados->decoracao_estilo, dados->cabecalho_tipo == 1 );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt] at ({8.75+%d*0.75},-0.6)", 40 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt, color=CorSerie] at ({8.75+%d*0.75},-0.64)", 40 ) == 0 ) {
         fprintf( pp, str, dados->cabecalho_tipo == 1, titulo_prova );
         continue;
      } else if ( strcmp( str, "% COLUNAS\n" ) == 0 ) {
         if ( strcmp( dados->decoracao_estilo, "Quadrados" ) == 0 || strcmp( dados->decoracao_estilo, "Ondas" ) == 0 ) {
            if ( dados->qtd_colunas == 2 ) {
               if ( dados->separadores == 1 )
                  fprintf( pp, "\\foreach \\i in {1,...,240}{\\fill[CorSerie!40] (9.5,{-\\a-0.1*\\i}) circle (0.5pt);}\n" );
               else
                  fprintf( pp, "\\draw[line width=0.8pt] (9.5,-\\a) -- (9.5,-\\k);\n" );
            } else if ( dados->qtd_colunas == 3 ) {
               if ( dados->separadores == 1 )
                  fprintf( pp, "\\pgfmathsetmacro{\\s}{%d/10}\n"
                           "\\foreach \\i in {1,...,240}{\n"
                           "\\fill[CorSerie!30] ({(38-\\s)/6},{-\\a-0.1*\\i}) circle (0.5pt);\n"
                           "\\fill[CorSerie!30] ({(76+\\s)/6},{-\\a-0.1*\\i}) circle (0.5pt);\n"
                           "}\n", 16 - 4 * dados->qtd_colunas );
               else
                  fprintf( pp, "\\pgfmathsetmacro{\\s}{%d/10}\n\\draw[line width=0.8pt] ({(38-\\s)/6},-\\a) -- ({(38-\\s)/6},-\\k) ({(76+\\s)/6},-\\a) -- ({(76+\\s)/6},-\\k);\n", 16 - 4 * dados->qtd_colunas );
            }
         } else {
            if ( dados->qtd_colunas == 2 ) {
               if ( dados->separadores == 1 )
                  fprintf( pp, "\\foreach \\i in {1,...,240}{\\fill[CorSerie!40] (9.5,{-\\a-0.1*\\i}) circle (0.5pt);}\n" );
               else
                  fprintf( pp, "\\draw[line width=0.8pt] (9.5,-\\a) -- (9.5,-\\k);\n" );
            } else if ( dados->qtd_colunas == 3 ) {
               if ( dados->separadores == 1 )
                  fprintf( pp, "\\pgfmathsetmacro{\\s}{%d/10}\n\\foreach \\i in {1,...,240}{\\fill ({(38-\\s)/6},{-\\a-0.1*\\i}) circle (0.6pt); \\fill ({(76+\\s)/6},{-\\a-0.1*\\i}) circle (0.6pt);}\n", 16 - 4 * dados->qtd_colunas );
               else
                  fprintf( pp, "\\pgfmathsetmacro{\\s}{%d/10}\n\\draw[line width=0.8pt] ({(38-\\s)/6},-\\a) -- ({(38-\\s)/6},-\\k) ({(76+\\s)/6},-\\a) -- ({(76+\\s)/6},-\\k);\n", 16 - 4 * dados->qtd_colunas );
            }
         }
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt,right,color=blue] at (2.1,-2.96) {", 54 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right,color=blue] at (2.1,-1.61) {", 54 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right,color=CorSerie] at (2.1,-3.12) {", 57 ) == 0 ) {
         if ( !dados->naopresencial ) continue;
         fprintf( pp, str, diario[jj].aluno );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt,right,color=blue] at (16.4,-2.96) {", 54 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right,color=blue] at (16.4,-1.61) {", 54 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right,color=CorSerie] at (16.4,-3.12) {", 57 ) == 0 ) {
         if ( !dados->naopresencial ) continue;
         fprintf( pp, str, jj + 1 );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt,right] at (0.1,-1.84)", 40 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right] at (0.09,-2.4)", 40 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right] at (0.1,-2.00)", 40 ) == 0 ) {
         fprintf( pp, str, dados->escola );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt,right] at (7.3,-1.59)", 40 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right] at (0.09,-2.98)", 40 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right] at (7.3,-1.75)", 40 ) == 0 ) {
         fprintf( pp, str, dados->gestor );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt,right] at (7.3,-2.09)", 40 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right] at (0.09,-3.5)", 40 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right] at (7.3,-2.25)", 40 ) == 0 ) {
         fprintf( pp, str, dados->professor );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt,right] at (0.09,-4.69)", 40 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right] at (0.09,-4.69)", 40 ) == 0
                ) {
         fprintf( pp, str, dados->serie );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt,right] at (13.3,-1.59)", 40 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right] at (0.09,-4.185)", 40 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right] at (13.3,-1.75)", 40 ) == 0 ) {
         fprintf( pp, str, data->ano );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt,right,color=blue] at (14.55,-1.55) {", 54 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right,color=blue] at (1.34,-4.145) {", 54 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right,color=CorSerie] at (14.55,-1.71) {", 54 ) == 0 ) {
         if ( !dados->naopresencial ) continue;
         fprintf( pp, str, data->dia, data->mes );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt,right] at (13.3,-2.09)", 40 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right] at (0.09,-5.195)", 40 ) == 0 ||
                  strncmp( str, "\\node[inner sep=0pt,right] at (13.3,-2.25)", 40 ) == 0 ) {
         fprintf( pp, str, dados->turma );
         continue;
      } else if ( strcmp( str, "\\pgfmathsetmacro{\\t}{%d};\n" ) == 0 ) {
         fprintf( pp, str, ( dados->turma[4] == 'm' ) * 0 + ( dados->turma[4] == 'v' ) * 1 + ( dados->turma[4] == 'i' ) * 2 + ( dados->turma[4] == 'n' ) * 3 );
         continue;
      } else if ( strcmp( str, "\\setlength{\\columnsep}{%.1fcm}\n" ) == 0 ) {
         fprintf( pp, str, 1.2 - 0.2 * dados->qtd_colunas );
         continue;
      } else if ( strcmp( str, "\\begin{multicols}{%d}\n" ) == 0 ) {
         fprintf( pp, str, dados->qtd_colunas );
         continue;
      } else if ( strcmp( str, "% QUESTOES\n" ) == 0 ) {
         break;
      }
      fputs( str, pp );
   }



   for ( q = 0; q < dados->total_questoes; q++ ) {


      if ( q == 5 && dados->qtd_paginas == 2 ) {
         while ( fgets( str, sizeof str, pm ) != NULL ) {
            if ( strcmp( str, "% COLUNAS\n" ) == 0 ) {
               if ( strcmp( dados->decoracao_estilo, "Quadrados" ) == 0 || strcmp( dados->decoracao_estilo, "Ondas" ) == 0 ) {
                  if ( dados->qtd_colunas == 2 ) {
                     if ( dados->separadores == 1 )
                        fprintf( pp, "\\foreach \\i in {1,...,276}{\\fill[CorSerie!40] (9.5,{-\\a-0.1*\\i}) circle (0.5pt);}\n" );
                     else
                        fprintf( pp, "\\draw[line width=0.8pt] (9.5,-\\a) -- (9.5,-\\k);\n" );
                  } else if ( dados->qtd_colunas == 3 ) {
                     if ( dados->separadores == 1 )
                        fprintf( pp, "\\pgfmathsetmacro{\\s}{%d/10}\n"
                                 "\\foreach \\i in {1,...,276}{\n"
                                 "\\fill[CorSerie!30] ({(38-\\s)/6},{-\\a-0.1*\\i}) circle (0.5pt);\n"
                                 "\\fill[CorSerie!30] ({(76+\\s)/6},{-\\a-0.1*\\i}) circle (0.5pt);\n"
                                 "}\n", 16 - 4 * dados->qtd_colunas );
                     else
                        fprintf( pp, "\\pgfmathsetmacro{\\s}{%d/10}\n\\draw[line width=0.8pt] ({(38-\\s)/6},-\\a) -- ({(38-\\s)/6},-\\k) ({(76+\\s)/6},-\\a) -- ({(76+\\s)/6},-\\k);\n", 16 - 4 * dados->qtd_colunas );
                  }
               } else {
                  if ( dados->qtd_colunas == 2 ) {
                     if ( dados->separadores == 1 )
                        fprintf( pp, "\\foreach \\i in {1,...,273}{\\fill (9.5,{-\\a-0.1-0.1*\\i}) circle (0.6pt);}\n" );
                     else
                        fprintf( pp, "\\draw[line width=0.8pt] (9.5,-\\a) -- (9.5,-\\k);\n" );
                  } else if ( dados->qtd_colunas == 3 ) {
                     if ( dados->separadores == 1 )
                        fprintf( pp, "\\pgfmathsetmacro{\\s}{%d/10}\n\\foreach \\i in {1,...,273}{\\fill ({(38-\\s)/6},{-\\a-0.1-0.1*\\i}) circle (0.6pt); \\fill ({(76+\\s)/6},{-\\a-0.1-0.1*\\i}) circle (0.6pt);}\n", 16 - 4 * dados->qtd_colunas );
                     else
                        fprintf( pp, "\\pgfmathsetmacro{\\s}{%d/10}\n\\draw[line width=0.8pt] ({(38-\\s)/6},{-0.1-\\a}) -- ({(38-\\s)/6},-\\k) ({(76+\\s)/6},{-0.1-\\a}) -- ({(76+\\s)/6},-\\k);\n", 16 - 4 * dados->qtd_colunas );
                  }
               }
               continue;
            } else if ( strcmp( str, "\\begin{multicols}{%d}\n" ) == 0 ) {
               fprintf( pp, str, dados->qtd_colunas );
               continue;
            } else if ( strcmp( str, "% QUESTOES\n" ) == 0 ) {
               break;
            }
            fputs( str, pp );
         }
      }

      int g = G[i].str[q] - 65; // índice da alternativa certa

      while ( fgets( str, sizeof str, pb[q] ) != NULL ) {

         if ( strcmp( str, "% QUESTAO\n" ) == 0 ) {
            fputs( "\n\n", pp );
            fputs( str, pp );
            if ( strcmp( dados->decoracao_estilo, "Quadrados" ) == 0 )
               fprintf( pp, "\\item{$\\questao%sColorida{CorSerie}{black}{%d}{%.2d}$}\\\\\n", dados->decoracao_estilo, ( 3 - dados->qtd_colunas ) * ( dados->cabecalho_tipo == 1 ), q + 1 );
            else if ( strcmp( dados->decoracao_estilo, "Ondas" ) == 0 )
               fprintf( pp, "\\item{$\\questao%sColorida{CorSerie}{%d}{%.2d}$}\\\\\n", dados->decoracao_estilo, ( 3 - dados->qtd_colunas ) * ( dados->cabecalho_tipo == 1 ), q + 1 );
            else
               fprintf( pp, "\\item{$\\questao%s{%d}{%d}{%.2d}$}\\\\\n", dados->decoracao_estilo, 0, ( 3 - dados->qtd_colunas ) * ( dados->cabecalho_tipo == 1 ), q + 1 );

            if ( fgets( str, sizeof str, pb[q] ) == NULL ) {
               fprintf( stderr, "Erro ao ler linha de configuração.\n" );
            }
            while ( str[0] != '\n' ) {
               fputs( str, pp );
               if ( fgets( str, sizeof str, pb[q] ) == NULL ) {
                  fprintf( stderr, "Erro ao ler linha de configuração.\n" );
               }
            }
            fputs( "\n", pp );
         } else if ( strncmp( str, "% ALTERNATIVAS", 14 ) == 0 ) {
            fputs( str, pp );
            fputs( "\\vspace{-2mm}\n", pp );
            fputs( "\\begin{enumerate}[\\hspace{0.42cm}]\n", pp );

            g_autofree int *rnd = randperm( 5 );

            // 1. Encontra em qual índice (j) da permutação aleatória o valor 'g' caiu
            int indice_g = 0;
            while ( rnd[indice_g] != g ) {
               indice_g++;
            }

            // 2. Permuta direta: garante que rnd[0] passe a valer 'g'
            int c = rnd[0];
            rnd[0] = rnd[indice_g];
            rnd[indice_g] = c;


            for ( j = 0; j < 5; j++ ) {
               if ( fgets( alternativas[ rnd[j] ], sizeof( alternativas[ rnd[j] ] ), pb[q] ) == NULL ) {
                  fprintf( stderr, "Erro ao ler linha de configuração.\n" );
               }
               alternativas[ rnd[j] ][ strlen( alternativas[ rnd[j] ] ) - 1 ] = '\0';
            }


            for ( letra = 65; letra < 70; letra++ ) {
               if ( strcmp( dados->decoracao_estilo, "Quadrados" ) == 0 ||
                     strcmp( dados->decoracao_estilo, "Ondas" ) == 0 ) {

                  fprintf( pp, "\\item[$\\circledColorida{CorSerie}{20}{%c}$] %s", letra, alternativas[letra - 65] );
               } else {
                  fprintf( pp, "\\item[$\\circled{1}{%c}$] %s", letra, alternativas[letra - 65] );
               }
            }
            fputs( "\\end{enumerate}\n", pp );
         } else {
            while ( str[0] != '\n' ) {
               fputs( str, pp );
               if ( fgets( str, sizeof str, pb[q] ) == NULL ) {
                  fprintf( stderr, "Erro ao ler linha de configuração.\n" );
               }
            }
            fputs( "\n", pp );
         }
      }

   }

   char direcao = ( dados->qtd_colunas == 2 ) ? 'h' : 'v';

   while ( fgets( str, sizeof str, pm ) != NULL ) {
      if ( strcmp( str, "% RESPOSTAS\n" ) == 0 && dados->qtd_paginas == 1 ) {
         fputs( "\\begin{center}\n", pp );
         quadro_de_respostas( pp, diario[jj].aluno, jj + 1, i, direcao, dados->naopresencial, dados, foco );
         fputs( "\\end{center}\n", pp );
         fputs( "\\end{multicols}\n\\end{document}", pp );
         break;
      } else if ( strcmp( str, "% RESPOSTAS\n" ) == 0 && dados->qtd_paginas == 2 ) {
         // fputs( "\\hspace{-10mm}\\begin{center}\n", pp );
         fputs( "\\noindent\\hspace{-3mm}", pp );
         quadro_de_respostas( pp, diario[jj].aluno, jj + 1, i, direcao, dados->naopresencial, dados, foco );
         // fputs( "\\end{center}\n", pp );
         continue;
      }
      fputs( str, pp );
   }


   fclose( pp );

}
//========================================================================================================//





//========================================================================================================//
void prova( const InterfaceDados *dados, const FocoCoordenadas *foco, const FichaAluno *diario,
            const CaminhoDiretorio *caminho, const CalendarioData *data, const ItemTextoCurto *G ) {

   char titulo_prova[512];
   gerar_tex_lista_frequencia( "./dados/temporarios/frequencia.tex", titulo_prova, diario, dados, data );

   char pasta_tema[1000], questao[2000], modelo_pvo[1000];

   int ii, i, j, q, narq;

   FILE *pm, **pb;

   sprintf( modelo_pvo, "./dados/templates/template_pvo%d%d.tex",
            dados->cabecalho_tipo, dados->separadores );

   // #pragma omp parallel for schedule(dynamic) // Evitar núcleos ociosos
   // #pragma omp parallel for private(ii, q, i, j, narq, N, pasta_tema, questao, pb, pm)
   for ( ii = 0; ii < dados->qtd_alunos_ativos; ii++ ) {

      pb = ( FILE** ) calloc( dados->total_questoes, sizeof( FILE* ) );

      pm = fopen( modelo_pvo, "r" );

      q = 0;

      for ( i = 0; i < NTI; i++ ) {

         if ( dados->qtd_questoes[i] != 0 ) {

            sprintf( pasta_tema, "%s/%s", caminho->banco_questoes, dados->temas_prova_sequencia[i].str );

            if ( quantidade_arquivos_por_extensao( pasta_tema, ".c" ) == 1 ) {
               // Monta o comando: entra na pasta, executa o Q e sai,
               // tudo dentro de uma sub-shell do sistema.
               sprintf( questao, "cd '%s' && ./Q", pasta_tema );

               if ( system( questao ) != 0 ) {
                  fprintf( stderr, "Erro ao gerar questões em: %s\n", pasta_tema );
               }
               // NOTA: Não precisa de chdir(..), seu programa principal nunca saiu do lugar!
            }

            narq = quantidade_arquivos_por_extensao( pasta_tema, ".tex" );
            g_autofree int *N = randperm( narq );

            for ( j = 0; j < dados->qtd_questoes[i]; j++ ) {
               sprintf( questao, "%s/Q%d.tex", pasta_tema, N[j] + 1 );
               pb[q] = fopen( questao, "r" );
               q++;
            }
         }
      }

      file_permute( pb, dados->total_questoes );

      provinha( pm, pb, ii, titulo_prova, dados, foco, diario, data, G );

      for ( i = 0; i < dados->total_questoes; i++ ) {
         fclose( pb[i] );
      }

      free( pb );
      fclose( pm );

   }

   compilacao_latex_e_manipulacao_de_arquivos( diario, dados, caminho );


}
//========================================================================================================//







static void copiar_arquivos_prova_externamente( const InterfaceDados *dados, const CaminhoDiretorio *caminho,
      const char *destino_relatorio ) {
   g_autofree char *nome_arquivo_escola = NULL;
   if ( dados->periodo[0] == 'R' ) {
      nome_arquivo_escola = g_strdup_printf( "Recuperação Final - %s - %s - %s.pdf",
                                             dados->ano, dados->turma, dados->disciplina );
   } else {
      nome_arquivo_escola = g_strdup_printf( "%s Prova - %s_%c - %s - %s.pdf",
                                             dados->prova_sequencia, dados->ano, dados->periodo[0],
                                             dados->turma, dados->disciplina );
   }

   g_autofree char *pasta_provas_escola = g_build_filename( caminho->externo_escola, "Provas", NULL );
   g_autofree char *destino_escola      = g_build_filename( pasta_provas_escola, nome_arquivo_escola, NULL );

   // Garante que a pasta "Provas" exista lá no drive/nuvem da escola
   g_mkdir_with_parents( pasta_provas_escola, 0777 );

   if ( !gio_copiar_arquivo( destino_relatorio, destino_escola ) ) {
      g_printerr( "Erro ao salvar a cópia institucional na pasta Provas da Escola!\n" );
   }
}

//========================================================================================================//
static void copiar_arquivos_prova_nao_presencial( const FichaAluno *diario, const InterfaceDados *dados,
      const CaminhoDiretorio *caminho ) {
   g_autofree char *diretorio_provas  = NULL;
   g_autofree char *diretorio_imagens = NULL;

   // 1. Definição Limpa de Caminhos (Usando dados->periodo para manter o padrão)
   if ( dados->periodo[0] == 'R' ) {
      diretorio_provas  = g_build_filename( caminho->externo, "Provas", "Recuperação Final", NULL );
      diretorio_imagens = g_build_filename( caminho->externo, "Provas", "Recuperação Final Imagens", NULL );

   } else {
      g_autofree char *pasta_provas  = g_strdup_printf( "%s Prova", dados->prova_sequencia );
      g_autofree char *pasta_imagens = g_strdup_printf( "%s Prova Imagens", dados->prova_sequencia );

      diretorio_provas  = g_build_filename( caminho->externo, "Provas", pasta_provas, NULL );
      diretorio_imagens = g_build_filename( caminho->externo, "Provas", pasta_imagens, NULL );
   }

   // 2. O g_mkdir_with_parents já cria tudo do zero (inclusive a pasta raiz "Provas" se faltar)
   if ( g_mkdir_with_parents( diretorio_provas, 0777 ) != 0 ||
         g_mkdir_with_parents( diretorio_imagens, 0777 ) != 0 ) {
      g_printerr( "ERRO CRÍTICO: Falha ao criar a hierarquia em %s\n", caminho->externo );
      return;
   }

   // 3. Mapeamento dos Ativos
   g_autofree int *mapa_ativos = g_new( int, dados->qtd_alunos_ativos );
   int cont_ativos = 0;

   for ( int j = 0; j < dados->qtd_alunos_total; j++ ) {
      if ( diario[j].ativo ) {
         mapa_ativos[cont_ativos++] = j;
      }
   }

   // 4. Laço Paralelo Multi-Core
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < cont_ativos; i++ ) {
      int aluno_idx = mapa_ativos[i];

      // Exportação do PDF da Prova
      g_autofree char *thread_origem = g_strdup_printf( "./dados/temporarios/prova%.2d.pdf", i );
      g_autofree char *nome_arquivo_pdf = g_strdup_printf( "%.2d - %s.pdf", aluno_idx + 1, diario[aluno_idx].aluno );
      g_autofree char *thread_destino = g_build_filename( diretorio_provas, nome_arquivo_pdf, NULL );

      if ( !gio_copiar_arquivo( thread_origem, thread_destino ) ) {
         g_printerr( "[ERRO] Falha ao salvar prova do aluno %s\n", diario[aluno_idx].aluno );
      }

      // Conversão e exportação da Imagem (A conversão consome CPU, por isso brilha no OpenMP)
      g_autofree char *thread_caminho_pdf = g_strdup_printf( "./dados/temporarios/img%.2d.pdf", i );
      g_autofree char *nome_arquivo_png = g_strdup_printf( "%.2d - %s.png", aluno_idx + 1, diario[aluno_idx].aluno );
      g_autofree char *thread_caminho_png = g_build_filename( diretorio_imagens, nome_arquivo_png, NULL );

      if ( !pdf2png( thread_caminho_pdf, thread_caminho_png, 6.0 ) ) {
         g_printerr( "[AVISO] Falha ao converter e mover imagem %s\n", thread_caminho_png );
      }

      // Limpeza imediata do temporário individual da imagem (economiza espaço em disco na hora)
      g_autofree char *thread_nome_base = g_strdup_printf( "img%.2d", i );
      apagar_arquivos_temporarios_latex_nativamente( "./dados/temporarios", thread_nome_base, 5 );
   }

}
//========================================================================================================//





//========================================================================================================//
void compilacao_latex_e_manipulacao_de_arquivos( const FichaAluno *diario, const InterfaceDados *dados,
      const CaminhoDiretorio *caminho ) {

   // 1. Compilação paralela do LaTeX
   g_pdflatex_parallel( "./dados/temporarios" );

   if ( dados->naopresencial ) {
      copiar_arquivos_prova_nao_presencial( diario, dados, caminho );
   }

   // =========================================================================
   // PREPARAÇÃO DO DESTINO FINAL DO RELATÓRIO
   // =========================================================================
   g_autofree char *nome_arquivo = NULL;
   if ( dados->periodo[0] == 'R' ) {
      nome_arquivo = g_strdup( "Recuperação Final.pdf" );
   } else {
      nome_arquivo = g_strdup_printf( "%s Prova.pdf", dados->prova_sequencia );
   }
   g_autofree char *destino_relatorio = g_build_filename( caminho->relatorios, nome_arquivo, NULL );


   // =========================================================================
   // UNIÃO DOS PDFS (Frequência + Provas) DIRETAMENTE NO DESTINO FINAL
   // =========================================================================
   int qtd_pdfs = dados->qtd_alunos_ativos + 1; // +1 para acomodar a frequência
   g_auto( GStrv ) arquivos_pdf = g_new0( char *, qtd_pdfs + 1 ); // +1 para o NULL

   arquivos_pdf[0] = g_strdup( "frequencia.pdf" );
   for ( int i = 0; i < dados->qtd_alunos_ativos; i++ ) {
      arquivos_pdf[i + 1] = g_strdup_printf( "prova%.2d.pdf", i );
   }

   // Blindagem: remove arquivo anterior para evitar falsos positivos
   g_remove( destino_relatorio );

   // Geração unificada direto na pasta Relatórios (Elimina o gio_mover_arquivo)
   g_pdfunite( "./dados/temporarios/", ( const char ** )arquivos_pdf, qtd_pdfs, destino_relatorio );

   // =========================================================================
   // LIMPEZA PARALELA DE ARQUIVOS NATIVOS
   // =========================================================================
   apagar_arquivos_temporarios_latex_nativamente( "./dados/temporarios", "frequencia", 5 );
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < dados->qtd_alunos_ativos; i++ ) {
      g_autofree char *nome_base = g_strdup_printf( "prova%.2d", i );
      apagar_arquivos_temporarios_latex_nativamente( "./dados/temporarios", nome_base, 5 );
   }

   // =========================================================================
   // CÓPIA INSTITUCIONAL (ESCOLA)
   // =========================================================================
   // Só copiamos se a flag estiver ativa E se o pdfunite teve sucesso!
   if ( g_file_test( destino_relatorio, G_FILE_TEST_EXISTS ) ) {
      if ( dados->expor ) {
         copiar_arquivos_prova_externamente( dados, caminho, destino_relatorio );
      }
      g_xdg_open( destino_relatorio );

   } else {
      g_printerr( "[AVISO] O arquivo final (%s) não foi gerado.\n", destino_relatorio );
   }

}
//========================================================================================================//

