/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#include "comum.h"
#include "gabaritos.h"
#include "basicas.h"
#include "interface.h"



/*
* Função para verificar a integridade dos 27 bits extraídos da imagem.
* Retorna TRUE se a leitura estiver íntegra, FALSE se houve rasura/ruído.
*/
gboolean verificar_paridade_matriz( uint32_t payload_lido ) {
   // Isola os 23 bits de dados (Máscara 0x7FFFFF)
   uint32_t dados = payload_lido & 0x7FFFFF;

   // Isola os 4 bits de paridade que estão nas posições 23 a 26
   uint32_t paridade_esperada = ( payload_lido >> 23 ) & 0x0F;

   uint32_t paridade_calculada = 0;

   // Recalcula o XOR em blocos de 4 bits
   while ( dados > 0 ) {
      paridade_calculada ^= ( dados & 0x0F );
      dados >>= 4;
   }

   return ( paridade_calculada == paridade_esperada );
}

/*
* Verifica a integridade e extrai os identificadores originais a partir do payload lido.
* Retorna TRUE se a imagem estava íntegra e os dados foram extraídos com sucesso,
* ou FALSE se a verificação de paridade falhar.
*/
gboolean decodificar_payload_matriz( MapeamentoGabarito *info, const LimitesFiltro *limite ) {
   // 1. Validação defensiva dos ponteiros de saída usando macros da GLib
   g_return_val_if_fail( info != NULL, FALSE );
   g_return_val_if_fail( limite != NULL, FALSE );

   // 2. Executa o teste de integridade via checksum/paridade
   if ( !verificar_paridade_matriz( info->payload ) ) {
      // g_warning("Falha na integridade do payload lido: paridade invalida.");
      return FALSE;
   }

   // 3. Desempacotamento (Bitwise Shift reverso e aplicação de máscaras)
   info->id    = ( uint8_t )( info->payload & 0x3F );      // Isola os bits 0 a 5
   info->turma = ( uint8_t )( ( info->payload >> 6 )  & 0xFF ); // Desloca 6 bits e isola 8 bits (6 a 13)
   info->disc  = ( uint8_t )( ( info->payload >> 14 ) & 0x0F ); // Desloca 14 bits e isola 4 bits (14 a 17)
   info->per   = ( uint8_t )( ( info->payload >> 18 ) & 0x07 ); // Desloca 18 bits e isola 3 bits (18 a 20)
   info->seq   = ( uint8_t )( ( info->payload >> 21 ) & 0x03 ); // Desloca 21 bits e isola 2 bits (21 a 22)

   // A sequência da prova é o único valor que não pode ser zero (valores 1, 2 e 3)
   // Pois a leitura do binário numa área branca retornava um payload válido onde todos os bits são zero
   // Mudando a sequência da prova de 0, 1, 2 para 1, 2, 3 elimina essa vulnerabilidade.
   if ( info->turma >= limite->turmas || info->per + 1 >= limite->periodos || info->seq == 0 ) {
      return FALSE;
   }

   return TRUE;
}







/**
 * Extrai o payload de 27 bits armazenado na grade 3x9 da imagem PGM binarizada.
 * Suporta orientações horizontais (14p x 11p) e verticais (10p x 15p).
 */
