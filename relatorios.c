/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "comum.h"
#include "relatorios.h"
#include "basicas.h"
#include "interface.h"
#include "mensagens.h"
#include "glib_gio.h"



GrupoHorario id_horarios[QTD_GRUPOS] = {
   {{{0, 0, 0, 0}, {0, 0, 0, 0}, {2858954, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},       "Letramento 1"},
   {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {2859150, 0, 0, 0}},       "Letramento 2"},
   {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {2859298, 0, 0, 0}},       "Letramento 3"},
   {{{0, 0, 0, 0}, {2859456, 2859465, 0, 0}, {2859497, 0, 0, 0}, {0, 0, 0, 0}, {2859522, 0, 0, 0}}, "Matemática 1"},
   {{{0, 0, 0, 0}, {2859639, 0, 0, 0}, {2859641, 2859644, 0, 0}, {0, 0, 0, 0}, {2859691, 0, 0, 0}}, "Matemática 2"},
   {{{0, 0, 0, 0}, {2829645, 2829648, 0, 0}, {2829664, 0, 0, 0}, {0, 0, 0, 0}, {2829729, 0, 0, 0}}, "Matemática 3"},
   {{{0, 0, 0, 0}, {2829824, 0, 0, 0}, {2829841, 0, 0, 0}, {0, 0, 0, 0}, {2829900, 2829904, 0, 0}}, "Matemática 4"},
   {{{0, 0, 0, 0}, {0, 0, 0, 0}, {2830418, 2830423, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, "Aprofundamento"},
   {{{0, 0, 0, 0}, {2830395, 2830398, 0, 0}, {2830402, 0, 0, 0}, {0, 0, 0, 0}, {2830477, 0, 0, 0}}, "Matemática 5"}
};



//==================================================================================================
static void gerar_arquivo_siaep_notas( const AppContext *ctx ) {
   g_return_if_fail( ctx );

   const FocoCoordenadas *foco    = &( ctx->cascata.foco );
   const InterfaceDados  *dados   = &( ctx->dados );
   const CaminhoDiretorio *caminho = &( ctx->caminho );

   int qtd_alunos = dados->qtd_alunos_total;
   if ( qtd_alunos <= 0 ) return;

   // 1. Montagem segura do caminho
   g_autofree gchar *arquivo_saida = g_build_filename( caminho->relatorios, "siaep_notas.dat", NULL );

   // 2. Mapeamento por ponteiros para restaurar a ordem original (SIAEP)
   g_autofree const FichaAluno **ordem_original = g_new0( const FichaAluno *, qtd_alunos );
   for ( int i = 0; i < qtd_alunos; i++ ) {
      const FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, i );
      int idx = ficha->idx_siaep;
      if ( idx >= 0 && idx < qtd_alunos ) {
         ordem_original[idx] = ficha;
      }
   }

   int d = foco->disciplina;
   int per = foco->periodo;

   // Construção em memória usando GString (Velocidade Extrema de I/O)
   g_autoptr(GString) siaep = g_string_sized_new( qtd_alunos * 10 * 8 );

   // 3. Exporta exatamente 10 linhas fixas (5 avaliações x 2: AV e REC)
   for ( int j = 0; j < 10; j++ ) {
      int avaliacao_idx = j / 2;

      for ( int i = 0; i < qtd_alunos; i++ ) {
         const FichaAluno *aluno = ordem_original[i];

         if ( !aluno ) {
            g_string_append_printf( siaep, "%s", ( i == qtd_alunos - 1 ) ? "\n" : "|" );
            continue;
         }

         const NotaAvaliacao *nota = &aluno->disciplina[d].periodo[per].avaliacoes[avaliacao_idx];
         float nota_av  = nota->av;
         float nota_rec = nota->rec;
         float nota_atual = ( j % 2 == 0 ) ? nota_av : nota_rec;

         if ( nota_atual < 0.0f ) {
            g_string_append_printf( siaep, "%s", ( i == qtd_alunos - 1 ) ? "\n" : "|" );
         }
         else if ( j % 2 == 0 && nota_av >= 6.0f && nota_rec > nota_av ) {
            g_string_append_printf( siaep, "%.2f%s", nota_rec, ( i == qtd_alunos - 1 ) ? "\n" : "|" );
         }
         else {
            g_string_append_printf( siaep, "%.2f%s", nota_atual, ( i == qtd_alunos - 1 ) ? "\n" : "|" );
         }
      }
   }

   // 4. Descarga única de memória no disco
   g_autoptr(GError) error = NULL;
   if ( !g_file_set_contents( arquivo_saida, siaep->str, siaep->len, &error ) ) {
      g_printerr( "Erro ao salvar arquivo SIAEP em: %s (%s)\n", arquivo_saida, error->message );
   }
}
//==================================================================================================



