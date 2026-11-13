/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "tex.h"



// ====================================================================================
// CODIFICADOR E GERADOR TIKZ DA MATRIZ BINÁRIA OMR (27 BITS - GRADE 3x9)
// ====================================================================================
static void gerar_matriz_identificacao_aux( GString *tex, guint8 id, guint8 turma,
      guint8 disc, guint8 per, guint8 seq ) {
   g_return_if_fail( tex != NULL );

   // 1. Empacotamento de 23 bits de dados em uint32_t (Literais 'U' evitam -Wsign-conversion)
   uint32_t payload = 0U;
   payload |= ( ( uint32_t )id    & 0x3FU );        // 6 bits (0 a 5)
   payload |= ( ( uint32_t )turma & 0xFFU ) << 6U;  // 8 bits (6 a 13)
   payload |= ( ( uint32_t )disc  & 0x0FU ) << 14U; // 4 bits (14 a 17)
   payload |= ( ( uint32_t )per   & 0x07U ) << 18U; // 3 bits (18 a 20)
   payload |= ( ( uint32_t )seq   & 0x03U ) << 21U; // 2 bits (21 a 22)

   // 2. Cálculo dos 4 bits de paridade (XOR acumulado de nibbles de 4 bits)
   uint32_t paridade = 0U;
   uint32_t temp = payload;

   while ( temp > 0U ) {
      paridade ^= ( temp & 0x0FU );
      temp >>= 4U;
   }

   // Insere os 4 bits de paridade nos bits 23 a 26
   payload |= ( paridade & 0x0FU ) << 23U;

   // 3. Desenho da grade 3x9 no LaTeX (27 bits)
   const double passo_celula = 2.0 / 3.0;
   const double offset_ini   = 1.0 + ( 1.0 / 6.0 );
   const double offset_fim   = 1.0 + ( 3.0 / 6.0 );

   for ( guint i = 0U; i < 27U; i++ ) {
      if ( ( payload >> i ) & 1U ) {
         guint linha  = i / 3U;
         guint coluna = i % 3U;

         double desl_x = ( double )coluna * passo_celula;
         double desl_y = ( double )linha  * passo_celula;

         double x_start = offset_ini + desl_x;
         double x_end   = offset_fim + desl_x;
         double y_start = -( offset_ini + desl_y );
         double y_end   = -( offset_fim + desl_y );

         g_string_append_printf( tex, "\\fill (%.4f,%.4f) rectangle (%.4f,%.4f);\n", x_start, y_start, x_end, y_end );
      }
   }
}

// ====================================================================================
// CONSTRUTOR DO QUADRO DE RESPOSTAS OMR EM TIKZ (RAM BUFFERED)
// ====================================================================================
void quadro_de_respostas( GString *tex, const char *aluno, int numero, const guint8 id, char direcao,
                          gboolean assinalar_nome_numero, const InterfaceDados *dados,
                          const InterfaceCabecalho *cabecalho ) {
   g_return_if_fail( tex != NULL && aluno != NULL && dados != NULL && cabecalho != NULL );

   // 1. Parâmetros de Transformação e Orientação
   int    rotacao = ( direcao == 'h' ) ? 0 : 90;
   double xscale  = ( direcao == 'h' ) ? 0.6 : -0.6;
   double yscale  = 0.6;

   g_string_append_printf(
      tex,
      "\\begin{tikzpicture}[baseline=(current bounding box.center), rotate=%d, xscale=%.4f, yscale=%.4f]\n"
      "\\node[color=CorSerie,rotate=%d,inner sep=0pt] at (0.32,-5) {\\bf\\Large RESPOSTAS};\n",
      rotacao, xscale, yscale, 90 - rotacao
   );

   // 2. Âncoras de Calibração (6,5 mm) e Assinatura do Estudante
   if ( direcao == 'h' ) {
      g_string_append( tex, "\\draw[CorSerie] (1,-10.3) -- (13,-10.3);\n" );
      if ( assinalar_nome_numero ) {
         g_string_append_printf( tex, "\\node[inner sep=0pt,color=black] at (7,-9.9) {\\normalsize %s};\n", aluno );
      }
      g_string_append(
         tex,
         "\\node[color=CorSerie,inner sep=0pt] at (7,-10.6) {\\scriptsize NOME DO ESTUDANTE};\n"
         "\\fill (0,0) rectangle (0.65,-0.65) (0,-11) rectangle (0.65,-10.35) "
         "(14,-11) rectangle (13.35,-10.35) (14,0) rectangle (13.35,-0.65);\n"
      );
   } else {
      g_string_append( tex, "\\draw[CorSerie] (14.3,-1.0) -- (14.3,-9.0);\n" );
      if ( assinalar_nome_numero ) {
         g_autofree char *aluno_curto = g_utf8_substring( aluno, 0, MIN( g_utf8_strlen( aluno, -1 ), 32 ) );
         g_string_append_printf( tex, "\\node[inner sep=0pt,color=black] at (13.9,-5) {\\normalsize %s};\n", aluno_curto );
      }
      g_string_append(
         tex,
         "\\node[color=CorSerie,inner sep=0pt] at (14.6,-5) {\\scriptsize NOME DO ESTUDANTE};\n"
         "\\fill (0,0) rectangle (0.65,-0.65) (0,-10) rectangle (0.65,-9.35) "
         "(15,-10) rectangle (14.35,-9.35) (15,0) rectangle (14.35,-0.65);\n"
      );
   }

   // 3. Injeção do Payload Binário (27 bits)
   guint8 turma = ( guint8 )cabecalho->foco_turma;
   guint8 disc  = ( guint8 )cabecalho->foco_disciplina;
   guint8 per   = ( guint8 )cabecalho->foco_periodo;
   guint8 seq   = ( guint8 )dados->seq_prova;

   gerar_matriz_identificacao_aux( tex, id, turma, disc, per, seq );

   // 4. Estrutura da Grade Principal e Divisórias Verticais (Coordenadas pré-somadas em C)
   g_string_append(
      tex,
      "\\draw[CorSerie] (1,-1) rectangle (13,-9) (1,-7) -- (13,-7) (3,-1) -- (3,-9) (3,-2) -- (13,-2) "
      "(4,-1) -- (4,-7) (5,-1) -- (5,-7) (6,-1) -- (6,-7) (7,-1) -- (7,-7) (8,-1) -- (8,-7) "
      "(9,-1) -- (9,-7) (10,-1) -- (10,-7) (11,-1) -- (11,-7) (12,-1) -- (12,-7);\n"
      "\\node[color=CorSerie,inner sep=0pt] at (2,-8) {\\Huge{Nº}};\n"
   );

   // Indicadores visuais de Período e Sequência da Prova
   g_string_append_printf(
      tex,
      "\\node[color=CorSerie,inner sep=0pt] at (11,-7.5) {\\small%dº};\n"
      "\\node[color=CorSerie,inner sep=0pt] at (12,-7.5) {\\small%d};\n",
      cabecalho->foco_periodo + 1, dados->seq_prova
   );

   // 5. Bolhas do Número de Chamada (Dezena: 0..6 | Unidade: 0..9)
   for ( int col_num = 0; col_num < 10; col_num++ ) {
      double x_pos = 3.5 + ( double )col_num;

      // Linha da Dezena (-7.5): desenha apenas de 0 a 6
      if ( col_num < 7 ) {
         g_string_append_printf(
            tex,
            "\\draw[color=CorSerie,line width=0.75pt] (%.1f,-7.5) circle (0.3);\n"
            "\\node[color=CorSerie,inner sep=0pt] at (%.1f,-7.5) {\\tiny%d};\n",
            x_pos, x_pos, col_num
         );
      }

      // Linha da Unidade (-8.5): desenha de 0 a 9
      g_string_append_printf(
         tex,
         "\\draw[color=CorSerie,line width=0.75pt] (%.1f,-8.5) circle (0.3);\n"
         "\\node[color=CorSerie,inner sep=0pt] at (%.1f,-8.5) {\\tiny%d};\n",
         x_pos, x_pos, col_num
      );
   }

   // 6. Matriz de Questões e Alternativas (A..E)
   int limite_questoes = CLAMP( dados->total_quest_prova, 0, 10 );

   for ( int i = 0; i < limite_questoes; i++ ) {
      double x_pos = 3.5 + ( double )i;

      g_string_append_printf(
         tex,
         "\\node[color=CorSerie,inner sep=0pt] at (%.1f,-1.5) {\\small\\bf%.2d};\n",
         x_pos, i + 1
      );

      for ( int j = 0; j < 5; j++ ) {
         double y_pos = -2.5 - ( double )j;
         char letra   = ( char )( 'A' + j );

         g_string_append_printf(
            tex,
            "\\draw[color=CorSerie,line width=0.75pt] (%.1f,%.1f) circle (0.3);\n"
            "\\node[color=CorSerie,inner sep=0pt] at (%.1f,%.1f) {\\tiny{%c}};\n",
            x_pos, y_pos, x_pos, y_pos, letra
         );
      }

      // Assinalar alternativas para teste
      // double y_aleatorio = -2.5 - g_random_int_range( 0, 4 );
      // g_string_append_printf( tex, "\\fill[color=black] (%.2f,%.2f) circle (0.24);\n", 3.5 + i, y_aleatorio );
   }

   // 7. Marcação Automática do Número de Chamada (Quando habilitada)
   if ( assinalar_nome_numero && numero >= 0 && numero < 64 ) {
      int dezena  = numero / 10;
      int unidade = numero % 10;

      double x_dez = 3.5 + ( double )dezena;
      double x_uni = 3.5 + ( double )unidade;
      g_string_append_printf( tex, "\\fill (%.1f,-7.5) circle (0.24) (%.1f,-8.5) circle (0.24);\n", x_dez, x_uni );
   }

   g_string_append( tex, "\\end{tikzpicture}\n" );
}
//========================================================================================================//






