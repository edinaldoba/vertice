/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <glib/gstdio.h> // Necessário para g_remove e g_mkdir_with_parents

#include "comum.h"
#include "imagens.h"
#include "imgcore.h"
#include "basicas.h"   // Para nFile e Files
#include "gabaritos.h" // Para mudar_numero_na_imagem, gabaritos e imagens_corrigidas
#include "mensagens.h"
#include "glibrary.h"
#include "latex.h"
#include "pds.h"
#include "gas.h"






//========================================================================================================//
static void nome_aleatorio( char *nome, size_t tam ) {
   const char alfanumerico[] = "0123456789abcdefghijklmnopqrstuvwxyz";
   size_t len = tam - 1;

   for ( size_t j = 0; j < len; j++ ) {
      int i = g_random_int_range( 0, 36 );
      nome[j] = alfanumerico[i];
   }
   nome[len] = '\0';
}

static int converter_e_copiar_imagens( const char *origem, const char *destino, ItemTextoCurto **imgs_png ) {
   if ( !origem || !destino || !imgs_png ) return 0;

   GError *error = NULL;

   GDir *dir = g_dir_open( origem, 0, &error );
   if ( dir == NULL ) {
      g_printerr( "Aviso: Não foi possível abrir a pasta de origem: %s (%s)\n", origem, error->message );
      g_clear_error( &error );
      return 0;
   }

   // 1. FASE DE COLETA: Vetor dinâmico da GLib para guardar os nomes válidos
   GPtrArray *arquivos_validos = g_ptr_array_new_with_free_func( g_free );
   const char *filename;

   while ( ( filename = g_dir_read_name( dir ) ) != NULL ) {
      char *filename_lc = g_utf8_strdown( filename, -1 );

      gboolean e_imagem = ( g_str_has_suffix( filename_lc, ".png" )  ||
                            g_str_has_suffix( filename_lc, ".jpg" )  ||
                            g_str_has_suffix( filename_lc, ".jpeg" ) ||
                            g_str_has_suffix( filename_lc, ".bmp" ) );
      g_free( filename_lc );

      if ( e_imagem ) {
         // Guarda uma cópia da string no vetor
         g_ptr_array_add( arquivos_validos, g_strdup( filename ) );
      }
   }
   g_dir_close( dir );

   *imgs_png = g_new0( ItemTextoCurto, arquivos_validos->len );

   // 2. FASE DE PROCESSAMENTO PARALELO
   // OTIMIZAÇÃO: Usamos 'schedule(dynamic)' em vez de 'static'.
   // Como arquivos de imagem têm tamanhos diferentes, algumas conversões demoram mais que outras.
   // O 'dynamic' garante que threads rápidas peguem novos arquivos, evitando tempo ocioso.
   #pragma omp parallel for schedule(dynamic)
   for ( guint i = 0; i < arquivos_validos->len; i++ ) {

      // Resgata o nome do arquivo da lista
      const char *file_atual = ( const char * ) g_ptr_array_index( arquivos_validos, i );

      char nome_img[17];
      nome_aleatorio( nome_img, sizeof( nome_img ) );
      snprintf( ( *imgs_png )[i].str, sizeof( ( *imgs_png )[i].str ), "%s.png", nome_img );

      // Montagem de caminhos independente para cada thread
      char *path_origem = g_build_filename( origem, file_atual, NULL );
      char *path_destino = g_build_filename( destino, ( *imgs_png )[i].str, NULL );

      // O processamento interno pesado
      if ( converter_para_png_otimizado( path_origem, path_destino ) ) {
         // g_remove(path_origem);
      } else {
         // g_printerr é thread-safe no Linux, não corrompe o terminal
         g_printerr( "Falha no processamento da imagem: %s\n", file_atual );
      }

      g_free( path_origem );
      g_free( path_destino );
   }

   // 3. LIMPEZA FINAL
   // Libera o vetor e, automaticamente, todas as strings copiadas lá dentro
   int qtd_img = arquivos_validos->len;
   g_ptr_array_unref( arquivos_validos );

   return qtd_img;
}
//========================================================================================================//