//===================================================================================================
// BLOCO que gera o relatório de médias das avaliações
//===================================================================================================
static void gerar_tex_avaliacoes( const char *nome_base, const AppContext *ctx, const gboolean validas[5] ) {
   g_return_if_fail( ctx && ctx->fichas );
   const InterfaceDados  *dados = &( ctx->dados );
   const FocoCoordenadas *foco  = &( ctx->cascata.foco );

   g_autofree gchar *nome_tex = g_strdup_printf( "%s.tex", nome_base );
   g_autofree gchar *arquivo_tex = g_build_filename( ".", "dados", "temporarios", nome_tex, NULL );

   g_autoptr(GString) tex = g_string_sized_new( 16384 ); // Resumo alocado na memória

   // 1. Escrita do Preâmbulo e Configurações Iniciais
   g_string_append( tex,
      "\\documentclass[11pt,a4paper]{report}\n"
      "\\usepackage[utf8]{inputenc}\n"
      "\\usepackage[T1]{fontenc}\n" );

   if ( dados->fonte_latex == 1 ) g_string_append( tex, "\\usepackage{cmbright}\n" );

   g_string_append( tex,
      "\\usepackage[brazil]{babel}\n"
      "\\usepackage[left=0.58cm,right=0.7cm,top=0.7cm,bottom=0.7cm]{geometry}\n"
      "\\usepackage[table]{xcolor}\n"
      "\\usepackage{tikz}\n"
      "\\usepackage{multicol}\n"
      "\\usepackage{array,multirow,graphicx}\n"
      "\\usepackage{amssymb}\n"
      "\\usepackage{setspace}\n"
      "\\usepackage{ulem}\n"
      "\\newcolumntype{L}[1]{>{\\raggedright\\arraybackslash}p{#1}}\n"
      "\\newcolumntype{C}[1]{>{\\centering\\arraybackslash}p{#1}}\n"
      "\\newcolumntype{R}[1]{>{\\raggedleft\\arraybackslash}p{#1}}\n"
      "\\onehalfspacing\n"
      "\\pagestyle{empty}\n"
      "\\begin{document}\n" );

   // 2. Cálculo dinâmico do espaçamento vertical (spacing)
   double spacing = ( 297.0 - 6.5 - 6.5 - 4.0 * 7.0 ) / ( 4.96 * dados->qtd_alunos_total );
   g_string_append_printf( tex, "\\begin{spacing}{%.4f}\n", spacing );

   // 3. Cabeçalho da Tabela
   // A mágica acontece em "\multicolumn{1}{c|}{\rule[-1.5mm]{0mm}{5.5mm}\bf Av1}".
   // Isso centraliza o texto na vertical (pela expansão da \rule) e na horizontal (pelo c|).
   g_string_append_printf( tex,
      "\\noindent\\begin{tabular}{|c|p{50mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{7.5mm}|R{11mm}|}\\hline\n"
      "\\multicolumn{2}{|l|}{\\rule{0mm}{5.5mm}\\multirow{3}{50mm}{\\bf\\underline{SEDUC} / \\underline{São Luis $-$ MA}\\\\\\underline{%s}\\\\\\underline{%s}}}& \\multicolumn{11}{c|}{\\multirow{2}{130mm}{\\centering\\bf\\resizebox{13cm}{0.44cm}{Avaliações de %s do %s / %s}}} \\\\\n"
      "\\multicolumn{2}{|c|}{\\rule{0mm}{5.5mm}} & \\multicolumn{11}{c|}{} \\\\\\cline{3-13}\n"
      "\\multicolumn{2}{|c|}{\\rule{0mm}{5.5mm}} & "
      "\\multicolumn{1}{c|}{\\rule[-2.5mm]{0mm}{3mm}\\bf Av1} & \\multicolumn{1}{c|}{Rec} & "
      "\\multicolumn{1}{c|}{\\bf Av2} & \\multicolumn{1}{c|}{Rec} & "
      "\\multicolumn{1}{c|}{\\bf Av3} & \\multicolumn{1}{c|}{Rec} & "
      "\\multicolumn{1}{c|}{\\bf Av4} & \\multicolumn{1}{c|}{Rec} & "
      "\\multicolumn{1}{c|}{\\bf Av5} & \\multicolumn{1}{c|}{Rec} & "
      "\\multicolumn{1}{c|}{\\bf Média}\\\\\\hline\n",
      dados->escola, dados->turma, dados->disciplina, dados->periodo, dados->ano );

   int d   = foco->disciplina;
   int per = foco->periodo;

   // 4. Loop da Lista de Alunos (Formatação On-the-Fly Limpa e com Autoclean da Memória)
   for ( int j = 0; j < dados->qtd_alunos_total; j++ ) {
      const FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, j );

      // PASSO A: Nome do Aluno
      g_autofree gchar *s_nome = ficha->ativo
         ? g_strdup_printf( "%.*s", ficha->limite_corte, ficha->aluno )
         : g_strdup_printf( "\\textcolor{gray!70}{%.*s}", ficha->limite_corte, ficha->aluno );

      g_string_append_printf( tex, "%.2d & %s", j + 1, s_nome );

      // PASSO B: Extração e formatação das 5 avaliações
      for ( int k = 0; k < 5; k++ ) {
         if ( !validas[k] ) {
            g_string_append( tex, " & &" );
            continue;
         }

         const NotaAvaliacao *nota = &ficha->disciplina[d].periodo[per].avaliacoes[k];

         // Av
         g_autofree gchar *s_av = NULL;
         if ( nota->av >= 0.0f ) {
            const char *cor = ficha->ativo ? ( ( nota->av < 6.0f ) ? "red" : "black" ) : "gray!70";
            s_av = g_strdup_printf( "\\textcolor{%s}{%.1f}", cor, nota->av );
            g_strdelimit( s_av, ".", ',' ); // Troca nativa sem laço
         } else {
            s_av = g_strdup( "" );
         }

         // Rec
         g_autofree gchar *s_rec = NULL;
         if ( nota->rec >= 0.0f ) {
            const char *cor = ficha->ativo ? ( ( nota->rec < 6.0f ) ? "red" : "black" ) : "gray!70";
            s_rec = g_strdup_printf( "\\textcolor{%s}{%.1f}", cor, nota->rec );
            g_strdelimit( s_rec, ".", ',' );
         } else {
            s_rec = g_strdup( "" );
         }

         g_string_append_printf( tex, " & %s & %s", s_av, s_rec );
      }

      // PASSO C: Nota Média Consolidada
      g_autofree gchar *s_nota_periodo = NULL;
      if ( ficha->ativo && ficha->disciplina[d].relatorio.notas_periodos[per] >= 0.0f ) {
         float np = ficha->disciplina[d].relatorio.notas_periodos[per];
         const char *cor = ( np < 6.0f ) ? "red" : "black";
         s_nota_periodo = g_strdup_printf( "\\textcolor{%s}{\\bf %.2f}", cor, np );
         g_strdelimit( s_nota_periodo, ".", ',' );
      } else {
         s_nota_periodo = g_strdup( "" );
      }

      g_string_append_printf( tex, " & %s \\\\\\hline\n", s_nota_periodo );
   }

   // 5. Rodapé Seguro e Encerramento
   g_autofree gchar *datatex = g_strdup_printf( "\\underline{\\,%.2d\\,}/\\underline{\\,%.2d\\,}/\\underline{\\,%d\\,}",
                                          ctx->data.dia, ctx->data.mes, ctx->data.ano );

   g_string_append_printf( tex,
      "\\multicolumn{13}{|c|}{\\rule{0mm}{5.5mm}Professor(a): \\underline{\\includegraphics[width=0.28\\linewidth]{./dados/informados/.assinatura.png}} \\hspace{3cm}  Data: %s } \\\\\\hline\n"
      "\\end{tabular}\n"
      "\\end{spacing}\n"
      "\\end{document}\n",
      datatex );

   g_autoptr(GError) error = NULL;
   if ( !g_file_set_contents( arquivo_tex, tex->str, tex->len, &error ) ) {
      g_printerr( "Falha ao salvar o arquivo LaTeX: %s\n", error->message );
   }
}
//---------------------------------------------------------------------------------------------
static int obter_avaliacoes_validas( GtkComboBox *combo, gboolean validas[5] ) {
   g_return_val_if_fail( GTK_IS_COMBO_BOX( combo ), 0 );

   // Inicializa o vetor de segurança (todas desativadas por padrão)
   for ( int i = 0; i < 5; i++ ) validas[i] = FALSE;

   GtkTreeModel *model = gtk_combo_box_get_model( combo );
   if ( !model ) return 0;

   GtkTreeIter iter;
   int qtd_validas = 0;
   int idx = 0;

   if ( gtk_tree_model_get_iter_first( model, &iter ) ) {
      do {
         if ( idx >= 5 ) break; // Proteção estrita contra estouro de limite

         gboolean desativada = FALSE;
         gtk_tree_model_get( model, &iter, 1, &desativada, -1 );

         if ( !desativada ) {
            validas[idx] = TRUE;
            qtd_validas++;
         }
         idx++;
      } while ( gtk_tree_model_iter_next( model, &iter ) );
   }

   return qtd_validas;
}
// ============================================================================
// CORE: LÓGICA DE CÁLCULO DE NOTAS
// ============================================================================

/**
 * Calcula a média de um ÚNICO PERÍODO com base nas avaliações marcadas como válidas.
 * Atualiza: ficha->disciplina[disciplina].relatorio.notas_periodos[periodo] e ficha_modificada
 */
static void calcular_media_do_periodo( FichaAluno *ficha, int disciplina, int periodo,
                                       const gboolean validas[5], int qtd_validas ) {
   float soma_notas = 0.0f;
   gboolean tem_nota_valida = FALSE;
   float divisor = ( qtd_validas > 0 ) ? ( float )qtd_validas : 1.0f;

   const PeriodoLetivo *p_letivo = &ficha->disciplina[disciplina].periodo[periodo];

   for ( int j = 0; j < 5; j++ ) {
      if ( !validas[j] ) continue;

      float av  = p_letivo->avaliacoes[j].av;
      float rec = p_letivo->avaliacoes[j].rec;

      float max_nota = MAX( av, rec );
      if ( max_nota >= 0.0f ) {
         soma_notas += max_nota;
         tem_nota_valida = TRUE;
      }
   }

   RelatorioAnual *rel = &ficha->disciplina[disciplina].relatorio;

   if ( !tem_nota_valida || qtd_validas == 0 ) {
      rel->notas_periodos[periodo] = -1.0f;
   } else {
      rel->notas_periodos[periodo] = soma_notas / divisor;
      ficha->ficha_modificada = TRUE;
   }
}

/**
 * Calcula a SOMA ANUAL e a MÉDIA FINAL consolidando os 4 períodos.
 * Atualiza: ficha->disciplina[disciplina].relatorio.soma, media_anual e ficha_modificada
 */
