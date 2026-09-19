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

#include "glib_gio.h"
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





void gerar_tex_lista_frequencia( const char *caminho_saida, char *titulo_prova, const GArray *fichas,
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
      const FichaAluno *ficha = &g_array_index( fichas, FichaAluno, idx_aluno );

      if ( ficha->ativo ) {
         fprintf( p1, "\\node[inner sep=0pt,right] at (1.75,{-2-\\p*(%d+0.5)}) {%.*s};\n",
                  i, ficha->limite_corte, ficha->aluno );
      } else {
         const char *motivo = ( dados->periodo[0] == 'R' ) ? "Aprovado(a) na Média" : "Não Frequenta";
         fprintf( p1, "\\node[inner sep=0pt,right] at (1.75,{-2-\\p*(%d+0.5)}) {\\color{gray!50}%.*s};\n",
                  i, ficha->limite_corte, ficha->aluno );
         fprintf( p1, "\\node[inner sep=0pt,color=gray!80,right] at (7.1,{-2-\\p*(%d+0.5)}) {%s};\n", i, motivo );
      }
   }

   // 8. Rodapé
   fprintf( p1, "\n\\node[inner sep=0pt] at (10.5,{-2.07-\\p*(%d+1.5)}) {SEDUC $-$ São Luis, \\underline{\\qquad} de \\underline{\\hspace{1.2cm}} de %d \\,/\\, Professor: \\underline{\\hspace{8.8cm}}};\n", dados->qtd_alunos_total, data->ano );

   fprintf( p1, "\n\\end{tikzpicture}\n%s\\end{document}\n", dados->qtd_paginas == 2 ? "\\newpage\\," : "" );
   fclose( p1 );
}




//========================================================================================================//
void imagens_para_prova( const int i, int numero, const GArray *fichas,
                         const InterfaceDados *dados, const FocoCoordenadas *foco ) {
   char arquivo[256];
   FILE *p;

   sprintf( arquivo, "./dados/temporarios/img%.2d.tex", i );
   p = fopen( arquivo, "w+" );

   // 1. Usamos a classe standalone passando o pacote tikz nativamente
   fprintf( p, "\\documentclass[11pt,tikz,margin=20mm]{standalone}\n" );
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
   const FichaAluno *ficha = &g_array_index( fichas, FichaAluno, numero - 1 );
   quadro_de_respostas( p, ficha->aluno, numero, i, direcao, true, dados, foco );

   fprintf( p, "\\end{document}\n" );

   fclose( p );
}
//========================================================================================================//





