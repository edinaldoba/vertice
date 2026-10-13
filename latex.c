/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "latex.h"



/*
* Função refinada para desenhar o payload binário dinamicamente em TikZ.
*/
static void gerar_matriz_identificacao( GString *tex, uint8_t id, uint8_t turma, uint8_t disc, uint8_t per, uint8_t seq ) {
   if ( !tex ) return;

   // 1. Empacotando 23 bits em um inteiro de 32 bits
   uint32_t payload = 0;
   payload |= ( id    & 0x3F );       // 6 bits (0 a 5)
   payload |= ( turma & 0xFF ) << 6;  // 8 bits (6 a 13)
   payload |= ( disc  & 0x0F ) << 14; // 4 bits (14 a 17)
   payload |= ( per   & 0x07 ) << 18; // 3 bits (18 a 20)
   payload |= ( seq   & 0x03 ) << 21; // 2 bits (21 a 22)

   // 2. Cálculo dos 4 bits de paridade (XOR de blocos de 4 bits)
   uint32_t paridade = 0;
   uint32_t temp = payload;

   while ( temp > 0 ) {
      paridade ^= ( temp & 0x0F );
      temp >>= 4;
   }

   // Insere os 4 bits de paridade calculados nos bits 23 a 26
   payload |= ( paridade & 0x0F ) << 23;

   // 3. Desenhando a grade 3x9 no LaTeX (Total: 27 iterações)
   for ( int i = 0; i < 27; i++ ) {
      int linha = i / 3;
      int coluna = i % 3;

      int bit_ativo = ( payload >> i ) & 1;

      if ( bit_ativo ) {
         // Fatores de deslocamento horizontal e vertical extraídos
         double x_start = 1.0 + ( 1.0 / 6.0 ) + ( coluna * 4.0 / 6.0 );
         double x_end   = 1.0 + ( 3.0 / 6.0 ) + ( coluna * 4.0 / 6.0 );

         double y_start = -( 1.0 + ( 1.0 / 6.0 ) + ( linha * 2.0 / 3.0 ) );
         double y_end   = -( 1.0 + ( 3.0 / 6.0 ) + ( linha * 2.0 / 3.0 ) );

         // Injeção limpa e direta na GString (memória RAM)
         g_string_append_printf( tex, "\\fill (%.4f,%.4f) rectangle (%.4f,%.4f);\n", x_start, y_start, x_end, y_end );
      }
   }
}