void calcular_consolidado_anual( FichaAluno *ficha, int disciplina ) {
   if ( !ficha->ativo ) return;

   RelatorioAnual *rel = &ficha->disciplina[disciplina].relatorio;
   rel->soma = 0.0f;
   gboolean possui_dados = FALSE;

   // Soma apenas os períodos que já possuem médias processadas
   for ( int p = 0; p < 4; p++ ) {
      if ( rel->notas_periodos[p] >= 0.0f ) {
         rel->soma += rel->notas_periodos[p];
         possui_dados = TRUE;
      }
   }

   // Mantém a correção do bug (preserva aluno com soma 0.0 legítima)
   if ( !possui_dados ) {
      rel->soma        = -1.0f;
      rel->media_anual = -1.0f;
   } else {
      rel->media_anual = rel->soma / 4.0f;
      ficha->ficha_modificada = TRUE;
   }
}

void relatorio_de_avaliacoes( InterfacePainel *painel, const AppContext *ctx ) {
    g_return_if_fail( painel != NULL && ctx != NULL && ctx->fichas != NULL );

    const InterfaceDados   *dados   = &ctx->dados;
    const FocoCoordenadas  *foco    = &ctx->cascata.foco;
    const CaminhoDiretorio *caminho = &ctx->caminho;

    gerar_arquivo_siaep_notas( ctx );

    // 1. Obtém o mapa de configurações da UI
    gboolean validas[5];
    int qtd_validas = obter_avaliacoes_validas( GTK_COMBO_BOX( ctx->ui_diario.combo_avaliacoes ), validas );

    // 2. Processa o cálculo chamando o núcleo matemático (Limpo e Direto)
    for ( guint i = 0; i < ctx->fichas->len; i++ ) {
        FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, i );
        calcular_media_do_periodo( ficha, foco->disciplina, foco->periodo, validas, qtd_validas );
    }

    // 3. Exporta para LaTeX
    gerar_tex_avaliacoes( "avaliações", ctx, validas );
    disparar_latex( "avaliações", caminho->relatorios, dados, caminho );
}
//==================================================================================================





//===================================================================================================
// ESTRUTURA AUXILIAR: Encapsula as strings formatadas de uma linha do relatório LaTeX
//===================================================================================================
typedef struct {
   char nome[128];
   char notas[4][32];
   char soma[32];
   char media[32];
   char rec[32];
   char cons[32];
   char obs[128];
} LinhaRelatorioTeX;

//===================================================================================================
// FUNÇÃO AUXILIAR: Processa as regras de negócio e preenche a estrutura da linha TeX
//===================================================================================================
static void preencher_linha_relatorio_tex( const FichaAluno *ficha, int disc, LinhaRelatorioTeX *linha ) {
   // Zera toda a estrutura de uma vez para garantir strings limpas
   memset( linha, 0, sizeof( LinhaRelatorioTeX ) );

   const RelatorioAnual *rel = &ficha->disciplina[disc].relatorio;

   // --- PASSO A: Nome do Aluno ---
   if ( ficha->ativo ) {
      g_snprintf( linha->nome, sizeof( linha->nome ), "%.*s", ficha->limite_corte, ficha->aluno );
   } else {
      g_snprintf( linha->nome, sizeof( linha->nome ), "\\textcolor{gray!50}{%.*s}", ficha->limite_corte, ficha->aluno );
   }

   // --- PASSO B: Notas dos 4 Períodos e Contagem ---
   int qtd_periodos = 0;
   for ( int k = 0; k < 4; k++ ) {
      float med = rel->notas_periodos[k];
      if ( med >= 0.0f ) {
         const char *cor = ficha->ativo ? ( ( med < 6.0f ) ? "red" : "black" ) : "gray!50";
         g_snprintf( linha->notas[k], sizeof( linha->notas[k] ), "\\textcolor{%s}{%.2f}", cor, med );
         qtd_periodos++;
      }
   }

   // --- PASSO C: Soma, Média, Final e Conselho ---
   float soma  = rel->soma;
   float media = rel->media_anual;
   float rec   = rel->rec_final;
   float cons  = rel->conselho;

   if ( soma >= 0.0f ) {
      if ( ficha->ativo ) {
         g_snprintf( linha->soma, sizeof( linha->soma ), "{\\bf %.2f}", soma );

         // Aplicação da regra de nota vermelha também na média anual
         const char *cor_media = ( media < 6.0f ) ? "red" : "black";
         g_snprintf( linha->media, sizeof( linha->media ), "\\textcolor{%s}{%.2f}", cor_media, media );
      } else {
         g_snprintf( linha->soma, sizeof( linha->soma ), "\\textcolor{gray!50}{\\bf %.2f}", soma );
         g_snprintf( linha->media, sizeof( linha->media ), "\\textcolor{gray!50}{%.2f}", media );
      }
   }

   if ( rec >= 0.0f ) {
      const char *cor = ficha->ativo ? ( ( rec < 6.0f ) ? "red" : "black" ) : "gray!50";
      g_snprintf( linha->rec, sizeof( linha->rec ), "\\textcolor{%s}{%.2f}", cor, rec );
   }

   if ( cons >= 0.0f ) {
      const char *cor = ficha->ativo ? ( ( cons < 6.0f ) ? "red" : "black" ) : "gray!50";
      g_snprintf( linha->cons, sizeof( linha->cons ), "\\textcolor{%s}{%.2f}", cor, cons );
   }

   // --- PASSO D: Lógica de Status de Aprovação ---
   if ( soma >= 0.0f ) {
      char temp_obs[128] = {0};

      if ( cons >= 0.0f ) {
         if ( soma >= 22.2f ) g_strlcpy( temp_obs, "Aprov. Direto", sizeof( temp_obs ) );
         else if ( rec >= 6.0f ) g_strlcpy( temp_obs, "Aprov. Final", sizeof( temp_obs ) );
         else if ( cons >= 6.0f ) g_strlcpy( temp_obs, "\\resizebox{29mm}{8pt}{Aprov Cons}", sizeof( temp_obs ) );
         else g_strlcpy( temp_obs, "Reprovado", sizeof( temp_obs ) );

      } else if ( rec >= 0.0f ) {
         if ( soma >= 22.2f ) g_strlcpy( temp_obs, "Aprov. Direto", sizeof( temp_obs ) );
         else if ( rec >= 6.0f ) g_strlcpy( temp_obs, "Aprov. Final", sizeof( temp_obs ) );
         else g_strlcpy( temp_obs, "\\resizebox{29mm}{8pt}{\\it Conselho}", sizeof( temp_obs ) );

      } else if ( qtd_periodos == 4 ) {
         if ( soma >= 22.2f ) g_strlcpy( temp_obs, "Aprov. Direto", sizeof( temp_obs ) );
         else g_strlcpy( temp_obs, "\\resizebox{29mm}{8pt}{\\it Rec. Final}", sizeof( temp_obs ) );

      } else if ( qtd_periodos == 3 ) {
         if ( soma >= 24.0f ) g_strlcpy( temp_obs, "Aprov. Direto", sizeof( temp_obs ) );
         else if ( soma < 12.2f ) g_strlcpy( temp_obs, "\\resizebox{29mm}{8pt}{\\it Rec. Final}", sizeof( temp_obs ) );
      }

      if ( ficha->ativo ) {
         g_snprintf( linha->obs, sizeof( linha->obs ), "%s", temp_obs );
      } else {
         g_snprintf( linha->obs, sizeof( linha->obs ), "\\textcolor{gray!50}{%s}", temp_obs );
      }
   }
}