static gboolean gas_mapear_ancoras( const ImagemCinza *img, MapeamentoGabarito *info, IndiceMatriz *ancora ) {
   // 1. Validação de segurança dos ponteiros de entrada
   g_return_val_if_fail( img && info && ancora, FALSE );

   guint32 sementes[4];
   gas_gerar_sementes( sementes );

   // Como usamos g_autoptr, a GLib fará o free automático no fim do escopo (não usar g_rand_free)
   g_autoptr( GRand ) rand_context = g_rand_new_with_seed_array( sementes, G_N_ELEMENTS( sementes ) );

   GasParametros par = {
      .n_pop        = 120,    // Tamanho da população (Calibrado)
      .n_gen        = 48,     // Quantidade de substituições (40%)
      .n_tor        = 2,      // Número de indivíduos no torneio
      .n_obj        = 4,      // Número de objetivos da coevolução
      .p_rec        = 0.80,   // Probabilidade de recombinação
      .p_mut        = 0.90,   // Probabilidade de mutação
      .peso_disp    = 1.8,    // Peso de dispersão inicial
      .toleracia    = 3.0e-1, // Tolerância geométrica
      .max_geracoes = 65,     // Limite máximo de gerações inicial
      .limiar       = 10,     // Limiar valor do pixel fitness local
      .rand         = rand_context
   };

   GasLimites *lim = gas_limites( img->nrow, img->ncol, par.n_obj );

   // 2. Primeira tentativa de convergência
   GasPopulacao *melhor = gas_pipeline( img, &par, lim );
   if ( melhor == NULL ) {
      gas_limites_liberar( lim, par.n_obj );
      return FALSE; // Força quarentena com segurança
   }

   double media_fitness = ( melhor[0].fitness + melhor[1].fitness + melhor[3].fitness + melhor[2].fitness ) / 4.0;
   gboolean sucesso = (media_fitness > 0.999);

   // 3. Mecanismo de resgate (Retry) para imagens com muito ruído
   if ( !sucesso ) {
      // CRÍTICO: Liberar a memória da primeira tentativa antes de alocar a nova
      gas_liberar_populacao( melhor, par.n_obj );

      par.peso_disp = 2.8; // Forçar exploração profunda
      par.max_geracoes = 100;
      melhor = gas_pipeline( img, &par, lim );

      media_fitness = ( melhor[0].fitness + melhor[1].fitness + melhor[3].fitness + melhor[2].fitness ) / 4.0;
      sucesso = (media_fitness > 0.999);
   }

   // 4. Extração das coordenadas reais
   for ( int k = 0; k < par.n_obj; k++ ) {
      ancora[k].j = (int)round(melhor[k].x[0]);
      ancora[k].i = (int)round(melhor[k].x[1]);
   }

   // 5. Determinação autônoma da direção da folha baseada na geometria das âncoras
   info->direcao = ( - ancora[0].i - ancora[1].i + ancora[2].i + ancora[3].i <
                     - ancora[0].j + ancora[1].j + ancora[2].j - ancora[3].j  ) ? 'h' : 'v';

   // ------------------------------------------------------------------------
   // LIMPEZA DE MEMÓRIA (DEEP FREE)
   // ------------------------------------------------------------------------

   gas_liberar_populacao( melhor, par.n_obj );
   gas_limites_liberar( lim, par.n_obj );

   return sucesso;
}



static void normalizar_ancora( const ImagemColorida *img_rgb, const ImagemCinza *img_gray, IndiceMatriz *ancora ) {
   // Validação de segurança padrão GLib
   g_return_if_fail( img_rgb && img_gray && ancora );
   g_return_if_fail( img_gray->ncol > 0 && img_gray->nrow > 0 ); // Proteção vital!

   // 1. Fatores de escala diretos (Largura e Altura)
   double scale_x = (double)img_rgb->ncol / (double)img_gray->ncol;
   double scale_y = (double)img_rgb->nrow / (double)img_gray->nrow;

   // 2. Aplicação da escala em cada uma das 4 âncoras
   for ( int ii = 0; ii < 4; ii++ ) {
      double x_scaled = ancora[ii].j * scale_x;
      double y_scaled = ancora[ii].i * scale_y;

      // Atualiza a âncora convertendo de volta para inteiro com arredondamento seguro
      ancora[ii].j = ( int )round( x_scaled );
      ancora[ii].i = ( int )round( y_scaled );
   }
}