//========================================================================================================//
static void anexar_preambulo_latex( GString *tex, const InterfaceDados *dados ) {
   g_string_append( tex,
                    "\\documentclass[11pt,a4paper]{report}\n"
                    "\\usepackage[utf8]{inputenc}\n"
                    "\\usepackage[T1]{fontenc}\n"
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
                    "\\pagestyle{empty}\n"
                  );

   if ( dados->fonte_latex == 1 ) {
      g_string_append( tex, "\\usepackage{cmbright}\n" );
   }

   g_string_append_printf( tex, "\\usepackage[%s]{professor}\n", dados->cor_destaque );

   g_string_append( tex,
                    "\\definecolor{cinza}{rgb}{0.4,0.4,0.4}\n"
                    "\\newcommand{\\linhas}[1]{\\tikz[cinza!40,line width=1pt]{\\fill (0,{0.8*(#1+0.8)}) circle (0pt);\\foreach \\i in {1,...,#1}{\\draw (0,0.8*\\i) -- (\\linewidth,0.8*\\i);}}}\n"
                    "\\newcommand*{\\vtext}[2]{\\parbox[t]{9pt}{\\multirow{#1}{*}{\\rotatebox[origin=c]{90}{#2}}}}\n"
                    "\\newcommand{\\sen}{\\mathrm{sen}\\hspace{2pt}}\n"
                    "\\newcommand{\\cossec}{\\mathrm{cossec}\\hspace{2pt}}\n"
                    "\\newcommand{\\tg}{\\mathrm{tg}\\hspace{2pt}}\n"
                    "\\newcommand{\\cotg}{\\mathrm{cotg}\\hspace{2pt}}\n"
                    "\\newcolumntype{L}[1]{>{\\raggedright\\arraybackslash}p{#1}}\n"
                    "\\newcolumntype{C}[1]{>{\\centering\\arraybackslash}p{#1}}\n"
                    "\\newcolumntype{R}[1]{>{\\raggedleft\\arraybackslash}p{#1}}\n\n"
                  );
}
//--------------------------------------------------------------------------------------------------------
static void anexar_identificadores_latex( GString *tex, uint8_t id_turma, uint8_t id_prova ) {
   g_string_append_printf( tex, "\\def\\turma{{\"%d\",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\"}}\n",
                           ( id_turma >> 5 & 1 ) * 255, ( id_turma >> 4 & 1 ) * 255, ( id_turma >> 3 & 1 ) * 255,
                           ( id_turma >> 2 & 1 ) * 255, ( id_turma >> 1 & 1 ) * 255, ( id_turma & 1 ) * 255 );

   g_string_append_printf( tex, "\\def\\id{{\"%d\",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\"}}\n",
                           ( id_prova >> 5 & 1 ) * 255, ( id_prova >> 4 & 1 ) * 255, ( id_prova >> 3 & 1 ) * 255,
                           ( id_prova >> 2 & 1 ) * 255, ( id_prova >> 1 & 1 ) * 255, ( id_prova & 1 ) * 255 );
}
//--------------------------------------------------------------------------------------------------------
static void anexar_cabecalho_base_latex( GString *tex, const InterfaceDados *dados, const FichaAluno *ficha,
      int num_chamada, const CalendarioData *data, const char *titulo_prova, gboolean is_page1 ) {
   const char *cor_aluno = dados->naopresencial ? "blue" : "CorSerie";

   if ( dados->cabecalho_tipo == 1 ) { // PVO11 e PVO12
      if ( is_page1 ) {
         if ( strcmp( dados->decoracao_estilo, "Quadrados" ) == 0 || strcmp( dados->decoracao_estilo, "Ondas" ) == 0 )
            g_string_append_printf( tex, "\\tema%sColorida{CorSerie}{1}\n", dados->decoracao_estilo );
         else
            g_string_append_printf( tex, "\\tema%s{0}{1}\n", dados->decoracao_estilo );

         g_string_append_printf( tex, "\\node[inner sep=0pt, color=CorSerie] at ({8.75+1*0.75},-0.64) {\\LARGE\\bf %s};\n", titulo_prova );
         g_string_append( tex, "\\draw[line width=1pt, color=CorSerie, rounded corners] (0,-1.4) rectangle (17.5,-2.6);\n" );
         g_string_append( tex, "\\draw[line width=1pt, color=CorSerie] (7,-1.4)--(7,-2.6) (13,-1.4)--(13,-2.6);\n" );

         g_string_append_printf( tex, "\\node[inner sep=0pt,right] at (0.1,-2.00) {\\resizebox{6.5cm}{0.75cm}{\\bf %s}};\n", dados->escola );
         g_string_append_printf( tex, "\\node[inner sep=0pt,right] at (7.3,-1.75) {{\\color{CorSerie}\\bf Gestor(a):} %s};\n", dados->gestor );
         g_string_append_printf( tex, "\\node[inner sep=0pt,right] at (7.3,-2.25) {{\\color{CorSerie}\\bf Professor:} %s};\n", dados->professor );

         g_string_append_printf( tex, "\\node[inner sep=0pt,right] at (13.3,-1.75) {{\\color{CorSerie}\\bf Data:} \\underline{\\hspace{6mm}}/\\underline{\\hspace{6mm}}/\\underline{%d}};\n", data->ano );
         if ( dados->naopresencial ) g_string_append_printf( tex, "\\node[inner sep=0pt,right,color=blue] at (14.55,-1.71) {\\bf %.2d\\hspace{4mm}%.2d};\n", data->dia, data->mes );
         g_string_append_printf( tex, "\\node[inner sep=0pt,right] at (13.3,-2.25) {{\\color{CorSerie}\\bf Turma:} %s};\n", dados->turma );

         g_string_append( tex, "\\draw[line width=1pt,rounded corners,color=Vinho!70] (17.6,-1.40) rectangle (19,-3.36);\n\\node[inner sep=0pt] at (18.3,-1.71) {\\bf\\uuline{NOTA}};\n" );

         g_string_append( tex, "\\node[inner sep=0pt,left] at (1.9,-3.16) {\\bf Aluno(a):};\n" );
         g_string_append_printf( tex, "\\node[inner sep=0pt,right,color=%s] at (2.1,-3.12) {\\bf %s};\n", cor_aluno, ficha->aluno );
         g_string_append( tex, "\\draw (2,-3.36) -- (15.2,-3.36);\n" );

         g_string_append( tex, "\\node[inner sep=0pt,left] at (16.2,-3.16) {\\bf Nº:};\n" );
         g_string_append_printf( tex, "\\node[inner sep=0pt,right,color=%s] at (16.4,-3.12) {\\bf %.2d};\n", cor_aluno, num_chamada );
         g_string_append( tex, "\\draw (16.3,-3.36) -- (17.3,-3.36);\n" );
      }

      g_string_append( tex, is_page1 ? "\\pgfmathsetmacro{\\a}{3.56};\n\\pgfmathsetmacro{\\k}{27.65};\n" : "\\pgfmathsetmacro{\\a}{0.01412};\n\\pgfmathsetmacro{\\k}{27.58588};\n" );

   } else { // PVO21 e PVO22 (Cabeçalho Completo)
      if ( is_page1 ) {
         g_string_append_printf( tex, "\\tema%s{0}{0}\n", dados->decoracao_estilo );
         g_string_append_printf( tex, "\\node[inner sep=0pt] at ({8.75},-0.6) {\\LARGE\\bf %s};\n", titulo_prova );

         g_string_append( tex, "\\draw[line width=0.8pt,rounded corners] (17.6,-0.01412) rectangle (19,-1.85);\n\\node[inner sep=0pt] at (18.3,-0.3) {\\bf\\uuline{NOTA}};\n" );
         g_string_append( tex, "\\node[inner sep=0pt,left] at (1.9,-1.65) {\\bf Aluno(a):};\n" );
         g_string_append_printf( tex, "\\node[inner sep=0pt,right,color=%s] at (2.1,-1.61) {\\bf %s};\n", cor_aluno, ficha->aluno );
         g_string_append( tex, "\\draw (2,-1.85) -- (15.2,-1.85);\n" );
         g_string_append( tex, "\\node[inner sep=0pt,left] at (16.2,-1.65) {\\bf Nº:};\n" );
         g_string_append_printf( tex, "\\node[inner sep=0pt,right,color=%s] at (16.4,-1.61) {\\bf %.2d};\n", cor_aluno, num_chamada );
         g_string_append( tex, "\\draw (16.3,-1.85) -- (17.3,-1.85);\n" );

         g_string_append( tex, "\\draw[line width=0.8pt,rounded corners] (0,-2.06) rectangle (91/15,-3.78);\n" );
         g_string_append_printf( tex, "\\node[inner sep=0pt,right] at (0.09,-2.4) {\\resizebox{5.88cm}{0.35cm}{\\bf %s}};\n", dados->escola );
         g_string_append_printf( tex, "\\node[inner sep=0pt,right] at (0.09,-2.98) {{\\bf Gestor(a):} %s};\n", dados->gestor );
         g_string_append_printf( tex, "\\node[inner sep=0pt,right] at (0.09,-3.5) {{\\bf Professor:} %s};\n", dados->professor );

         g_string_append( tex, "\\draw[line width=0.8pt,rounded corners] (0,-3.88) rectangle (3.8,-5.5);\n" );
         g_string_append_printf( tex, "\\node[inner sep=0pt,right] at (0.09,-4.185) {{\\bf Data:} \\underline{\\hspace{6mm}}/\\underline{\\hspace{6mm}}/\\underline{%d}};\n", data->ano );
         if ( dados->naopresencial ) g_string_append_printf( tex, "\\node[inner sep=0pt,right,color=blue] at (1.34,-4.145) {\\bf %.2d\\hspace{4mm}%.2d};\n", data->dia, data->mes );
         g_string_append_printf( tex, "\\node[inner sep=0pt,right] at (0.09,-4.69) {{\\bf Série:} %d};\n", dados->serie );
         g_string_append_printf( tex, "\\node[inner sep=0pt,right] at (0.09,-5.195) {{\\bf Turma:} %.3s};\n", dados->turma );

         int turno_idx = ( dados->turma[4] == 'm' ) * 0 + ( dados->turma[4] == 'v' ) * 1 + ( dados->turma[4] == 'i' ) * 2 + ( dados->turma[4] == 'n' ) * 3;
         g_string_append_printf( tex, "\\draw[line width=0.8pt,rounded corners] (3.9,-3.88) rectangle (91/15,-5.5);\n\\pgfmathsetmacro{\\t}{%d};\n", turno_idx );
         g_string_append( tex,
                          "\\foreach \\i in {0,...,3}{\\draw[line width=0.8pt] ({3.9+0.7*81/220},{-3.88-(\\i+0.7)*81/220}) circle (0.14cm);\n"
                          "\\ifthenelse{\\i=\\t}{\\fill ({3.9+0.7*81/220},{-3.88-(\\i+0.7)*81/220}) circle (0.08cm);}{}}\n"
                          "\\node[inner sep=0pt,right] at ({4.15+0.7*81/220},{-3.88-0.7*81/220}) {\\footnotesize Matutino};\n"
                          "\\node[inner sep=0pt,right] at ({4.15+0.7*81/220},{-3.88-1.7*81/220}) {\\footnotesize Vespertino};\n"
                          "\\node[inner sep=0pt,right] at ({4.15+0.7*81/220},{-3.88-2.7*81/220}) {\\footnotesize Integral};\n"
                          "\\node[inner sep=0pt,right] at ({4.15+0.7*81/220},{-3.88-3.7*81/220}) {\\footnotesize Noturno};\n"
                        );
      }
      g_string_append( tex, is_page1 ? "\\pgfmathsetmacro{\\a}{2.06};\n\\pgfmathsetmacro{\\k}{27.55764};\n\\pgfmathsetmacro{\\s}{0.4};\n" : "\\pgfmathsetmacro{\\a}{0.04236};\n\\pgfmathsetmacro{\\k}{27.55764};\n\\pgfmathsetmacro{\\s}{0.4};\n" );
   }

   // Lógica comum para as margens/dots óticos do Vértice (adaptado para 20 no lugar de %d)
   if ( dados->separadores == 2 && dados->cabecalho_tipo == 2 && !is_page1 ) {
      g_string_append( tex, "\\draw[line width=0.8pt] (0,-\\a) -- (19,-\\a);\n\\draw[line width=0.8pt] (0,{-0.1-\\a}) -- (19,{-0.1-\\a});\n" );
   }
   if ( dados->cabecalho_tipo == 1 ) {
      g_string_append( tex, "\\foreach \\i in {0,...,190}{\\fill (0.1*\\i,-\\a) circle (0.5pt); \\fill (0.1*\\i,-\\k) circle (0.5pt);}\n" );
   } else {
      g_string_append( tex, "\\foreach \\i in {0,...,20}{\\fill ({4.45*0+0.2+0.2*\\i},-\\k-0.1) circle (1.2pt);}\n" );
   }
}
//--------------------------------------------------------------------------------------------------------
static void anexar_colunas_separadoras_latex( GString *tex, const InterfaceDados *dados, gboolean is_page2 ) {
   gboolean decorado = ( strcmp( dados->decoracao_estilo, "Quadrados" ) == 0 || strcmp( dados->decoracao_estilo, "Ondas" ) == 0 );
   int dots = is_page2 ? ( decorado ? 276 : 273 ) : 240;

   if ( dados->qtd_colunas == 2 ) {
      if ( dados->separadores == 1 ) {
         g_string_append_printf( tex, "\\foreach \\i in {1,...,%d}{\\fill%s (9.5,{-\\a-%s0.1*\\i}) circle (%s);}\n",
                                 dots, decorado ? "[CorSerie!40]" : "", ( !is_page2 && !decorado ) ? "0.1-" : "", decorado ? "0.5pt" : "0.6pt" );
      } else {
         g_string_append( tex, "\\draw[line width=0.8pt] (9.5,-\\a) -- (9.5,-\\k);\n" );
      }
   } else if ( dados->qtd_colunas == 3 ) {
      int exp = 16 - 4 * dados->qtd_colunas;
      if ( dados->separadores == 1 ) {
         g_string_append_printf( tex, "\\pgfmathsetmacro{\\s}{%d/10}\n\\foreach \\i in {1,...,%d}{\\fill%s ({(38-\\s)/6},{-\\a-%s0.1*\\i}) circle (%s); \\fill%s ({(76+\\s)/6},{-\\a-%s0.1*\\i}) circle (%s);}\n",
                                 exp, dots, decorado ? "[CorSerie!30]" : "", ( !is_page2 && !decorado ) ? "0.1-" : "", decorado ? "0.5pt" : "0.6pt", decorado ? "[CorSerie!30]" : "", ( !is_page2 && !decorado ) ? "0.1-" : "", decorado ? "0.5pt" : "0.6pt" );
      } else {
         g_string_append_printf( tex, "\\pgfmathsetmacro{\\s}{%d/10}\n\\draw[line width=0.8pt] ({(38-\\s)/6},{-%.1f-\\a}) -- ({(38-\\s)/6},-\\k) ({(76+\\s)/6},{-%.1f-\\a}) -- ({(76+\\s)/6},-\\k);\n",
                                 exp, ( !decorado && is_page2 ) ? 0.1 : 0.0, ( !decorado && is_page2 ) ? 0.1 : 0.0 );
      }
   }
}
//--------------------------------------------------------------------------------------------------------
static void processar_questao_latex( GString *tex, FILE *f_questao, int q_idx, int gabarito_letra, const InterfaceDados *dados ) {
   char str[1024], alternativas[5][1024];

   while ( fgets( str, sizeof( str ), f_questao ) != NULL ) {
      if ( strcmp( str, "% QUESTAO\n" ) == 0 ) {
         g_string_append( tex, "\n\n% QUESTAO\n" );
         int col_factor = ( 3 - dados->qtd_colunas ) * ( dados->cabecalho_tipo == 1 );

         if ( strcmp( dados->decoracao_estilo, "Quadrados" ) == 0 )
            g_string_append_printf( tex, "\\item{$\\questao%sColorida{CorSerie}{black}{%d}{%.2d}$}\\\\\n", dados->decoracao_estilo, col_factor, q_idx + 1 );
         else if ( strcmp( dados->decoracao_estilo, "Ondas" ) == 0 )
            g_string_append_printf( tex, "\\item{$\\questao%sColorida{CorSerie}{%d}{%.2d}$}\\\\\n", dados->decoracao_estilo, col_factor, q_idx + 1 );
         else
            g_string_append_printf( tex, "\\item{$\\questao%s{%d}{%d}{%.2d}$}\\\\\n", dados->decoracao_estilo, 0, col_factor, q_idx + 1 );

         while ( fgets( str, sizeof( str ), f_questao ) && str[0] != '\n' ) g_string_append( tex, str );
         g_string_append( tex, "\n" );

      } else if ( strncmp( str, "% ALTERNATIVAS", 14 ) == 0 ) {
         g_string_append( tex, str );
         g_string_append( tex, "\\vspace{-2mm}\n\\begin{enumerate}[\\hspace{0.42cm}]\n" );

         g_autofree int *rnd = randperm( 5 );
         int indice_g = 0;
         while ( rnd[indice_g] != gabarito_letra ) indice_g++;

         int tmp = rnd[0];
         rnd[0] = rnd[indice_g];
         rnd[indice_g] = tmp;

         for ( int j = 0; j < 5; j++ ) {
            if ( fgets( alternativas[rnd[j]], sizeof( alternativas[rnd[j]] ), f_questao ) )
               alternativas[rnd[j]][strlen( alternativas[rnd[j]] ) - 1] = '\0';
         }

         for ( int letra = 65; letra < 70; letra++ ) {
            if ( strcmp( dados->decoracao_estilo, "Quadrados" ) == 0 || strcmp( dados->decoracao_estilo, "Ondas" ) == 0 )
               g_string_append_printf( tex, "\\item[$\\circledColorida{CorSerie}{20}{%c}$] %s\n", letra, alternativas[letra - 65] );
            else
               g_string_append_printf( tex, "\\item[$\\circled{1}{%c}$] %s\n", letra, alternativas[letra - 65] );
         }
         g_string_append( tex, "\\end{enumerate}\n" );
      } else {
         while ( str[0] != '\n' && str[0] != '\0' ) {
            g_string_append( tex, str );
            if ( fgets( str, sizeof( str ), f_questao ) == NULL ) break;
         }
         g_string_append( tex, "\n" );
      }
   }
}
//--------------------------------------------------------------------------------------------------------
void prova( const InterfaceDados *dados, const FocoCoordenadas *foco, const GArray *fichas,
            const CaminhoDiretorio *caminho, const CalendarioData *data, const ItemTextoCurto *G ) {

   char titulo_prova[512];
   gerar_tex_lista_frequencia( "./dados/temporarios/frequencia.tex", titulo_prova, fichas, dados, data );

   // 1. Pré-compilação do Banco de Questões
   for ( int i = 0; i < NTI; i++ ) {
      if ( dados->qtd_questoes[i] != 0 ) {
         g_autofree char *pasta_tema = g_build_filename( caminho->banco_questoes, dados->temas_prova_sequencia[i].str, NULL );
         if ( quantidade_arquivos_por_extensao( pasta_tema, ".c" ) == 1 ) {
            g_autofree char *cmd = g_strdup_printf( "cd '%s' && ./Q", pasta_tema );
            if ( system( cmd ) != 0 ) fprintf( stderr, "Erro ao gerar questões em: %s\n", pasta_tema );
         }
      }
   }

   // 2. Orquestração e Geração de Provas (Aluno por Aluno)
   int num_ativo = 0; // Equivalente ao antigo iterador i

   for ( guint jj = 0; jj < fichas->len; jj++ ) {
      const FichaAluno *ficha = &g_array_index( fichas, FichaAluno, jj );
      if ( !ficha->ativo ) continue;

      FILE **pb = g_new0( FILE*, dados->total_questoes );
      int q = 0;

      // Localiza e carrega os arquivos base de questões para o aluno
      for ( int i = 0; i < NTI; i++ ) {
         if ( dados->qtd_questoes[i] != 0 ) {
            g_autofree char *pasta_tema = g_build_filename( caminho->banco_questoes, dados->temas_prova_sequencia[i].str, NULL );
            int narq = quantidade_arquivos_por_extensao( pasta_tema, ".tex" );
            g_autofree int *N = randperm( narq );

            for ( int j = 0; j < dados->qtd_questoes[i]; j++ ) {
               g_autofree char *questao_path = g_strdup_printf( "%s/Q%d.tex", pasta_tema, N[j] + 1 );
               pb[q] = fopen( questao_path, "r" );
               q++;
            }
         }
      }

      file_permute( pb, dados->total_questoes );

      if ( dados->naopresencial ) {
         imagens_para_prova( num_ativo, jj + 1, fichas, dados, foco );
      }

      // 3. Montagem Atômica do Arquivo .tex via GLib
      g_autofree char *caminho_tex = g_strdup_printf( "./dados/temporarios/prova%.2d.tex", num_ativo );
      FILE *pp = fopen( caminho_tex, "w+" );

      if ( pp ) {
         g_autoptr( GString ) tex_corpo = g_string_sized_new( 16384 ); // Buffer otimizado pré-alocado (16kb)

         anexar_preambulo_latex( tex_corpo, dados );
         anexar_identificadores_latex( tex_corpo, foco->turma, num_ativo );

         g_string_append( tex_corpo, "\\begin{document}\n\\noindent\n\\begin{tikzpicture}\n" );
         anexar_cabecalho_base_latex( tex_corpo, dados, ficha, jj + 1, data, titulo_prova, TRUE );
         anexar_colunas_separadoras_latex( tex_corpo, dados, FALSE );
         g_string_append( tex_corpo, "\\end{tikzpicture}\n\n" );

         g_string_append_printf( tex_corpo, "\\vspace{%.2fcm}\n", dados->cabecalho_tipo == 1 ? -24.44 : -26.04 );
         g_string_append_printf( tex_corpo, "\\setlength{\\columnsep}{%.1fcm}\n\\begin{multicols}{%d}\n", 1.2 - 0.2 * dados->qtd_colunas, dados->qtd_colunas );
         if ( dados->cabecalho_tipo == 2 ) g_string_append( tex_corpo, "\\rule{0cm}{3.225cm}\n" );
         g_string_append( tex_corpo, "\\begin{enumerate}[\\hspace{-1.8mm}]\n" );

         // Insere o miolo embaralhado de questões
         for ( int qi = 0; qi < dados->total_questoes; qi++ ) {
            if ( qi == 5 && dados->qtd_paginas == 2 ) {
               g_string_append( tex_corpo, "\\end{enumerate}\n\\end{multicols}\n\\newpage\n\\noindent\n\\begin{tikzpicture}\n" );
               anexar_cabecalho_base_latex( tex_corpo, dados, ficha, jj + 1, data, titulo_prova, FALSE );
               anexar_colunas_separadoras_latex( tex_corpo, dados, TRUE );
               g_string_append_printf( tex_corpo, "\\end{tikzpicture}\n\n\\vspace{-27.85cm}\n\\begin{multicols}{%d}\n\\begin{enumerate}[\\hspace{-1.8mm}]\n", dados->qtd_colunas );
            }
            processar_questao_latex( tex_corpo, pb[qi], qi, G[num_ativo].str[qi] - 65, dados );
         }

         g_string_append( tex_corpo, "\\end{enumerate}\n" );

         // Descarrega o texto GString massivo no disco de uma vez
         fputs( tex_corpo->str, pp );

         // Chamada externa segura via ponteiro
         char direcao = ( dados->qtd_colunas == 2 ) ? 'h' : 'v';
         if ( dados->qtd_paginas == 1 ) {
            fputs( "\\begin{center}\n", pp );
            quadro_de_respostas( pp, ficha->aluno, jj + 1, num_ativo, direcao, dados->naopresencial, dados, foco );
            fputs( "\\end{center}\n", pp );
         } else {
            fputs( "\\noindent\\hspace{-3mm}\n", pp );
            quadro_de_respostas( pp, ficha->aluno, jj + 1, num_ativo, direcao, dados->naopresencial, dados, foco );
         }

         // Encerra arquivo local
         fputs( "\\end{multicols}\n\\end{document}\n", pp );
         fclose( pp );
      }

      // Cleanup do laço interno
      for ( int x = 0; x < dados->total_questoes; x++ ) if ( pb[x] ) fclose( pb[x] );
      g_free( pb );

      num_ativo++;
   }

   compilacao_latex_e_manipulacao_de_arquivos( fichas, dados, caminho );
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
static void copiar_arquivos_prova_nao_presencial( const GArray *fichas, const InterfaceDados *dados,
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
      const FichaAluno *ficha = &g_array_index( fichas, FichaAluno, j );
      if ( ficha->ativo ) {
         mapa_ativos[cont_ativos++] = j;
      }
   }

   // 4. Laço Paralelo Multi-Core
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < cont_ativos; i++ ) {
      int aluno_idx = mapa_ativos[i];
      const FichaAluno *ficha = &g_array_index( fichas, FichaAluno, aluno_idx );

      // Exportação do PDF da Prova
      g_autofree char *thread_origem = g_strdup_printf( "./dados/temporarios/prova%.2d.pdf", i );
      g_autofree char *nome_arquivo_pdf = g_strdup_printf( "%.2d - %s.pdf", aluno_idx + 1, ficha->aluno );
      g_autofree char *thread_destino = g_build_filename( diretorio_provas, nome_arquivo_pdf, NULL );

      if ( !gio_copiar_arquivo( thread_origem, thread_destino ) ) {
         g_printerr( "[ERRO] Falha ao salvar prova do aluno %s\n", ficha->aluno );
      }

      // Conversão e exportação da Imagem (A conversão consome CPU, por isso brilha no OpenMP)
      g_autofree char *thread_caminho_pdf = g_strdup_printf( "./dados/temporarios/img%.2d.pdf", i );
      g_autofree char *nome_arquivo_png = g_strdup_printf( "%.2d - %s.png", aluno_idx + 1, ficha->aluno );
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
void compilacao_latex_e_manipulacao_de_arquivos( const GArray *fichas, const InterfaceDados *dados,
      const CaminhoDiretorio *caminho ) {

   // 1. Compilação paralela do LaTeX
   g_pdflatex_parallel( "./dados/temporarios" );

   if ( dados->naopresencial ) {
      copiar_arquivos_prova_nao_presencial( fichas, dados, caminho );
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