//========================================================================================================//
void quadro_de_respostas( GString *tex, const char *aluno, int numero, const uint8_t id, char direcao,
                          bool assinalar_nome_numero, const InterfaceDados *dados, const FocoCoordenadas *foco ) {

   g_return_if_fail( aluno && dados && foco && tex );

   // 1. Lógica de Rotação transferida do LaTeX para o C
   int   rotacao = ( direcao == 'h' ) ? 0 : 90;
   float xscale = ( direcao == 'h' ) ? 0.6 : -0.6;
   float yscale = ( direcao == 'h' ) ? 0.6 : 0.6;

   // Injeção direta dos valores calculados
   g_string_append_printf( tex, "\\begin{tikzpicture}[baseline=(current bounding box.center), rotate=%d, xscale=%.4f, yscale=%.4f]\n",
                           rotacao, xscale, yscale );

   // Insere a palavra respostas
   g_string_append_printf( tex, "\\node[color=CorSerie,rotate=%d,inner sep=0pt] at (0.32,-5) {\\bf\\Large RESPOSTAS};\n", 90 - rotacao );

   // 2. Lógica if/else (antigo \ifthenelse) trazida para o C
   if ( direcao == 'h' ) {
      g_string_append( tex, "\\draw[CorSerie] (1,-10.3) -- (13,-10.3);\n" );
      // if ( assinalar_nome_numero ) {
      //    g_string_append_printf( tex, "\\node[inner sep=0pt,color=black] at (7,-9.9) {\\normalsize %s};\n", aluno );
      // }
      g_string_append( tex, "\\node[color=CorSerie,inner sep=0pt] at (7,-10.6) {\\scriptsize NOME DO ALUNO(A)};\n" );

      // g_string_append( tex, "\\draw[line width=2] ( 0,  0) circle (0.45) ( 0,  0) circle (0.25); \\fill ( 0,  0) circle (0.1);\n" );
      // g_string_append( tex, "\\draw[line width=2] ( 0,-11) circle (0.45) ( 0,-11) circle (0.25); \\fill ( 0,-11) circle (0.1);\n" );
      // g_string_append( tex, "\\draw[line width=2] (14,-11) circle (0.45) (14,-11) circle (0.25); \\fill (14,-11) circle (0.1);\n" );
      // g_string_append( tex, "\\draw[line width=2] (14,  0) circle (0.45) (14,  0) circle (0.25); \\fill (14,  0) circle (0.1);\n" );

      g_string_append( tex, "\\fill (0,0) rectangle (0.65,-0.65);\n" );
      g_string_append( tex, "\\fill (0,-11) rectangle (0.65,-10.35);\n" );
      g_string_append( tex, "\\fill (14,-11) rectangle (13.35,-10.35);\n" );
      g_string_append( tex, "\\fill (14,0) rectangle (13.35,-0.65);\n" );

   } else {
      g_string_append( tex, "\\draw[CorSerie] (14.3,-1.0) -- (14.3,-9.0);\n" );
      // if ( assinalar_nome_numero ) {
      //    g_string_append_printf( tex, "\\node[inner sep=0pt,color=black] at (13.9,-5) {\\normalsize %.32s};\n", aluno );
      // }
      g_string_append( tex, "\\node[color=CorSerie,inner sep=0pt] at (14.6,-5) {\\scriptsize NOME DO ALUNO(A)};\n" );

      // g_string_append( tex, "\\draw[line width=2] ( 0,  0) circle (0.45) ( 0,  0) circle (0.25); \\fill ( 0,  0) circle (0.1);\n" );
      // g_string_append( tex, "\\draw[line width=2] ( 0,-10) circle (0.45) ( 0,-10) circle (0.25); \\fill ( 0,-10) circle (0.1);\n" );
      // g_string_append( tex, "\\draw[line width=2] (15,-10) circle (0.45) (15,-10) circle (0.25); \\fill (15,-10) circle (0.1);\n" );
      // g_string_append( tex, "\\draw[line width=2] (15,  0) circle (0.45) (15,  0) circle (0.25); \\fill (15,  0) circle (0.1);\n" );

      g_string_append( tex, "\\fill (0,0) rectangle (0.65,-0.65);\n" );
      g_string_append( tex, "\\fill (0,-10) rectangle (0.65,-9.35);\n" );
      g_string_append( tex, "\\fill (15,-10) rectangle (14.35,-9.35);\n" );
      g_string_append( tex, "\\fill (15,0) rectangle (14.35,-0.65);\n" );
   }


   uint8_t turma = foco->turma;
   uint8_t disc  = foco->disciplina;
   uint8_t per   = foco->periodo;
   uint8_t seq   = dados->iprova; // Mudei de iprova-1 para iprova apenas, para evitar payload válido nulo.

   // ATENÇÃO: gerar_matriz_identificacao também precisará receber GString *tex
   gerar_matriz_identificacao( tex, id, turma, disc, per, seq );


   // Retângulo e linhas básicas
   g_string_append( tex, "\\draw[CorSerie] (1,-1) rectangle (13,-9);\n" );
   g_string_append( tex, "\\draw[CorSerie] (1,-7) -- (13,-7);\n" );
   g_string_append( tex, "\\draw[CorSerie] (3,-1) -- (3,-9);\n" );
   g_string_append( tex, "\\draw[CorSerie] (3,-2) -- (13,-2);\n" );

   // Linhas verticais das alternativas
   for ( int i = 0; i < 9; i++ ) {
      g_string_append_printf( tex, "\\draw[CorSerie] (4+%d,-1) -- (4+%d,-7);\n", i, i );
   }

   // Indicação do número da chamada
   g_string_append( tex, "\\node[color=CorSerie,inner sep=0pt] at (2,-8) {\\Huge{Nº}};\n" );

   // Período
   g_string_append_printf( tex, "\\node[color=CorSerie,inner sep=0pt] at (11,-7.5) {\\small%dº};\n", foco->periodo + 1 );

   // Prova
   g_string_append_printf( tex, "\\node[color=CorSerie,inner sep=0pt] at (12,-7.5) {\\small%d};\n", dados->iprova );

   // Matriz principal de questões e alternativas
   for ( int i = 0; i < NTI; i++ ) { // NTI=10 questões

      // Insere o número das questões
      g_string_append_printf( tex, "\\node[color=CorSerie,inner sep=0pt] at (3.5+%d,-1.5) {\\small\\bf%.2d};\n", i, i + 1 );

      for ( int j = 0; j < 2; j++ ) {
         if ( j != 0 || i < 7 ) {
            g_string_append_printf( tex, "\\draw[color=CorSerie,line width=0.75pt] (3.5+%d,-7.5-%d) circle (0.3);\n", i, j );
            g_string_append_printf( tex, "\\node[color=CorSerie,inner sep=0pt] at  (3.5+%d,-7.5-%d) {\\tiny%d};\n", i, j, i );
         }
      }

      for ( int j = 0; j < 5; j++ ) { // 5 alternativas
         g_string_append_printf( tex, "\\draw[color=CorSerie,line width=0.75pt] (3.5+%d,-2.5-%d) circle (0.3);\n", i, j );
         g_string_append_printf( tex, "\\node[color=CorSerie,inner sep=0pt] at  (3.5+%d,-2.5-%d) {\\tiny{%c}};\n", i, j, 65 + j );
      }

      // Assinalar alternativas para teste
      // float y_aleatorio = -2.5 - g_random_int_range( 0, 4 );
      // g_string_append_printf( tex, "\\fill[color=black] (%.2f,%.2f) circle (0.24);\n", 3.5 + i, y_aleatorio );
   }

   if ( assinalar_nome_numero ) {
      g_string_append_printf( tex, "\\fill (3.5+%d,-7.5 ) circle (0.24) (3.5+%d,-8.5) circle (0.24);\n", numero / 10, numero % 10 );
   }

   g_string_append( tex, "\\end{tikzpicture}\n" );
}
//========================================================================================================//