//========================================================================================================//
int processar_imagens( const InterfaceDados *dados, const LimitesFiltro *limite ) {
   if ( !dados || !limite ) return -1;

   const char *home = g_get_home_dir();
   if ( home == NULL ) home = ".";

   g_autofree char *origem = g_build_filename( home, "Downloads", "imagens", NULL );
   g_autofree char *destino = g_build_filename( ".", "dados", "gabaritos", dados->ano, dados->escola, "imagens", NULL );
   g_autofree char *respostas = g_build_filename( ".", "dados", "gabaritos", dados->ano, dados->escola, "respostas", NULL );
   g_autofree char *dir_rejeitadas = g_build_filename( destino, "rejeitadas", NULL );

   if ( g_mkdir_with_parents( destino, 0755 ) != 0 ||
        g_mkdir_with_parents( dir_rejeitadas, 0755 ) != 0 ||
        g_mkdir_with_parents( respostas, 0755 ) != 0 )
   {
      g_printerr( "Erro crítico: Não foi possível criar os diretórios de destino.\n" );
      return -1;
   }

   g_autofree char *gabaritos = g_build_filename( ".", "dados", "gabaritos", dados->ano, dados->escola, "gabaritos", NULL );
   int qtd_bin = quantidade_arquivos_por_extensao( gabaritos, ".bin" );
   if ( qtd_bin <= 0 ) {
      g_printerr( "[AVISO] Nenhuma prova foi gerada até o momento.\n" );
      return -1;
   }
   ItemTextoCurto *resp_bin = carregar_arquivos_por_extensao( gabaritos, ".bin", qtd_bin );
   qsort( resp_bin, qtd_bin, sizeof( ItemTextoCurto ), comparar_item_texto_curto );

   FILE **f = ( FILE ** ) g_malloc0( qtd_bin * sizeof( FILE * ) );
   for ( int i = 0; i < qtd_bin; i++ ) {
      g_autofree char *arquivo = g_build_filename( respostas, resp_bin[i].str, NULL );
      f[i] = fopen( arquivo, "ab" );
   }

   ItemTextoCurto *imgs_png = NULL;
   int qtd_img = converter_e_copiar_imagens( origem, destino, &imgs_png );

   int n_rejeitadas = 0;

   // =========================================================================
   // LAÇO PARALELO (O OpenMP processará múltiplas imagens simultaneamente)
   // =========================================================================
   #pragma omp parallel for schedule(dynamic) reduction(+:n_rejeitadas)
   for ( int i = 0; i < qtd_img; i++ ) {

      gboolean sucesso = TRUE; // Substitui o goto!

      MapeamentoGabarito info = {0};
      IndiceMatriz ancora[4] = {0};

      ImagemColorida img_rgb_orig  = {0};
      ImagemColorida img_rgb_crop  = {0};

      ImagemCinza img_gray_bin   = {0};
      ImagemCinza img_gray_crop  = {0};
      ImagemCinza img_gray_alloc = {0};

      g_autofree char *path_png = g_build_filename( destino, imgs_png[i].str, NULL );
      g_autofree char *path_ppm = trocar_extensao( path_png, "ppm" );

      converter_para_ppm( path_png, path_ppm );

      // FASE 1: Carregamento
      imread( &img_rgb_orig, path_ppm );
      rgb2gray( &img_rgb_orig, &img_gray_bin );
      g_remove( path_ppm );

      // FASE 2: Normalização e Deskewing (Em Cinza para Visão)
      int dim = 960;
      redimensionar_imagem_bilinear( &img_gray_bin, &img_gray_alloc, dim );

      // FASE 3: Visão Computacional
      if ( !gas_mapear_ancoras( &img_gray_alloc, &info, ancora ) ) {
         fprintf( stderr, "[ERRO] Falha de âncoras na imagem: %s\n", imgs_png[i].str );
         sucesso = FALSE;
      }

      if ( sucesso ) {
         // FASE 4: Correção Geométrica Dupla
         cortar_imagem_bilinear( &img_gray_alloc, &img_gray_crop, ancora );

         normalizar_ancora( &img_rgb_orig, &img_gray_alloc, ancora );
         cortar_imagem_colorida_bilinear( &img_rgb_orig, &img_rgb_crop, ancora );

         salvar_imagem_png( &img_rgb_crop, path_png );

         // Binarização maravilhosa usando o Método de Otsu
         binarizar_pgm_metodo_otsu( &img_gray_crop );

         // FASE 5: Leitura do Payload
         info.payload = extrair_payload_matriz( &img_gray_crop, info.direcao );

         if ( !decodificar_payload_matriz( &info, limite ) ) {
            fprintf( stderr, "[FALHA] Payload inválido. Imagem: %s\n", imgs_png[i].str );
            sucesso = FALSE;
         }
      }

      // FASE 6: Identificação e Escrita (Se tudo deu certo até aqui)
      if ( sucesso ) {
         ItemTextoCurto chave;
         nome_base_gabaritos_bin( chave.str, sizeof( chave.str ), info.turma, info.disc, info.per, info.seq );
         int j = buscar_indice_bsearch( &chave, resp_bin, qtd_bin, sizeof( chave ), comparar_item_texto_curto );

         if ( j >= 0 && f[j] != NULL ) {
            ler_respostas_gabarito( &img_gray_crop, info.direcao, info.resp );
            info.num = ler_numero_aluno( &img_gray_crop, info.direcao );
            g_strlcpy( info.nome_img, imgs_png[i].str, sizeof( info.nome_img ) );

            // PROTEÇÃO CRÍTICA: Múltiplas threads não podem escrever no mesmo arquivo juntas!
            #pragma omp critical(escrita_binario)
            {
               if ( fwrite( &info, sizeof( MapeamentoGabarito ), 1, f[j] ) != 1 ) {
                  g_printerr( "[ERRO] O registro da imagem %s não foi salvo.\n", imgs_png[i].str );
               }
               fflush( f[j] ); // Garante que o dado vá fisicamente para o disco
            }

            // printf( "%3d %3d %3d %3d %3d | Número: %2d | Respostas: %s\n",
            //         info.id, info.turma, info.disc, info.per, info.seq, info.num, info.resp );
         } else {
            g_printerr( "[ALERTA] Binário '%s' não encontrado para: %s\n", chave.str, imgs_png[i].str );
            sucesso = FALSE;
         }
      }

      // ====================================================================
      // TRATAMENTO DE ERROS (Quarentena)
      // ====================================================================
      if ( !sucesso ) {
         g_autofree char *path_erro = g_build_filename( dir_rejeitadas, imgs_png[i].str, NULL );
         salvar_imagem_png( &img_rgb_orig, path_erro );
         g_remove( path_png );
         n_rejeitadas++;
      }

      // ====================================================================
      // LIMPEZA SEGURA DE MEMÓRIA
      // ====================================================================
      // Não precisamos mais rastrear o `ptr_gray_work`. Basta checar se a
      // estrutura tem um ponteiro alocado e liberar. Simples, direto e blindado.
      if ( img_gray_crop.image )  liberar_matriz_pixels( img_gray_crop.image, img_gray_crop.nrow );
      if ( img_gray_bin.image )   liberar_matriz_pixels( img_gray_bin.image, img_gray_bin.nrow );
      if ( img_gray_alloc.image ) liberar_matriz_pixels( img_gray_alloc.image, img_gray_alloc.nrow );
      if ( img_rgb_crop.image )   liberar_matriz_pixels_colorida( img_rgb_crop.image, img_rgb_crop.nrow );

      liberar_imagem_imread( &img_rgb_orig );
   }

   // Limpeza final de arquivos e arrays
   for ( int i = 0; i < qtd_bin; i++ ) {
      if ( f[i] != NULL ) fclose( f[i] );
   }
   g_free( f );
   g_free( imgs_png );
   free( resp_bin );

   puts( "Processamento das imagens concluído com sucesso!" );

   return n_rejeitadas;
}
//========================================================================================================//