//===================================================================================================
// FUNÇÃO SECUNDÁRIA: Gera o arquivo LaTeX do Relatório Final diretamente na memória
//===================================================================================================
static void gerar_tex_relatorio_final( const char *nome_base, const AppContext *ctx ) {
   g_return_if_fail( ctx && ctx->fichas );
   const InterfaceDados *dados = &( ctx->dados );
   int disc = ctx->cascata.foco.disciplina;

   g_autofree gchar *nome_tex = g_strdup_printf( "%s.tex", nome_base );
   g_autofree gchar *arquivo_tex = g_build_filename( ".", "dados", "temporarios", nome_tex, NULL );

   g_autoptr(GString) tex = g_string_sized_new( 16384 ); // Resumo alocado na memória

   // 1. Escrita do Preâmbulo
   g_string_append( tex,
      "\\documentclass[11pt,a4paper]{report}\n"
      "\\usepackage[utf8]{inputenc}\n"
      "\\usepackage[T1]{fontenc}\n" );

   if ( dados->fonte_latex == 1 ) {
      g_string_append( tex, "\\usepackage{cmbright}\n" );
   }

   g_string_append( tex,
      "\\usepackage[brazil]{babel}\n"
      "\\usepackage[left=0.58cm,right=0.7cm,top=0.7cm,bottom=0.7cm]{geometry}\n"
      "\\usepackage{xcolor}\n"
      "\\usepackage{tikz}\n"
      "\\usepackage{multicol}\n"
      "\\usepackage{array,multirow,graphicx}\n"
      "\\usepackage{setspace}\n"
      "\\usepackage{ulem}\n"
      "\\newcolumntype{L}[1]{>{\\raggedright\\arraybackslash}p{#1}}\n"
      "\\newcolumntype{C}[1]{>{\\centering\\arraybackslash}p{#1}}\n"
      "\\newcolumntype{R}[1]{>{\\raggedleft\\arraybackslash}p{#1}}\n"
      "\\onehalfspacing\n"
      "\\pagestyle{empty}\n"
      "\\begin{document}\n" );

   // 2. Cálculo dinâmico do espaçamento vertical
   double spacing = ( 297.0 - 6.5 - 6.5 - 4.0 * 7.0 ) / ( 4.96 * dados->qtd_alunos_total );
   g_string_append_printf( tex, "\\begin{spacing}{%.4f}\n", spacing );

   // 3. Cabeçalho da Tabela
   // A mágica de centralização horizontal (c|) e vertical (\rule expandido) aplicada a cada coluna
   g_string_append_printf( tex,
      "\\noindent\\begin{tabular}{|c|p{5cm}|R{9.1mm}|R{9.1mm}|R{9.1mm}|R{9.1mm}|R{9.8mm}|R{9.1mm}|R{9.1mm}|R{9.1mm}|c|}\\hline\n"
      "\\multicolumn{2}{|l|}{\\rule{0mm}{5.5mm}\\multirow{3}{50mm}{\\bf\\underline{SEDUC} / \\underline{São Luis $-$ MA}\\\\\\underline{%s}\\\\\\underline{%s}}}& \\multicolumn{9}{c|}{\\multirow{2}{130mm}{\\centering\\bf\\LARGE Relatório Final de %s / %s}}\\\\\n"
      "\\multicolumn{2}{|c|}{\\rule{0mm}{5.5mm}} & \\multicolumn{9}{c|}{}\\\\\\cline{3-11}\n"
      "\\multicolumn{2}{|c|}{\\rule{0mm}{5.5mm}} & "
      "\\multicolumn{1}{c|}{\\rule[-2.5mm]{0mm}{3mm}\\bf 1º P} & "
      "\\multicolumn{1}{c|}{\\bf 2º P} & "
      "\\multicolumn{1}{c|}{\\bf 3º P} & "
      "\\multicolumn{1}{c|}{\\bf 4º P} & "
      "\\multicolumn{1}{c|}{\\bf Soma} & "
      "\\multicolumn{1}{c|}{\\bf Méd.} & "
      "\\multicolumn{1}{c|}{\\bf Final} & "
      "\\multicolumn{1}{c|}{\\bf Cons} & "
      "\\multicolumn{1}{c|}{\\bf Observação}\\\\\\hline\n",
      dados->escola, dados->turma, dados->disciplina, dados->ano );

   // 4. Loop dos Alunos com delegação para a função auxiliar
   for ( int j = 0; j < dados->qtd_alunos_total; j++ ) {
      const FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, j );
      LinhaRelatorioTeX linha;

      preencher_linha_relatorio_tex( ficha, disc, &linha );

      // Troca imediata de ponto para vírgula
      for ( int k = 0; k < 4; k++ ) {
         g_strdelimit( linha.notas[k], ".", ',' );
      }
      g_strdelimit( linha.soma, ".", ',' );
      g_strdelimit( linha.media, ".", ',' );
      g_strdelimit( linha.rec, ".", ',' );
      g_strdelimit( linha.cons, ".", ',' );

      g_string_append_printf( tex, "%.2d & %s & %s & %s & %s & %s & %s & %s & %s & %s & %s \\\\\\hline\n",
               j + 1, linha.nome,
               linha.notas[0], linha.notas[1], linha.notas[2], linha.notas[3],
               linha.soma, linha.media, linha.rec, linha.cons, linha.obs );
   }

   // 5. Rodapé Seguro e Encerramento
   g_autofree gchar *datatex = g_strdup_printf( "\\underline{\\,%.2d\\,}/\\underline{\\,%.2d\\,}/\\underline{\\,%d\\,}",
                                          ctx->data.dia, ctx->data.mes, ctx->data.ano );

   g_string_append_printf( tex,
      "\\multicolumn{11}{|c|}{\\rule{0mm}{5.5mm}Professor(a): \\underline{\\includegraphics[width=0.28\\linewidth]{../informados/.assinatura.png}} \\hspace{3cm}  Data: %s}\\\\\\hline\n"
      "\\end{tabular}\n"
      "\\end{spacing}\n"
      "\\end{document}\n",
      datatex );

   // 6. Descarga no Disco O(1)
   g_autoptr(GError) error = NULL;
   if ( !g_file_set_contents( arquivo_tex, tex->str, tex->len, &error ) ) {
      g_printerr( "Falha ao salvar o arquivo LaTeX: %s\n", error->message );
   }
}
//===================================================================================================

//===================================================================================================
// FUNÇÃO PRINCIPAL: Relatório Final
//===================================================================================================
void relatorio_final( InterfacePainel *painel, const AppContext *ctx ) {
   g_return_if_fail( painel != NULL && ctx != NULL && ctx->fichas != NULL );

   const InterfaceDados *dados = &( ctx->dados );
   const CaminhoDiretorio *caminho = &( ctx->caminho );
   int d = ctx->cascata.foco.disciplina;

   // 1. GARANTIA DE DADOS: Processa os cálculos de soma e média de todos os alunos
   // antes de gerar o PDF, blindando o sistema contra inconsistências.
   for ( guint i = 0; i < ctx->fichas->len; i++ ) {
      FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, i );
      calcular_consolidado_anual( ficha, d );
   }

   // 2. Chamada do Gerador LaTeX com os dados validados
   gerar_tex_relatorio_final( "Final", ctx );

   // 3. Dispara a compilação
   disparar_latex( "Final", caminho->relatorios_final, dados, caminho );
}





//==================================================================================================
static void gerar_preambulo_latex_frequencia( GString *tex, const AppContext *ctx ) {
   g_string_append( tex, "\\documentclass[11pt,a4paper]{report}\n"
                         "\\usepackage[utf8]{inputenc}\n"
                         "\\usepackage[T1]{fontenc}\n" );

   if ( ctx->dados.fonte_latex == 1 ) {
      g_string_append( tex, "\\usepackage{cmbright}\n" );
   }

   g_string_append( tex, "\\usepackage[brazil]{babel}\n"
                         "\\usepackage[left=0cm,right=0cm,top=0cm,bottom=0cm]{geometry}\n"
                         "\\usepackage[dvipsnames]{xcolor}\n"
                         "\\usepackage{pdflscape,lscape,tikz,ulem,graphicx}\n" // Removido 'ifthen'
                         "\\usetikzlibrary{calc}\n"
                         "\\pagestyle{empty}\n\n"
                         "\\begin{document}\n\\begin{landscape}\n\n" );
}