int imagens_corrigidas( const char *gab, const MapeamentoGabarito *map, const AppContext *ctx, const char *nome_base ) {

   if ( !gab || !map || !ctx ) return -1;

   const char *LATEX_PREAMBLE_TEMPLATE =
      "\\documentclass[11pt]{report}\n"
      "\\usepackage[utf8]{inputenc}\n"
      "\\usepackage[T1]{fontenc}\n"
      "\\usepackage[brazil]{babel}\n"
      "\\usepackage{cmbright, xcolor, ifthen, setspace, graphicx, tikz}\n"
      "\\usetikzlibrary{calc}\n\n"
      "\\usepackage[bottom=0cm,top=0cm,left=0cm,right=0cm]{geometry}\n"
      "\\geometry{paperwidth=%dcm,paperheight=%dcm}\n\n"
      "\\pagestyle{empty}\n\n"
      "\\definecolor{NeonVerde}{cmyk}{0.65,0,1,0.2}\n"
      "\\definecolor{LimaoFluo}{cmyk}{0.40,0,1,0}\n"
      "\\definecolor{VermelhoLaser}{cmyk}{0,1,1,0}\n"
      "\\definecolor{AzulProfessor}{cmyk} {1,0.50,0,0.33}\n"
      "\\definecolor{VerdeEscola}{cmyk} {1,0,1,0.47}\n\n"
      "% MACRO: Símbolo de Correto\n"
      "\\def\\correto{\n"
      "  \\fill[rounded corners=6, bottom color=NeonVerde, top color=LimaoFluo] ($(-0.43,-0.43)+(P)$) rectangle ($(0.43,0.43)+(P)$);\n"
      "  \\fill[rounded corners=1, color=white] ($(-0.172,-0.258)+(P)$) -- ($(-0.0215,-0.258)+(P)$) -- ($(0.344,0.258)+(P)$) -- ($(0.1935,0.258)+(P)$) -- cycle;\n"
      "  \\fill[rounded corners=1, color=white] ($(-0.0215,-0.258)+(P)$) -- ($(-0.172,-0.258)+(P)$) -- ($(-0.344,0.00)+(P)$) -- ($(-0.1935,0.00)+(P)$) -- cycle;\n"
      "}\n\n"
      "% MACRO: Símbolo de Incorreto\n"
      "\\def\\incorreto{\n"
      "  \\fill[rounded corners=1, color=VermelhoLaser] ($(-0.35,-0.35)+(-45:0.0875)+(P)$) -- ($(-0.35,-0.35)+(135:0.0875)+(P)$) -- ($(0.35,0.35)+(135:0.0875)+(P)$) -- ($(0.35,0.35)+(-45:0.0875)+(P)$) -- cycle;\n"
      "  \\fill[rounded corners=1, color=VermelhoLaser] ($(-0.35,0.35)+(45:0.0875)+(P)$) -- ($(-0.35,0.35)+(225:0.0875)+(P)$) -- ($(0.35,-0.35)+(225:0.0875)+(P)$) -- ($(0.35,-0.35)+(45:0.0875)+(P)$) -- cycle;\n"
      "}\n"
      "\\pagecolor{yellow!20}\n";

   const InterfaceDados *dados  = &ctx->dados;
   const CalendarioData *data   = &ctx->data;

   // 1. BLINDAGEM DE MEMÓRIA: Previne acesso negativo ou além do limite na struct 'diario'
   if ( map->num < 1 || map->num > dados->qtd_alunos_total ) {
      g_printerr( "[ERRO CRÍTICO] Número do aluno (%d) inválido. Cancelando renderização LaTeX.\n", map->num );
      return -1;
   }

   const char *meses[] = {
      "janeiro", "fevereiro", "março", "abril", "maio", "junho",
      "julho", "agosto", "setembro", "outubro", "novembro", "dezembro"
   };

   // Caminho do arquivo seguro com g_strdup_printf
   g_autofree char *arquivo = g_strdup_printf( "./dados/temporarios/%s.tex", nome_base );

   // Pré-alocação otimizada (16KB) na RAM
   g_autoptr( GString ) tex = g_string_sized_new( 16384 );

   int largura = ( map->direcao == 'h' ) ? 18 : 14;
   int altura  = ( map->direcao == 'h' ) ? 15 : 19;

   // 2. INJEÇÃO DO CABEÇALHO LATEX
   g_string_append_printf( tex, LATEX_PREAMBLE_TEMPLATE, largura, altura );

   g_autofree gchar *raiz_projeto = g_get_current_dir();
   g_autofree char *imagem = g_build_filename( raiz_projeto, "dados", "gabaritos", dados->ano,
                             dados->escola, "imagens", map->nome_img, NULL );

   g_string_append_printf( tex,
           "\\begin{document}\n"
           "{\\noindent\\small\n"
           "\\begin{tikzpicture}[baseline=(current bounding box.center)]\n"
           "\\node at (%.4f,%.4f) {\\includegraphics[width=%dcm, height=%dcm]{\"%s\"}};\n",
           0.5 * largura, -0.5 * altura, largura - 4, altura - 4, imagem );

   // Camada de "Máscara" Branca (Limpeza das bordas do escaneamento para âncoras de 7 mm)
   g_string_append_printf( tex, "\\filldraw[white] (2,-2)rectangle(2.7,-2.7) (%d,-2)rectangle(%.4f,-2.7) (%d,%d)rectangle(%.4f,%.4f) (2,%d)rectangle(2.7,%.4f);\n", largura - 2, largura - 2.7, largura - 2, 2 - altura, largura - 2.7, 2.7 - altura, 2 - altura, 2.7 - altura );

   // 3. GRADE DE QUADRADOS (Payload Visual)
   for ( int i = 0; i < 27; i++ ) {
      int linha  = ( map->direcao == 'h' ) ? i / 3 : i % 3;
      int coluna = ( map->direcao == 'h' ) ? i % 3 : i / 3;

      int bit_ativo = ( map->payload >> i ) & 1;

      if ( bit_ativo ) {
         // Multiplicação e soma exatas primeiro, divisão apenas no final
         double x1 = ( 37.0 + 8.0 * coluna ) / 12.0;
         double y1 = -( 37.0 + 8.0 * linha ) / 12.0;

         double x3 = ( 43.0 + 8.0 * coluna ) / 12.0;
         double y3 = -( 43.0 + 8.0 * linha ) / 12.0;

         g_string_append_printf( tex, "\\draw[red,thick] (%.4f,%.4f) rectangle (%.4f,%.4f);\n", x1, y1, x3, y3 );
      }
   }

   // 4. AVALIAÇÃO DAS RESPOSTAS E PINTURA DO GABARITO
   int nota = 0;

   // Assumindo que NTI (Número Total de Itens) seja uma macro definida no escopo superior
   for ( int j = 0; j < NTI; j++ ) {
      double linha  = ( map->direcao == 'h' ) ? -4.5 - gab[j] + 65 : -5.5 - j;
      double coluna = ( map->direcao == 'h' ) ?  5.5 + j : 4.5 + gab[j] - 65;
      double x      = ( map->direcao == 'h' ) ?  5.5 + j : 2.5;
      double y      = ( map->direcao == 'h' ) ? -2.5 : -5.5 - j;

      g_string_append_printf( tex, "\\coordinate (P) at (%.4f,%.4f);\n", x, y );

      if ( gab[j] == map->resp[j] ) {
         nota++;
         g_string_append_printf( tex, "\\correto\n\\draw[line width=4, green] (%.4f,%.4f) circle (0.35);\n", coluna, linha );
      } else {
         g_string_append_printf( tex, "\\incorreto\n\\draw[line width=4, red] (%.4f,%.4f) circle (0.35);\n", coluna, linha );
      }
   }

   // 5. INJEÇÃO DOS METADADOS E TEXTOS INSTITUCIONAIS
   g_string_append_printf( tex, "\\node[left] at (%d,-0.4) {\\Large São Luís, %d de %s de %d};\n",
            largura, data->dia, meses[data->mes - 1], data->ano );
   g_string_append_printf( tex, "\\node[left] at (%d,-1.3) {\\Large IDENTIFICADOR $\\to$ \\bf\\Huge %d};\n", largura, map->id );

   g_string_append_printf( tex, "\\node[right,color=AzulProfessor] at (0.04,-0.4) {\\Large\\bf %s};\n", dados->escola );
   g_string_append_printf( tex, "\\node[right,color=AzulProfessor] at (0.04,-1.0) {\\Large\\textbf{Gest.} %s};\n", dados->gestor );
   g_string_append_printf( tex, "\\node[right,color=AzulProfessor] at (0.04,-1.6) {\\Large\\textbf{Prof.} %s};\n\n", dados->professor );

   // Formatação Dinâmica do Título da Prova
   char titulo_prova[512];
   if ( dados->periodo[0] == 'R' ) {
      snprintf( titulo_prova, sizeof( titulo_prova ), "%s de %s / \\textbf{%s}",
                dados->periodo, dados->disciplina, dados->ano );
   } else {
      snprintf( titulo_prova, sizeof( titulo_prova ), "%s Prova de %s do \\textbf{%s / %s}",
                dados->prova_sequencia, dados->disciplina, dados->periodo, dados->ano );
   }

   g_string_append_printf( tex, "\\node[right,color=VerdeEscola] at (0.04,%.4f) {\\Large %s};\n", 1.6 - altura, titulo_prova );
   g_string_append_printf( tex, "\\node[right,color=VerdeEscola] at (0.04,%.4f) {\\Large Turma: {\\bf %s}};\n", 1.0 - altura, dados->turma );

   // Uso seguro de memória com g_autofree para a string em UpperCase
   const FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, map->num - 1 );
   g_autofree gchar *nome_aluno = g_utf8_strup( ficha->aluno, -1 );
   g_string_append_printf( tex, "\\node[right,color=VerdeEscola] at (0.04,%.4f) {\\Large %s $\\to$ Nº: {\\bf\\Huge %.2d}};\n", 0.4 - altura, nome_aluno, map->num );

   // Resultado Final
   g_string_append_printf( tex, "\\node[color=blue,left] at (%d,%.4f) {\\Large NOTA $\\to$ \\textbf{\\fontsize{12mm}{15mm}\\selectfont %d}};\n\n", largura, 1.3 - altura, nota );

   // 6. FECHAMENTO DO ARQUIVO
   g_string_append( tex, "\\end{tikzpicture}}\n" );
   g_string_append( tex, "\\end{document}\n" );

   // 7. GRAVAÇÃO ATÔMICA E SEGURA NO DISCO
   GError *error = NULL;
   if ( !g_file_set_contents( arquivo, tex->str, tex->len, &error ) ) {
      g_printerr( "[ERRO] Não foi possível salvar o arquivo latex (%s): %s\n", arquivo, error->message );
      g_error_free( error );
      return -1;
   }

   return nota;
}