// ====================================================================================
// GERADOR DE ESPELHO DE CORREÇÃO INDIVIDUAL EM LATEX/TIKZ (RAM BUFFERED)
// ====================================================================================
int imagens_corrigidas( const char *gab, const MapeamentoGabarito *map, const AppContext *ctx, const char *nome_base ) {
   g_return_val_if_fail( gab != NULL && map != NULL && ctx != NULL && nome_base != NULL, -1 );

   const InterfaceDiario *diario = &ctx->diario;
   const InterfaceDados  *dados  = &ctx->dados;
   const CalendarioData  *hoje   = &ctx->hoje;

   // 1. BLINDAGEM DE MEMÓRIA: Previne acesso fora dos limites em 'diario->fichas'
   if ( !diario->fichas || map->num < 1 ||
         map->num > dados->qtd_alunos_total ||
         ( guint )map->num > diario->fichas->len ) {
      g_printerr( "[ERRO CRÍTICO] Número do aluno (%d) inválido. Cancelando renderização LaTeX.\n", map->num );
      return -1;
   }

   int largura   = ( map->direcao == 'h' ) ? 18 : 14;
   int altura    = ( map->direcao == 'h' ) ? 15 : 19;
   double larg_f = ( double )largura;
   double alt_f  = ( double )altura;

   // Pré-alocação otimizada (16KB) na RAM
   g_autoptr( GString ) tex = g_string_sized_new( 16384 );

   // 2. INJEÇÃO DO CABEÇALHO E MACROS TIKZ
   g_string_append_printf(
      tex,
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
      "\\definecolor{AzulProfessor}{cmyk}{1,0.50,0,0.33}\n"
      "\\definecolor{VerdeEscola}{cmyk}{1,0,1,0.47}\n\n"
      "%% MACRO: Símbolo de Correto\n"
      "\\def\\correto{\n"
      "  \\fill[rounded corners=6, bottom color=NeonVerde, top color=LimaoFluo] ($(-0.43,-0.43)+(P)$) rectangle ($(0.43,0.43)+(P)$);\n"
      "  \\fill[rounded corners=1, color=white] ($(-0.172,-0.258)+(P)$) -- ($(-0.0215,-0.258)+(P)$) -- ($(0.344,0.258)+(P)$) -- ($(0.1935,0.258)+(P)$) -- cycle;\n"
      "  \\fill[rounded corners=1, color=white] ($(-0.0215,-0.258)+(P)$) -- ($(-0.172,-0.258)+(P)$) -- ($(-0.344,0.00)+(P)$) -- ($(-0.1935,0.00)+(P)$) -- cycle;\n"
      "}\n\n"
      "%% MACRO: Símbolo de Incorreto\n"
      "\\def\\incorreto{\n"
      "  \\fill[rounded corners=1, color=VermelhoLaser] ($(-0.35,-0.35)+(-45:0.0875)+(P)$) -- ($(-0.35,-0.35)+(135:0.0875)+(P)$) -- ($(0.35,0.35)+(135:0.0875)+(P)$) -- ($(0.35,0.35)+(-45:0.0875)+(P)$) -- cycle;\n"
      "  \\fill[rounded corners=1, color=VermelhoLaser] ($(-0.35,0.35)+(45:0.0875)+(P)$) -- ($(-0.35,0.35)+(225:0.0875)+(P)$) -- ($(0.35,-0.35)+(225:0.0875)+(P)$) -- ($(0.35,-0.35)+(45:0.0875)+(P)$) -- cycle;\n"
      "}\n"
      "\\pagecolor{yellow!20}\n",
      largura, altura
   );

   static const char * const meses[12] = {
      "janeiro", "fevereiro", "março", "abril", "maio", "junho",
      "julho", "agosto", "setembro", "outubro", "novembro", "dezembro"
   };

   // Construção segura de caminhos usando a raiz configurada no contexto
   g_autofree char *nome_tex = g_strdup_printf( "%s.tex", nome_base );
   g_autofree char *arquivo  = g_build_filename( dados->raiz, "dados", "temporarios", nome_tex, NULL );
   g_autofree char *imagem   = g_build_filename( dados->raiz, "dados", "gabaritos", dados->ano,
                               dados->escola, "imagens", map->nome_img, NULL );

   g_string_append_printf(
      tex,
      "\\begin{document}\n"
      "{\\noindent\\small\n"
      "\\begin{tikzpicture}[baseline=(current bounding box.center)]\n"
      "\\node at (%.4f,%.4f) {\\includegraphics[width=%dcm, height=%dcm]{\"%s\"}};\n",
      0.5 * larg_f, -0.5 * alt_f, largura - 4, altura - 4, imagem
   );

   // Camada de "Máscara" Branca sincronizada com as âncoras de 6,5 mm (0.7 cm) prevenção contra pequenos desalinhamentos
   g_string_append_printf(
      tex,
      "\\filldraw[white] (2,-2) rectangle (2.7,-2.7) "
      "(%d,-2) rectangle (%.4f,-2.7) "
      "(%d,%d) rectangle (%.4f,%.4f) "
      "(2,%d) rectangle (2.7,%.4f);\n",
      largura - 2, larg_f - 2.7,
      largura - 2, 2 - altura, larg_f - 2.7, 2.7 - alt_f,
      2 - altura, 2.7 - alt_f
   );

   // 3. GRADE DE QUADRADOS (Payload Visual - 27 bits)
   // Variáveis inteiras separadas antes do cast evitam alerta bugprone-integer-division no clang-tidy
   for ( guint i = 0U; i < 27U; i++ ) {
      if ( ( map->payload >> i ) & 1U ) {
         guint lin_idx = ( map->direcao == 'h' ) ? ( i / 3U ) : ( i % 3U );
         guint col_idx = ( map->direcao == 'h' ) ? ( i % 3U ) : ( i / 3U );

         double lin_f = ( double )lin_idx;
         double col_f = ( double )col_idx;

         double x1 = ( 37.0 + ( 8.0 * col_f ) ) / 12.0;
         double y1 = -( 37.0 + ( 8.0 * lin_f ) ) / 12.0;
         double x3 = ( 43.0 + ( 8.0 * col_f ) ) / 12.0;
         double y3 = -( 43.0 + ( 8.0 * lin_f ) ) / 12.0;

         g_string_append_printf( tex, "\\draw[red,thick] (%.4f,%.4f) rectangle (%.4f,%.4f);\n", x1, y1, x3, y3 );
      }
   }

   // 4. AVALIAÇÃO DAS RESPOSTAS E PINTURA DO GABARITO
   int acertos = 0;
   int limite_q = CLAMP( dados->total_quest_prova, 0, 10 );

   for ( int j = 0; j < limite_q; j++ ) {
      int offset_letra = CLAMP( ( int )( gab[j] - 'A' ), 0, 4 );
      double j_f       = ( double )j;
      double letra_f   = ( double )offset_letra;

      double linha  = ( map->direcao == 'h' ) ? ( -4.5 - letra_f ) : ( -5.5 - j_f );
      double coluna = ( map->direcao == 'h' ) ? ( 5.5 + j_f )     : ( 4.5 + letra_f );
      double x      = ( map->direcao == 'h' ) ? ( 5.5 + j_f )     :    2.5;
      double y      = ( map->direcao == 'h' ) ?   -2.5             : ( -5.5 - j_f );

      g_string_append_printf( tex, "\\coordinate (P) at (%.4f,%.4f);\n", x, y );

      if ( gab[j] == map->resp[j] ) {
         acertos++;
         g_string_append_printf( tex, "\\correto\n\\draw[line width=4, green] (%.4f,%.4f) circle (0.35);\n", coluna, linha );
      } else {
         g_string_append_printf( tex, "\\incorreto\n\\draw[line width=4, red] (%.4f,%.4f) circle (0.35);\n", coluna, linha );
      }
   }

   // 5. INJEÇÃO DOS METADADOS E TEXTOS INSTITUCIONAIS
   int idx_mes = CLAMP( hoje->mes - 1, 0, 11 );

   g_string_append_printf( tex, "\\node[left] at (%d,-0.4) {\\Large São Luís, %d de %s de %d};\n",
                           largura, hoje->dia, meses[idx_mes], hoje->ano );
   g_string_append_printf( tex, "\\node[left] at (%d,-1.3) {\\Large IDENTIFICADOR $\\to$ \\bf\\Huge %u};\n",
                           largura, ( guint )map->id );

   g_string_append_printf( tex, "\\node[right,color=AzulProfessor] at (0.04,-0.4) {\\Large\\bf %s};\n", dados->escola );
   g_string_append_printf( tex, "\\node[right,color=AzulProfessor] at (0.04,-1.0) {\\Large\\textbf{Gest.} %s};\n",
                           dados->gestor );
   g_string_append_printf( tex, "\\node[right,color=AzulProfessor] at (0.04,-1.6) {\\Large\\textbf{Prof.} %s};\n\n",
                           dados->professor );

   // Formatação Dinâmica do Título da Prova
   char titulo_prova[512] = { 0 };
   if ( ctx->cabecalho.foco_periodo == RECUPERACAO_FINAL ) {
      g_snprintf( titulo_prova, sizeof( titulo_prova ), "%s de %s / \\textbf{%s}",
                  dados->periodo, dados->disciplina, dados->ano );
   } else {
      g_snprintf( titulo_prova, sizeof( titulo_prova ), "%s Prova de %s do \\textbf{%s / %s}",
                  dados->prova_sequencia, dados->disciplina, dados->periodo, dados->ano );
   }

   g_string_append_printf( tex, "\\node[right,color=VerdeEscola] at (0.04,%.4f) {\\Large %s};\n",
                           1.6 - alt_f, titulo_prova );
   g_string_append_printf( tex, "\\node[right,color=VerdeEscola] at (0.04,%.4f) {\\Large Turma: {\\bf %s}};\n",
                           1.0 - alt_f, dados->turma );

   const FichaAluno *ficha = ( const FichaAluno * )g_ptr_array_index( diario->fichas, ( guint )( map->num - 1 ) );
   const char *aluno_str   = ( ficha != NULL ) ? ficha->aluno : "ALUNO NÃO IDENTIFICADO";
   g_autofree gchar *nome_aluno = g_utf8_strup( aluno_str, -1 );

   g_string_append_printf( tex, "\\node[right,color=VerdeEscola] at (0.04,%.4f) {\\Large %s $\\to$ Nº: {\\bf\\Huge %.2d}};\n",
                           0.4 - alt_f, nome_aluno, map->num );

   // Resultado Final
   g_string_append_printf( tex, "\\node[color=blue,left] at (%d,%.4f) {\\Large NOTA $\\to$ \\textbf{\\fontsize{12mm}{15mm}\\selectfont %d}};\n\n", largura, 1.3 - alt_f, acertos );

   // 6. FECHAMENTO DO ARQUIVO
   g_string_append( tex, "\\end{tikzpicture}}\n\\end{document}\n" );

   // 7. GRAVAÇÃO ATÔMICA E SEGURA NO DISCO (gssize evita -Wsign-conversion)
   g_autoptr( GError ) error = NULL;
   if ( !g_file_set_contents( arquivo, tex->str, ( gssize )tex->len, &error ) ) {
      g_printerr( "[ERRO] Não foi possível salvar o arquivo LaTeX (%s): %s\n", arquivo, error->message );
      return -1;
   }

   return acertos;
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
void anexar_tema_pagina_quadrados( GString *tex, gint k ) {
   double largura_barra = 17.5 + 1.5 * k;
   int num_elementos = 45 + 4 * k;
   double espacamento_x = ( largura_barra - 0.2 ) / num_elementos;

   g_string_append_printf( tex,
                           "\\begin{tikzpicture}[remember picture, overlay]\n"
                           "  \\draw[line width=0.8pt, color=CorSerie] (0,0) -- (%.4f,0) (0,-0.28) -- (%.4f,-0.28);\n"
                           "  \\draw[line width=0.8pt, color=CorSerie] (0,-1.00) -- (%.4f,-1.00) (0,-1.28) -- (%.4f,-1.28);\n",
                           largura_barra, largura_barra, largura_barra, largura_barra
                         );

   for ( int i = 0; i <= num_elementos; i++ ) {
      double pos_x = 0.1 + ( i * espacamento_x );

      // Quadrados preenchidos, sem bordas, perfeitamente centralizados entre as linhas
      g_string_append_printf( tex,
                              "  \\fill[CorSerie!50] (%.4f, -0.09) rectangle (%.4f, -0.19);\n"
                              "  \\fill[CorSerie!50] (%.4f, -1.09) rectangle (%.4f, -1.19);\n",
                              pos_x - 0.05, pos_x + 0.05,
                              pos_x - 0.05, pos_x + 0.05
                            );
   }
   g_string_append( tex, "\\end{tikzpicture}\n" );
}

//-------------------------------------------------------------------------------------------------------
void anexar_cabecalho_questao_quadrados( GString *tex, const gchar *cor_texto, gint k, gint num_questao ) {
   double largura_barra = 3.2 + 3.25 * k;
   int num_elementos = 8 + 8 * k;
   double espacamento_x = ( largura_barra - 0.2 ) / num_elementos;
   double offset_pilula = 2.8; // Deslocamento padronizado para as linhas começarem após a pílula

   g_string_append( tex, "\\item{\\noindent\\tikz[baseline=(char.base)]{\n" );

   // Caixa Principal (A "Pílula") ancorada à esquerda (west) para alinhar perfeitamente com os outros temas
   g_string_append_printf( tex,
                           "  \\node[shape=rectangle, rounded corners, draw=CorSerie, fill=CorSerie!10, align=center, inner xsep=4pt, inner ysep=2pt, line width=0.8pt, font=\\bf\\small, text=%s, anchor=west] (char) at (0,0) {\\resizebox{2.5cm}{0.32cm}{\\bf Quest\\~ao %.2d}};\n",
                           cor_texto, num_questao
                         );

   // Linhas Superior e Inferior do trilho direito
   g_string_append_printf( tex,
                           "  \\draw[line width=0.8pt, color=CorSerie] (%.4f, -0.14) -- (%.4f, -0.14) (%.4f, 0.14) -- (%.4f, 0.14);\n",
                           offset_pilula, offset_pilula + largura_barra,
                           offset_pilula, offset_pilula + largura_barra
                         );

   // Mini-Quadrados da Direita flutuando no centro do trilho
   for ( int i = 0; i <= num_elementos; i++ ) {
      double pos_x = offset_pilula + 0.1 + ( i * espacamento_x );
      g_string_append_printf( tex,
                              "  \\fill[CorSerie!50] (%.4f, -0.05) rectangle (%.4f, 0.05);\n",
                              pos_x - 0.05, pos_x + 0.05
                            );
   }

   g_string_append( tex, "}}\\\\\n" );
}
//=======================================================================================================




//=======================================================================================================
void anexar_tema_pagina_linhas( GString *tex, gint k ) {
   double largura_barra = 17.5 + 1.5 * k;
   int num_elementos = 133 + 12 * k;
   double espacamento_x = largura_barra / ( num_elementos + 1 );

   g_string_append( tex, "\\begin{tikzpicture}[remember picture, overlay]\n" );

   // Linhas verticais internas desenroladas em C
   for ( int i = 0; i <= num_elementos; i++ ) {
      double pos_x = ( 0.56 * espacamento_x ) + ( i * espacamento_x );

      g_string_append_printf( tex,
                              "  \\draw[line width=0.8pt, color=CorSerie!25] (%.4f,  0) -- (%.4f, -0.28);\n"
                              "  \\draw[line width=0.8pt, color=CorSerie!25] (%.4f, -1) -- (%.4f, -1.28);\n",
                              pos_x, pos_x, pos_x, pos_x
                            );
   }

   // Linhas horizontais principais (Trilhos)
   g_string_append_printf( tex,
                           "  \\draw[line width=0.8pt, color=CorSerie] (0, 0) -- (%.4f, 0) (0,-0.28) -- (%.4f,-0.28);\n"
                           "  \\draw[line width=0.8pt, color=CorSerie] (0,-1) -- (%.4f,-1) (0,-1.28) -- (%.4f,-1.28);\n",
                           largura_barra, largura_barra, largura_barra, largura_barra
                         );

   g_string_append( tex, "\\end{tikzpicture}\n" );
}

//-------------------------------------------------------------------------------------------------------
void anexar_cabecalho_questao_linhas( GString *tex, const gchar *cor_texto, gint k, gint num_questao ) {
   double largura_barra = 3.2 + 3.15 * k;
   int num_elementos = 23 + 24 * k; // Ajustado para manter a densidade visual semelhante
   double espacamento_x = largura_barra / ( num_elementos + 1 );
   double offset_pilula = 2.8; // Ponto de partida do trilho após a cápsula

   g_string_append( tex, "\\item{\\noindent\\tikz[baseline=(char.base)]{\n" );

   // Caixa Principal (A "Pílula") ancorada à esquerda (west) para alinhar perfeitamente com os outros temas
   g_string_append_printf( tex,
                           "  \\node[shape=rectangle, rounded corners, draw=CorSerie, fill=CorSerie!10, align=center, inner xsep=4pt, inner ysep=2pt, line width=0.8pt, font=\\bf\\small, text=%s, anchor=west] (char) at (0,0) {\\resizebox{2.5cm}{0.32cm}{\\bf Quest\\~ao %.2d}};\n",
                           cor_texto, num_questao
                         );

   // Linhas verticais internas escoando à direita
   for ( int i = 0; i <= num_elementos; i++ ) {
      double pos_x = offset_pilula + ( 0.56 * espacamento_x ) + ( i * espacamento_x );

      g_string_append_printf( tex,
                              "  \\draw[line width=0.8pt, color=CorSerie!25] (%.4f, -0.14) -- (%.4f, +0.14);\n", pos_x, pos_x );
   }

   // Trilhos horizontais apenas à direita da pílula
   g_string_append_printf( tex,
                           "  \\draw[line width=0.8pt, color=CorSerie] (%.4f, -0.14) -- (%.4f, -0.14) (%.4f, 0.14) -- (%.4f, 0.14);\n",
                           offset_pilula, offset_pilula + largura_barra,
                           offset_pilula, offset_pilula + largura_barra
                         );

   // Fechamento limpo, sem o loop de pontilhados de base
   g_string_append( tex, "}}\\\\\n" );
}
//=======================================================================================================





//=======================================================================================================
void anexar_tema_pagina_senoides( GString *tex, gint k ) {
   // 1. Cálculos de largura e passos transferidos para o C
   double largura = 17.5 + 1.5 * k;
   int amostras = ( int )( 15.0 * largura );
   double passo = largura / amostras;

   g_string_append( tex, "\\begin{tikzpicture}[remember picture, overlay]\n" );
   g_string_append( tex, "  \\draw[line width=0.8pt, CorSerie!25, smooth]\n" );

   // Onda 1: Superior Positiva
   g_string_append( tex, "    plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = -0.14 + 0.088 * sin( 11.0 * x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", x, y );
   }
   g_string_append( tex, "}\n" );

   // Onda 2: Superior Negativa (Espelhada)
   g_string_append( tex, "    plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = -0.14 - 0.088 * sin( 11.0 * x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", x, y );
   }
   g_string_append( tex, "}\n" );

   // Onda 3: Inferior Positiva
   g_string_append( tex, "    plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = -1.14 + 0.088 * sin( 11.0 * x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", x, y );
   }
   g_string_append( tex, "}\n" );

   // Onda 4: Inferior Negativa (Espelhada)
   g_string_append( tex, "    plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = -1.14 - 0.088 * sin( 11.0 * x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", x, y );
   }
   g_string_append( tex, "};\n" ); // Fim do comando \draw

   // Linhas retas de contenção (Superior e Inferior)
   g_string_append_printf( tex,
                           "  \\draw[line width=0.8pt, color=CorSerie] (0, 0) -- (%.4f, 0) (0,-0.28) -- (%.4f,-0.28);\n"
                           "  \\draw[line width=0.8pt, color=CorSerie] (0,-1) -- (%.4f,-1) (0,-1.28) -- (%.4f,-1.28);\n"
                           "\\end{tikzpicture}\n",
                           largura, largura, largura, largura
                         );
}