uint32_t extrair_payload_matriz( const ImagemCinza *IMG, char direcao ) {
   // 1. Validações defensivas robustas via GLib
   g_return_val_if_fail( IMG != NULL, 0 );
   g_return_val_if_fail( IMG->image != NULL, 0 );
   g_return_val_if_fail( direcao == 'h' || direcao == 'v', 0 );

   uint32_t payload_lido = 0;
   double p_x, p_y;

   // 2. Define dinamicamente as proporções de 'p' com base na orientação
   if ( direcao == 'h' ) {
      p_x = ( double )IMG->ncol / 14.0;
      p_y = ( double )IMG->nrow / 11.0;
   } else { // 'v'
      p_x = ( double )IMG->ncol / 10.0;
      p_y = ( double )IMG->nrow / 15.0;
   }

   // Ponto de corte para separar branco e preto
   int limiar_binario = IMG->max / 2;

   // Kernel dinâmico ajustado para cobrir precisamente ~50% da área total do quadrado (raio = 35% do lado).
   // Isso equivale a uma varredura de 70% da largura e 70% da altura (0.7 * 0.7 ≈ 0.50).
   int raio_x = MAX( 1, ( int )( ( p_x / 3.0 ) * 0.8 ) );
   int raio_y = MAX( 1, ( int )( ( p_y / 3.0 ) * 0.8 ) );

   // 3. Varredura dos 27 bits do payload
   for ( int i = 0; i < 27; i++ ) {

      int linha  = ( direcao == 'h' )  ?  i / 3  :  i % 3;
      int coluna = ( direcao == 'h' )  ?  i % 3  :  i / 3;

      double x_centro_p, y_centro_p;
      double passo_grade = 2.0 / 3.0;

      x_centro_p = 1.0 + ( coluna * passo_grade ) + ( passo_grade / 2.0 );
      y_centro_p = 1.0 + ( linha  * passo_grade ) + ( passo_grade / 2.0 );

      // Converte a unidade fracionária 'p' para a coordenada real de pixels
      int px = ( int )( x_centro_p * p_x );
      int py = ( int )( y_centro_p * p_y );

      // Garante que o centro calculado está dentro dos limites físicos da imagem
      if ( px >= 0 && px < IMG->ncol && py >= 0 && py < IMG->nrow ) {
         int pixels_pretos = 0;
         int cont_pixels = 0;

         // 5. Votação por Maioria usando o Kernel Dinâmico Adaptativo
         for ( int dy = -raio_y; dy <= raio_y; dy++ ) {
            for ( int dx = -raio_x; dx <= raio_x; dx++ ) {
               int nx = px + dx;
               int ny = py + dy;

               if ( nx >= 0 && nx < IMG->ncol && ny >= 0 && ny < IMG->nrow ) {
                  if ( IMG->image[ny][nx] < limiar_binario ) {
                     pixels_pretos++;
                  }
                  cont_pixels++;
                  // IMG->image[ny][nx] = 0; // apenas para verificação visual
               }
            }
         }

         // Valida o bit se a densidade de tinta no miolo do quadrado for maior que 50%
         if ( pixels_pretos > ( cont_pixels / 3 ) ) {
            payload_lido |= ( 1U << i );
         }
      }
   }

   return payload_lido;
}




static float calcular_densidade_celula( const ImagemCinza *IMG, int i_celula, int j_celula,
                                        float p_x, float p_y, float raio, float raio_quad ) {
   float cx = ( j_celula + 0.5f ) * p_x;
   float cy = ( i_celula + 0.5f ) * p_y;

   int x_min = ( int )floorf( cx - raio );
   int x_max = ( int )ceilf( cx + raio );
   int y_min = ( int )floorf( cy - raio );
   int y_max = ( int )ceilf( cy + raio );

   if ( x_min < 0 ) x_min = 0;
   if ( x_max >= IMG->ncol ) x_max = IMG->ncol - 1;
   if ( y_min < 0 ) y_min = 0;
   if ( y_max >= IMG->nrow ) y_max = IMG->nrow - 1;

   int total_pixels = 0;
   int pixels_pretos = 0;

   for ( int y = y_min; y <= y_max; y++ ) {
      for ( int x = x_min; x <= x_max; x++ ) {
         float dx = x - cx;
         float dy = y - cy;

         if ( ( dx * dx + dy * dy ) <= raio_quad ) {
            total_pixels++;
            if ( IMG->image[y][x] == 0 ) {
               pixels_pretos++;
            }
         }
      }
   }

   return total_pixels > 0 ? ( float )pixels_pretos / total_pixels : 0.0f;
}





