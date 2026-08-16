/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "relatorios.h"
#include "basicas.h"
#include "interface.h"
#include "mensagens.h"
#include "glibrary.h"



GrupoHorario id_horarios[QTD_GRUPOS] = {
   {{{0, 0, 0, 0}, {0, 0, 0, 0}, {2858954, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},       "Letramento 1"},
   {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {2859150, 0, 0, 0}},       "Letramento 2"},
   {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {2859298, 0, 0, 0}},       "Letramento 3"},
   {{{0, 0, 0, 0}, {2859456, 2859465, 0, 0}, {2859497, 0, 0, 0}, {0, 0, 0, 0}, {2859522, 0, 0, 0}}, "Matemática 1"},
   {{{0, 0, 0, 0}, {2829645, 2829648, 0, 0}, {2829664, 0, 0, 0}, {0, 0, 0, 0}, {2829729, 0, 0, 0}}, "Matemática 2"},
   {{{0, 0, 0, 0}, {2829824, 0, 0, 0}, {2829841, 0, 0, 0}, {0, 0, 0, 0}, {2829900, 2829904, 0, 0}}, "Matemática 3"},
   {{{0, 0, 0, 0}, {2859639, 0, 0, 0}, {2859641, 2859644, 0, 0}, {0, 0, 0, 0}, {2859691, 0, 0, 0}}, "Matemática 4"},
   {{{0, 0, 0, 0}, {0, 0, 0, 0}, {2830418, 2830423, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, "Aprofundamento"},
   {{{0, 0, 0, 0}, {2830395, 2830398, 0, 0}, {2830402, 0, 0, 0}, {0, 0, 0, 0}, {2830477, 0, 0, 0}}, "Matemática 5"}
};




//########################################################################################################//
void atividades( const InterfaceDados *dados, const CaminhoDiretorio *caminho ) {

   int i, j, len, n, c, divisor, pesos[5], conceitos[dados->qtd_alunos_total][5];
   struct {
      char str[200];
   } AV[10]; // Recebe a 'string respostas' de todos os alunos de uma turma

   char str[2000];

   float fnota;
   int inota;

   bool teste;
   FILE *p;

   sprintf( str, "%s/avaliações.dat", caminho->dados );

   p = fopen( str, "r" );
   for ( i = 0; i < 10; i++ ) {
      snprintf( AV[i].str, sizeof AV[i].str, "%s", "" );
      if ( fgets( AV[i].str, sizeof AV[i].str, p ) == NULL ) {
         fprintf( stderr, "Erro ao ler linha de configuração.\n" );
      }
   }
   fclose( p );

   snprintf( AV[2].str, sizeof AV[2].str, "%s", "" );
   snprintf( AV[4].str, sizeof AV[4].str, "%s", "" );

   sprintf( str, "%s/Notas/atividades_av2.dat", caminho->externo );
   p = fopen( str, "r" );

   n = 0;
   while ( fscanf( p, "%d", &c ) != EOF ) {
      n++;
   }
   rewind( p );
   int nn = n / ( dados->qtd_alunos_total + 1 );

   n = 0;
   divisor = 0;
   while ( fscanf( p, "%d", &c ) != EOF ) {
      conceitos[n / nn][n % nn] = c;
      if ( n >= dados->qtd_alunos_total * nn ) {
         pesos[n % nn] = c;
         divisor += 4 * c;
      }
      n++;
   }
   fclose( p );

   for ( i = 0; i < dados->qtd_alunos_total; i++ ) {
      fnota = 0.;
      for ( j = 0; j < nn; j++ ) {
         fnota += conceitos[i][j] * pesos[j];
      }

      fnota /= divisor;
      inota = ( int )ceil( 10 * fnota );
      if ( inota == 10 ) {
         AV[2].str[i] = '#';
      } else if ( inota == 0 ) {
         AV[2].str[i] = '*';
      } else {
         AV[2].str[i] = inota + '0';
      }
   }

   sprintf( str, "%s/Notas/atividades_av3.dat", caminho->externo );
   p = fopen( str, "r" );

   n = 0;
   while ( fscanf( p, "%d", &c ) != EOF ) {
      n++;
   }
   rewind( p );
   nn = n / ( dados->qtd_alunos_total + 1 );

   n = 0;
   divisor = 0;
   while ( fscanf( p, "%d", &c ) != EOF ) {
      conceitos[n / nn][n % nn] = c;
      if ( n >= dados->qtd_alunos_total * nn ) {
         pesos[n % nn] = c;
         divisor += 4 * c;
      }
      n++;
   }
   fclose( p );

   for ( i = 0; i < dados->qtd_alunos_total; i++ ) {
      fnota = 0.;
      for ( j = 0; j < nn; j++ ) {
         fnota += conceitos[i][j] * pesos[j];
      }

      fnota /= divisor;
      inota = ( int )ceil( 10 * fnota );
      if ( inota == 10 ) {
         AV[4].str[i] = '#';
      } else if ( inota == 0 ) {
         AV[4].str[i] = '*';
      } else {
         AV[4].str[i] = inota + '0';
      }
   }



   for ( i = 0; i < 9; i++ ) {
      teste = false;
      len = strlen( AV[i].str );
      if ( len == 0 ) {
         for ( j = i + 1; j < 10; j++ ) {
            teste = teste || strlen( AV[j].str ) != 0;
         }
         if ( teste ) {
            AV[i].str[0] = '\n';
         }
      } else if ( len == dados->qtd_alunos_total ) {
         AV[i].str[dados->qtd_alunos_total] = '\n';
      } else if ( len == 2 * dados->qtd_alunos_total ) {
         AV[i].str[2 * dados->qtd_alunos_total] = '\n';
      }
   }


   sprintf( str, "%s/avaliações.dat", caminho->dados );

   p = fopen( str, "w+" );
   for ( i = 0; i < 10; i++ ) {
      fputs( AV[i].str, p );
   }
   fclose( p );
}
//########################################################################################################//





//########################################################################################################//
void atividadesQ( const InterfaceDados *dados, const CaminhoDiretorio *caminho ) {

   int i, j, len, n, c, nquestoes[5], conceitos[dados->qtd_alunos_total][5];
   struct {
      char str[200];
   } AV[10]; // Recebe a 'string respostas' de todos os alunos de uma turma

   char str[2000];

   float fnota;
   int inota;

   bool teste;
   FILE *p;


   sprintf( str, "%s/avaliações.dat", caminho->dados );

   p = fopen( str, "r" );
   for ( i = 0; i < 10; i++ ) {
      snprintf( AV[i].str, sizeof AV[i].str, "%s", "" );
      if ( fgets( AV[i].str, sizeof AV[i].str, p ) == NULL ) {
         fprintf( stderr, "Erro ao ler linha de configuração.\n" );
      }
   }
   fclose( p );

   snprintf( AV[2].str, sizeof AV[2].str, "%s", "" );
   snprintf( AV[4].str, sizeof AV[4].str, "%s", "" );

   sprintf( str, "%s/Notas/atividades_av2_av3.dat", caminho->externo );
   p = fopen( str, "r" );


   n = 0;
   while ( fscanf( p, "%d", &c ) != EOF ) {
      n++;
   }
   rewind( p );
   int nn = n / ( dados->qtd_alunos_total + 1 );

   n = 0;
   while ( fscanf( p, "%d", &c ) != EOF ) {
      if ( n >= dados->qtd_alunos_total * nn ) {
         nquestoes[n % nn] = c;
      } else {
         conceitos[n / nn][n % nn] = c;
      }
      n++;
   }
   fclose( p );


   for ( j = 0; j < nn; j++ ) {

      for ( i = 0; i < dados->qtd_alunos_total; i++ ) {

         fnota = ( float )conceitos[i][j] / nquestoes[j];

         inota = ( int )ceil( 10 * fnota );

         if ( inota == 10 ) {
            AV[2 * ( j + 1 )].str[i] = '#';
         } else if ( inota == 0 ) {
            AV[2 * ( j + 1 )].str[i] = '*';
         } else {
            AV[2 * ( j + 1 )].str[i] = inota + '0';
         }

      }
   }








   for ( i = 0; i < 9; i++ ) {
      teste = false;
      len = strlen( AV[i].str );
      if ( len == 0 ) {
         for ( j = i + 1; j < 10; j++ ) {
            teste = teste || strlen( AV[j].str ) != 0;
         }
         if ( teste ) {
            AV[i].str[0] = '\n';
         }
      } else if ( len == dados->qtd_alunos_total ) {
         AV[i].str[dados->qtd_alunos_total] = '\n';
      } else if ( len == 2 * dados->qtd_alunos_total ) {
         AV[i].str[2 * dados->qtd_alunos_total] = '\n';
      }
   }


   sprintf( str, "%s/avaliações.dat", caminho->dados );

   p = fopen( str, "w+" );
   for ( i = 0; i < 10; i++ ) {
      fputs( AV[i].str, p );
   }
   fclose( p );
}
//########################################################################################################//





//########################################################################################################//
void atividadesQT( const InterfaceDados *dados, const CaminhoDiretorio *caminho ) {

   int i, j, len, n, c, nquestoes[5], conceitos[dados->qtd_alunos_total][5];
   struct {
      char str[200];
   } AV[10]; // Recebe a 'string respostas' de todos os alunos de uma turma

   char str[2000];

   float fnota;
   int inota;

   bool teste;
   FILE *p;


   sprintf( str, "%s/avaliações.dat", caminho->dados );

   p = fopen( str, "r" );
   for ( i = 0; i < 10; i++ ) {
      snprintf( AV[i].str, sizeof AV[i].str, "%s", "" );
      if ( fgets( AV[i].str, sizeof AV[i].str, p ) == NULL ) {
         fprintf( stderr, "Erro ao ler linha de configuração.\n" );
      }
   }
   fclose( p );

   snprintf( AV[0].str, sizeof AV[0].str, "%s", "" );
   snprintf( AV[2].str, sizeof AV[2].str, "%s", "" );
   snprintf( AV[4].str, sizeof AV[4].str, "%s", "" );

   sprintf( str, "%s/Notas/atividades_av1_av2_av3.dat", caminho->externo );
   p = fopen( str, "r" );


   n = 0;
   while ( fscanf( p, "%d", &c ) != EOF ) {
      n++;
   }
   rewind( p );
   int nn = n / ( dados->qtd_alunos_total + 1 );

   n = 0;
   while ( fscanf( p, "%d", &c ) != EOF ) {
      if ( n >= dados->qtd_alunos_total * nn ) {
         nquestoes[n % nn] = c;
      } else {
         conceitos[n / nn][n % nn] = c;
      }
      n++;
   }
   fclose( p );


   for ( j = 0; j < nn; j++ ) {

      for ( i = 0; i < dados->qtd_alunos_total; i++ ) {

         fnota = ( float )conceitos[i][j] / nquestoes[j];

         inota = ( int )ceil( 10 * fnota );

         if ( inota == 10 ) {
            AV[2 * ( j + 0 )].str[i] = '#';
         } else if ( inota == 0 ) {
            AV[2 * ( j + 0 )].str[i] = '*';
         } else {
            AV[2 * ( j + 0 )].str[i] = inota + '0';
         }

      }
   }








   for ( i = 0; i < 9; i++ ) {
      teste = false;
      len = strlen( AV[i].str );
      if ( len == 0 ) {
         for ( j = i + 1; j < 10; j++ ) {
            teste = teste || strlen( AV[j].str ) != 0;
         }
         if ( teste ) {
            AV[i].str[0] = '\n';
         }
      } else if ( len == dados->qtd_alunos_total ) {
         AV[i].str[dados->qtd_alunos_total] = '\n';
      } else if ( len == 2 * dados->qtd_alunos_total ) {
         AV[i].str[2 * dados->qtd_alunos_total] = '\n';
      }
   }


   sprintf( str, "%s/avaliações.dat", caminho->dados );

   p = fopen( str, "w+" );
   for ( i = 0; i < 10; i++ ) {
      fputs( AV[i].str, p );
   }
   fclose( p );
}
//########################################################################################################//





//########################################################################################################//
void gerar_arquivo_siaep_notas( int qtd_linhas_av_rec, const FichaAluno *diario, const AppContext *ctx ) {
   const FocoCoordenadas *foco = &( ctx->cascata.foco );
   const InterfaceDados *dados = &( ctx->dados );
   const CaminhoDiretorio *caminho = &( ctx->caminho );

   char str[2048];
   int i, j, idx, nota, nota_rec;
   int n_aux[10][64] = {0};

   // Monta o caminho do arquivo de saída
   sprintf( str, "%s/siaep_notas.dat", caminho->relatorios );

   FILE *p = fopen( str, "w" );
   if ( !p ) {
      printf( "Erro ao criar arquivo SIAEP em: %s\n", str );
      return;
   }

   // Passo 1: Desfaz a ordenação alfabética usando o idx original
   // Colocamos as notas na ordem exata em que aparecem no portal (n_aux)
   for ( j = 0; j < qtd_linhas_av_rec; j++ ) {
      for ( i = 0; i < dados->qtd_alunos_total; i++ ) {
         idx = diario[i].idx;
         n_aux[j][idx] = diario[i].avaliacoes[foco->periodo][j];
      }
   }

   // Passo 2: Exporta para o formato de pipe (|) para o JavaScript
   for ( j = 0; j < qtd_linhas_av_rec; j++ ) {
      for ( i = 0; i < dados->qtd_alunos_total; i++ ) {
         // Lógica de recuperação (compara nota par com nota ímpar subsequente)
         if ( j % 2 == 0 ) {
            nota = n_aux[j][i];
            nota_rec = n_aux[j + 1][i];
         }

         // Tratamento de faltas (*) ou alunos que não fizeram a prova
         if ( n_aux[j][i] == -1 ) {
            fprintf( p, " %s", ( i == dados->qtd_alunos_total - 1 ) ? "\n" : "|" );
         }
         // Regra especial: se tirou >= 6 mas a recuperação foi maior, prevalece a recuperação
         else if ( j % 2 == 0 && nota >= 6 && nota_rec > nota ) {
            fprintf( p, "%d%s", nota_rec, ( i == dados->qtd_alunos_total - 1 ) ? "\n" : "|" );
         }
         // Nota padrão
         else {
            fprintf( p, "%d%s", n_aux[j][i], ( i == dados->qtd_alunos_total - 1 ) ? "\n" : "|" );
         }
      }
   }

   fclose( p );
}
//########################################################################################################//






//########################################################################################################//
int carregar_avaliacoes_do_periodo( char *arquivo_av, FichaAluno *diario, const InterfaceDados *dados,
                                    const FocoCoordenadas *foco ) {

   int i, j, len;

   for ( i = 0; i < dados->qtd_alunos_total; i++ ) {
      diario[i].media[ foco->periodo ] = ( float )0;
      for ( j = 0; j < 10; j++ ) {
         diario[i].avaliacoes[ foco->periodo ][ j ] = -1;
      }
   }

   char notas[256];
   notas[0] = '\0';

   FILE *p = fopen( arquivo_av, "r" );
   if ( !p ) {
      perror( "ERRO: Falha a abrir arquivo avaliações.dat" );
      return -1;
   }

   j = 0;
   while ( fgets( notas, sizeof notas, p ) != NULL ) {
      notas[ strcspn( notas, "\r\n" ) ] = '\0';
      len = strlen( notas );
      if ( len > 0 ) {
         if ( len == dados->qtd_alunos_total ) {
            for ( i = 0; i < dados->qtd_alunos_total; i++ ) {
               if ( notas[i] == '#' ) {
                  diario[i].avaliacoes[foco->periodo][j] = 10;
               } else if ( notas[i] != '*' ) {
                  diario[i].avaliacoes[foco->periodo][j] = notas[i] - '0';
               }
            }
            // } else if ( len == 2 * dados->qtd_alunos_total ) {
            //    for ( i = 0; i < dados->qtd_alunos_total; i+=2 ) {
            //       if ( notas[i] == '#' && notas[i+1] == '#' ) {
            //          diario[i].avaliacoes[foco->periodo][j] = 100;
            //       } else if ( notas[i] != '*' || notas[i+1] != '*' ) {
            //          diario[i].avaliacoes[foco->periodo][j] = notas[i] - '0';
            //       }
            //    }
            // } else if ( len == 3 * dados->qtd_alunos_total ) {
            //    for ( i = 0; i < dados->qtd_alunos_total; i+=3 ) {
            //       if ( notas[i] == '#' && notas[i+1] == '#' && notas[i+2] == '#' ) {
            //          diario[i].avaliacoes[foco->periodo][j] = 1000;
            //       } else if ( notas[i] != '*' || notas[i+1] != '*' || notas[i+2] != '*' ) {
            //          diario[i].avaliacoes[foco->periodo][j] = notas[i] - '0';
            //       }
            //    }
         } else {
            fprintf( stderr, "\nAVISO: Comprimento da linha %d de avaliações.dat não é múltiplo do total de alunos dessa turma\n", j + 1 );
         }
      }
      j++;
   }
   fclose( p );

   /*------ ESSE TRECHO CALCULA A MÉDIA DO PERÍODO -------*/
   int qtd_av = j / 2 + j % 2; // Quantidade de avaliações
   int qtd_linhas = j;
   int nota, rec;
   for ( i = 0; i < dados->qtd_alunos_total; i++ ) {
      for ( j = 0; j < qtd_linhas; j += 2 ) {
         nota = diario[i].avaliacoes[ foco->periodo ][ j ];
         rec = ( qtd_linhas == j + 1 ) ? 0 : diario[i].avaliacoes[ foco->periodo ][ j + 1 ];
         nota = ( rec > nota ) ? rec : nota;
         diario[i].media[ foco->periodo ] += ( nota == -1 ) ? 0 : nota;
      }
      diario[i].media[foco->periodo] /= ( float )qtd_av;
   }

   return qtd_linhas;
}
//########################################################################################################//





//########################################################################################################//
void gerar_tex_avaliacoes( StringNota notas[][10], StringNota *media, const char *nome_base, const AppContext *ctx ) {
   if ( !ctx ) return;
   const InterfaceDados *dados = &( ctx->dados );
   const FichaAluno *diario = ctx->diario;

   char arquivo_tex[512];
   sprintf( arquivo_tex, "./dados/temporarios/%s.tex", nome_base );
   FILE *p = fopen( arquivo_tex, "w+" );
   if ( p == NULL ) return;

   int j;

   // 1. Escrita do Preâmbulo e Configurações Iniciais
   fprintf( p,
            "\\documentclass[11pt,a4paper]{report}\n"
            "\\usepackage[utf8]{inputenc}\n"
            "\\usepackage[T1]{fontenc}\n" );

   // Seleção dinâmica da fonte conforme sua struct 'dados'
   if ( dados->fonte_latex == 1 ) {
      fputs( "\\usepackage{cmbright}\n", p );
   }

   fprintf( p,
            "\\usepackage[brazil]{babel}\n"
            "\\usepackage[left=0.58cm,right=0.7cm,top=0.7cm,bottom=0.7cm]{geometry}\n"
            "\\usepackage[table]{xcolor}\n"
            "\\usepackage{tikz}\n"
            "\\usepackage{multicol}\n"
            "\\usepackage{enumerate}\n"
            "\\usepackage{wasysym}\n"
            "\\usepackage{array,multirow,graphicx}\n"
            "\\usepackage{amssymb}\n"
            "\\usepackage{ifthen}\n"
            "\\usepackage{setspace}\n"
            "\\usepackage{ulem}\n"
            "\\newcolumntype{L}[1]{>{\\raggedright\\arraybackslash}p{#1}}\n"
            "\\newcolumntype{C}[1]{>{\\centering\\arraybackslash}p{#1}}\n"
            "\\newcolumntype{R}[1]{>{\\raggedleft\\arraybackslash}p{#1}}\n"
            "\\onehalfspacing\n"
            "\\pagestyle{empty}\n"
            "\\begin{document}\n" );

   // 2. Cálculo dinâmico do espaçamento vertical (spacing)
   // Mantendo sua fórmula original para ajuste automático na página A4
   double spacing = ( 297.0 - 6.5 - 6.5 - 4.0 * 7.0 ) / ( 4.96 * dados->qtd_alunos_total );
   fprintf( p, "\\begin{spacing}{%.4f}\n", spacing );

   // 3. Cabeçalho da Tabela (SEDUC / São Luís - MA)
   fprintf( p,
            "\\noindent\\begin{tabular}{|c|p{50mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{11mm}|}\\hline\n"
            "\\multicolumn{2}{|l|}{\\rule{0mm}{5.5mm}\\multirow{3}{50mm}{\\bf\\underline{SEDUC} / \\underline{São Luis $-$ MA}\\\\\\underline{%s}\\\\\\underline{%s}}}& \\multicolumn{11}{c|}{\\multirow{2}{130mm}{\\centering\\bf\\resizebox{13cm}{0.44cm}{Avaliações de %s do %s / %s}}} \\\\\n"
            "\\multicolumn{2}{|c|}{\\rule{0mm}{5.5mm}} & \\multicolumn{11}{c|}{} \\\\\\cline{3-13}\n"
            "\\multicolumn{2}{|c|}{\\rule{0mm}{5.5mm}} & \\resizebox{7.5mm}{11pt}{\\bf Av1} & \\resizebox{7.5mm}{11pt}{Rec} & \\resizebox{7.5mm}{11pt}{\\bf Av2} & \\resizebox{7.5mm}{11pt}{Rec} & \\resizebox{7.5mm}{11pt}{\\bf Av3} & \\resizebox{7.5mm}{11pt}{Rec} & \\resizebox{7.5mm}{11pt}{\\bf Av4} & \\resizebox{7.5mm}{11pt}{Rec} & \\resizebox{7.5mm}{11pt}{\\bf Av5} & \\resizebox{7.5mm}{11pt}{Rec} & \\resizebox{11mm}{11pt}{\\bf Média}\\\\\\hline\n",
            dados->escola, dados->turma, dados->disciplina, dados->periodo, dados->ano );

   // 4. Loop da Lista de Alunos (Sua lógica de ativos/inativos)
   for ( j = 0; j < dados->qtd_alunos_total; j++ ) {

      if ( diario[j].ativo ) {
         fprintf( p, "%.2d & %.*s &%s&%s&%s&%s&%s&%s&%s&%s&%s&%s&{\\bf %s} \\\\\\hline\n",
                  j + 1, diario[j].limite_corte, diario[j].aluno, notas[j][0].str, notas[j][1].str, notas[j][2].str,
                  notas[j][3].str, notas[j][4].str, notas[j][5].str, notas[j][6].str,
                  notas[j][7].str, notas[j][8].str, notas[j][9].str, media[j].str );

      } else {
         fprintf( p, "%.2d & \\textcolor{gray!70}{%.*s} & \\textcolor{gray!70}{%s} & \\textcolor{gray!70}{%s} & \\textcolor{gray!70}{%s} & \\textcolor{gray!70}{%s} & \\textcolor{gray!70}{%s} & \\textcolor{gray!70}{%s} & \\textcolor{gray!70}{%s} & \\textcolor{gray!70}{%s} & \\textcolor{gray!70}{%s} & \\textcolor{gray!70}{%s} & \\\\\\hline\n",
                  j + 1, diario[j].limite_corte, diario[j].aluno, notas[j][0].str, notas[j][1].str, notas[j][2].str,
                  notas[j][3].str, notas[j][4].str, notas[j][5].str, notas[j][6].str,
                  notas[j][7].str, notas[j][8].str, notas[j][9].str );
      }
   }

   // 5. Rodapé (Assinatura e Data)
   // Buscando a data atual ou a de dados->data
   char datatex[128];
   sprintf( datatex, "\\underline{\\,%.2d\\,}/\\underline{\\,%.2d\\,}/\\underline{\\,%d\\,}", ctx->data.dia, ctx->data.mes, ctx->data.ano );

   fprintf( p,
            "\\multicolumn{13}{|c|}{\\rule{0mm}{5.5mm}Professor(a): \\underline{\\includegraphics[width=0.28\\linewidth]{./dados/informados/.assinatura.png}} \\hspace{3cm}  Data: %s } \\\\\\hline\n"
            "\\end{tabular}\n"
            "\\end{spacing}\n"
            "\\end{document}\n",
            datatex );

   fclose( p );
}
//########################################################################################################//






//########################################################################################################//
void relatorio_de_avaliacoes( InterfacePainel *painel, const AppContext *ctx ) {
   const InterfaceDados   *dados   = &ctx->dados;
   const FocoCoordenadas  *foco    = &ctx->cascata.foco;
   const CaminhoDiretorio *caminho = &ctx->caminho;

   FichaAluno *diario = ctx->diario;

   char nome_base[64], arquivo[1024];
   sprintf( nome_base, "%s", "avaliações" );
   snprintf( arquivo, sizeof( arquivo ), "%s/%s.dat", caminho->dados, nome_base );
   if ( !verificar_estado_de_arquivo( arquivo, painel, dados ) ) return;



   int qtd_linhas_av_rec = carregar_avaliacoes_do_periodo( arquivo, diario, dados, foco );

   if ( qtd_linhas_av_rec == -1 ) {
      return;
   }

   gerar_arquivo_siaep_notas( qtd_linhas_av_rec, diario, ctx );

   snprintf( arquivo, sizeof( arquivo ), "%s/média.dat", caminho->dados );
   FILE *p = fopen( arquivo, "w" );
   if ( !p ) {
      fprintf( stderr, "ERRO: falha ao abrir arquivo %s", arquivo );
      return;
   }

   StringNota notas[dados->qtd_alunos_total][10], media[dados->qtd_alunos_total];

   int nota;
   float med;
   char snota[8], smedia[8];

   for ( int i = 0; i < dados->qtd_alunos_total; i++ ) {
      for ( int j = 0; j < 10; j++ ) {
         nota = diario[i].avaliacoes[ foco->periodo ][ j ];
         sprintf( snota, "%.1f", ( float )nota );
         // trocar_ponto_por_virgula( snota );
         sprintf( notas[i][j].str, "%s", ( nota == -1 ) ? "" : snota );
      }
      med = diario[i].media[ foco->periodo ];
      sprintf( smedia, "%.2f", med );
      // trocar_ponto_por_virgula( smedia );
      sprintf( media[i].str, "%s", smedia );
      fprintf( p, "%s\n", ( med == 0.0 ) ? "" : smedia );
   }

   fclose( p );

   gerar_tex_avaliacoes( notas, media, nome_base, ctx );

   disparar_latex( nome_base, caminho->relatorios, dados, caminho );

}
//########################################################################################################//





//########################################################################################################//
void relatorio_final( InterfacePainel *painel, const AppContext *ctx ) {
   ( void )painel;

   int foco_periodo = ctx->cascata.foco.periodo;
   char arquivo[1024];
   snprintf( arquivo, sizeof( arquivo ), "%s/lista.dat", ctx->caminho.dados );

   while ( !( verificar_arquivo( arquivo ) & ARQUIVO_PRONTO ) )  {
      if ( foco_periodo == 0 ) return;
      gtk_combo_box_set_active( GTK_COMBO_BOX( ctx->entry.periodo ), --foco_periodo );
      snprintf( arquivo, sizeof( arquivo ), "%s/lista.dat", ctx->caminho.dados );
   }

   const InterfaceDados *dados = &( ctx->dados );
   const InterfaceListas *listas = &( ctx->listas );
   const FichaAluno *diario = ctx->diario;
   const CaminhoDiretorio *caminho = &( ctx->caminho );

   int i, j;

   float fsoma, rec[dados->qtd_alunos_total], cons[dados->qtd_alunos_total];

   char str[3000], nota[10];

   struct {
      char str[100];
   } notas[dados->qtd_alunos_total][4], soma[dados->qtd_alunos_total], media[dados->qtd_alunos_total], recfinal[dados->qtd_alunos_total], conselho[dados->qtd_alunos_total], observacao[dados->qtd_alunos_total];

   for ( i = 0; i < dados->qtd_alunos_total; i++ ) {
      rec[i] = 0.;
      cons[i] = 0.;
      soma[i].str[0] = '\0';
      media[i].str[0] = '\0';
      recfinal[i].str[0] = '\0';
      conselho[i].str[0] = '\0';
      observacao[i].str[0] = '\0';
      for ( j = 0; j < 4; j++ ) {
         notas[i][j].str[0] = '\0';
      }
   }



   // Ponteiros para as médias dos períodos, recuperação final e conselho de classe
   FILE **p = ( FILE** )malloc( 6 * sizeof( FILE* ) );

   // 1. Copia o caminho base
   snprintf( str, sizeof( str ), "%s", caminho->dados );


   // 2. Localiza a última barra (o separador da turma para o período)
   char *saux = strrchr( str, '/' );

   // 3. Se não encontrar a barra, algo está errado com o caminho, então sai
   if ( saux == NULL ) return;

   for ( j = 0; j < 6; j++ ) {
      // 4. USAMOS O TAMANHO RESTANTE REAL: sizeof(str) - (saux - str)
      // saux aponta para a barra. Vamos escrever POR CIMA da barra ou logo após ela.
      // Para manter a barra original:
      snprintf( saux, sizeof( str ) - ( saux - str ), "/%s/%s", listas->periodos[j].str, "média.dat" );

      p[j] = fopen( str, "r" );

      if ( p[j] ) {
         i = 0;
         while ( fgets( nota, sizeof( nota ), p[j] ) != NULL ) {
            if ( nota[0] != '\n' ) {

               nota[strlen( nota ) - 1] = '\0';

               switch ( j + 1 ) {
               case 5 :
                  sprintf( recfinal[i].str, "%s", nota );
                  rec[i] = atof( nota );
                  break;
               case 6 :
                  sprintf( conselho[i].str, "%s", nota );
                  cons[i] = atof( nota );
                  break;
               default:
                  if ( diario[i].ativo ) {

                     sprintf( notas[i][j].str, "%s", nota );
                  } else {
                     sprintf( notas[i][j].str, "\\textcolor{gray!50}{%s}", nota );
                  }

               }
            }
            i++;
         }
         rewind( p[j] );
      }
   }


   for ( i = 0; i < dados->qtd_alunos_total; i++ ) {


      fsoma = atof( notas[i][0].str ) + atof( notas[i][1].str ) + atof( notas[i][2].str ) + atof( notas[i][3].str );

      if ( fsoma != 0 ) {

         sprintf( soma[i].str, "{\\bf %.2f}", fsoma );
         sprintf( media[i].str, "%.2f", 0.25 * fsoma );

         if ( p[5] && fgets( nota, sizeof nota, p[5] ) != NULL ) {
            if ( fsoma >= 22.2 )
               snprintf( observacao[i].str, sizeof observacao[i].str, "%s", "Aprov. na Média" );
            else if ( rec[i] >= 6.0 )
               snprintf( observacao[i].str, sizeof observacao[i].str, "%s", "Aprov. na Final" );
            else if ( cons[i] == 6.0 )
               snprintf( observacao[i].str, sizeof observacao[i].str, "%s", "\\resizebox{29mm}{8pt}{Aprov. no Conselho}" );
            else
               snprintf( observacao[i].str, sizeof observacao[i].str, "%s", "Reprovado(a)" );
         } else if ( p[4] && fgets( nota, sizeof nota, p[4] ) != NULL ) {
            if ( fsoma >= 22.2 )
               snprintf( observacao[i].str, sizeof observacao[i].str, "%s", "Aprov. na Média" );
            else if ( rec[i] >= 6.0 )
               snprintf( observacao[i].str, sizeof observacao[i].str, "%s", "Aprov. na Final" );
            else
               snprintf( observacao[i].str, sizeof observacao[i].str, "%s", "\\resizebox{29mm}{8pt}{\\it Conselho de Classe}" );
         } else if ( p[3] ) {
            if ( fsoma >= 22.2 )
               snprintf( observacao[i].str, sizeof observacao[i].str, "%s", "Aprov. na Média" );
            else
               snprintf( observacao[i].str, sizeof observacao[i].str, "%s", "\\resizebox{29mm}{8pt}{\\it Recuperação Final}" );
         } else if ( p[2] ) {
            if ( fsoma >= 24.0 )
               snprintf( observacao[i].str, sizeof observacao[i].str, "%s", "Aprov. na Média" );
            else if ( fsoma < 12.2 )
               snprintf( observacao[i].str, sizeof observacao[i].str, "%s", "\\resizebox{29mm}{8pt}{\\it Recuperação Final}" );
         }
      }
   }



   FILE *p0 = fopen( "./dados/templates/template_final.tex", "r" );

   FILE *p1 = fopen( "./dados/temporarios/Final.tex", "w+" );


   while ( fgets( str, sizeof str, p0 ) != NULL ) {
      if ( strcmp( str, "% FONTE\n" ) == 0 ) {
         if ( dados->fonte_latex == 1 )
            fputs( "\\usepackage{cmbright}\n", p1 );
         continue;
      } else if ( strcmp( str, "\\begin{spacing}{%f}\n" ) == 0 ) {
         fprintf( p1, str, ( 297. - 6.5 - 6.5 - 4 * 7. ) / ( 4.96 * dados->qtd_alunos_total ) );
         continue;
      } else if ( strncmp( str, "\\multicolumn{2}{|l|}{\\rule{0mm}{5.5mm}", 30 ) == 0 ) {
         fprintf( p1, str, dados->escola, dados->turma, dados->disciplina, dados->ano );
         continue;
      } else if ( strcmp( str, "% LISTA\n" ) == 0 ) {
         break;
      }
      fputs( str, p1 );
   }



   for ( j = 0; j < dados->qtd_alunos_total; j++ ) {
      if ( diario[j].ativo ) {
         fprintf( p1, "%.2d & %.*s & %s & %s & %s & %s & %s & %s & %s & %s & %s \\\\\\hline\n",
                  j + 1, diario[j].limite_corte, diario[j].aluno, notas[j][0].str, notas[j][1].str, notas[j][2].str, notas[j][3].str, soma[j].str, media[j].str, recfinal[j].str, conselho[j].str, observacao[j].str );
      } else {
         fprintf( p1, "%.2d & \\textcolor{gray!50}{%.*s} & %s & %s & %s & %s & %s & %s & %s & %s & %s \\\\\\hline\n",
                  j + 1, diario[j].limite_corte, diario[j].aluno, notas[j][0].str, notas[j][1].str, notas[j][2].str, notas[j][3].str, soma[j].str, media[j].str, recfinal[j].str, conselho[j].str, observacao[j].str );
      }
   }


   char datatex[128];
   sprintf( datatex, "\\underline{\\,%.2d\\,}/\\underline{\\,%.2d\\,}/\\underline{\\,%d\\,}", ctx->data.dia, ctx->data.mes, ctx->data.ano );

   while ( fgets( str, sizeof str, p0 ) != NULL ) {
      if ( strncmp( str, "\\multicolumn{11}{|c|}{\\rule{0mm}{5.5mm}", 30 ) == 0 ) {
         fprintf( p1, str, datatex );
         continue;
      }
      fputs( str, p1 );
   }


   fclose( p0 );
   fclose( p1 );
   for ( j = 0; j < 6; j++ ) {
      if ( p[j] ) fclose( p[j] );
   }
   free( p ); // Adicione isso para liberar a memória do array de ponteiros


   disparar_latex( "Final", caminho->relatorios_final, dados, caminho );

}
//########################################################################################################//





//########################################################################################################//
void relatorio_de_frequencia( InterfacePainel *painel, const AppContext *ctx ) {
   const InterfaceDados  *dados   = &( ctx->dados );
   const FichaAluno *diario = ctx->diario;
   const CaminhoDiretorio *caminho = &( ctx->caminho );

   char str[4000];
   sprintf( str, "%s/frequência.dat", caminho->dados );
   if ( !verificar_estado_de_arquivo( str, painel, dados ) ) return;

   FILE *p = fopen( str, "r" );

   bool eh_recuperacao = ( dados->periodo[0] == 'R' );



   char num[3000], str0[128], num0[100], str1[1000], P[1000], F[1000], P0[100], F0[100];

   struct {
      char str[5];
   } Meses[12] = {{"jan"}, {"fev"}, {"mar"}, {"abr"}, {"mai"}, {"jun"}, {"jul"}, {"ago"}, {"set"}, {"out"}, {"nov"}, {"dez"}}, meses[3] = {{""}, {""}, {""}};

   int i, j, k, len, nn = 0, ad = 0, ap = 0, ndias[4] = {0, 0, 0, 0}, pres[dados->qtd_alunos_total], falt[dados->qtd_alunos_total];

   memset( falt, 0, sizeof( falt ) ); // Zera todos os bytes do array

   while ( fgets( str, sizeof str, p ) != NULL ) nn++;
   rewind( p );

   int dia[nn], mes[nn], ch[nn];
   mes[0] = 1;

   struct {
      char str[5000];
      bool t;
   } colunas[nn];

   for ( i = 0; i < nn; i++ ) {
      if ( fgets( str, sizeof str, p ) == NULL ) {
         fprintf( stderr, "Erro ao ler linha de configuração.\n" );
      }
      len = strlen( str );
      str[ --len ] = '\0';
      dia[i] = ( str[0] - '0' ) * 10 + str[1] - '0';
      mes[i] = ( str[2] - '0' ) * 10 + str[3] - '0';
      ch[i] = str[5] - '0';
      ap += ch[i];
      ad += ch[i];
      snprintf( str1, sizeof str1, "%s", "" );
      if ( len > 6 ) {
         if ( str[7] == 'b' ) {
            colunas[i].t = false;
            ad -= ch[i];
            sprintf( colunas[i].str, "\\node[rotate=90,inner sep=0pt,color=blue] at ({7+(0.5+%d)*\\p},{-1.2-16.5*\\p}) {%s};\n", i, &str[8] );
         } else if ( str[7] == 'r' ) {
            colunas[i].t = false;
            ad -= ch[i];
            ap -= ch[i];
            sprintf( colunas[i].str, "\\node[rotate=90,inner sep=0pt,color=red] at ({7+(0.5+%d)*\\p},{-1.2-16.5*\\p}) {%s};\n", i, &str[8] );
         } else {
            colunas[i].t = true;
            size_t tam = strlen( &str[7] ); // Calcula uma vez só
            for ( j = 0; ( size_t )j < tam; j += 2 ) {
               k = ( str[7 + j] - '0' ) * 10 + str[8 + j] - '0';
               falt[k - 1] += ch[i];
               sprintf( str0, " \\OR \\i=%d", k );
               snprintf( str1 + strlen( str1 ), sizeof( str1 ) - strlen( str1 ), "%s", str0 );
            }
            sprintf( colunas[i].str, "\\foreach \\i in {%%d,...,%%d}{  \\ifthenelse{ \\i=0%%s }{}{ \\ifthenelse{ \\i=0%s }{ \\f%c{\\i-%%d}{%d} }{ \\p%c{\\i-%%d}{%d}}}}\n", str1, 96 + ch[i], i, 96 + ch[i], i );
         }
      } else if ( str[6] == '*' ) {
         colunas[i].t = true;
         sprintf( colunas[i].str, "\\foreach \\i in {%%d,...,%%d}{ \\ifthenelse{ \\i=0%%s }{}{ \\ifthenelse{ \\i=0%s }{ \\f%c{\\i-%%d}{%d} }{ \\p%c{\\i-%%d}{%d}}}}\n", str1, 96 + ch[i], i, 96 + ch[i], i );
      } else {
         ad -= ch[i];
         colunas[i].t = true;
         snprintf( colunas[i].str, sizeof colunas[i].str, "%s", "" );
      }

   }
   fclose( p );

   for ( i = 0; i < dados->qtd_alunos_total; i++ ) {
      pres[i] = ad - falt[i];
   }

   for ( i = 0; i < nn; i++ ) {
      ndias[1] += ( mes[0] + 0 == mes[i] );
      ndias[2] += ( mes[0] + 1 == mes[i] );
      ndias[3] += ( mes[0] + 2 == mes[i] );
   }

   ndias[3] += ( ndias[1] + ndias[2] ) * ( ndias[3] != 0 );
   ndias[2] += ndias[1] * ( ndias[2] != 0 );

   snprintf( meses[0].str, sizeof meses[0].str, "%s", Meses[mes[0] - 1].str );
   snprintf( meses[1].str, sizeof meses[1].str, "%s", Meses[mes[0]  ].str );
   snprintf( meses[2].str, sizeof meses[2].str, "%s", Meses[mes[0] + 1].str );



   p = fopen( "./dados/templates/template_freq.tex", "r" );

   FILE *p1 = fopen( "./dados/temporarios/Frequência.tex", "w+" );

   while ( fgets( str, sizeof str, p ) != NULL ) {
      if ( strcmp( str, "% FONTE\n" ) == 0 ) {
         if ( dados->fonte_latex == 1 )
            fputs( "\\usepackage{cmbright}\n", p1 );
         continue;
      } else if ( strcmp( str, "\\pgfmathsetmacro{\\nal}{%d}\n" ) == 0 ) {
         fprintf( p1, str, dados->qtd_alunos_total );
         continue;
      } else if ( strcmp( str, "\\pgfmathsetmacro{\\nn}{%d}\n" ) == 0 ) {
         fprintf( p1, str, nn );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt,right] at (1.2,{-1-1.5*(3*\\p+0.2)/3})", 45 ) == 0 ) {
         fprintf( p1, str, dados->escola );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt,right] at (1.2,{-1-2.4*(3*\\p+0.2)/3})", 45 ) == 0 ) {
         fprintf( p1, str, dados->turma );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt] at (17.85,{-1.05-(0.2+\\p)/2})", 45 ) == 0 ) {
         fprintf( p1, str, dados->disciplina, eh_recuperacao ? 'a' : 'o', dados->periodo, dados->ano );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt,above] at (14.85,-19.97)", 40 ) == 0 ) {
         if ( nn > 0 ) {
            fprintf( p1, str, ctx->data.dia, ctx->data.mes, ctx->data.ano, ad, ap );
         } else {
            fprintf( p1, "\\node[inner sep=0pt,above] at (14.85,-19.97) {Data: \\underline{\\qquad}/\\underline{\\qquad}/\\underline{\\,%d\\,} \\qquad Aulas dadas: \\underline{\\qquad} \\qquad Aulas previstas: \\underline{\\qquad} \\qquad Professor(a): \\underline{\\hspace{11cm}}};\n", ctx->data.ano );
         }
         continue;
      } else if ( strcmp( str, "\\foreach \\j in {%s}{\n" ) == 0 ) {
         snprintf( str1, sizeof str1, "%s", "" );
         for ( j = 0; j < fmin( dados->qtd_alunos_total, 27 ); j++ ) {
            if ( !( diario[j].ativo ) ) {
               sprintf( str0, "%d,", j + 1 );
               snprintf( str1 + strlen( str1 ), sizeof( str1 ) - strlen( str1 ), "%s", str0 );
            }
         }
         str1[ strlen( str1 ) - 1 ] = '\0';
         fprintf( p1, str, str1 );
         continue;
      } else if ( strcmp( str, "\\foreach \\j in {%s}{ \n" ) == 0 ) {
         snprintf( str1, sizeof str1, "%s", "" );
         for ( j = 27; j < dados->qtd_alunos_total; j++ ) {
            if ( !( diario[j].ativo ) ) {
               sprintf( str0, "%d,", j + 1 );
               snprintf( str1 + strlen( str1 ), sizeof( str1 ) - strlen( str1 ), "%s", str0 );
            }
         }
         int len1 = strlen( str1 );
         str1[( len1 <= 0 ) ? 0 : len1 - 1 ] = '\0'; // temos que ver isso aqui
         fprintf( p1, str, str1 );
         continue;
      } else if ( strcmp( str, "\\ifthenelse{\\i=0%s}{}{\n" ) == 0 ) {
         snprintf( str1, sizeof str1, "%s", "" );
         for ( j = 0; j < nn; j++ ) {
            if ( !colunas[j].t ) {
               sprintf( str0, " \\OR \\i=%d", j + 1 );
               snprintf( str1 + strlen( str1 ), sizeof( str1 ) - strlen( str1 ), "%s", str0 );
            }
         }
         fprintf( p1, str, str1 );
         continue;
      } else if ( strcmp( str, "% FERIADOS\n" ) == 0 ) {
         for ( j = 0; j < nn; j++ ) {
            if ( !colunas[j].t ) {
               fputs( colunas[j].str, p1 );
            }
         }
         continue;
      } else if ( strcmp( str, "% FREQUENCIA I\n" ) == 0 ) {
         snprintf( str1, sizeof str1, "%s", "" );
         for ( j = 0; j < dados->qtd_alunos_total; j++ ) {
            if ( !( diario[j].ativo ) ) {
               sprintf( str0, " \\OR \\i=%d", j + 1 );
               snprintf( str1 + strlen( str1 ), sizeof( str1 ) - strlen( str1 ), "%s", str0 );
            }
         }
         for ( j = 0; j < nn; j++ ) {
            if ( colunas[j].t && strlen( colunas[j].str ) != 0 ) {
               fprintf( p1, colunas[j].str, 1, ( int )fmin( dados->qtd_alunos_total, 27 ), str1, 0, 0 );
            }
         }
         continue;
      } else if ( strcmp( str, "% FREQUENCIA II\n" ) == 0 ) {
         snprintf( str1, sizeof str1, "%s", "" );
         for ( j = 0; j < dados->qtd_alunos_total; j++ ) {
            if ( !( diario[j].ativo ) ) {
               sprintf( str0, " \\OR \\i=%d", j + 1 );
               snprintf( str1 + strlen( str1 ), sizeof( str1 ) - strlen( str1 ), "%s", str0 );
            }
         }
         for ( j = 0; j < nn; j++ ) {
            if ( colunas[j].t && strlen( colunas[j].str ) != 0 ) {
               fprintf( p1, colunas[j].str, 28, dados->qtd_alunos_total, str1, 27, 27 );
            }
         }
         continue;
      } else if ( dados->qtd_alunos_total < 28 && strcmp( str, "% PAGINA 2\n" ) == 0 ) {
         fputs( "\\end{landscape}\n\\end{document}\n\n\n", p1 );
         break;
      } else if ( strcmp( str, "% DADOS\n" ) == 0 ) {
         snprintf( str, sizeof str, "%s",  "\\def\\alunos{{" );
         snprintf( num, sizeof num, "%s",  "\\def\\num{{" );
         for ( j = 0; j < dados->qtd_alunos_total; j++ ) {
            if ( diario[j].ativo ) {
               sprintf( str0, "\"%.*s\",", diario[j].limite_corte, diario[j].aluno );
               sprintf( num0, "\"%.2d\",", j + 1 );
            } else {
               sprintf( str0, "\"{\\color{gray!50}%.*s}\",", diario[j].limite_corte, diario[j].aluno );
               sprintf( num0, "\"{\\color{gray!50}%.2d}\",", j + 1 );
            }
            snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", str0 );
            snprintf( num + strlen( num ), sizeof( num ) - strlen( num ), "%s", num0 );
         }
         snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", "\"\"}}\n" );
         snprintf( num + strlen( num ), sizeof( num ) - strlen( num ), "%s", "\"\"}}\n" );

         fputs( str, p1 );
         fputs( num, p1 );


         if ( nn > 0 ) {

            snprintf( P, sizeof P, "%s",  "\\def\\pres{{" );
            snprintf( F, sizeof F, "%s",  "\\def\\falt{{" );
            for ( j = 0; j < dados->qtd_alunos_total; j++ ) {
               if ( diario[j].ativo ) {
                  sprintf( P0, "\"%d\",", pres[j] );
                  sprintf( F0, "\"%d\",", falt[j] );
               } else {
                  sprintf( P0, "\"\"," );
                  sprintf( F0, "\"\"," );
               }
               snprintf( P + strlen( P ), sizeof( P ) - strlen( P ), "%s", P0 );
               snprintf( F + strlen( F ), sizeof( F ) - strlen( F ), "%s", F0 );
            }
            snprintf( P + strlen( P ), sizeof( P ) - strlen( P ), "%s", "\"\"}}\n" );
            snprintf( F + strlen( F ), sizeof( F ) - strlen( F ), "%s", "\"\"}}\n" );

            fputs( P, p1 );
            fputs( F, p1 );

            fprintf( p1, "\\def\\ndias{{\"0\",\"%d\",\"%d\",\"%d\"}}\n", ndias[1], ndias[2], ndias[3] );
            fprintf( p1, "\\def\\mes{{\"%s\",\"%s\",\"%s\"}}\n", meses[0].str, meses[1].str, meses[2].str );

            snprintf( str, sizeof str, "%s",  "\\def\\dia{{" );
            for ( j = 0; j < nn; j++ ) {
               sprintf( str0, "\"%.2d\",", dia[j] );
               snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", str0 );
            }
            snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", "\"\"}}\n" );
            fputs( str, p1 );

         }

         continue;
      }

      fputs( str, p1 );
   }

   fclose( p );
   fclose( p1 );


   disparar_latex( "Frequência", caminho->relatorios, dados, caminho );


}
//########################################################################################################//