//-------------------------------------------------------------------------------------------------------------
static void gerar_pagina_frequencia( GString *tex, int pagina, const AppContext *ctx, int ad, int ap, int *pres, int *falt, GString *feriados, GString *marcacoes, int *meses_acumulados, int *meses_id, int num_meses ) {
   int start = ( pagina == 1 ) ? 1 : 28;
   int end = ( pagina == 1 ) ? 27 : 54;
   int offset = ( pagina == 1 ) ? 0 : 27;

   const InterfaceDados *dados = &ctx->dados;
   gboolean eh_recuperacao = ( dados->periodo[0] == 'R' );
   int nal = dados->qtd_alunos_total;
   int nn = ctx->diarios->len;
   double p = 199.0 / 330.0;

   g_string_append( tex, "\\noindent\\begin{tikzpicture}\n\\fill (0,0) circle (0pt);\n\\draw (1,-1) rectangle (28.7,-20);\n" );

   // Estruturas Fixas
   g_string_append_printf( tex,
      "\\draw (1.6,%.4f) -- (1.6,%.4f)\n"
      "(7,-1) -- (7,%.4f)\n"
      "(27.8,%.4f) -- (27.8,%.4f)\n"
      "(26.9,%.4f) -- (26.9,%.4f);\n"
      "\\node at (27.35,%.4f) {\\bf\\Large P};\n"
      "\\node at (28.25,%.4f) {\\bf\\Large F};\n"
      "\\draw (7,%.4f) -- (28.7,%.4f)\n"
      "(7,%.4f) -- (28.7,%.4f);\n",
      -1.2 - 3*p, -1.2 - 30*p,
      -1.2 - 30*p,
      -1.2 - 2*p, -1.2 - 30*p,
      -1.2 - 1*p, -1.2 - 30*p,
      -1.2 - 2.5*p, -1.2 - 2.5*p,
      -1.2 - p, -1.2 - p,
      -1.2 - 2*p, -1.2 - 2*p
   );

   // Grid Vertical
   for ( int i = 0; i <= 32; i++ ) {
      g_string_append_printf( tex, "\\draw (%.4f,%.4f) -- (%.4f,%.4f);\n", 7.0 + i*p, -1.2 - 2*p, 7.0 + i*p, -1.2 - 30*p );
   }

   // Topo (Datas)
   for ( int i = 0; i < nn; i++ ) {
      RegistroDiario *r = &g_array_index( ctx->diarios, RegistroDiario, i );
      int dia = 0;
      sscanf( r->data, "%d", &dia );
      g_string_append_printf( tex, "\\node[inner sep=0pt] at (%.4f,%.4f) {%02d};\n", 7.0 + p*(i + 0.5), -1.2 - 2.5*p, dia );
   }

   // Cabeçalhos de Meses (Calculado Dinamicamente)
   const char *m_str[] = {"", "jan", "fev", "mar", "abr", "mai", "jun", "jul", "ago", "set", "out", "nov", "dez"};
   int colunas_acumuladas_prev = 0;

   for ( int i = 0; i < num_meses; i++ ) {
      int colunas_acumuladas_curr = meses_acumulados[i];
      int m_val = meses_id[i];

      if ( colunas_acumuladas_curr > 0 && m_val >= 1 && m_val <= 12 ) {
         // Linha divisória vertical do fim do mês
         g_string_append_printf( tex, "\\draw (%.4f,%.4f) -- (%.4f,%.4f);\n",
                                 7.0 + colunas_acumuladas_curr*p, -1.2 - p,
                                 7.0 + colunas_acumuladas_curr*p, -1.2 - 2*p );

         // Centraliza o texto do mês entre a coluna final do mês passado e a atual
         double centro_x = 7.0 + 0.5 * (colunas_acumuladas_curr + colunas_acumuladas_prev) * p;
         g_string_append_printf( tex, "\\node at (%.4f,%.4f) {\\Large %s};\n",
                                 centro_x, -1.2 - 1.5*p, m_str[m_val] );
      }
      colunas_acumuladas_prev = colunas_acumuladas_curr;
   }

   // Nomes, Totais e processamento dos Alunos Inativos direto no laço C
   for ( int i = start; i <= end; i++ ) {
      int j = i - 1;
      double y_center = -1.2 - p * (i - offset + 2.5);
      double y_name   = -1.2 - p * (i - offset + 2.52);

      if ( i <= nal ) {
         const FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, j );

         if ( ficha->ativo ) {
            g_string_append_printf( tex, "\\node[inner sep=0pt] at (1.3,%.4f) {%02d};\n", y_center, i );
            g_string_append_printf( tex, "\\node[inner sep=0pt,right] at (1.7,%.4f) {%.*s};\n", y_name, ficha->limite_corte, ficha->aluno );
            g_string_append_printf( tex, "\\node[inner sep=0pt] at (27.35,%.4f) {\\bf %d};\n", y_center, pres[j] );
            g_string_append_printf( tex, "\\node[inner sep=0pt] at (28.25,%.4f) {\\bf %d};\n", y_center, falt[j] );
         } else {
            g_string_append_printf( tex, "\\node[inner sep=0pt] at (1.3,%.4f) {%02d};\n", y_center, i );
            g_string_append_printf( tex, "\\node[inner sep=0pt,right] at (1.7,%.4f) {\\color{gray!50}%.*s};\n", y_name, ficha->limite_corte, ficha->aluno );

            // Desenha os traços cinzas por cima das linhas do aluno inativo calculando X e Y nativamente
            int r_idx = i - offset;
            double y1_inativo = -1.2 - p*(2.8 + r_idx);
            double y2_inativo = -1.2 - p*(2.2 + r_idx);

            for( int k = 1; k <= 33; k++ ) {
               g_string_append_printf( tex, "\\draw[line width=0.8pt,gray!50] (%.4f,%.4f) -- (%.4f,%.4f);\n",
                  7.0 + (-0.8 + k)*p, y1_inativo, 7.0 + (-0.2 + k)*p, y2_inativo );
            }
            g_string_append_printf( tex, "\\draw[line width=0.8pt,gray!50] (%.4f,%.4f) -- (%.4f,%.4f) (%.4f,%.4f) -- (%.4f,%.4f);\n",
               27.35 - 0.3*p, y1_inativo, 27.35 + 0.3*p, y2_inativo, 28.25 - 0.3*p, y1_inativo, 28.25 + 0.3*p, y2_inativo );
         }
      } else {
         g_string_append_printf( tex, "\\node[inner sep=0pt] at (1.3,%.4f) {%d};\n", y_center, i ); // Linhas vazias
      }
   }

   // Identificadores de Turma e Disciplina
   g_string_append_printf( tex,
      "\\node[inner sep=0pt,right] at (1.2,%.4f) {\\large\\bf\\underline{SEDUC} / \\underline{São Luis$-$MA}};\n"
      "\\node[inner sep=0pt,right] at (1.2,%.4f) {\\large\\bf\\underline{%s}};\n"
      "\\node[inner sep=0pt,right] at (1.2,%.4f) {\\large\\bf\\underline{%s}};\n"
      "\\node[inner sep=0pt] at (17.85,%.4f) {\\bf\\resizebox{20cm}{0.55cm}{Relatório de Frequência em %s d%c %s / %s}};\n",
      -1.0 - 0.6*(3*p+0.2)/3.0,
      -1.0 - 1.5*(3*p+0.2)/3.0, dados->escola,
      -1.0 - 2.4*(3*p+0.2)/3.0, dados->turma,
      -1.05 - (0.2+p)/2.0, dados->disciplina, eh_recuperacao ? 'a' : 'o', dados->periodo, dados->ano );

   // Grid Horizontal
   for ( int i = 0; i <= 27; i++ ) {
      g_string_append_printf( tex, "\\draw (1,%.4f) -- (28.7,%.4f);\n", -1.2 - p*(i+3), -1.2 - p*(i+3) );
   }

   // Rodapé
   if ( nn > 0 ) {
      g_string_append_printf( tex, "\\node[inner sep=0pt,above] at (14.85,-19.97) {Data: \\underline{\\,%.2d\\,}/\\underline{\\,%.2d\\,}/\\underline{\\,%d\\,} \\qquad Aulas dadas: \\underline{\\,%d\\,} \\qquad Aulas previstas: \\underline{\\,%d\\,} \\qquad Professor(a): \\underline{\\includegraphics[width=0.2\\linewidth]{../informados/.assinatura.png}}};\n",
                              ctx->data.dia, ctx->data.mes, ctx->data.ano, ad, ap );
   } else {
      g_string_append_printf( tex, "\\node[inner sep=0pt,above] at (14.85,-19.97) {Data: \\underline{\\qquad}/\\underline{\\qquad}/\\underline{\\,%d\\,} \\qquad Aulas dadas: \\underline{\\qquad} \\qquad Aulas previstas: \\underline{\\qquad} \\qquad Professor(a): \\underline{\\hspace{11cm}}};\n", ctx->data.ano );
   }

   // Injeta os blocos processados no código chamador
   g_string_append( tex, marcacoes->str );
   g_string_append( tex, feriados->str );
   g_string_append( tex, "\\end{tikzpicture}\n" );
}