//-------------------------------------------------------------------------------------------------------
void anexar_cabecalho_questao_senoides( GString *tex, const gchar *cor_texto, gint k, gint num_questao ) {
   // 1. Cálculos geométricos no C
   double largura = 3.15 + 3.05 * k;
   int amostras = ( int )( 15.0 * largura );
   double passo = largura / amostras;
   double offset_pilula = 2.8; // Ponto de partida do trilho após a cápsula

   g_string_append( tex, "\\item{\\noindent\\tikz[baseline=(char.base)]{\n" );

   g_string_append( tex, "  \\draw[line width=0.8pt, CorSerie!25, smooth]\n" );

   // Onda 1: Positiva com offset X
   g_string_append( tex, "    plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = +0.088 * sin( 11.0 * x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", x + offset_pilula, y );
   }
   g_string_append( tex, "}\n" );

   // Onda 2: Negativa com offset X
   g_string_append( tex, "    plot coordinates {" );
   for ( int i = 0; i <= amostras; i++ ) {
      double x = i * passo;
      double y = -0.088 * sin( 11.0 * x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", x + offset_pilula, y );
   }
   g_string_append( tex, "};\n" ); // Fim do comando \draw

   // Caixa Principal (A "Pílula") ancorada à esquerda (west) para alinhar perfeitamente com os outros temas
   g_string_append_printf( tex,
                           "  \\node[shape=rectangle, rounded corners, draw=CorSerie, fill=CorSerie!10, align=center, inner xsep=4pt, inner ysep=2pt, line width=0.8pt, font=\\bf\\small, text=%s, anchor=west] (char) at (0,0) {\\resizebox{2.5cm}{0.32cm}{\\bf Quest\\~ao %.2d}};\n",
                           cor_texto, num_questao
                         );

   // Linhas laterais que fecham a caixa
   double limite_direito = offset_pilula + largura;
   g_string_append_printf( tex,
                           "  \\draw[line width=0.8pt, color=CorSerie]\n"
                           "    (%.4f,-0.14) -- (%.4f,-0.14) (%.4f,+0.14) -- (%.4f,+0.14);\n"
                           "}}\\\\\n",
                           offset_pilula, limite_direito, offset_pilula, limite_direito
                         );
}
//=======================================================================================================