//########################################################################################################//
void relatorio_de_conteudos( InterfacePainel *painel, const AppContext *ctx ) {
   const InterfaceDados  *dados   = &( ctx->dados );
   const CascataControle *cascata = &( ctx->cascata );
   const CaminhoDiretorio *caminho = &( ctx->caminho );

   char str[3000];
   sprintf( str, "%s/conteúdos.dat", caminho->dados );
   if ( !verificar_estado_de_arquivo( str, painel, dados ) ) return;

   FILE *p = fopen( str, "r" );

   bool eh_recuperacao = ( dados->periodo[0] == 'R' );

   char str0[1024], format[1000];

   struct {
      char str[5];
   } Meses[12] = {{"jan"}, {"fev"}, {"mar"}, {"abr"}, {"mai"}, {"jun"}, {"jul"}, {"ago"}, {"set"}, {"out"}, {"nov"}, {"dez"}}, meses[3] = {{""}, {""}, {""}};

   int i, j, k, idx, len, somach = 0, nn = 0, ad = 0, ap = 0, ndias[4] = {0, 0, 0, 0};

   int offsets[] = {0, 1, 4}; // Onde começa cada disciplina (Só serve para uma única escola)

   int index = offsets[ cascata->foco.disciplina % cascata->limite.disciplinas ] +
               cascata->foco.turma % cascata->limite.turmas;

   long int ( *id_h )[4] = id_horarios[index].ids;

   while ( fgets( str, sizeof str, p ) != NULL ) nn++;
   rewind( p );

   int dia[nn], mes[nn], ch[nn], nhoras[nn];
   mes[0] = 1;

   struct {
      char str[5000];
   } linhas[nn];

   sprintf( str, "%s/siaep_cont.dat", caminho->relatorios );

   FILE *p2 = fopen( str, "w+" );

   for ( i = 0; i < nn; i++ ) {
      if ( fgets( str, sizeof str, p ) == NULL ) {
         fprintf( stderr, "Erro ao ler linha de configuração.\n" );
      }
      len = ( int )strlen( str );

      dia[i] = ( str[0] - '0' ) * 10 + str[1] - '0';
      mes[i] = ( str[2] - '0' ) * 10 + str[3] - '0';
      ch[i] = str[5] - '0';

      char *pontos = strchr( str, ':' );
      pontos += 2;
      idx = sdatefind( ':', str, len );

      //----------- Gravação dos dados no arquivo SIAEP -----------//
      k = dia_da_semana( dia[i], mes[i], ctx->data.ano );
      if ( k == 0 || k == 1 ) {
         snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", " (ERRO: DATA DE SÁBADO OU DOMINGO)" );
      } else if ( idx != -1 ) {
         for ( j = 0; j < ch[i]; j++ ) {
            if ( str[7] == 'r' || str[7] == 'b' ) {
               break;
            }
            int num_aux = ( i > 0 && dia[i] == dia[i - 1] ) ? 1 : 0;
            fprintf( p2, "%02d/%02d/%d;%ld;%d;%.*s;%s", dia[i], mes[i], ctx->data.ano, id_h[k - 2][j + num_aux],
                     cascata->foco.periodo + 1, idx - 7, &str[7], pontos );
         }
      }
      //----------------------------------------------------------//

      somach += ch[i];
      ap += ch[i];
      ad += ch[i];
      nhoras[i] = ( i > 0 ) ? nhoras[i - 1] + ch[i] : ch[i];
      str[--len] = '\0';
      if ( len > 7 ) {
         if ( str[7] == 'b' ) {
            ad -= ch[i];
            sprintf( linhas[i].str, "{\\bf\\color{blue}%s}", &str[8] );
         } else if ( str[7] == 'r' ) {
            ad -= ch[i];
            ap -= ch[i];
            sprintf( linhas[i].str, "{\\bf\\color{red}%s}", &str[8] );
         } else {
            if ( mes[i] > ctx->data.mes ) {
               ad -= ch[i];
            } else if ( mes[i] == ctx->data.mes && dia[i] > ctx->data.dia ) {
               ad -= ch[i];
            }
            j = sdatefind( ':', str, ( int )strlen( str ) );
            sprintf( format, "\\textbf{%%.%ds} $-$ %%s", j - 7 );
            sprintf( linhas[i].str, format, &str[7], &str[j + 1] );
         }
      } else {
         ad -= ch[i];
         linhas[i].str[0] = '\0';
      }
   }
   fclose( p );
   fclose( p2 );


   for ( i = 0; i < nn; i++ ) {
      ndias[1] += ( mes[0] + 0 == mes[i] ) * ch[i];
      ndias[2] += ( mes[0] + 1 == mes[i] ) * ch[i];
      ndias[3] += ( mes[0] + 2 == mes[i] ) * ch[i];
   }

   ndias[3] += ( ndias[1] + ndias[2] ) * ( ndias[3] != 0 );
   ndias[2] += ndias[1] * ( ndias[2] != 0 );


   snprintf( meses[0].str, sizeof meses[0].str, "%s", Meses[mes[0] - 1].str );
   snprintf( meses[1].str, sizeof meses[1].str, "%s", Meses[mes[0]  ].str );
   snprintf( meses[2].str, sizeof meses[2].str, "%s", Meses[mes[0] + 1].str );



   p = fopen( "./dados/templates/template_cont.tex", "r" );

   FILE *p1 = fopen( "./dados/temporarios/Conteúdos.tex", "w+" );

   while ( fgets( str, sizeof str, p ) != NULL ) {
      if ( strcmp( str, "% FONTE\n" ) == 0 ) {
         if ( dados->fonte_latex == 1 )
            fputs( "\\usepackage{cmbright}\n", p1 );
         continue;
      } else if ( strcmp( str, "\\pgfmathsetmacro{\\nn}{%d}\n" ) == 0 ) {
         fprintf( p1, str, nn );
         continue;
      } else if ( strcmp( str, "\\pgfmathsetmacro{\\p}{277/%d}\n" ) == 0 ) {
         if ( nn <= 22 ) {
            fprintf( p1, str, 480 );
            fprintf( p1, "\\pgfmathsetmacro{\\s}{%d}\n", 47 );
         } else {
            fprintf( p1, str, 10 * ( somach + 4 ) );
            fprintf( p1, "\\pgfmathsetmacro{\\s}{%d}\n", somach + 3 );
         }
         continue;
      } else if ( strncmp( str, "\\node[right] at (1,{-1-3*\\p/2})", 30 ) == 0 ) {
         fprintf( p1, str, dados->escola );
         continue;
      } else if ( strncmp( str, "\\node[right] at (1,{-1-5*\\p/2})", 30 ) == 0 ) {
         fprintf( p1, str, dados->turma );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt] at (13.5,{-1-\\p})", 35 ) == 0 ) {
         fprintf( p1, str, dados->disciplina, eh_recuperacao ? 'a' : 'o', dados->periodo, dados->ano );
         continue;
      } else if ( strncmp( str, "\\node[inner sep=0pt,above] at (10.5,-28.65)", 40 ) == 0 ) {
         fprintf( p1, str, ctx->data.dia, ctx->data.mes, ctx->data.ano, ad, ap );
         continue;
      } else if ( strcmp( str, "% DADOS\n" ) == 0 ) {

         snprintf( str, sizeof str, "%s",  "\\def\\cont{{" );
         for ( j = 0; j < nn; j++ ) {
            snprintf( str0, sizeof( str0 ), "\"%s\",", linhas[j].str );
            snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", str0 );
         }
         snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", "\"\"}}\n" );
         fputs( str, p1 );

         snprintf( str, sizeof str, "%s",  "\\def\\nhoras{{\"0\"," );

         for ( j = 0; j < nn; j++ ) {
            sprintf( str0, "\"%d\",", nhoras[j] );
            snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", str0 );
         }
         snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", "\"\"}}\n" );
         fputs( str, p1 );


         fprintf( p1, "\\def\\ndias{{\"0\",\"%d\",\"%d\",\"%d\"}}\n", ndias[1], ndias[2], ndias[3] );
         fprintf( p1, "\\def\\mes{{\"%s\",\"%s\",\"%s\"}}\n", meses[0].str, meses[1].str, meses[2].str );

         snprintf( str, sizeof str, "%s",  "\\def\\dia{{" );
         for ( j = 0; j < nn; j++ ) {
            sprintf( str0, "\"%.2d\",", dia[j] );
            snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", str0 );
         }
         snprintf( str + strlen( str ), sizeof( str ) - strlen( str ), "%s", "\"\"}}\n" );
         fputs( str, p1 );


         continue;
      }

      fputs( str, p1 );
   }

   fclose( p );
   fclose( p1 );


   disparar_latex( "Conteúdos", caminho->relatorios, dados, caminho );
}
//########################################################################################################//