static void copiar_arquivos_correcao_externamente( const InterfaceDados *dados, const CaminhoDiretorio *caminho,
                                                   const char *arquivo_saida )
{
   g_autofree char *nome_arquivo_escola = NULL;
   if ( dados->periodo[0] == 'R' ) {
      nome_arquivo_escola = g_strdup_printf( "Correção Recuperação Final - %s - %s - %s.pdf",
                                             dados->ano, dados->turma, dados->disciplina );
   } else {
      nome_arquivo_escola = g_strdup_printf( "Correção %s Prova - %s_%c - %s - %s.pdf",
                                             dados->prova_sequencia, dados->ano, dados->periodo[0],
                                             dados->turma, dados->disciplina );
   }

   g_autofree char *pasta_provas_escola = g_build_filename( caminho->externo_escola, "Correções", NULL );
   g_autofree char *destino_escola      = g_build_filename( pasta_provas_escola, nome_arquivo_escola, NULL );

   // Garante que a pasta "Provas" exista lá no drive/nuvem da escola
   g_mkdir_with_parents( pasta_provas_escola, 0777 );

   if ( !gio_copiar_arquivo( arquivo_saida, destino_escola ) ) {
      g_printerr( "Erro ao salvar a cópia institucional na pasta Provas da Escola!\n" );
   }
}