void anexar_circulo_alternativa_colorida( GString *tex, gint intensidade, const gchar *texto ) {
   // A lógica de contraste é calculada instantaneamente no C
   const gchar *cor_texto = ( intensidade > 50 ) ? "white" : "black";

   g_string_append_printf( tex,
      "\\tikz[baseline=(char.base)]{\n"
      "  \\node[\n"
      "    shape=circle, draw=CorSerie, fill=CorSerie!%d,\n"
      "    line width=0.8pt, inner sep=1.5pt, font=\\bf\\small, text=%s\n"
      "  ] (char) {%s};\n"
      "}",
      intensidade, cor_texto, texto
   );
}




//=======================================================================================================
// Função C pura para renderizar o tema decorativo de página "Ondas"
void anexar_tema_pagina_ondas( GString *tex, gint k ) {

   // 1. Cálculos matemáticos resolvidos nativamente em C
   double largura = 17.5 + 1.5 * k;
   int amostras = (int)( 15.0 * largura );
   double passo = largura / amostras;

   // 2. Abertura do ambiente TikZ
   g_string_append( tex, "\\begin{tikzpicture}[remember picture, overlay]\n" );

   // ==========================================
   // --- Barra Superior ---
   // ==========================================
   g_string_append_printf( tex, "  \\fill[CorSerie!15] (0,0) rectangle (%.4f, -0.28);\n", largura );

   // MÁGICA: preaction + coordenadas pré-calculadas no C
   g_string_append( tex, "  \\draw[CorSerie, line width=0.8pt, preaction={draw=white, line width=2.8pt}, smooth] plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = -0.14 - 0.06 * sin( 12.0 * x ); // Calculado instantaneamente pela CPU
      g_string_append_printf( tex, "(%.3f,%.3f) ", x, y );
   }
   g_string_append( tex, "};\n" );

   // ==========================================
   // --- Barra Inferior ---
   // ==========================================
   g_string_append_printf( tex, "  \\fill[CorSerie!15] (0,-1) rectangle (%.4f, -1.28);\n", largura );

   g_string_append( tex, "  \\draw[CorSerie, line width=0.8pt, preaction={draw=white, line width=2.8pt}, smooth] plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = -1.14 - 0.06 * sin( 12.0 * x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", x, y );
   }
   g_string_append( tex, "};\n" );

   // Fechamento
   g_string_append( tex, "\\end{tikzpicture}\n" );
}