void expor_relatorio_escola( const char *path_pdf_origem, const char *nome_arquivo_base,
                             const InterfaceDados *dados, const CaminhoDiretorio *caminho ) {

   char path_pdf_destino[768];

   if ( strcmp( nome_arquivo_base, "Final" ) == 0 ) { // O Relatório final deve ser salvo junto as pastas dos períodos
      snprintf( path_pdf_destino, sizeof( path_pdf_destino ), "%s/%s.pdf", caminho->externo_final, nome_arquivo_base );
   } else {
      snprintf( path_pdf_destino, sizeof( path_pdf_destino ), "%s/%s.pdf", caminho->externo, nome_arquivo_base );
   }

   if ( copiar_arquivo( path_pdf_origem, path_pdf_destino ) != 0 ) {
      fprintf( stderr, "ERRO CRÍTICO: Não foi possível salvar a cópia institucional na pasta da Escola!\n" );
   }

   snprintf( path_pdf_destino, sizeof( path_pdf_destino ), "%s/%s/Relatório_%s - %s - %s_%c - %s.pdf",
             caminho->externo_escola, nome_arquivo_base, nome_arquivo_base, dados->turma, dados->ano, dados->periodo[0], dados->disciplina );

   if ( copiar_arquivo( path_pdf_origem, path_pdf_destino ) != 0 ) {
      fprintf( stderr, "ERRO CRÍTICO: Não foi possível salvar a cópia institucional na pasta da Escola!\n" );
   }

}