//=======================================================================================================
void anexar_tema_pagina_trapezios( GString *tex, gint k ) {
   double largura_barra = 17.5 + 1.5 * k;
   int num_elementos = 42 + 4 * k;
   double espacamento_x = 0.4;

   g_string_append_printf( tex,
                           "\\begin{tikzpicture}[remember picture, overlay]\n"
                           "  \\fill[CorSerie!20] (0,0) rectangle (%.4f, -0.28);\n"
                           "  \\fill[CorSerie!20] (0,-1.0) rectangle (%.4f, -1.28);\n",
                           largura_barra, largura_barra
                         );

   // Linhas finas superiores RETAS na cor principal (Topo de cada barra)
   g_string_append_printf( tex,
                           "  \\draw[CorSerie] (0,0) -- (%.4f,0);\n"
                           "  \\draw[CorSerie] (0,-1.0) -- (%.4f,-1.0);\n",
                           largura_barra, largura_barra
                         );

   // 1. Aplica as máscaras de recorte (fill white) de todos os trapézios
   // Foram descidos em 0.04 para tocar exatamente o fundo das barras (-0.28 e -1.28)
   for ( int i = 0; i <= num_elementos; i++ ) {
      double x_base = ( 0.2 - 0.05 * k ) + ( i * espacamento_x );

      // Recortes da barra superior
      g_string_append_printf( tex,
                              "  \\fill[white] (%.4f,-0.28)--(%.4f,-0.18)--(%.4f,-0.08)--(%.4f,-0.28)--cycle;\n",
                              x_base + 0.1, x_base, x_base + 0.1, x_base + 0.3
                            );
      // Recortes da barra inferior
      g_string_append_printf( tex,
                              "  \\fill[white] (%.4f,-1.28)--(%.4f,-1.18)--(%.4f,-1.08)--(%.4f,-1.28)--cycle;\n",
                              x_base + 0.1, x_base, x_base + 0.1, x_base + 0.3
                            );
   }

   // 2. Traça a linha de contorno INFERIOR contínua para a barra de CIMA
   g_string_append( tex, "  \\draw[CorSerie] (0,-0.28)" );
   for ( int i = 0; i <= num_elementos; i++ ) {
      double x_base = ( 0.2 - 0.05 * k ) + ( i * espacamento_x );
      g_string_append_printf( tex,
                              " -- (%.4f,-0.28) -- (%.4f,-0.18) -- (%.4f,-0.08) -- (%.4f,-0.28)",
                              x_base + 0.1, x_base, x_base + 0.1, x_base + 0.3
                            );
   }
   g_string_append_printf( tex, " -- (%.4f,-0.28);\n", largura_barra );

   // 3. Traça a linha de contorno INFERIOR contínua para a barra de BAIXO
   g_string_append( tex, "  \\draw[CorSerie] (0,-1.28)" );
   for ( int i = 0; i <= num_elementos; i++ ) {
      double x_base = ( 0.2 - 0.05 * k ) + ( i * espacamento_x );
      g_string_append_printf( tex,
                              " -- (%.4f,-1.28) -- (%.4f,-1.18) -- (%.4f,-1.08) -- (%.4f,-1.28)",
                              x_base + 0.1, x_base, x_base + 0.1, x_base + 0.3
                            );
   }
   g_string_append_printf( tex, " -- (%.4f,-1.28);\n", largura_barra );

   g_string_append( tex, "\\end{tikzpicture}\n" );
}

