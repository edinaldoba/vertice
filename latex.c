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
static void gerar_matriz_identificacao( FILE *p, uint8_t id, uint8_t turma, uint8_t disc, uint8_t per, uint8_t seq ) {
   if ( !p ) return;

   // 1. Empacotando 23 bits em um inteiro de 32 bits
   uint32_t payload = 0;
   payload |= ( id    & 0x3F );     // 6 bits (0 a 5)
   payload |= ( turma & 0xFF ) << 6; // 8 bits (6 a 13)
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

         // Injeção limpa e direta no arquivo TikZ
         fprintf( p, "\\fill (%.4f,%.4f) rectangle (%.4f,%.4f);\n", x_start, y_start, x_end, y_end );
      }
   }
}






//========================================================================================================//
void quadro_de_respostas( FILE *p, const char *aluno, int numero, const uint8_t id, char direcao, bool assinalar_nome_numero,
                          const InterfaceDados *dados, const FocoCoordenadas *foco ) {

   // 1. Lógica de Rotação transferida do LaTeX para o C
   int  rotacao = ( direcao == 'h' ) ? 0 : 90;
   float xscale = ( direcao == 'h' ) ? 0.6 : -0.6;
   float yscale = ( direcao == 'h' ) ? 0.6 : 0.6;

   // Injeção direta dos valores calculados
   fprintf( p, "\\begin{tikzpicture}[baseline=(current bounding box.center), rotate=%d, xscale=%.4f, yscale=%.4f]\n",
            rotacao, xscale, yscale );

   // Insere a palavra respostas
   fprintf( p, "\\node[color=CorSerie,rotate=%d,inner sep=0pt] at (0.32,-5) {\\bf\\Large RESPOSTAS};\n", 90 - rotacao );

   // 2. Lógica if/else (antigo \ifthenelse) trazida para o C
   if ( direcao == 'h' ) {
      fprintf( p, "\\draw[CorSerie] (0,-10.3) -- (14,-10.3);\n" );
      if ( assinalar_nome_numero ) {
         fprintf( p, "\\node[inner sep=0pt,color=black] at (7,-9.9) {\\normalsize %s};\n", aluno );
      }
      fprintf( p, "\\node[color=CorSerie,inner sep=0pt] at (7,-10.6) {\\scriptsize NOME DO ALUNO(A)};\n" );

      fprintf( p, "\\draw[line width=2] ( 0,  0) circle (0.45) ( 0,  0) circle (0.25); \\fill ( 0,  0) circle (0.1);\n" );
      fprintf( p, "\\draw[line width=2] ( 0,-11) circle (0.45) ( 0,-11) circle (0.25); \\fill ( 0,-11) circle (0.1);\n" );
      fprintf( p, "\\draw[line width=2] (14,-11) circle (0.45) (14,-11) circle (0.25); \\fill (14,-11) circle (0.1);\n" );
      fprintf( p, "\\draw[line width=2] (14,  0) circle (0.45) (14,  0) circle (0.25); \\fill (14,  0) circle (0.1);\n" );

      // fprintf( p, "\\fill (0,0) rectangle (0.6,-0.6);\n" );
      // fprintf( p, "\\fill (0,-11) rectangle (0.6,-10.4);\n" );
      // fprintf( p, "\\fill (14,-11) rectangle (13.4,-10.4);\n" );
      // fprintf( p, "\\fill (14,0) rectangle (13.4,-0.6);\n" );

   } else {
      fprintf( p, "\\draw[CorSerie] (14.3,0) -- (14.3,-10);\n" );
      if ( assinalar_nome_numero ) {
         fprintf( p, "\\node[inner sep=0pt,color=black] at (13.9,-5) {\\normalsize %.32s};\n", aluno );
      }
      fprintf( p, "\\node[color=CorSerie,inner sep=0pt] at (14.6,-5) {\\scriptsize NOME DO ALUNO(A)};\n" );

      fprintf( p, "\\draw[line width=2] ( 0,  0) circle (0.45) ( 0,  0) circle (0.25); \\fill ( 0,  0) circle (0.1);\n" );
      fprintf( p, "\\draw[line width=2] ( 0,-10) circle (0.45) ( 0,-10) circle (0.25); \\fill ( 0,-10) circle (0.1);\n" );
      fprintf( p, "\\draw[line width=2] (15,-10) circle (0.45) (15,-10) circle (0.25); \\fill (15,-10) circle (0.1);\n" );
      fprintf( p, "\\draw[line width=2] (15,  0) circle (0.45) (15,  0) circle (0.25); \\fill (15,  0) circle (0.1);\n" );

      // fprintf( p, "\\fill (0,0) rectangle (0.6,-0.6);\n" );
      // fprintf( p, "\\fill (0,-10) rectangle (0.6,-9.4);\n" );
      // fprintf( p, "\\fill (15,-10) rectangle (14.4,-9.4);\n" );
      // fprintf( p, "\\fill (15,0) rectangle (14.4,-0.6);\n" );
   }


   uint8_t turma = foco->turma;
   uint8_t disc  = foco->disciplina;
   uint8_t per   = foco->periodo;
   uint8_t seq   = dados->iprova; // Mudei de iprova-1 para iprova apenas, para evitar payload válido nulo.
   gerar_matriz_identificacao( p, id, turma, disc, per, seq );


   // Retângulo e linhas básicas
   fprintf( p, "\\draw[CorSerie] (1,-1) rectangle (13,-9);\n" );
   fprintf( p, "\\draw[CorSerie] (1,-7) -- (13,-7);\n" );
   fprintf( p, "\\draw[CorSerie] (3,-1) -- (3,-9);\n" );
   fprintf( p, "\\draw[CorSerie] (3,-2) -- (13,-2);\n" );

   // Linhas verticais das alternativas
   for ( int i = 0; i < 9; i++ ) {
      fprintf( p, "\\draw[CorSerie] (4+%d,-1) -- (4+%d,-7);\n", i, i );
   }

   // Indicação do número da chamada
   fprintf( p, "\\node[color=CorSerie,inner sep=0pt] at (2,-8) {\\Huge{Nº}};\n" );

   // Período
   fprintf( p, "\\node[color=CorSerie,inner sep=0pt] at (11,-7.5) {\\small%dº};\n", foco->periodo + 1 );

   // Prova
   fprintf( p, "\\node[color=CorSerie,inner sep=0pt] at (12,-7.5) {\\small%d};\n", dados->iprova );

   // Matriz principal de questões e alternativas
   for ( int i = 0; i < NTI; i++ ) { // NTI=10 questões

      // Insere o número das questões
      fprintf( p, "\\node[color=CorSerie,inner sep=0pt] at (3.5+%d,-1.5) {\\small\\bf%.2d};\n", i, i + 1 );

      for ( int j = 0; j < 2; j++ ) {

         if ( j != 0 || i < 7 ) {

            fprintf( p, "\\draw[color=CorSerie,line width=0.75pt] (3.5+%d,-7.5-%d) circle (0.3);\n", i, j );

            fprintf( p, "\\node[color=CorSerie,inner sep=0pt] at  (3.5+%d,-7.5-%d) {\\tiny%d};\n", i, j, i );
         }
      }

      for ( int j = 0; j < 5; j++ ) { // 5 alternativas
         fprintf( p, "\\draw[color=CorSerie,line width=0.75pt] (3.5+%d,-2.5-%d) circle (0.3);\n", i, j );
         fprintf( p, "\\node[color=CorSerie,inner sep=0pt] at  (3.5+%d,-2.5-%d) {\\tiny{%c}};\n", i, j, 65 + j );
      }
      // Assinalar alternativas para teste
      float y_aleatorio = -2.5 - g_random_int_range( 0, 4 );
      fprintf( p, "\\fill[color=black] (%.2f,%.2f) circle (0.24);\n", 3.5 + i, y_aleatorio );
   }

   if ( assinalar_nome_numero ) {
      fprintf( p, "\\fill (3.5+%d,-7.5 ) circle (0.24) (3.5+%d,-8.5) circle (0.24);\n", numero / 10, numero % 10 );
   }

   fprintf( p, "\\end{tikzpicture}\n" );
}
//========================================================================================================//