//-------------------------------------------------------------------------------------------------------
// Função C pura para renderizar o cabeçalho "Ondas" da questão
void anexar_cabecalho_questao_ondas( GString *tex, gint k, gint num_questao ) {

   // 1. Cálculos matemáticos resolvidos nativamente em C
   double largura_barra = 3.2 + 3.25 * k;
   int amostras = (int)( 15.0 * largura_barra );
   double passo = largura_barra / amostras;
   int num_pontos = 59 + 32 * k;

   // 2. Abertura do ambiente TikZ
   g_string_append( tex, "\\item{\\noindent\\tikz[baseline=(char.base)]{\n" );

   // Nó do texto da questão
   g_string_append_printf( tex, "  \\node[inner sep=0pt, font=\\bf\\small, anchor=west] (char) at (0,0) {\\resizebox{2.6cm}{0.38cm}{QUEST\\~AO %.2d}\\,\\,};\n", num_questao );

   // Retângulo de fundo
   g_string_append_printf( tex, "  \\fill[CorSerie!15] (2.7, -0.14) rectangle (%.4f, 0.14);\n", 2.7 + largura_barra );

   // MÁGICA: preaction + coordenadas pré-calculadas
   g_string_append( tex, "  \\draw[CorSerie, line width=0.8pt, preaction={draw=white, line width=2.8pt}, smooth] plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = 0.06 * sin( 12.0 * x ); // O C cuida da matemática
      g_string_append_printf( tex, "(%.3f,%.3f) ", x + 2.7, y ); // O offset +2.7 de X aplicado aqui
   }
   g_string_append( tex, "};\n" );

   // 3. O loop do TikZ (\foreach) convertido em pura força bruta no C
   for ( int i = 0; i <= num_pontos; i++ ) {
      g_string_append_printf( tex, "  \\fill[CorSerie!50] (%.4f, -0.35) circle (0.7pt);\n", 0.1 * i );
   }

   // Fechamento
   g_string_append( tex, "}}\\\\\n" );
}
//=======================================================================================================