//-------------------------------------------------------------------------------------------------------
void anexar_cabecalho_questao_trapezios( GString *tex, gint k, gint num_questao ) {
   double largura_barra = 3.2 + 3.25 * k;
   int num_elementos = 7 + 8 * k;
   double espacamento_x = 0.4;
   int num_pontilhados = 59 + 32 * k;

   // Calcula a margem para manter os trapézios centralizados dentro da largura dinâmica
   double espaco_ocupado = espacamento_x * ( num_elementos + 1 );
   double offset_x = ( largura_barra + 0.1 - espaco_ocupado ) / 2.0;

   g_string_append_printf( tex,
                           "\\item{\\noindent\\tikz[baseline=(char.base)]{\n"
                           "  \\node[inner sep=0pt, anchor=west] (char) at (0,0) {\\resizebox{2.6cm}{0.35cm}{\\bf Quest\\~ao %.2d}\\,};\n"
                           "  \\fill[CorSerie!20] (2.7, -0.14) rectangle (%.4f, 0.14);\n",
                           num_questao, 2.7 + largura_barra
                         );

   // Moldura fina superior para travar o contorno
   g_string_append_printf( tex,
                           "  \\draw[CorSerie!50] (2.7,0.14) -- (%.4f,0.14);\n",
                           2.7 + largura_barra
                         );

   double y_descer = 0.04;

   // 1. Aplica as máscaras de recorte (fill white) de todos os trapézios
   for ( int i = 0; i <= num_elementos; i++ ) {
      double x_base = 2.7 + offset_x + ( i * espacamento_x );

      g_string_append_printf( tex,
                              "  \\fill[white] (%.4f,%.4f)--(%.4f,%.4f)--(%.4f,%.4f)--(%.4f,%.4f)--cycle;\n",
                              x_base + 0.1, -0.1 - y_descer,
                              x_base,       -y_descer,
                              x_base + 0.1, 0.1 - y_descer,
                              x_base + 0.3, -0.1 - y_descer
                            );
   }

   // 2. Traça a linha de contorno INFERIOR contínua acompanhando os recortes
   // Inicia a linha no canto inferior esquerdo da barra
   g_string_append( tex, "  \\draw[CorSerie!50] (2.7,-0.14)" );

   // Adiciona os vértices dos trapézios ao caminho da linha
   for ( int i = 0; i <= num_elementos; i++ ) {
      double x_base = 2.7 + offset_x + ( i * espacamento_x );

      g_string_append_printf( tex,
                              " -- (%.4f,%.4f) -- (%.4f,%.4f) -- (%.4f,%.4f) -- (%.4f,%.4f)",
                              x_base + 0.1, -0.1 - y_descer,
                              x_base,       -y_descer,
                              x_base + 0.1, 0.1 - y_descer,
                              x_base + 0.3, -0.1 - y_descer
                            );
   }

   // Finaliza a linha no canto inferior direito da barra
   g_string_append_printf( tex, " -- (%.4f,-0.14);\n", 2.7 + largura_barra );

   // Macro de estilo para os pontos perfeitos de alta performance:
   // line width = 1.2pt (diâmetro do círculo exato), espaçamento exato = 0.1cm (mesmo do laço antigo)
   const char *estilo_ponto = "line width=1.2pt, line cap=round, dash pattern=on 0pt off 0.1cm";

   // 3. Adiciona a linha pontilhada inferior
   g_string_append_printf( tex, "  \\draw[color=CorSerie!50, %s] (0,-0.35) -- (%.4f,-0.35);\n",
                           estilo_ponto, 0.1 * ( num_pontilhados + 1.0 ) );

   g_string_append( tex, "}}\\\\\n" );
}
//=======================================================================================================