int imagens_corrigidas( const char *gab, const MapeamentoGabarito *info, const AppContext *ctx, const char *nome_base ) {

   if ( !gab || !info || !ctx ) return -1;


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
   const FichaAluno     *diario = ctx->diario;
   const CalendarioData       *data   = &ctx->data;



   // 1. BLINDAGEM DE MEMÓRIA: Previne acesso negativo ou além do limite na struct 'diario'
   if ( info->num < 1 || info->num > dados->qtd_alunos_total ) {
      g_printerr( "[ERRO CRÍTICO] Número do aluno (%d) inválido. Cancelando renderização LaTeX.\n", info->num );
      return -1;
   }

   const char *meses[] = {
      "janeiro", "fevereiro", "março", "abril", "maio", "junho",
      "julho", "agosto", "setembro", "outubro", "novembro", "dezembro"
   };

   char arquivo[256];
   snprintf( arquivo, sizeof( arquivo ), "./dados/temporarios/%s.tex", nome_base );

   FILE *tex_file = fopen( arquivo, "w" );
   if ( !tex_file ) {
      g_printerr( "[ERRO] Não foi possível criar o arquivo latex: %s\n", arquivo );
      return -1;
   }

   int largura = ( info->direcao == 'h' ) ? 18 : 14;
   int altura  = ( info->direcao == 'h' ) ? 15 : 19;

   // 2. INJEÇÃO DO CABEÇALHO LATEX
   fprintf( tex_file, LATEX_PREAMBLE_TEMPLATE, largura, altura );

   g_autofree gchar *raiz_projeto = g_get_current_dir();
   g_autofree char *imagem = g_build_filename( raiz_projeto, "dados", "gabaritos", dados->ano,
                             dados->escola, "imagens", info->nome_img, NULL );

   fprintf( tex_file,
            "\\begin{document}\n"
            "{\\noindent\\small\n"
            "\\begin{tikzpicture}[baseline=(current bounding box.center)]\n"
            "\\node at (%.4f,%.4f) {\\includegraphics[width=%dcm, height=%dcm]{\"%s\"}};\n",
            0.5 * largura, -0.5 * altura, largura - 4, altura - 4, imagem );

   // Camada de "Máscara" Branca (Limpeza das bordas do escaneamento)
   fprintf( tex_file, "\\filldraw[white] (2,-2)rectangle(2.65,-2.65) (%d,-2)rectangle(%.4f,-2.65) (%d,%d)rectangle(%.4f,%.4f) (2,%d)rectangle(2.65,%.4f);\n",
            largura - 2, largura - 2.65, largura - 2, 2 - altura, largura - 2.65, 2.65 - altura, 2 - altura, 2.65 - altura );

   // 3. GRADE DE QUADRADOS (Payload Visual)
   for ( int i = 0; i < 27; i++ ) {
      int linha  = ( info->direcao == 'h' ) ? i / 3 : i % 3;
      int coluna = ( info->direcao == 'h' ) ? i % 3 : i / 3;

      int bit_ativo = ( info->payload >> i ) & 1;

      if ( bit_ativo ) {
         // Multiplicação e soma exatas primeiro, divisão apenas no final
         double x1 = ( 37.0 + 8.0 * coluna ) / 12.0;
         double y1 = -( 37.0 + 8.0 * linha ) / 12.0;

         double x3 = ( 43.0 + 8.0 * coluna ) / 12.0;
         double y3 = -( 43.0 + 8.0 * linha ) / 12.0;

         fprintf( tex_file, "\\draw[red,thick] (%.4f,%.4f) rectangle (%.4f,%.4f);\n", x1, y1, x3, y3 );
      }
   }

   // 4. AVALIAÇÃO DAS RESPOSTAS E PINTURA DO GABARITO
   int nota = 0;

   // Assumindo que NTI (Número Total de Itens) seja uma macro definida no escopo superior
   for ( int j = 0; j < NTI; j++ ) {
      double linha  = ( info->direcao == 'h' ) ? -4.5 - gab[j] + 65 : -5.5 - j;
      double coluna = ( info->direcao == 'h' ) ?  5.5 + j : 4.5 + gab[j] - 65;
      double x      = ( info->direcao == 'h' ) ?  5.5 + j : 2.5;
      double y      = ( info->direcao == 'h' ) ? -2.5 : -5.5 - j;

      fprintf( tex_file, "\\coordinate (P) at (%.4f,%.4f);\n", x, y );

      if ( gab[j] == info->resp[j] ) {
         nota++;
         fprintf( tex_file, "\\correto\n\\draw[line width=4, green] (%.4f,%.4f) circle (0.35);\n", coluna, linha );
      } else {
         fprintf( tex_file, "\\incorreto\n\\draw[line width=4, red] (%.4f,%.4f) circle (0.35);\n", coluna, linha );
      }
   }

   // 5. INJEÇÃO DOS METADADOS E TEXTOS INSTITUCIONAIS
   fprintf( tex_file, "\\node[left] at (%d,-0.4) {\\Large São Luis, %d de %s de %d};\n",
            largura, data->dia, meses[data->mes - 1], data->ano );
   fprintf( tex_file, "\\node[left] at (%d,-1.3) {\\Large IDENTIFICADOR $\\to$ \\bf\\Huge %d};\n", largura, info->id );

   fprintf( tex_file, "\\node[right,color=AzulProfessor] at (0.04,-0.4) {\\Large\\bf %s};\n", dados->escola );
   fprintf( tex_file, "\\node[right,color=AzulProfessor] at (0.04,-1.0) {\\Large\\textbf{Gest.} %s};\n", dados->gestor );
   fprintf( tex_file, "\\node[right,color=AzulProfessor] at (0.04,-1.6) {\\Large\\textbf{Prof.} %s};\n\n", dados->professor );

   // Formatação Dinâmica do Título da Prova
   char titulo_prova[512];
   if ( dados->periodo[0] == 'R' ) {
      snprintf( titulo_prova, sizeof( titulo_prova ), "%s de %s / \\textbf{%s}",
                dados->periodo, dados->disciplina, dados->ano );
   } else {
      snprintf( titulo_prova, sizeof( titulo_prova ), "%s Prova de %s do \\textbf{%s / %s}",
                dados->prova_sequencia, dados->disciplina, dados->periodo, dados->ano );
   }

   fprintf( tex_file, "\\node[right,color=VerdeEscola] at (0.04,%.4f) {\\Large %s};\n", 1.6 - altura, titulo_prova );
   fprintf( tex_file, "\\node[right,color=VerdeEscola] at (0.04,%.4f) {\\Large Turma: {\\bf %s}};\n", 1.0 - altura, dados->turma );

   // Uso seguro de memória com g_autofree para a string em UpperCase
   g_autofree gchar *nome_aluno = g_utf8_strup( diario[info->num - 1].aluno, -1 );
   fprintf( tex_file, "\\node[right,color=VerdeEscola] at (0.04,%.4f) {\\Large %s $\\to$ Nº: {\\bf\\Huge %.2d}};\n", 0.4 - altura, nome_aluno, info->num );

   // Resultado Final
   fprintf( tex_file, "\\node[color=blue,left] at (%d,%.4f) {\\Large NOTA $\\to$ \\textbf{\\fontsize{12mm}{15mm}\\selectfont %d}};\n\n", largura, 1.3 - altura, nota );

   // 6. FECHAMENTO DO ARQUIVO
   fprintf( tex_file, "\\end{tikzpicture}}\n" );
   fprintf( tex_file, "\\end{document}\n" );

   fclose( tex_file );

   return nota;
}