//=======================================================================================================
// Função C pura para renderizar o tema da página "Quadrados"
void anexar_tema_pagina_quadrados( GString *tex, gint intensidade ) {

   // 1. Cálculos matemáticos resolvidos nativamente em C
   int n = 86; // Número de quadrados
   double s = ( 19.0 / ( n + 1 ) ) - 0.14; // Cálculo de espaçamento da largura

   // 2. Linhas horizontais principais na cor forte
   g_string_append_printf( tex,
      "  \\draw[line width=1pt, color=CorSerie] (0,0) -- (19,0)    (0,-0.28) -- (19,-0.28);\n"
      "  \\draw[line width=1pt, color=CorSerie] (0,-1.00) -- (19,-1.00)    (0,-1.28) -- (19,-1.28);\n"
   );

   // 3. Loop desenrolado no C (Aniquila o gargalo do \foreach no LaTeX)
   for ( int i = 0; i <= n; i++ ) {
      double x_left = 0.5 * s + ( 0.14 + s ) * i;
      double x_right = x_left + 0.14;

      // Quadrado superior
      g_string_append_printf( tex,
         "  \\draw[line width=0.8pt, color=CorSerie, fill=CorSerie!%d] (%.4f, -0.07) rectangle (%.4f, -0.21);\n",
         intensidade, x_left, x_right
      );

      // Quadrado inferior
      g_string_append_printf( tex,
         "  \\draw[line width=0.8pt, color=CorSerie, fill=CorSerie!%d] (%.4f, -1.07) rectangle (%.4f, -1.21);\n",
         intensidade, x_left, x_right
      );
   }
}
//-------------------------------------------------------------------------------------------------------
// Função C pura para renderizar o cabeçalho "Quadrados" da questão
void anexar_cabecalho_questao_quadrados( GString *tex, const gchar *cor_texto, gint k, gint num_questao ) {

   // 1. Cálculos do layout baseados em K
   int k_math = 15 * k;
   int n = 10 + k_math;

   g_string_append( tex, "\\item{\\noindent\\tikz[baseline=(char.base)]{\n" );

   // 2. Caixa Principal com cantos arredondados (Node)
   g_string_append_printf( tex,
      "  \\node[\n"
      "    shape=rectangle, rounded corners, draw=CorSerie,\n"
      "    fill=CorSerie!10, align=center, inner sep=2pt,\n"
      "    line width=1pt, font=\\bf\\small, text=%s\n"
      "  ] (char) {\\hspace{1mm}\\resizebox{2.6cm}{0.38cm}{QUEST\\~AO %.2d}\\hspace{1mm}};\n",
      cor_texto, num_questao
   );

   // 3. Mini-Quadrados da Esquerda
   for ( int i = 0; i <= 2; i++ ) {
      double x_left = -2.10 + 0.21 * i;
      double x_right = -1.96 + 0.21 * i;

      g_string_append_printf( tex,
         "  \\draw[line width=0.8pt, color=CorSerie, fill=CorSerie!10] (%.4f, -0.07) rectangle (%.4f, +0.07);\n",
         x_left, x_right
      );
   }

   // 4. Mini-Quadrados da Direita
   for ( int i = 0; i <= n; i++ ) {
      double x_left = 1.54 + 0.21 * i;
      double x_right = 1.68 + 0.21 * i;

      g_string_append_printf( tex,
         "  \\draw[line width=0.8pt, color=CorSerie, fill=CorSerie!10] (%.4f, -0.07) rectangle (%.4f, +0.07);\n",
         x_left, x_right
      );
   }

   // 5. Linhas Laterais duplas calculadas
   double limite_direito = 3.815 + 0.21 * k_math;

   g_string_append_printf( tex,
      "  \\draw[line width=0.8pt, color=CorSerie]\n"
      "  (-2.135,-0.14) -- (-1.47,-0.14) (1.475,-0.14) -- (%.4f,-0.14)\n"
      "  (-2.135,+0.14) -- (-1.47,+0.14) (1.475,+0.14) -- (%.4f,+0.14);\n",
      limite_direito, limite_direito
   );

   // Fechamento da sub-árvore TikZ
   g_string_append( tex, "}}\\\\\n" );
}
//=======================================================================================================





//=======================================================================================================
void anexar_tema_pagina_trapezios( GString *tex, gint k ) {
   double n = 42.0 + 4.0 * k;
   double largura = 0.7 - 0.1 * k + 0.4 * n;

   g_string_append_printf( tex,
      "\\begin{tikzpicture}[remember picture, overlay]\n"
      "  \\fill[CorSerie!30] (0,0) rectangle (%.4f, -0.24);\n"
      "  \\fill[CorSerie!30] (0,-0.9) rectangle (%.4f, -1.14);\n",
      largura, largura
   );

   for ( int i = 0; i <= (int)n; i++ ) {
      double offset = -0.05 * k + 0.4 * i;
      // Recortes superiores (Branco)
      g_string_append_printf( tex,
         "  \\fill[white] (%.4f,-0.24)--(%.4f,-0.14)--(%.4f,-0.04)--(%.4f,-0.24)--cycle;\n",
         0.3 + offset, 0.2 + offset, 0.3 + offset, 0.5 + offset
      );
      // Recortes inferiores (Branco)
      g_string_append_printf( tex,
         "  \\fill[white] (%.4f,-1.14)--(%.4f,-1.04)--(%.4f,-0.94)--(%.4f,-1.14)--cycle;\n",
         0.3 + offset, 0.2 + offset, 0.3 + offset, 0.5 + offset
      );
   }
   g_string_append( tex, "\\end{tikzpicture}\n" );
}
//-------------------------------------------------------------------------------------------------------
void anexar_cabecalho_questao_trapezios( GString *tex, gint k, gint num_questao ) {
   double largura = (101.0 / 30.0) + (91.0 / 30.0) * k;
   int nn = 7 + 8 * k;
   double s = ( largura + 0.1 - 0.4 * (nn + 1) ) / 2.0;
   int num_pontos = 60 + 30 * k;

   g_string_append_printf( tex,
      "\\item{\\noindent\\tikz[baseline=(char.base)]{\n"
      "  \\node[inner sep=0pt, anchor=west] (char) at (0,-0.1) {\\resizebox{2.6cm}{0.35cm}{\\bf Quest\\~ao %.2d}\\,};\n"
      "  \\fill[CorSerie!30] (2.7,0) rectangle (%.4f, -0.24);\n",
      num_questao, 2.7 + largura
   );

   for ( int i = 0; i <= nn; i++ ) {
      double x = s + 2.7 + 0.4 * i;
      g_string_append_printf( tex,
         "  \\fill[white] (%.4f,-0.24)--(%.4f,-0.14)--(%.4f,-0.04)--(%.4f,-0.24)--cycle;\n",
         x + 0.1, x, x + 0.1, x + 0.3
      );
   }

   for ( int i = 0; i <= num_pontos; i++ ) {
      g_string_append_printf( tex, "  \\fill[CorSerie!50] (%.4f, -0.34) circle (0.6pt);\n", 0.02118 + 0.1 * i );
   }
   g_string_append( tex, "}}\\\\\n" );
}
//=======================================================================================================