//------------------------------------------------------------------------------------------------------------------
void relatorio_de_frequencia( InterfacePainel *painel, const AppContext *ctx ) {
   g_return_if_fail( painel && ctx && ctx->diarios );
   const InterfaceDados *dados = &( ctx->dados );
   const CaminhoDiretorio *caminho = &( ctx->caminho );

   g_autoptr( GString ) bloco_feriados = g_string_new( "% FERIADOS E PEDAGÓGICOS\n" );
   g_autoptr( GString ) marcacoes_pg1 = g_string_new( "% MARCAÇÕES DE FREQUÊNCIA (PG 1)\n" );
   g_autoptr( GString ) marcacoes_pg2 = g_string_new( "% MARCAÇÕES DE FREQUÊNCIA (PG 2)\n" );

   int falt[64] = {0};
   int pres[64] = {0};
   int ad = 0, ap = 0;

   // Arrays para o mapeamento dinâmico dos meses (suporta até 12 meses no mesmo período)
   int meses_id[12] = {0};
   int meses_acumulados[12] = {0};
   int num_meses = 0;

   double p = 199.0 / 330.0;
   double sqrt3_6 = 0.28867513459; // sqrt(3)/6 pré-calculado

   // 1. Processamento direto do GArray em RAM
   for ( guint i = 0; i < ctx->diarios->len; i++ ) {
      RegistroDiario *r = &g_array_index( ctx->diarios, RegistroDiario, i );

      int dia, mes, ano;
      sscanf( r->data, "%d/%d/%d", &dia, &mes, &ano );

      // Verifica se é um mês novo e guarda no mapeamento dinâmico
      if ( num_meses == 0 || meses_id[num_meses - 1] != mes ) {
         if ( num_meses < 12 ) { // Blindagem contra overflow
            meses_id[num_meses] = mes;
            num_meses++;
         }
      }

      // Diferença em relação aos "Conteúdos": aqui a tabela cresce em colunas, não em carga horária.
      // O índice da coluna é "i". Logo, até agora, temos "i + 1" colunas acumuladas.
      if ( num_meses > 0 && num_meses <= 12 ) {
         meses_acumulados[num_meses - 1] = i + 1;
      }

      int ch = r->qtd_aulas;
      ap += ch;

      g_autofree gchar *tema_upper = NULL;
      g_autofree gchar *descricao_upper = NULL;

      double x_col = 7.0 + p * (i + 0.5); // Centro do Eixo X para a coluna da aula

      switch ( r->tipo_registro ) {
      case TIPO_REGISTRO_FERIADO:
      case TIPO_REGISTRO_PEDAGOGICO:
         ap -= (r->tipo_registro == TIPO_REGISTRO_FERIADO) ? ch : 0;
         tema_upper = g_utf8_strup( r->tema, -1 );
         descricao_upper = g_utf8_strup( r->descricao, -1 );
         g_string_append_printf( bloco_feriados,
               "\\node[rotate=90,inner sep=0pt,color=%s] at (%.4f,%.4f) {%s $\\to$ %s};\n",
               (r->tipo_registro == TIPO_REGISTRO_FERIADO) ? "red" : "blue",
               x_col, -1.2 - 16.5*p, tema_upper, descricao_upper );
         break;

      default: // Aulas Normais e Extras
         ad += ch;

         for ( int j = 0; j < dados->qtd_alunos_total; j++ ) {
            const FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, j );
            if ( !ficha->ativo ) continue;

            GString *target = ( j < 27 ) ? marcacoes_pg1 : marcacoes_pg2;
            int r_idx = ( j < 27 ) ? j : j - 27;
            double y_row = -1.2 - p * (r_idx + 3.5); // Centro do Eixo Y para a linha do aluno

            switch ( r->chamada[j].status ) {
            case PRESENTE:
               pres[j] += ch;
               if (ch == 1) {
                  g_string_append_printf( target, "\\fill (%.4f,%.4f) circle (1.5pt);\n", x_col, y_row );
               } else if (ch == 2) {
                  g_string_append_printf( target, "\\fill (%.4f,%.4f) circle (1.5pt) (%.4f,%.4f) circle (1.5pt);\n",
                     7.0 + p*(i + 1.0/3.0), y_row, 7.0 + p*(i + 2.0/3.0), y_row );
               } else {
                  g_string_append_printf( target, "\\fill (%.4f,%.4f) circle (1.5pt) (%.4f,%.4f) circle (1.5pt) (%.4f,%.4f) circle (1.5pt);\n",
                     x_col, -1.2 - p*(r_idx + 3.65 - sqrt3_6),
                     7.0 + p*(i + 1.0/3.0), -1.2 - p*(r_idx + 3.65), 7.0 + p*(i + 2.0/3.0), -1.2 - p*(r_idx + 3.65) );
               }
               break;

            case AUSENTE:
               falt[j] += ch;
               if (ch == 1) {
                  g_string_append_printf( target, "\\node at (%.4f,%.4f) {F};\n", x_col, y_row );
               } else if (ch == 2) {
                  g_string_append_printf( target, "\\node at (%.4f,%.4f) {\\scriptsize\\bf F}; \\node at (%.4f,%.4f) {\\scriptsize\\bf F};\n",
                     7.0 + p*(i + 1.0/3.0), y_row, 7.0 + p*(i + 2.0/3.0), y_row );
               } else {
                  g_string_append_printf( target, "\\node at (%.4f,%.4f) {\\tiny\\bf F}; \\node at (%.4f,%.4f) {\\tiny\\bf F}; \\node at (%.4f,%.4f) {\\tiny\\bf F};\n",
                     x_col, -1.2 - p*(r_idx + 3.65 - sqrt3_6),
                     7.0 + p*(i + 1.0/3.0), -1.2 - p*(r_idx + 3.65), 7.0 + p*(i + 2.0/3.0), -1.2 - p*(r_idx + 3.65) );
               }
               break;

            case FALTA_JUSTIFICADA:
               g_string_append_printf( target, "\\draw[Aquamarine,text=black,thick] (%.4f,%.4f) circle (0.23) node {\\scriptsize\\bf FJ};\n", x_col, y_row );
               break;
            case DISPENSADO:
               g_string_append_printf( target, "\\draw[YellowOrange,text=black,thick] (%.4f,%.4f) circle (0.23) node {D};\n", x_col, y_row );
               break;
            case FORA_DE_SALA:
               g_string_append_printf( target, "\\draw[YellowOrange,text=black,thick] (%.4f,%.4f) circle (0.23) node {\\scriptsize\\bf FS};\n", x_col, y_row );
               break;
            case FOI_EMBORA:
               falt[j] += ch;
               g_string_append_printf( target, "\\draw[red,text=black,thick] (%.4f,%.4f) circle (0.23) node {\\scriptsize\\bf FE};\n", x_col, y_row );
               break;
            case ATIVIDADE_DOMICILIAR:
               g_string_append_printf( target, "\\draw[Orchid,text=black,thick] (%.4f,%.4f) circle (0.23) node {\\scriptsize\\bf AD};\n", x_col, y_row );
               break;
            case SUSPENSO:
               falt[j] += ch;
               g_string_append_printf( target, "\\draw[red,text=black,thick] (%.4f,%.4f) circle (0.23) node {S};\n", x_col, y_row );
               break;
            case NOVO_ALUNO:
               g_string_append_printf( target, "\\draw[gray,thick] (%.4f,%.4f) circle (0.23) node {N};\n", x_col, y_row );
               break;
            default: break;
            }
         }
         break;
      }
   }

   // 3. Montagem do Arquivo LaTeX Final (Gerenciamento de Memória Dinâmico)
   g_autoptr( GString ) tex = g_string_sized_new( 16384 ); // Buffer otimizado

   gerar_preambulo_latex_frequencia( tex, ctx );

   // Renderiza Página 1 passando as variáveis de meses corrigidas
   gerar_pagina_frequencia( tex, 1, ctx, ad, ap, pres, falt, bloco_feriados, marcacoes_pg1, meses_acumulados, meses_id, num_meses );

   if ( dados->qtd_alunos_total >= 28 ) {
      g_string_append( tex, "\n\\newpage\n" );
      gerar_pagina_frequencia( tex, 2, ctx, ad, ap, pres, falt, bloco_feriados, marcacoes_pg2, meses_acumulados, meses_id, num_meses );
   }

   g_string_append( tex, "\n\\end{landscape}\n\\end{document}\n" );

   // 4. Salvamento e Disparo Atômico
   g_autofree gchar *arquivo_tex = g_build_filename( ".", "dados", "temporarios", "Frequência.tex", NULL );
   g_file_set_contents( arquivo_tex, tex->str, tex->len, NULL );

   disparar_latex( "Frequência", caminho->relatorios, dados, caminho );
}
//==================================================================================================