void disparar_latex( const char *nome_base, const char *pasta_destino,
                     const InterfaceDados *dados, const CaminhoDiretorio *caminho ) {
   char comando[2048];
   char path_pdf_temp[512];
   char path_pdf_final[768];
   const char *pasta_temp = "./dados/temporarios";

   // 1. pdflatex ainda precisa do system
   snprintf( comando, sizeof( comando ),
             "pdflatex -synctex=1 -interaction=nonstopmode -output-directory='%s' '%s/%s.tex'",
             pasta_temp, pasta_temp, nome_base );

   if ( system( comando ) != 0 ) {
      fprintf( stderr, "Erro na compilação.\n" );
      return;
   }

   snprintf( path_pdf_temp, sizeof( path_pdf_temp ), "%s/%s.pdf", pasta_temp, nome_base );

   if ( dados->expor ) {
      expor_relatorio_escola( path_pdf_temp, nome_base, dados, caminho );
   }

   snprintf( path_pdf_final, sizeof( path_pdf_final ), "%s/%s.pdf", pasta_destino, nome_base );

   // 2. Mover nativamente
   if ( !gio_mover_arquivo( path_pdf_temp, path_pdf_final ) ) {
      perror( "Erro ao mover PDF" );
   }

   // 3. Limpeza seletiva e nativa
   apagar_arquivos_temporarios_latex_nativamente( "./dados/temporarios", nome_base, 4 );


   g_xdg_open( path_pdf_final );

}









void abrir_arquivos_de_dados( InterfacePainel *painel, const AppContext *ctx ) {
   ( void )painel;
   const InterfaceDados *dados = &( ctx->dados );
   const CaminhoDiretorio *caminho = &( ctx->caminho );

   const char *arquivos[5] = { "avaliações.dat", "lista.dat", "conteúdos.dat", "frequência.dat", "média.dat" };

   for ( int i = 0; i < 5; i++ ) {
      if ( dados->periodo[0] == 'R' && i == 0 ) continue;
      if ( dados->periodo[0] == 'C' && i != 4 ) continue;
      if ( i == 4 && dados->periodo[0] != 'C' ) continue;

      g_autofree char *arquivo_alvo = g_build_filename( caminho->dados, arquivos[i], NULL );

      // Olha que maravilha: apenas uma chamada limpa!
      // A verificação de existência e o tratamento de erro já estão blindados lá dentro.
      g_xdg_open( arquivo_alvo );
   }

   // O arquivo LEIA-ME também se resume a uma única linha agora
   g_xdg_open( "./dados/informados/.LEIA-ME.dat" );
}