//=======================================================================================================
void anexar_tema_pagina_linhas( GString *tex, gint k ) {
   int n = 145 + 13 * k;
   double largura = 17.5 + 1.5 * k;
   double s = largura / ( n + 1 );

   g_string_append( tex, "\\begin{tikzpicture}[remember picture, overlay]\n" );

   for ( int i = 0; i <= n; i++ ) {
      double x = 0.5 * s + s * i;
      g_string_append_printf( tex,
         "  \\draw[line width=0.8pt, color=CorSerie] (%.4f, -0.0141) -- (%.4f, -0.2541);\n"
         "  \\draw[line width=0.8pt, color=CorSerie] (%.4f, -0.9000) -- (%.4f, -1.1400);\n",
         x, x, x, x
      );
   }

   g_string_append_printf( tex,
      "  \\draw[line width=0.8pt, color=CorSerie] (0,-0.0141) -- (%.4f,-0.0141) (0,-0.2541) -- (%.4f,-0.2541);\n"
      "  \\draw[line width=0.8pt, color=CorSerie] (0,-0.9000) -- (%.4f,-0.9000) (0,-1.1400) -- (%.4f,-1.1400);\n"
      "\\end{tikzpicture}\n",
      largura, largura, largura, largura
   );
}
//-------------------------------------------------------------------------------------------------------
void anexar_cabecalho_questao_linhas( GString *tex, const gchar *cor_texto, gint k, gint num_questao ) {
   int n = 21 + 25 * k;
   double offset_k = 3.0 * k; // 0.12 * 25 = 3

   g_string_append_printf( tex,
      "\\item{\\noindent\\tikz[baseline=(char.base)]{\n"
      "  \\node[shape=rectangle, rounded corners, draw=CorSerie, fill=CorSerie!10, align=center, inner sep=2pt, line width=1pt, font=\\bf\\small, text=%s]\n"
      "    (char) {\\hspace{1mm}\\resizebox{2.6cm}{0.38cm}{QUEST\\~AO %.2d}\\hspace{1mm}};\n",
      cor_texto, num_questao
   );

   for ( int i = 0; i <= 4; i++ ) {
      g_string_append_printf( tex, "  \\draw[line width=0.8pt, color=CorSerie] (%.4f, -0.12) -- (%.4f, +0.12);\n", -2.0 + 0.12 * i, -2.0 + 0.12 * i );
   }

   for ( int i = 1; i <= n; i++ ) {
      g_string_append_printf( tex, "  \\draw[line width=0.8pt, color=CorSerie] (%.4f, -0.12) -- (%.4f, +0.12);\n", 1.4 + 0.12 * i, 1.4 + 0.12 * i );
   }

   g_string_append_printf( tex,
      "  \\draw[line width=0.8pt, color=CorSerie]\n"
      "    (-2.06,-0.12) -- (-1.40,-0.12) (1.40,-0.12) -- (%.4f,-0.12)\n"
      "    (-2.06,+0.12) -- (-1.40,+0.12) (1.40,+0.12) -- (%.4f,+0.12);\n"
      "}}\\\\\n",
      3.98 + offset_k, 3.98 + offset_k
   );
}
//=======================================================================================================





//=======================================================================================================
void anexar_tema_pagina_senoides( GString *tex, gint k ) {
   // 1. Cálculos de largura e passos transferidos para o C
   double largura = 17.5 + 1.5 * k;
   int amostras = (int)( 15.0 * largura );
   double passo = largura / amostras;

   g_string_append( tex, "\\begin{tikzpicture}[remember picture, overlay]\n" );
   g_string_append( tex, "  \\draw[line width=0.8pt, CorSerie, smooth]\n" );

   // Onda 1: Superior Positiva
   g_string_append( tex, "    plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = -0.1341 + 0.088 * sin( 11.0 * x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", x, y );
   }
   g_string_append( tex, "}\n" );

   // Onda 2: Superior Negativa (Espelhada)
   g_string_append( tex, "    plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = -0.1341 - 0.088 * sin( 11.0 * x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", x, y );
   }
   g_string_append( tex, "}\n" );

   // Onda 3: Inferior Positiva
   g_string_append( tex, "    plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = -1.0200 + 0.088 * sin( 11.0 * x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", x, y );
   }
   g_string_append( tex, "}\n" );

   // Onda 4: Inferior Negativa (Espelhada)
   g_string_append( tex, "    plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = -1.0200 - 0.088 * sin( 11.0 * x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", x, y );
   }
   g_string_append( tex, "};\n" ); // Fim do comando \draw

   // Linhas retas de contenção (Superior e Inferior)
   g_string_append_printf( tex,
      "  \\draw[line width=0.8pt, color=CorSerie] (0,-0.0141) -- (%.4f,-0.0141) (0,-0.2541) -- (%.4f,-0.2541);\n"
      "  \\draw[line width=0.8pt, color=CorSerie] (0,-0.9000) -- (%.4f,-0.9000) (0,-1.1400) -- (%.4f,-1.1400);\n"
      "\\end{tikzpicture}\n",
      largura, largura, largura, largura
   );
}