static void copiar_arquivos_correcao_nao_presencial( const MapeamentoGabarito *info, const int qtd_linhas,
                                                     const FichaAluno *diario, const InterfaceDados *dados,
                                                     const CaminhoDiretorio *caminho )
{
   g_autofree char *diretorio_imagens = NULL;

   // 1. Construção simplificada: Não precisamos criar o diretório "Notas" separadamente.
   // O g_mkdir_with_parents já cria toda a árvore genealógica de pastas se não existirem.
   if ( dados->periodo[0] == 'R' ) {
      diretorio_imagens = g_build_filename( caminho->externo, "Notas", "Recuperação Final Imagens Corrigidas", NULL );

   } else {
      g_autofree char *pasta_imagens = g_strdup_printf( "%s Prova Imagens Corrigidas", dados->prova_sequencia );
      diretorio_imagens = g_build_filename( caminho->externo, "Notas", pasta_imagens, NULL );
   }

   // 2. Apenas uma chamada de criação de pasta resolve tudo
   if ( g_mkdir_with_parents( diretorio_imagens, 0777 ) != 0 ) {
      g_printerr( "ERRO CRÍTICO: Falha ao criar a hierarquia externa de pastas: %s\n", diretorio_imagens );
      return;
   }

   // 3. Libere o poder do OpenMP! A conversão de PDF para PNG consome muita CPU.
   // Fazer isso em paralelo para 40 alunos economiza dezenas de segundos.
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < qtd_linhas; i++ ) {

      if ( info[i].status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
         int num_aluno = info[i].num;

         g_autofree char *thread_caminho_pdf = g_strdup_printf( "./dados/temporarios/%.2d.pdf", i );
         g_autofree char *nome_arquivo_png   = g_strdup_printf( "%.2d - %s.png", num_aluno, diario[num_aluno-1].aluno );
         g_autofree char *thread_caminho_png = g_build_filename( diretorio_imagens, nome_arquivo_png, NULL );

         if ( !pdf2png( thread_caminho_pdf, thread_caminho_png, 1.5 ) ) {
            // Em laços OpenMP, evite GTK, mas g_printerr é seguro.
            g_printerr( "[AVISO] Falha ao converter e mover imagem %s\n", thread_caminho_png );
         }
      }
   }

}


static StatusMapeamento validar_prova_escaneada( const MapeamentoGabarito *info, const InterfaceDados *dados,
      const FichaAluno *diario ) {

   // 1º TESTE (Crítico): O número da folha protege o array 'diario'
   if ( info->num <= 0 || info->num > dados->qtd_alunos_total ) {
      return ERRO_NUMERO_ALUNO_INVALIDO;
   }

   // 2º TESTE (Crítico): O ID protege o array 'G' (Gabaritos)
   if ( info->id >= dados->qtd_alunos_ativos ) {
      return ERRO_ID_GABARITO_INVALIDO;
   }

   // 3º TESTE (Regra de Negócio): Se chegou aqui, a memória está segura!
   if ( diario[info->num - 1].ativo == false ) {
      return STATUS_PROVA_OK | AVISO_ALUNO_INATIVO; // Acumula os estados perfeitamente
   }

   return STATUS_PROVA_OK;
}