// Função para escanear a matriz de pixels e detectar a resposta assinalada
void ler_respostas_gabarito( const ImagemCinza *IMG, char direcao, char *respostas_out ) {
   if ( !IMG || !IMG->image || !respostas_out ) return;

   // 1. Definição da malha matricial com base na orientação
   int cols_grid = ( direcao == 'h' ) ? 14 : 10;
   int rows_grid = ( direcao == 'h' ) ? 11 : 15;

   // 2. Tamanho da unidade 'p' em pixels nos eixos X e Y
   float p_x = ( float )IMG->ncol / cols_grid;
   float p_y = ( float )IMG->nrow / rows_grid;

   // Para manter a área de busca perfeitamente circular, usamos o menor dos dois 'p'
   float p_min = ( p_x < p_y ) ? p_x : p_y;
   float raio = 0.35f * p_min;
   float raio_quad = raio * raio; // Evita usar sqrt() dentro dos laços

   // 3. Varredura das 10 questões
   for ( int q = 0; q < 10; q++ ) {

      float max_densidade = -1.0f;
      int alt_marcada = -1; // 0=A, 1=B, 2=C, 3=D, 4=E

      // Varredura das 5 alternativas para a questão atual
      for ( int alt = 0; alt < 5; alt++ ) {
         int i_celula, j_celula;

         if ( direcao == 'h' ) {
            j_celula = q + 3;     // Colunas: 3 a 12
            i_celula = alt + 2;   // Linhas:  2 a 6
         } else {
            // Transposição Matricial ('v')
            i_celula = q + 3;     // Linhas:  3 a 12
            j_celula = alt + 2;   // Colunas: 2 a 6
         }

         // Calcula a porcentagem de preenchimento da bolinha
         float densidade = calcular_densidade_celula( IMG, i_celula, j_celula, p_x, p_y, raio, raio_quad );

         // Registra a alternativa com a marcação mais forte
         if ( densidade > max_densidade ) {
            max_densidade = densidade;
            alt_marcada = alt;
         }
      }

      // 4. Critério de Aceitação (Tratamento de rasuras e questões em branco)
      // Exigimos no mínimo 50% de preenchimento para considerar a bolinha marcada.
      // Caso contrário, a questão foi deixada em branco (assinalada com '-').
      if ( max_densidade > 0.50f ) {
         respostas_out[q] = 'A' + alt_marcada;
      } else {
         respostas_out[q] = '-';
      }
   }

   // Finaliza a string C corretamente
   respostas_out[10] = '\0';
}






int ler_numero_aluno( const ImagemCinza *IMG, char direcao ) {
   if ( !IMG || !IMG->image ) return -1;

   int cols_grid = ( direcao == 'h' ) ? 14 : 10;
   int rows_grid = ( direcao == 'h' ) ? 11 : 15;

   float p_x = ( float )IMG->ncol / cols_grid;
   float p_y = ( float )IMG->nrow / rows_grid;

   float p_min = ( p_x < p_y ) ? p_x : p_y;
   float raio = 0.35f * p_min;
   float raio_quad = raio * raio;

   int dezena_marcada = -1;
   float max_den_dezena = -1.0f;

   // Varredura da DEZENA (0 a 6)
   for ( int val = 0; val <= 6; val++ ) {
      int i_cel = ( direcao == 'h' ) ? 7 : 3 + val;
      int j_cel = ( direcao == 'h' ) ? 3 + val : 7;

      float densidade = calcular_densidade_celula( IMG, i_cel, j_cel, p_x, p_y, raio, raio_quad );

      if ( densidade > max_den_dezena ) {
         max_den_dezena = densidade;
         dezena_marcada = val;
      }
   }

   int unidade_marcada = -1;
   float max_den_unidade = -1.0f;

   // Varredura da UNIDADE (0 a 9)
   for ( int val = 0; val <= 9; val++ ) {
      int i_cel = ( direcao == 'h' ) ? 8 : 3 + val;
      int j_cel = ( direcao == 'h' ) ? 3 + val : 8;

      float densidade = calcular_densidade_celula( IMG, i_cel, j_cel, p_x, p_y, raio, raio_quad );

      if ( densidade > max_den_unidade ) {
         max_den_unidade = densidade;
         unidade_marcada = val;
      }
   }

   // Tratamento de segurança (Limiar de marcação em 50%)
   // Se o aluno deixou em branco ou a marcação for fraca demais, retornamos -1
   // para o programa principal acionar um alerta no painel de revisão.
   if ( max_den_dezena <= 0.50f || max_den_unidade <= 0.50f ) {
      return -1;
   }

   return ( dezena_marcada * 10 ) + unidade_marcada;
}