//=======================================================================================================
void anexar_tema_pagina_ondas( GString *tex, gint k ) {
   double largura_barra = 17.5 + 1.5 * k;
   int num_elementos = ( int )( 15.0 * largura_barra );
   double espacamento_x = largura_barra / num_elementos;

   g_string_append( tex, "\\begin{tikzpicture}[remember picture, overlay]\n" );

   // Barra Superior
   g_string_append_printf( tex, "  \\fill[CorSerie!20] (0,0) rectangle (%.4f, -0.28);\n", largura_barra );
   g_string_append( tex, "  \\draw[CorSerie, line width=0.8pt, preaction={draw=white, line width=2.8pt}, smooth] plot coordinates {" );
   for ( int i = 0; i <= num_elementos; i++ ) {
      double pos_x = i * espacamento_x;
      double pos_y = -0.14 - 0.06 * sin( 12.0 * pos_x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", pos_x, pos_y );
   }
   g_string_append( tex, "};\n" );

   // Barra Inferior
   g_string_append_printf( tex, "  \\fill[CorSerie!20] (0,-1) rectangle (%.4f, -1.28);\n", largura_barra );
   g_string_append( tex, "  \\draw[CorSerie, line width=0.8pt, preaction={draw=white, line width=2.8pt}, smooth] plot coordinates {" );
   for ( int i = 0; i <= num_elementos; i++ ) {
      double pos_x = i * espacamento_x;
      double pos_y = -1.14 - 0.06 * sin( 12.0 * pos_x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", pos_x, pos_y );
   }
   g_string_append( tex, "};\n" );

   // Linhas finas de borda para dar o acabamento técnico e estrutural
   g_string_append_printf( tex,
                           "  \\draw[CorSerie] (0,0) -- (%.4f,0) (0,-0.28) -- (%.4f,-0.28);\n"
                           "  \\draw[CorSerie] (0,-1.0) -- (%.4f,-1.0) (0,-1.28) -- (%.4f,-1.28);\n",
                           largura_barra, largura_barra, largura_barra, largura_barra
                         );

   g_string_append( tex, "\\end{tikzpicture}\n" );
}

//-------------------------------------------------------------------------------------------------------
void anexar_cabecalho_questao_ondas( GString *tex, gint k, gint num_questao ) {
   double largura_barra = 3.2 + 3.25 * k;
   int num_elementos = ( int )( 15.0 * largura_barra );
   double espacamento_x = largura_barra / num_elementos;
   int num_pontilhados = 59 + 32 * k;

   g_string_append_printf( tex,
                           "\\item{\\noindent\\tikz[baseline=(char.base)]{\n"
                           "  \\node[inner sep=0pt, anchor=west] (char) at (0,0) {\\resizebox{2.6cm}{0.35cm}{\\bf Quest\\~ao %.2d}\\,};\n"
                           "  \\fill[CorSerie!20] (2.7, -0.14) rectangle (%.4f, 0.14);\n",
                           num_questao, 2.7 + largura_barra
                         );

   g_string_append( tex, "  \\draw[CorSerie, line width=0.8pt, preaction={draw=white, line width=2.8pt}, smooth] plot coordinates {" );
   for ( int i = 0; i <= num_elementos; i++ ) {
      double pos_x = i * espacamento_x;
      double pos_y = 0.06 * sin( 12.0 * pos_x );
      g_string_append_printf( tex, "(%.3f,%.3f) ", pos_x + 2.7, pos_y );
   }
   g_string_append( tex, "};\n" );

   // Macro de estilo para os pontos perfeitos de alta performance:
   // line width = 1pt (diâmetro do círculo exato), espaçamento exato = 0.1cm (mesmo do laço antigo)
   const char *estilo_ponto = "line width=1.2pt, line cap=round, dash pattern=on 0pt off 0.1cm";

   // 3. Adiciona a linha pontilhada inferior
   g_string_append_printf( tex, "  \\draw[color=CorSerie!50, %s] (0,-0.35) -- (%.4f,-0.35);\n",
                           estilo_ponto, 0.1 * ( num_pontilhados + 1.0 ) );

   g_string_append( tex, "}}\\\\\n" );
}
//=======================================================================================================





//=======================================================================================================
void anexar_tema_pagina_circulos( GString *tex, gint k ) {
   double largura_barra = 17.5 + 1.5 * k;
   int num_elementos = 34 + 3 * k;
   double espacamento_x = ( largura_barra - 0.28 ) / num_elementos;

   g_string_append_printf( tex,
                           "\\begin{tikzpicture}[remember picture, overlay]\n"
                           "  \\fill[CorSerie!15] (0,0) rectangle (%.4f, -0.28);\n"
                           "  \\fill[CorSerie!15] (0,-1.0) rectangle (%.4f, -1.28);\n",
                           largura_barra, largura_barra
                         );

   // Linhas finas de borda para dar o acabamento técnico e estrutural
   g_string_append_printf( tex,
                           "  \\draw[CorSerie] (0,0) -- (%.4f,0) (0,-0.28) -- (%.4f,-0.28);\n"
                           "  \\draw[CorSerie] (0,-1.0) -- (%.4f,-1.0) (0,-1.28) -- (%.4f,-1.28);\n",
                           largura_barra, largura_barra, largura_barra, largura_barra
                         );

   // 1. Os Trilhos Suaves
   g_string_append_printf( tex,
                           "  \\draw[CorSerie!30, line width=1.2pt] (0,-0.14) -- (%.4f,-0.14);\n"
                           "  \\draw[CorSerie!30, line width=1.2pt] (0,-1.14) -- (%.4f,-1.14);\n",
                           largura_barra, largura_barra
                         );

   // 2. Os "Nós Ópticos" aplicados em ambas as barras simultaneamente
   for ( int i = 0; i <= num_elementos; i++ ) {
      double pos_x = 0.14 + ( i * espacamento_x );

      // Barra Superior
      g_string_append_printf( tex,
                              "  \\fill[white] (%.4f, -0.14) circle (0.12);\n"
                              "  \\draw[CorSerie, line width=0.7pt] (%.4f, -0.14) circle (0.09);\n"
                              "  \\fill[CorSerie!40] (%.4f, -0.14) circle (0.035);\n",
                              pos_x, pos_x, pos_x
                            );

      // Barra Inferior
      g_string_append_printf( tex,
                              "  \\fill[white] (%.4f, -1.14) circle (0.12);\n"
                              "  \\draw[CorSerie, line width=0.7pt] (%.4f, -1.14) circle (0.09);\n"
                              "  \\fill[CorSerie!40] (%.4f, -1.14) circle (0.035);\n",
                              pos_x, pos_x, pos_x
                            );
   }

   g_string_append( tex, "\\end{tikzpicture}\n" );
}
//-------------------------------------------------------------------------------------------------------
void anexar_cabecalho_questao_circulos( GString *tex, gint k, gint num_questao ) {
   double largura_barra = 3.2 + 3.25 * k;
   int num_elementos = 6 + 6 * k;
   double espacamento_x = ( largura_barra - 0.28 ) / num_elementos;
   int num_pontilhados = 59 + 32 * k;

   g_string_append_printf( tex,
                           "\\item{\\noindent\\tikz[baseline=(char.base)]{\n"
                           "  \\node[inner sep=0pt, anchor=west] (char) at (0,0) {\\resizebox{2.6cm}{0.35cm}{\\bf Quest\\~ao %.2d}\\,};\n"
                           "  \\fill[CorSerie!15] (2.7, -0.14) rectangle (%.4f, 0.14);\n",
                           num_questao, 2.7 + largura_barra
                         );

   // 1. O Trilho (Linha conectora suave que corre por trás dos nós)
   g_string_append_printf( tex,
                           "  \\draw[CorSerie!30, line width=1.2pt] (2.7, 0) -- (%.4f, 0);\n",
                           2.7 + largura_barra
                         );

   // 2. Os "Nós Ópticos" (Camadas concêntricas)
   for ( int i = 0; i <= num_elementos; i++ ) {
      double pos_x = 2.7 + 0.14 + ( i * espacamento_x );

      g_string_append_printf( tex,
                              "  \\fill[white] (%.4f, 0) circle (0.12);\n"                  // Máscara de limpeza
                              "  \\draw[CorSerie, line width=0.7pt] (%.4f, 0) circle (0.09);\n" // Anel de precisão nítido
                              "  \\fill[CorSerie!40] (%.4f, 0) circle (0.035);\n",          // Núcleo suave
                              pos_x, pos_x, pos_x
                            );
   }

   // Macro de estilo para os pontos perfeitos de alta performance:
   // line width = 1pt (diâmetro do círculo exato), espaçamento exato = 0.1cm (mesmo do laço antigo)
   const char *estilo_ponto = "line width=1.2pt, line cap=round, dash pattern=on 0pt off 0.1cm";

   // 3. Adiciona a linha pontilhada inferior
   g_string_append_printf( tex, "  \\draw[color=CorSerie!50, %s] (0,-0.35) -- (%.4f,-0.35);\n",
                           estilo_ponto, 0.1 * ( num_pontilhados + 1.0 ) );

   g_string_append( tex, "}}\\\\\n" );
}
//=======================================================================================================





//=======================================================================================================
void anexar_tema_pagina_hexagonos( GString *tex, gint k ) {
   double largura_barra = 17.5 + 1.5 * k;
   int num_elementos = 32 + 3 * k;
   double espacamento_x = ( largura_barra - 0.28 ) / num_elementos;

   g_string_append_printf( tex,
                           "\\begin{tikzpicture}[remember picture, overlay]\n"
                           "  \\fill[CorSerie!20] (0,0) rectangle (%.4f, -0.28);\n"
                           "  \\fill[CorSerie!20] (0,-1.0) rectangle (%.4f, -1.28);\n",
                           largura_barra, largura_barra
                         );

   // O C plota os mini-hexágonos vazados em branco e com realces
   for ( int i = 0; i <= num_elementos; i++ ) {
      double pos_x = 0.14 + ( i * espacamento_x );

      // Hexágonos superiores recortados em branco (efeito estêncil)
      g_string_append_printf( tex,
                              "  \\filldraw[fill=white,draw=CorSerie!50] (%.4f,-0.14) + (30:0.11) -- + (90:0.11) -- + (150:0.11) -- + (210:0.11) -- + (270:0.11) -- + (330:0.11) -- cycle;\n",
                              pos_x
                            );

      // Hexágonos inferiores recortados em branco
      g_string_append_printf( tex,
                              "  \\filldraw[fill=white,draw=CorSerie!50] (%.4f,-1.14) + (30:0.11) -- + (90:0.11) -- + (150:0.11) -- + (210:0.11) -- + (270:0.11) -- + (330:0.11) -- cycle;\n",
                              pos_x
                            );
   }

   // Linhas finas de borda na cor principal para dar acabamento técnico
   g_string_append_printf( tex,
                           "  \\draw[CorSerie,line width=0.5pt] (0,0) -- (%.4f,0) (0,-0.28) -- (%.4f,-0.28);\n"
                           "  \\draw[CorSerie,line width=0.5pt] (0,-1.0) -- (%.4f,-1.0) (0,-1.28) -- (%.4f,-1.28);\n"
                           "\\end{tikzpicture}\n",
                           largura_barra, largura_barra, largura_barra, largura_barra
                         );
}
//-------------------------------------------------------------------------------------------------------
void anexar_cabecalho_questao_hexagonos( GString *tex, gint k, gint num_questao ) {
   double largura_barra = 3.2 + 3.25 * k;
   int num_elementos = 6 + 6 * k;
   double espacamento_x = ( largura_barra - 0.28 ) / num_elementos;
   int num_pontilhados = 59 + 32 * k;

   g_string_append_printf( tex,
                           "\\item{\\noindent\\tikz[baseline=(char.base)]{\n"
                           "  \\node[inner sep=0pt, anchor=west] (char) at (0,0) {\\resizebox{2.6cm}{0.35cm}{\\bf Quest\\~ao %.2d}\\,};\n"
                           "  \\fill[CorSerie!20] (2.7, -0.14) rectangle (%.4f, 0.14);\n",
                           num_questao, 2.7 + largura_barra
                         );

   // Mosaico de Hexágonos brancos vazados
   for ( int i = 0; i <= num_elementos; i++ ) {
      double pos_x = 2.7 + 0.14 + ( i * espacamento_x );

      g_string_append_printf( tex,
                              "  \\filldraw[fill=white,draw=CorSerie!30] (%.4f,0) + (30:0.11) -- + (90:0.11) -- + (150:0.11) -- + (210:0.11) -- + (270:0.11) -- + (330:0.11) -- cycle;\n",
                              pos_x
                            );
   }

   // Moldura fina para travar o contorno
   g_string_append_printf( tex,
                           "  \\draw[CorSerie] (2.7,-0.14) -- (%.4f,-0.14) (2.7,0.14) -- (%.4f,0.14);\n",
                           2.7 + largura_barra, 2.7 + largura_barra
                         );

   // Macro de estilo para os pontos perfeitos de alta performance:
   // line width = 1pt (diâmetro do círculo exato), espaçamento exato = 0.1cm (mesmo do laço antigo)
   const char *estilo_ponto = "line width=1.2pt, line cap=round, dash pattern=on 0pt off 0.1cm";

   // 3. Adiciona a linha pontilhada inferior
   g_string_append_printf( tex, "  \\draw[color=CorSerie!50, %s] (0,-0.35) -- (%.4f,-0.35);\n",
                           estilo_ponto, 0.1 * ( num_pontilhados + 1.0 ) );

   g_string_append( tex, "}}\\\\\n" );
}
//=======================================================================================================