static void gerar_arquivo_siaep_cont( const AppContext *ctx, GArray *registros, const char *caminho_saida ) {
   g_autoptr( GString ) conteudo_siaep = g_string_new( "" );
   const CascataControle *cascata = &( ctx->cascata );

   int offsets[] = { 0, 1, 4 };
   int index = offsets[ cascata->foco.disciplina % cascata->limite.disciplinas ] +
               cascata->foco.turma % cascata->limite.turmas;

   // Gambiarra por eu ter modificado a lógica do combo da disciplina (GtkComboBox com liststore)
   if ( cascata->foco.turma >= 4 && cascata->foco.turma <= 6 ) index--;

   long int ( *id_h )[4] = id_horarios[index].ids;

   for ( guint i = 0; i < registros->len; i++ ) {
      RegistroDiario *d = &g_array_index( registros, RegistroDiario, i );

      if ( d->tipo_registro == TIPO_REGISTRO_FERIADO || d->tipo_registro == TIPO_REGISTRO_AULA_EXTRA ) continue;

      int dia = 0, mes = 0, ano = 0;
      sscanf( d->data, "%d/%d/%d", &dia, &mes, &ano );

      int k = dia_da_semana( dia, mes, ano );

      if ( k == 0 || k == 1 ) {
         g_string_append_printf( conteudo_siaep, " (ERRO: DATA DE SÁBADO OU DOMINGO %02d/%02d/%d)\n", dia, mes, ano );
      } else {
         for ( int j = 0; j < d->qtd_aulas; j++ ) {
            int num_aux = 0;
            if ( i > 0 ) {
               RegistroDiario *anterior = &g_array_index( registros, RegistroDiario, i - 1 );
               int dia_ant = 0;
               sscanf( anterior->data, "%d/", &dia_ant );
               if ( dia == dia_ant ) num_aux = 1;
            }

            // Formato exigido pelo seu script Java: DD/MM/YYYY;ID;PERIODO;LEN_TEMA;TEMA;DESCRICAO
            g_string_append_printf( conteudo_siaep, "%02d/%02d/%d;%ld;%d;%s;%s\n",
                                    dia, mes, ano,
                                    id_h[k - 2][j + num_aux],
                                    cascata->foco.periodo + 1,
                                    d->tema, d->descricao );
         }
      }
   }

   g_file_set_contents( caminho_saida, conteudo_siaep->str, -1, NULL );
}