void corrigir_prova( InterfacePainel *painel, const AppContext *ctx ) {
   if ( !painel || !ctx ) return;

   const InterfaceDados   *dados   = &ctx->dados;
   const FocoCoordenadas  *foco    = &ctx->cascata.foco;
   const CaminhoDiretorio *caminho = &ctx->caminho;
   const FichaAluno       *diario  =  ctx->diario;

   char nome_bin[64];
   nome_base_gabaritos_bin( nome_bin, sizeof( nome_bin ), foco->turma, foco->disciplina, foco->periodo, dados->iprova - 1 );

   g_autofree char *path_resp = g_build_filename( ".", "dados", "gabaritos", dados->ano, dados->escola,
                                "respostas", nome_bin, NULL );

   g_autofree char *path_gab  = g_build_filename( ".", "dados", "gabaritos", dados->ano, dados->escola,
                                "gabaritos", nome_bin, NULL );

   // ====================================================================================
   // 1. VALIDAÇÃO DE SEGURANÇA
   // ====================================================================================
   FILE *fr = g_fopen( path_resp, "rb" );
   if ( !fr ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Respostas Não Encontradas" );
      painel->format_subtitulo = meu_gerador_variadico( "O arquivo de escaneamento %s não existe.", nome_bin );
      painel->format_instrucao = meu_gerador_variadico( "Realize o processamento das folhas da turma antes de corrigir." );
      criar_mensagem_painel( AVISO, painel );
      return;
   }

   FILE *fg = g_fopen( path_gab, "rb" );
   if ( !fg ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Gabarito Mestre Ausente" );
      painel->format_subtitulo = meu_gerador_variadico( "Nenhum gabarito foi gerado para esta turma ainda." );
      painel->format_instrucao = meu_gerador_variadico( "Acesse a aba de geração de provas e crie o arquivo de gabaritos." );
      criar_mensagem_painel( ERRO, painel );
      fclose( fr );
      return;
   }

   // ====================================================================================
   // 2. LEITURA BINÁRIA
   // ====================================================================================
   int qtd_linhas = contar_registros_binarios( path_resp, sizeof( MapeamentoGabarito ) );
   if ( qtd_linhas <= 0 ) {
      g_printerr( "Aviso: Arquivo de respostas vazio ou corrompido.\n" );
      fclose( fr );
      fclose( fg );
      return;
   }

   g_autofree MapeamentoGabarito *info = g_new0( MapeamentoGabarito, qtd_linhas );
   size_t lidos_resp = fread( info, sizeof( MapeamentoGabarito ), qtd_linhas, fr );
   fclose( fr );

   qsort( info, qtd_linhas, sizeof( MapeamentoGabarito ), comparar_mapeamento_gabarito );

   g_autofree ItemTextoCurto *G = g_new0( ItemTextoCurto, dados->qtd_alunos_ativos );
   size_t lidos_gab = fread( G, sizeof( ItemTextoCurto ), dados->qtd_alunos_ativos, fg );
   fclose( fg );

   if ( lidos_resp != ( size_t )qtd_linhas || lidos_gab != ( size_t )dados->qtd_alunos_ativos ) {
      g_printerr( "Aviso de I/O: Tamanhos lidos não batem perfeitamente com os registrados.\n" );
   }

   // ====================================================================================
   // 3. LAÇO PARALELO BLINDADO
   // ====================================================================================
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < qtd_linhas; i++ ) {

      StatusMapeamento status = validar_prova_escaneada( &info[i], dados, diario );
      info[i].status = status;

      // Se o status POSSUI (OK) OU POSSUI (INATIVO)
      if ( status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {

         // O caminho feliz! Tudo perfeito.
         g_autofree gchar *nome_base = g_strdup_printf( "%.2d", i );
         const char *gab = G[ info[i].id ].str;

         int nota = imagens_corrigidas( gab, &info[i], ctx, nome_base );
         info[i].nota = nota;

      } else {
         // O caminho de tratamento de erros
         int thread_id = omp_get_thread_num();

         if ( status & ERRO_NUMERO_ALUNO_INVALIDO ) {
            g_printerr( "Alerta [Thread %d]: Número da prova (%d) corrompido ou fora dos limites.\n",
                        thread_id, info[i].num );

         } else if ( status & ERRO_ID_GABARITO_INVALIDO ) {
            g_printerr( "Alerta [Thread %d]: O ID lido (%d) na prova Nº %d não possui gabarito correspondente.\n",
                        thread_id, info[i].id, info[i].num );
         }
      }

   }

   // ====================================================================================
   // 4. COMPILAÇÃO PARALELA (LaTeX Multi-Core)
   // ====================================================================================
   g_pdflatex_parallel( "./dados/temporarios" );

   if ( dados->naopresencial ) {
      copiar_arquivos_correcao_nao_presencial( info, qtd_linhas, diario, dados, caminho );
   }

   // ====================================================================================
   // 5. UNIFICAÇÃO DOS PDFS E LIMPEZA NATIVA
   // ====================================================================================
   g_auto(GStrv) arquivos_pdf = g_new0( char *, qtd_linhas + 1 );
   int qtd_sucessos = 0;

   for ( int i = 0; i < qtd_linhas; i++ ) {
      if ( info[i].status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
         arquivos_pdf[qtd_sucessos] = g_strdup_printf( "%.2d.pdf", i );
         qtd_sucessos++;
      }
   }

   g_autofree char *nome_arquivo  = g_strdup_printf( "Correção_%d.pdf", dados->iprova );
   g_autofree char *arquivo_saida = g_build_filename( caminho->relatorios, nome_arquivo, NULL );

   // ---------------------------------------------------------------------------------
   // BLINDAGEM CONTRA FALSOS POSITIVOS (O seu toque de mestre de volta)
   // Removemos o arquivo anterior preventivamente. Se o pdfunite falhar,
   // nada será criado no lugar, e o passo 6 capturará o erro perfeitamente.
   // ---------------------------------------------------------------------------------
   g_remove( arquivo_saida );

   // Unifica os PDFs enviando DIRETAMENTE para o destino final
   g_pdfunite( "./dados/temporarios/", (const char **)arquivos_pdf, qtd_sucessos, arquivo_saida );

   // Limpeza nativa paralela
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < qtd_linhas; i++ ) {
      if ( info[i].status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
         g_autofree gchar *nome_base = g_strdup_printf( "%.2d", i );
         apagar_arquivos_temporarios_latex_nativamente( "./dados/temporarios/", nome_base, 5 );
      }
   }

   // ====================================================================================
   // 6. EXIBIÇÃO AUTOMÁTICA E FEEDBACK NA INTERFACE
   // ====================================================================================
   // Como apagamos o arquivo velho no passo 5, se este arquivo existe agora,
   // temos 100% de certeza de que ele acabou de ser gerado pelo g_pdfunite!
   if ( g_file_test( arquivo_saida, G_FILE_TEST_EXISTS ) ) {

      // Fluxo adicional: Se precisar exportar, copiamos o arquivo recém-criado
      if ( dados->expor ) {
         copiar_arquivos_correcao_externamente( dados, caminho, arquivo_saida );
      }

      // Abre o PDF APENAS se temos 100% de certeza do sucesso!
      g_xdg_open( arquivo_saida );

      painel->format_titulo    = meu_gerador_variadico( "✔ Correção Finalizada" );
      painel->format_subtitulo = meu_gerador_variadico( "%d provas unificadas com sucesso.", qtd_sucessos );
      painel->format_instrucao = meu_gerador_variadico( "As imagens corrigidas estão prontas no diretório base." );
      criar_mensagem_painel( SUCESSO, painel );

   } else {
      painel->format_titulo    = meu_gerador_variadico( "✘ Falha na Geração do PDF" );
      painel->format_subtitulo = meu_gerador_variadico( "Ocorreu um erro ao unificar os arquivos." );
      painel->format_instrucao = meu_gerador_variadico( "Verifique se o LaTeX apresentou erros ou se o pdfunite falhou." );
      criar_mensagem_painel( ERRO, painel );
   }

}