//-------------------------------------------------------------------------------------------------------
void anexar_cabecalho_questao_senoides( GString *tex, const gchar *cor_texto, gint k, gint num_questao ) {
   // 1. Cálculos geométricos no C
   double largura = 3.15 + 3.05 * k;
   int amostras = (int)( 15.0 * largura );
   double passo = largura / amostras;

   g_string_append( tex, "\\item{\\noindent\\tikz[baseline=(char.base)]{\n" );

   // Nó do texto
   g_string_append_printf( tex,
      "  \\node[shape=rectangle, rounded corners, draw=CorSerie, fill=CorSerie!10, align=center, inner sep=2pt, line width=1pt, font=\\bf\\small, text=%s]\n"
      "    (char) {\\hspace{1mm}\\resizebox{2.6cm}{0.38cm}{QUEST\\~AO %.2d}\\hspace{1mm}};\n",
      cor_texto, num_questao
   );

   g_string_append( tex, "  \\draw[line width=0.8pt, CorSerie, smooth]\n" );

   // Onda 1: Positiva com offset X
   g_string_append( tex, "    plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = +0.088 * sin( 11.0 * x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", x + 1.45, y ); // Offset X de 1.45 aplicado diretamente
   }
   g_string_append( tex, "}\n" );

   // Onda 2: Negativa com offset X
   g_string_append( tex, "    plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = -0.088 * sin( 11.0 * x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", x + 1.45, y );
   }
   g_string_append( tex, "};\n" ); // Fim do comando \draw

   // Linhas laterais que fecham a caixa
   double limite_direito = 1.46 + largura;
   g_string_append_printf( tex,
      "  \\draw[line width=0.8pt, color=CorSerie]\n"
      "    (1.45,-0.12) -- (%.4f,-0.12) (1.45,+0.12) -- (%.4f,+0.12);\n"
      "}}\\\\\n",
      limite_direito, limite_direito
   );
}
//=======================================================================================================




//=======================================================================================================
void anexar_tema_pagina_circulos( GString *tex, gint k ) {
   double largura = 17.5 + 1.5 * k;
   int n = 34 + 3 * k;
   double s = ( largura - 0.24 ) / n;

   // Mantemos o gradiente bonito original, mas agora usando a cor principal desvanecendo para branco!
   g_string_append_printf( tex,
      "\\begin{tikzpicture}[remember picture, overlay]\n"
      "  \\fill[left color=CorSerie, right color=white] (0,0) rectangle (%.4f, -0.24);\n"
      "  \\fill[left color=CorSerie, right color=white] (0,-0.9) rectangle (%.4f, -1.14);\n",
      largura, largura
   );

   for ( int i = 0; i <= n; i++ ) {
      double x = s * i + 0.12;
      g_string_append_printf( tex,
         "  \\fill[white] (%.4f, -0.12) circle (0.12);\n"
         "  \\fill[white] (%.4f, -1.02) circle (0.12);\n",
         x, x
      );
   }
   g_string_append( tex, "\\end{tikzpicture}\n" );
}
//-------------------------------------------------------------------------------------------------------
void anexar_cabecalho_questao_circulos( GString *tex, gint k, gint num_questao ) {
   double largura = 3.23 + 3.02 * k;
   int n_circ = 6 + 6 * k;
   int n_pontos = 60 + 30 * k;

   g_string_append_printf( tex,
      "\\item{\\noindent\\hspace{-1.5mm}\\tikz[baseline=(char.base)]{\n"
      "  \\node[inner sep=0pt, anchor=west] (char) at (0,0.13) {\\resizebox{2.6cm}{0.35cm}{\\bf Quest\\~ao %.2d}\\,};\n"
      "  \\fill[left color=CorSerie, right color=white] (2.7, 0) rectangle (%.4f, 0.24);\n",
      num_questao, 2.7 + largura
   );

   for ( int i = 0; i <= n_circ; i++ ) {
      g_string_append_printf( tex, "  \\fill[white] (%.4f, 0.12) circle (0.12);\n", (0.5 * i) + 2.82 );
   }

   for ( int i = 0; i <= n_pontos; i++ ) {
      g_string_append_printf( tex, "  \\fill[CorSerie!50] (%.4f, -0.1) circle (0.6pt);\n", (0.1 * i) );
   }
   g_string_append( tex, "}}\\\\\n" );
}
//=======================================================================================================