static void gerar_latex_conteudos( const AppContext *ctx, GArray *registros, const char *caminho_saida ) {
   g_autoptr( GString ) tex = g_string_new( "" );
   const InterfaceDados *dados = &( ctx->dados );
   bool eh_recuperacao = ( dados->periodo[0] == 'R' );

   int nn = registros->len;
   int somach = 0, ad = 0, ap = 0;

   // Arrays para o mapeamento dinâmico dos meses (suporta até 12 meses no mesmo período)
   int meses_id[12] = {0};
   int meses_acumulados[12] = {0};
   int num_meses = 0;

   const char *Meses[] = { "", "jan", "fev", "mar", "abr", "mai", "jun", "jul", "ago", "set", "out", "nov", "dez" };

   // =========================================================================
   // PASSO 1: MAPEAMENTO DINÂMICO E CÁLCULO DOS TOTAIS
   // =========================================================================
   for ( int i = 0; i < nn; i++ ) {
      RegistroDiario *d = &g_array_index( registros, RegistroDiario, i );
      int dia = 0, mes = 0, ano = 0;
      sscanf( d->data, "%d/%d/%d", &dia, &mes, &ano );

      // Verifica se é um mês novo (assume que 'registros' está em ordem cronológica)
      if ( num_meses == 0 || meses_id[num_meses - 1] != mes ) {
         if ( num_meses < 12 ) { // Blindagem contra overflow
            meses_id[num_meses] = mes;
            num_meses++;
         }
      }

      somach += d->qtd_aulas;

      // Atualiza automaticamente o limite inferior (cumulativo) do mês atual
      if ( num_meses > 0 && num_meses <= 12 ) {
         meses_acumulados[num_meses - 1] = somach;
      }

      if ( d->tipo_registro == TIPO_REGISTRO_AULA_NORMAL ) ad += d->qtd_aulas; // Aulas Dadas
      if ( d->tipo_registro == TIPO_REGISTRO_AULA_EXTRA  ) ad += d->qtd_aulas; // Aulas Dadas
      if ( d->tipo_registro != TIPO_REGISTRO_FERIADO     ) ap += d->qtd_aulas; // Aulas Previstas
   }

   // =========================================================================
   // PASSO 2: CÁLCULOS MATEMÁTICOS DE COORDENADAS
   // =========================================================================
   int valor_p = ( nn <= 22 ) ? 480 : 10 * ( somach + 4 );
   int valor_s = ( nn <= 22 ) ? 47  : somach + 3;

   double p = 277.0 / (double)valor_p;
   double s = (double)valor_s;

   // --- CABEÇALHO DO DOCUMENTO LATEX ---
   g_string_append( tex, "\\documentclass[11pt,a4paper]{report}\n"
                         "\\usepackage[utf8]{inputenc}\n"
                         "\\usepackage[T1]{fontenc}\n" );

   if ( dados->fonte_latex == 1 ) g_string_append( tex, "\\usepackage{cmbright}\n" );

   g_string_append( tex, "\\usepackage[brazil]{babel}\n"
                         "\\usepackage[left=0cm,right=0cm,top=0cm,bottom=0cm]{geometry}\n"
                         "\\usepackage{tikz,ulem,graphicx}\n"
                         "\\usetikzlibrary{calc}\n"
                         "\\pagestyle{empty}\n\n"
                         "\\begin{document}\n"
                         "  \\noindent\\begin{tikzpicture}\n"
                         "  \\fill (0,0) circle (0pt);\n"
                         "  \\draw (1,-1) rectangle (20,-28.7);\n" );

   // Renderização das Linhas Estruturais Fixas
   g_string_append_printf( tex,
                         "  \\draw (2,%.4f) -- (2,%.4f)\n"
                         "        (%.4f,%.4f) -- (%.4f,%.4f)\n"
                         "        (7,-1) -- (7,%.4f)\n"
                         "        (7,%.4f) -- (20,%.4f)\n"
                         "        (1,%.4f) -- (20,%.4f)\n"
                         "        (1,%.4f) -- (20,%.4f);\n"
                         "  \\node[right] at (7,%.4f) {\\LARGE\\it Conteúdos};\n",
                         -1.0 - 3.0*p, -1.0 - s*p,
                         2.0 + p, -1.0 - 3.0*p, 2.0 + p, -1.0 - s*p,
                         -1.0 - 3.0*p,
                         -1.0 - 1.8*p, -1.0 - 1.8*p,
                         -1.0 - 3.0*p, -1.0 - 3.0*p,
                         -1.0 - s*p, -1.0 - s*p,
                         -1.0 - 2.45*p );

   // =========================================================================
   // PASSO 3: LAÇO DOS CONTEÚDOS
   // =========================================================================
   if ( nn > 0 ) {
      int h_acumulado_prev = 0;
      int h_acumulado_curr = 0;

      for ( int i = 0; i < nn; i++ ) {
         RegistroDiario *d = &g_array_index( registros, RegistroDiario, i );
         int dia = 0, mes = 0, ano = 0;
         sscanf( d->data, "%d/%d/%d", &dia, &mes, &ano );

         h_acumulado_prev = h_acumulado_curr;
         h_acumulado_curr += d->qtd_aulas;

         // Coordenadas Y calculadas diretamente em C
         double y_centro = -1.0 - ( 3.0 + 0.5 * ( h_acumulado_prev + h_acumulado_curr ) ) * p;
         double y_base   = -1.0 - p * ( h_acumulado_curr + 3.0 );

         g_autofree gchar *str_conteudo = NULL;
         if ( d->tipo_registro == 1 ) {
            str_conteudo = g_strdup_printf( "{\\color{blue}\\textbf{%s} $-$ %s}", d->tema, d->descricao );
         } else if ( d->tipo_registro == 2 ) {
            str_conteudo = g_strdup_printf( "{\\color{red}\\textbf{%s} $-$ %s}", d->tema, d->descricao );
         } else if ( d->tipo_registro == 3 ) {
            str_conteudo = g_strdup_printf( "{\\color{olive}\\textbf{%s} $-$ %s}", d->tema, d->descricao );
         } else {
            str_conteudo = g_strdup_printf( "\\textbf{%s} $-$ %s", d->tema, d->descricao );
         }

         g_string_append_printf( tex,
            "    \\node[inner sep=0pt] at (%.4f,%.4f) {%02d};\n"
            "    \\node[right] at (%.4f,%.4f) {\\parbox{17.15cm}{%s}};\n"
            "    \\draw (2,%.4f) -- (20,%.4f);\n",
            2.0 + p/2.0, y_centro, dia,
            2.0 + p, y_centro, str_conteudo,
            y_base, y_base );
      }
   }

   // =========================================================================
   // PASSO 4: DESENHO DINÂMICO DOS MESES
   // =========================================================================
   int h_mes_prev = 0;

   for ( int i = 0; i < num_meses; i++ ) {
      int mes_atual = meses_id[i];
      int h_mes_curr = meses_acumulados[i];

      // Validação extra de segurança para os índices de meses (1 a 12)
      if ( mes_atual >= 1 && mes_atual <= 12 ) {
         double y_linha_mes = -1.0 - p * ( 3.0 + h_mes_curr );
         double y_centro_mes = -1.0 - 3.0*p - 0.5 * ( h_mes_curr + h_mes_prev ) * p;

         g_string_append_printf( tex,
            "    \\draw (1,%.4f) -- (2,%.4f);\n"
            "    \\node at (1.5,%.4f) {\\Large %s};\n",
            y_linha_mes, y_linha_mes,
            y_centro_mes, Meses[mes_atual] );
      }
      h_mes_prev = h_mes_curr;
   }

   // =========================================================================
   // PASSO 5: RODAPÉ E FINALIZAÇÃO ESTRUTURAL
   // =========================================================================
   g_string_append_printf( tex,
      "  \\node[right] at (1,%.4f) {\\large\\bf\\underline{SEDUC} / \\underline{São Luis$-$MA}};\n"
      "  \\node[right] at (1,%.4f) {\\large\\bf\\underline{%s}};\n"
      "  \\node[right] at (1,%.4f) {\\large\\bf\\underline{%s}};\n"
      "  \\node[inner sep=0pt] at (13.5,%.4f) {\\bf\\resizebox{12.5cm}{0.55cm}{Conteúdos de %s d%c %s / %s}};\n"
      "  \\node[inner sep=0pt,above] at (10.5,-28.65) {Data: \\underline{\\,%02d\\,}/\\underline{\\,%02d\\,}/\\underline{\\,%d\\,} \\hspace{5mm} Aulas dadas: \\underline{\\,%d\\,} \\hspace{5mm} Aulas previstas: \\underline{\\,%d\\,} \\hspace{5mm} Professor(a): \\underline{\\includegraphics[width=0.2\\linewidth]{../informados/.assinatura.png}}};\n"
      "  \\end{tikzpicture}\n"
      "\\end{document}\n",
      -1.0 - p/2.0,
      -1.0 - 3.0*p/2.0, dados->escola,
      -1.0 - 5.0*p/2.0, dados->turma,
      -1.0 - p, dados->disciplina, ( eh_recuperacao ? 'a' : 'o' ), dados->periodo, dados->ano,
      ctx->data.dia, ctx->data.mes, ctx->data.ano, ad, ap );

   g_file_set_contents( caminho_saida, tex->str, -1, NULL );
}

//==================================================================================================
void relatorio_de_conteudos( InterfacePainel *painel, const AppContext *ctx ) {
   g_return_if_fail( painel && ctx );

   const InterfaceDados *dados = &( ctx->dados );
   const CaminhoDiretorio *caminho = &( ctx->caminho );

   // 1. Valida diretamente o GArray em memória em vez de checar o arquivo
   if ( !ctx->diarios || ctx->diarios->len == 0 ) {
      g_print( "⚠ Nenhum registro de diário disponível na memória.\n" );
      // Opcional: Você pode acionar um criar_mensagem_painel() aqui se quiser notificar a UI
      return;
   }

   // 2. Chama a geração do arquivo para o Script Java passando ctx->diarios
   g_autofree gchar *arquivo_siaep = g_build_filename( caminho->relatorios, "siaep_cont.dat", NULL );
   gerar_arquivo_siaep_cont( ctx, ctx->diarios, arquivo_siaep );

   // 3. Chama a geração do arquivo LaTeX substituindo o template descontinuado
   g_autofree gchar *arquivo_latex = g_build_filename( ".", "dados", "temporarios", "Conteúdos.tex", NULL );
   gerar_latex_conteudos( ctx, ctx->diarios, arquivo_latex );

   // 4. Dispara o compilador
   disparar_latex( "Conteúdos", caminho->relatorios, dados, caminho );
}
//==================================================================================================









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