//========================================================================================================//
void mudar_numero_na_imagem( float l, float h, float pp, int rot, Ponto2D *C, int n0, int n, unsigned char t, char *img, const InterfaceDados *dados ) {

   char str[2000], sx[100], sy[100];

   if ( dados->periodo[0] == 'R' ) {
      sprintf( str, "cp ./dados/'Gabaritos'/'%s'/'%s'/'%s'/'Imagens Recuperação Final'/%s 'Relatórios e Provas'/", dados->ano, dados->disciplina, dados->escola, img );
      if ( system( str ) == -1 ) {
         fprintf( stderr, "Erro crítico: Não foi possível executar o comando: %s\n", str );
      }
   } else {
      sprintf( str, "cp ./dados/'Gabaritos'/'%s'/'%s'/'%s'/'Imagens %s Prova%d'/%s 'Relatórios e Provas'/", dados->ano, dados->disciplina, dados->escola, dados->periodo, dados->iprova, img );
      if ( system( str ) == -1 ) {
         fprintf( stderr, "Erro crítico: Não foi possível executar o comando: %s\n", str );
      }
   }

   FILE *p = fopen( "Relatórios e Provas/img.tex", "w+" );

   fprintf( p, "\\documentclass[11pt]{report}\n" );
   fprintf( p, "\\usepackage[latin1]{inputenc}\n" );
   fprintf( p, "\\usepackage[T1]{fontenc}\n" );
   fprintf( p, "\\usepackage[brazil]{babel}\n" );
   fprintf( p, "\\usepackage{cmbright,ifthen,setspace,tikz}\n" );
   fprintf( p, "\\usetikzlibrary{calc}\n" );
   fprintf( p, "\\usepackage{wallpaper}\n" );
   fprintf( p, "\\pagestyle{empty}\n" );
   fprintf( p, "\\usepackage[bottom=0cm,top=0cm,left=0cm,right=0cm]{geometry}\n" );

   sprintf( sx, "%10.6f", l );
   sx[3] = '.';
   sprintf( sy, "%10.6f", h );
   sy[3] = '.';
   fprintf( p, "\\geometry{paperwidth=%scm,paperheight=%scm}\n", sx, sy );

   fprintf( p, "\\pgfmathsetmacro{\\rot}{%d}\n", rot );
   sprintf( str, "%10.6f", pp );
   str[3] = '.';
   fprintf( p, "\\pgfmathsetmacro{\\p}{%s}\n", str );

   fprintf( p, "\\ThisULCornerWallPaper{1}{%s}\n", img );

   fprintf( p, "\\makeatletter\n" );
   fprintf( p, "\\newcommand{\\dist}[3]{\n" );
   fprintf( p, "\\tikz@scan@one@point\\pgfutil@firstofone($#2-#3$)\\relax\n" );
   fprintf( p, "\\pgfmathsetmacro{#1}{round(0.99626*veclen(\\the\\pgf@x,\\the\\pgf@y)/0.0283465)/1000}\n" );
   fprintf( p, "}\n" );
   fprintf( p, "\\makeatother\n" );

   fprintf( p, "\\begin{document}\n" );

   fprintf( p, "{\\noindent\\small\n" );
   fprintf( p, "\\begin{tikzpicture}[baseline=(current bounding box.center)]\n" );

   sprintf( sx, "%10.6f", C[0].x );
   sx[3] = '.';
   sprintf( sy, "%10.6f", C[0].y );
   sy[3] = '.';
   fprintf( p, "\\coordinate (A) at (%s,-%s);\n", sy, sx );
   sprintf( sx, "%10.6f", C[1].x );
   sx[3] = '.';
   sprintf( sy, "%10.6f", C[1].y );
   sy[3] = '.';
   fprintf( p, "\\coordinate (B) at (%s,-%s);\n", sy, sx );
   sprintf( sx, "%10.6f", C[2].x );
   sx[3] = '.';
   sprintf( sy, "%10.6f", C[2].y );
   sy[3] = '.';
   fprintf( p, "\\coordinate (C) at (%s,-%s);\n", sy, sx );
   sprintf( sx, "%10.6f", C[3].x );
   sx[3] = '.';
   sprintf( sy, "%10.6f", C[3].y );
   sy[3] = '.';
   fprintf( p, "\\coordinate (D) at (%s,-%s);\n", sy, sx );

   fprintf( p, "\\ifthenelse{\\rot=90}{\n" );
   fprintf( p, "\\coordinate (F) at (B);\n" );
   fprintf( p, "\\coordinate (B) at (D);\n" );
   fprintf( p, "\\coordinate (D) at (F);\n" );
   fprintf( p, "}{}\n" );

   fprintf( p, "\\dist{\\dAB}{(A)}{(B)};\n" );
   fprintf( p, "\\dist{\\dBC}{(B)}{(C)};\n" );
   fprintf( p, "\\dist{\\dCD}{(C)}{(D)};\n" );
   fprintf( p, "\\dist{\\dAD}{(A)}{(D)};\n" );

   fprintf( p, "\\fill (0,0) circle (0pt);\n" );

   // Muda a dezena do número
   fprintf( p, "\\pgfmathsetmacro{\\a}{0.75}\n" );
   fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAD/(\\dBC/\\a+\\dAD-\\dBC)}\n" );
   fprintf( p, "\\coordinate (X) at ($(A)!\\d!(B)$);\n" );
   fprintf( p, "\\coordinate (Y) at ($(D)!\\d!(C)$);\n" );
   if ( n0 / 10 != n / 10 && t >> 6 & 1 ) {
      fprintf( p, "\\pgfmathsetmacro{\\a}{7/28+%d/14}\n", n0 / 10 );
      fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAB/(\\dCD/\\a+\\dAB-\\dCD)}\n" );
      fprintf( p, "\\fill[color=white] ($(X)!\\d!(Y)$) circle (0.24*\\p);\n" );
      fprintf( p, "\\pgfmathsetmacro{\\a}{7/28+%d/14}\n", n / 10 );
      fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAB/(\\dCD/\\a+\\dAB-\\dCD)}\n" );
      fprintf( p, "\\fill[color=black] ($(X)!\\d!(Y)$) circle (0.24*\\p);\n" );
   }

   // Insere a dezena do número quando este está ausente
   if ( !( t >> 6 & 1 ) ) {
      fprintf( p, "\\pgfmathsetmacro{\\a}{7/28+%d/14}\n", n / 10 );
      fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAB/(\\dCD/\\a+\\dAB-\\dCD)}\n" );
      fprintf( p, "\\fill[color=black] ($(X)!\\d!(Y)$) circle (0.24*\\p);\n" );
   }

   // Muda a unidade do número
   fprintf( p, "\\pgfmathsetmacro{\\a}{0.85}\n" );
   fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAD/(\\dBC/\\a+\\dAD-\\dBC)}\n" );
   fprintf( p, "\\coordinate (X) at ($(A)!\\d!(B)$);\n" );
   fprintf( p, "\\coordinate (Y) at ($(D)!\\d!(C)$);\n" );
   if ( n0 % 10 != n % 10 && t >> 7 & 1 ) {
      fprintf( p, "\\pgfmathsetmacro{\\a}{7/28+%d/14}\n", n0 % 10 );
      fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAB/(\\dCD/\\a+\\dAB-\\dCD)}\n" );
      fprintf( p, "\\fill[color=white] ($(X)!\\d!(Y)$) circle (0.24*\\p);\n" );
      fprintf( p, "\\pgfmathsetmacro{\\a}{7/28+%d/14}\n", n % 10 );
      fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAB/(\\dCD/\\a+\\dAB-\\dCD)}\n" );
      fprintf( p, "\\fill[color=black] ($(X)!\\d!(Y)$) circle (0.24*\\p);\n" );
   }

   // Insere a unidade do número quando este está ausente
   if ( !( t >> 7 & 1 ) ) {
      fprintf( p, "\\pgfmathsetmacro{\\a}{7/28+%d/14}\n", n % 10 );
      fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAB/(\\dCD/\\a+\\dAB-\\dCD)}\n" );
      fprintf( p, "\\fill[color=black] ($(X)!\\d!(Y)$) circle (0.24*\\p);\n" );
   }

   fprintf( p, "\\end{tikzpicture}}\n" );
   fprintf( p, "\\end{document}\n" );

   fclose( p );

   if ( chdir( "Relatórios e Provas/" ) != 0 ) {
      perror( "Erro ao acessar diretório de Relatórios" );
   }
   if ( system( "pdflatex -synctex=1 -interaction=nonstopmode img.tex" ) == -1 ) {
      fprintf( stderr, "Erro crítico: Não foi possível executar o pdflatex\n" );
   }
   sprintf( str, "convert -density 300 img.pdf -quality 100 %s", img );
   if ( system( str ) == -1 ) {
      fprintf( stderr, "Erro crítico: Não foi possível executar o comando: %s\n", str );
   }
   if ( dados->periodo[0] == 'R' ) {
      sprintf( str, "cp %s ../dados/'Gabaritos'/'%s'/'%s'/'%s'/'Imagens Recuperação Final'/", img, dados->ano, dados->disciplina, dados->escola );
   } else {
      sprintf( str, "cp %s ../dados/'Gabaritos'/'%s'/'%s'/'%s'/'Imagens %s Prova%d'/", img, dados->ano, dados->disciplina, dados->escola, dados->periodo, dados->iprova );
   }

   if ( system( str ) == -1 ) {
      fprintf( stderr, "Erro crítico: Não foi possível executar o comando: %s\n", str );
   }
   if ( system( "rm -f *.aux *.log *.synctex.gz *.tex *.pdf" ) == -1 ) {
      fprintf( stderr, "Erro crítico: Não foi possível remover arquivos latex\n" );
   }
   if ( chdir( "../" ) != 0 ) {
      perror( "Erro ao acessar diretório de Relatórios" );
   }

}
//========================================================================================================//



