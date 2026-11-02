/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>

#include "auxiliar.h"
#include "basicas.h"
#include "comum.h"
#include "interface.h"
#include "seduc-ma.h"
#include "glib_gio.h"

#include <inttypes.h>

/* ESTE ARQUIVO É EXCLUSIVO PARA DEPENDÊNCIAS DE INTERFACE.C */



CalendarioData data_de_hoje( void ) {
   time_t mytime;
   mytime = time( NULL );
   struct tm tm = *localtime( &mytime );
   CalendarioData data;
   data.dia = tm.tm_mday;
   data.mes = tm.tm_mon + 1;
   data.ano = tm.tm_year + 1900;
   return data;
}

/**
 * @brief Transforma uma data (dia, mes, ano) em um único número inteiro exclusivo (w).
 * Baseado em uma simplificação da fórmula do Período Juliano.
 * * @return long int Um número único para cada data. Datas posteriores geram números maiores.
 */
long int mapear_data_para_id( int dia, int mes, int ano ) {
   // Ajuste para o início do ano legislado pela fórmula (março vira o mês 0)
   if ( mes <= 2 ) {
      mes += 12;
      ano -= 1;
   }

   // Fórmula matemática de mapeamento linear contínuo
   long int w = ( 1461 * ano ) / 4 + ( 153 * ( mes + 1 ) ) / 5 + dia;

   return w;
}

/**
 * @brief Função inversa: Transforma o ID único (w) de volta na data original.
 */
void mapear_id_para_data( long int w, int *dia, int *mes, int *ano ) {
   long int j = w;
   long int y = ( 4 * j - 1 ) / 1461;
   long int d = j - ( 1461 * y ) / 4;
   long int m = ( 5 * d - 3 ) / 153;

   *dia = d - ( 153 * m + 3 ) / 5;
   *mes = m + 3;
   *ano = y;

   if ( *mes > 12 ) {
      *mes -= 12;
      *ano += 1;
   }
}

int foco_periodo_corrente( int escalar_hoje ) {
   int foco;
   long int fim_do_periodo[5];

   fim_do_periodo[0] = mapear_data_para_id( 17,  4, 2026 ); // fim do 1º período
   fim_do_periodo[1] = mapear_data_para_id( 30,  6, 2026 ); // fim do 2º período
   fim_do_periodo[2] = mapear_data_para_id( 9,  10, 2026 ); // fim do 3º período
   fim_do_periodo[3] = mapear_data_para_id( 29, 12, 2026 ); // fim do 4º período
   fim_do_periodo[4] = mapear_data_para_id( 15,  1, 2027 ); // fim da recuperação e do conselho de classe

   // Substituição perfeita do switch por lógica condicional dinâmica:
   if ( fim_do_periodo[0] + 10 > escalar_hoje )  foco = 0;
   else if ( fim_do_periodo[1] + 30 > escalar_hoje )  foco = 1;
   else if ( fim_do_periodo[2] + 10 > escalar_hoje )  foco = 2;
   else if ( fim_do_periodo[3] + 10 > escalar_hoje )  foco = 3;
   else if ( fim_do_periodo[4] + 10 > escalar_hoje )  foco = 4;
   else return 0;

   return foco;
}





int obter_foco_inicial( const int limite, const AppContext *ctx ) {
   int i;
   for ( i = 0; i < limite; i++ ) {
      FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, i );
      if ( ficha->ativo ) {
         break;
      }
   }
   int foco = ( i < limite ) ? i : 0;
   return foco;
}
void mapear_alunos( GtkListStore *store, GtkTreeIter *iter, const void *dados, int i ) {
   g_return_if_fail( store && iter && dados );

   const GArray *fichas = ( const GArray * )dados;

   if ( ( guint )i >= fichas->len ) return;

   const FichaAluno *ficha = &g_array_index( fichas, FichaAluno, i );

   int len = calcular_len_limpo( ficha->aluno, 30 );
   char aluno[64];
   snprintf( aluno, 64, "%.2d-%.*s", i + 1, len, ficha->aluno );

   gtk_list_store_set( store, iter,
                       0,  aluno,
                       1,  ficha->ativo,          // Sensibilidade
                       2, !ficha->ativo, -1 );   // Riscar nome
}



//=========================================================================================================
int obter_foco_inicial_disciplina( const AppContext *ctx, int qtd_disciplinas ) {
   g_return_val_if_fail( ctx != NULL, 0 );

   for ( int i = 0; i < qtd_disciplinas; i++ ) {

      g_autofree gchar *caminho = g_build_filename( ".", "dados", "informados", ctx->dados.ano, ctx->dados.escola,
                                  ctx->dados.turma, ctx->cabecalho.disciplinas[i], NULL );

      // Se o diretório NÃO estiver vazio, encontramos o foco ideal!
      if ( !diretorio_esta_vazio( caminho ) ) {
         return i;
      }
   }

   // Fallback: se tudo estiver vazio, para no primeiro item para o GTK não quebrar
   return 0;
}
//---------------------------------------------------------------------------------------------------------
void mapear_disciplinas( GtkListStore *store, GtkTreeIter *iter, const void *dados, int i ) {
   g_return_if_fail( store && iter && dados );

   const AppContext *ctx = ( const AppContext * )dados;

   // CORREÇÃO: g_autofree garante que a string alocada será limpa ao sair da função
   g_autofree gchar *caminho = g_build_filename( ".", "dados", "informados", ctx->dados.ano, ctx->dados.escola,
                               ctx->dados.turma, ctx->cabecalho.disciplinas[i], NULL );

   gboolean riscar = diretorio_esta_vazio( caminho );

   gtk_list_store_set( store, iter,
                       0,  ctx->cabecalho.disciplinas[i],
                       1, !riscar,         // Sensibilidade (TRUE = habilitado)
                       2,  riscar, -1 );   // Riscar nome
}
//=========================================================================================================




int quantidade_diretorios( const char *diretorio ) {
   g_return_val_if_fail( diretorio, 0 );

   DIR *dir = opendir( diretorio );
   if ( dir == NULL ) {
      // Se falhar ao abrir (ex: pasta não existe), retorna 0 com segurança
      return 0;
   }

   int cont = 0;
   struct dirent *dp;
   struct stat st;
   char caminho_completo[2048];

   // 2. Varre o diretório inspecionando cada item
   while ( ( dp = readdir( dir ) ) != NULL ) {

      // 3. Ignora os seletores virtuais do Linux ("." e "..")
      if ( g_strcmp0( dp->d_name, "." ) == 0 || g_strcmp0( dp->d_name, ".." ) == 0 || g_strcmp0( dp->d_name, "alunos" ) == 0 ) {
         continue;
      }

      // 4. Monta o caminho completo para que o 'stat' encontre os metadados corretos
      snprintf( caminho_completo, sizeof( caminho_completo ), "%s/%s", diretorio, dp->d_name );

      // 5. Interroga o sistema operacional sobre a natureza do item
      if ( stat( caminho_completo, &st ) == 0 ) {
         // Garante que é estritamente um DIRETÓRIO (ignora arquivos, mesmo sem extensão)
         if ( S_ISDIR( st.st_mode ) ) {
            cont++;
         }
      }
   }

   // 6. Fecha o descritor de diretório de forma limpa
   closedir( dir );

   return cont;
}



gboolean diretorio_existe( const char *caminhodir ) {
   struct stat sb;
   // stat retorna 0 se o caminho existir
   // S_ISDIR verifica se o caminho é de fato uma pasta
   return ( stat( caminhodir, &sb ) == 0 && S_ISDIR( sb.st_mode ) );
}



gchar **carregar_diretorios_temas( int qtd_dir, const gchar *diretorio, int ( *comparadora )( const void *, const void * ) ) {
   if ( diretorio == NULL || qtd_dir <= 0 ) return NULL;

   // 1. GDir (GLib Directory) é multiplataforma e já ignora "." e ".." por padrão
   GDir *dir = g_dir_open( diretorio, 0, NULL );
   if ( !dir ) return NULL;

   // 2. Aloca um array de ponteiros (gchar*).
   // Adicionamos +1 para o terminador NULL, criando um formato padrão GStrv.
   gchar **tms_combo = g_new0( gchar *, qtd_dir + 1 );

   int i = 0;
   const gchar *nome_arquivo;

   // 3. Lê o diretório iterativamente
   while ( ( nome_arquivo = g_dir_read_name( dir ) ) != NULL && i < qtd_dir ) {

      // Filtra a pasta especial de alunos
      if ( g_strcmp0( nome_arquivo, "alunos" ) == 0 ) {
         continue;
      }

      // Montagem de caminho 100% segura e dinâmica
      gchar *caminho_completo = g_build_filename( diretorio, nome_arquivo, NULL );

      // 4. g_file_test substitui a estrutura stat do POSIX de forma mais elegante
      if ( g_file_test( caminho_completo, G_FILE_TEST_IS_DIR ) ) {

         // 5. Aloca no Heap apenas os bytes exatos necessários para o nome da pasta
         tms_combo[i] = g_strdup( nome_arquivo );
         i++;
      }

      g_free( caminho_completo );
   }
   g_dir_close( dir );

   int num_dir = i;

   // 6. Ordenação do array de ponteiros
   if ( num_dir > 0 ) {
      if ( comparadora == NULL ) {
         qsort( tms_combo, num_dir, sizeof( gchar * ), comparar_item_combo );
      } else {
         qsort( tms_combo, num_dir, sizeof( gchar * ), comparadora );
      }
   }

   return tms_combo;
}





void renderizar_combo_box_ellipsize( GtkWidget *widget, int qtd_caracteres ) {
   // 1. Recupere o renderizador de texto que você associou ao combo box de temas
   // (Se você usou gtk_combo_box_text_new(), o GTK cria um internamente.
   // Para customizar, pegamos a lista de renderizadores dele):
   GList *renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( widget ) );

   if ( renderers != NULL ) {
      // O primeiro renderizador (data) é o GtkCellRendererText responsável pelas strings
      GtkCellRenderer *renderer = GTK_CELL_RENDERER( renderers->data );

      // 2. Ativa o corte elegante com reticências no final (Ellipsize)
      g_object_set( G_OBJECT( renderer ), "ellipsize", PANGO_ELLIPSIZE_END, NULL );

      // 3. Opcional: Define a largura máxima em caracteres que o combo aceita exibir
      // antes de começar a colocar os três pontos (ex: 20 ou 25 caracteres)
      g_object_set( G_OBJECT( renderer ), "max-width-chars", qtd_caracteres, NULL );

      // Libera a lista temporária usada para a captura
      g_list_free( renderers );
   }
}




/**
 * @brief Libera todas as strings alocadas dinamicamente na estrutura CaminhoDiretorio
 *        e reseta os ponteiros para NULL com segurança.
 *
 * @param caminho Ponteiro para a estrutura CaminhoDiretorio a ser liberada.
 */
static void caminho_diretorio_free( CaminhoDiretorio *caminho ) {
   if ( !caminho ) return;
   // 1. Liberação segura de cada campo dinâmico
   g_clear_pointer( &caminho->dados, g_free );
   g_clear_pointer( &caminho->gabaritos, g_free );
   g_clear_pointer( &caminho->relatorios, g_free );
   g_clear_pointer( &caminho->externo, g_free );
   g_clear_pointer( &caminho->relatorios_final, g_free );
   g_clear_pointer( &caminho->externo_final, g_free );
   g_clear_pointer( &caminho->externo_escola, g_free );
   g_clear_pointer( &caminho->base, g_free );
   g_clear_pointer( &caminho->questoes_tex, g_free );
}


void caminhos_uteis_de_diretorios( const InterfaceDados *dados, CaminhoDiretorio *caminho ) {
   g_return_if_fail( dados != NULL );
   g_return_if_fail( caminho != NULL );

   // 1. Libera qualquer alocação anterior para evitar vazamento de memória (Memory Leak)
   caminho_diretorio_free( caminho );

   // 2. Base Completa e Base Parcial (eliminando a necessidade do strrchr)
   // g_build_filename já aloca a string dinamicamente e coloca as barras '/' corretamente
   caminho->base = g_build_filename( dados->ano, dados->escola, dados->turma, dados->disciplina, dados->periodo, NULL );

   gchar *base_parcial = g_build_filename( dados->ano, dados->escola, dados->turma, dados->disciplina, NULL );

   // 3. Montagem dos caminho internos
   caminho->dados = g_build_filename( ".", "dados", "informados", caminho->base, NULL );
   caminho->gabaritos = g_build_filename( ".", "dados", "gabaritos", caminho->base, NULL );
   caminho->relatorios = g_build_filename( ".", "relatorios", caminho->base, NULL );
   caminho->relatorios_final = g_build_filename( ".", "relatorios", base_parcial, NULL );

   // 4. Montagem dos caminho externos (Documentos)
   const gchar *home = g_get_home_dir(); // Função segura nativa do GLib
   if ( !home ) home = ".";

   caminho->externo = g_build_filename( home, "Documentos", caminho->base, NULL );
   caminho->externo_final = g_build_filename( home, "Documentos", base_parcial, NULL );
   caminho->externo_escola = g_build_filename( home, "Documentos", dados->ano, dados->escola, NULL );

   // Libera a string temporária que usamos apenas para montar os relatórios finais
   g_free( base_parcial );

   // 5. Criação das estruturas de diretórios em lote
   const gchar *pastas_para_criar[] = {
      caminho->dados,
      caminho->gabaritos,
      caminho->relatorios,
      caminho->relatorios_final,
      caminho->externo,
      caminho->externo_final,
      caminho->externo_escola
   };

   // g_mkdir_with_parents já age como um "mkdir -p" e não falha se a pasta já existir,
   // mas mantemos sua função diretorio_existe() por segurança arquitetural
   for ( guint i = 0; i < G_N_ELEMENTS( pastas_para_criar ); i++ ) {
      if ( !diretorio_existe( pastas_para_criar[i] ) ) {
         if ( g_mkdir_with_parents( pastas_para_criar[i], 0755 ) != 0 ) {
            g_printerr( "[ERRO] Nao foi possivel criar/acessar a pasta de destino: %s\n", pastas_para_criar[i] );
         }
      }
   }
}









void gerar_gabaritos( const char *arquivo, const int qtd_linhas, const int total_questoes, const char *modo ) {
   if ( !arquivo || !modo ) return;

   if ( total_questoes != NTI ) {
      g_printerr( "Erro: Quantidade de questões (%d) inválida.\n", total_questoes );
      return;
   }

   FILE *p = g_fopen( arquivo, modo );
   if ( !p ) {
      g_printerr( "Erro ao abrir o arquivo binário: %s\n", arquivo );
      return;
   }

   // CONDIÇÃO 3: Alocação de um bloco contíguo na RAM para todos os alunos
   // g_new0 já zera a memória, garantindo que o espaço não usado dos 32 bytes seja '\0' limpo.
   ItemTextoCurto *bloco = g_new0( ItemTextoCurto, qtd_linhas );

   const char *alternativas = "AABBCCDDEE";

   // Processamento ultra-rápido isolado na RAM (sem tocar no HD)
   for ( int i = 0; i < qtd_linhas; i++ ) {

      g_strlcpy( bloco[i].str, alternativas, sizeof( bloco[i].str ) );
      bloco[i].str[total_questoes] = '\0';

      chrperm( bloco[i].str, total_questoes );
   }

   // CONDIÇÃO 4: A Gravação em Lote (Zero Latência)
   // Descarrega todo o array de estruturas de uma única vez para o disco
   size_t blocos_gravados = fwrite( bloco, sizeof( ItemTextoCurto ), qtd_linhas, p );

   if ( blocos_gravados != ( size_t )qtd_linhas ) {
      g_printerr( "Erro crítico de I/O: Apenas %zu de %d registros foram salvos.\n", blocos_gravados, qtd_linhas );
   }

   // Limpeza cirúrgica
   g_free( bloco );
   fclose( p );
}





//==================================================================================================
void sincronizar_registro_diario_com_turma_siaep( AppContext *ctx ) {
   g_return_if_fail( ctx );

   // 1. Aborta se não houver registros carregados na memória
   if ( !ctx->diarios || ctx->diarios->len == 0 ) return;

   // 2. Acessa o primeiro registro diretamente do GArray em O(1)
   RegistroDiario *primeiro_registro = &g_array_index( ctx->diarios, RegistroDiario, 0 );

   // 3. Conta os alunos presentes no primeiro registro salvo na RAM
   int qtd_fichas = 0;
   while ( qtd_fichas < 64 && primeiro_registro->chamada[qtd_fichas].cod_aluno != 0 ) {
      qtd_fichas++;
   }

   // 4. Validação do Gatilho: Verifica mudanças na quantidade
   gboolean precisa_sincronizar = ( qtd_fichas != ctx->dados.qtd_alunos_total );

   // 5. Verificação profunda: A ordem alfabética ou os IDs mudaram?
   if ( !precisa_sincronizar ) {
      for ( int k = 0; k < qtd_fichas; k++ ) {
         FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, k );
         if ( primeiro_registro->chamada[k].cod_aluno != ficha->cod_aluno ) {
            precisa_sincronizar = TRUE;
            break;
         }
      }
   }

   // Tudo em sincronia, aborta precocemente.
   if ( !precisa_sincronizar ) return;

   // =========================================================================
   // 6. Sincronização em Lote de Alta Performance (O(N) por registro)
   // =========================================================================

   // Aloca a Hash Table uma única vez fora do laço para não estressar o alocador de memória
   GHashTable *hash_status = g_hash_table_new( g_direct_hash, g_direct_equal );

   for ( guint r = 0; r < ctx->diarios->len; r++ ) {
      RegistroDiario *registro = &g_array_index( ctx->diarios, RegistroDiario, r );
      RegistroChamada nova_chamada[64] = {0}; // Zera magicamente todos os bytes

      // Limpa as chaves do dia anterior sem destruir a Hash Table
      g_hash_table_remove_all( hash_status );

      // A) Popula a Hash com a chamada desatualizada (cod_aluno -> status)
      for ( int j = 0; j < 64 && registro->chamada[j].cod_aluno != 0; j++ ) {
         g_hash_table_insert( hash_status,
                              GINT_TO_POINTER( registro->chamada[j].cod_aluno ),
                              GINT_TO_POINTER( registro->chamada[j].status ) );
      }

      // B) Reconstrói a chamada do zero, espelhando a ordem oficial de ctx->fichas
      for ( int k = 0; k < ctx->dados.qtd_alunos_total && k < 64; k++ ) {
         FichaAluno *ficha = &g_array_index( ctx->fichas, FichaAluno, k );
         uint32_t cod_alvo = ficha->cod_aluno;
         nova_chamada[k].cod_aluno = cod_alvo;

         gpointer ptr_status = NULL;

         // Busca O(1): O aluno já estava matriculado neste dia específico?
         if ( g_hash_table_lookup_extended( hash_status, GINT_TO_POINTER( cod_alvo ), NULL, &ptr_status ) ) {
            nova_chamada[k].status = GPOINTER_TO_INT( ptr_status ); // Restaura o status (Presença, Falta, etc.)
         } else {
            nova_chamada[k].status = NOVO_ALUNO; // Aluno injetado tardiamente
         }
      }

      // Substitui o bloco desatualizado pelo novo na memória do GArray
      memcpy( registro->chamada, nova_chamada, sizeof( nova_chamada ) );
   }

   // Limpeza final O(1)
   g_hash_table_destroy( hash_status );
}
//--------------------------------------------------------------------------------------------------
static void _copiar_acesso_bin( AppContext *ctx ) {
   g_return_if_fail( ctx != NULL );

   InterfaceDados *dados = &ctx->dados;
   GtkComboBox *combo = GTK_COMBO_BOX( ctx->cabecalho.combo_periodo );
   int target_periodo = ctx->cabecalho.foco_periodo;

   g_autofree gchar *arq_origem_valido = NULL;

   // =========================================================================
   // 1. VARREDURA PARA TRÁS: Busca o acesso.bin mais recente já consolidado
   // =========================================================================
   // Começa do período IMEDIATAMENTE ANTERIOR ao atual e desce até o 1º período (0)
   for ( int p = target_periodo - 1; p >= 0; p-- ) {
      g_autofree gchar *nome_periodo = combo_box_text_get_text_at( combo, p );
      if ( !nome_periodo ) continue;

      g_autofree gchar *dir_periodo = g_build_filename( ".", "dados", "informados",
                                      dados->ano, dados->escola,
                                      dados->turma, dados->disciplina,
                                      nome_periodo, NULL );

      g_autofree gchar *caminho_teste = g_build_filename( dir_periodo, "acesso.bin", NULL );

      // Verifica se o arquivo de acesso existe neste período anterior
      if ( g_file_test( caminho_teste, G_FILE_TEST_EXISTS ) ) {
         // Rouba o ponteiro local para a variável externa sem causar Memory Leak
         arq_origem_valido = g_steal_pointer( &caminho_teste );
         break; // Encontrou a versão confiável mais recente, interrompe a busca!
      }
   }

   if ( !arq_origem_valido ) {
      // Não encontrou absolutamente nenhum acesso.bin no passado para copiar.
      return;
   }

   // =========================================================================
   // 2. PREENCHIMENTO PARA FRENTE: Cobre as lacunas do período selecionado
   // =========================================================================
   g_autofree gchar *nome_target = combo_box_text_get_text_at( combo, target_periodo );
   if ( !nome_target ) return;

   g_autofree gchar *dir_target = g_build_filename( ".", "dados", "informados",
                                  dados->ano, dados->escola,
                                  dados->turma, dados->disciplina,
                                  nome_target, NULL );

   g_autofree gchar *arq_target = g_build_filename( dir_target, "acesso.bin", NULL );

   gio_copiar_arquivo( arq_origem_valido, arq_target );
}
//--------------------------------------------------------------------------------------------------
void acessar_e_carregar_ficha_dos_alunos_da_turma( AppContext *ctx ) {
   g_return_if_fail( ctx );

   // 1. Limpeza segura do GArray anterior
   if ( ctx->fichas ) {
      g_array_unref( ctx->fichas );
      ctx->fichas = NULL;
   }

   InterfaceDados   *dados   = &ctx->dados;
   CaminhoDiretorio *caminho = &ctx->caminho;

   g_autofree gchar *arquivo_acesso = g_build_filename( caminho->dados, "acesso.bin", NULL );

   EstadoArquivo estado = verificar_arquivo( arquivo_acesso );
   if ( estado & ( ARQUIVO_INEXISTENTE | ARQUIVO_VAZIO ) ) {
      if ( !siaep_atualizar_alunos( &ctx->painel, ctx ) ) {
         _copiar_acesso_bin( ctx );
      }
   }

   gsize tamanho_arquivo = 0;
   gchar *conteudo_acessos = NULL;
   dados->qtd_alunos_ativos = 0;

   // 3. Leitura do arquivo de acesso binário
   if ( !g_file_get_contents( arquivo_acesso, &conteudo_acessos, &tamanho_arquivo, NULL ) ) {
      g_printerr( "Falha ao ler o arquivo de acesso: %s\n", arquivo_acesso );
      dados->qtd_alunos_total = 0;
      return;
   }

   // Transforma o buffer raw de chars na struct e garante a liberação automática
   g_autofree AcessoFicha *buffer_acessos = ( AcessoFicha * )conteudo_acessos;

   dados->qtd_alunos_total = tamanho_arquivo / sizeof( AcessoFicha );
   if ( dados->qtd_alunos_total == 0 ) return;

   // 4. Inicialização do GArray já com o tamanho exato pré-alocado
   ctx->fichas = g_array_sized_new( FALSE, TRUE, sizeof( FichaAluno ), dados->qtd_alunos_total );

   g_autofree gchar *dir_base_alunos = g_build_filename( ".", "dados", "informados",
                                       dados->ano, dados->escola, "alunos", NULL );

   for ( int i = 0; i < dados->qtd_alunos_total; i++ ) {
      AcessoFicha *acesso = &buffer_acessos[i];

      // Constrói o caminho de forma segura para o SO (evita concatenar "%s/%d.bin" na mão)
      g_autofree gchar *nome_arquivo = g_strdup_printf( "%" PRIu32 ".bin", acesso->cod_aluno );
      g_autofree gchar *ficha_aluno = g_build_filename( dir_base_alunos, nome_arquivo, NULL );

      gsize tamanho_ficha = 0;
      gchar *conteudo_ficha = NULL;

      // 5. Carregamento binário purista sem misturar FILE* do C padrão com GLib
      if ( g_file_get_contents( ficha_aluno, &conteudo_ficha, &tamanho_ficha, NULL ) ) {

         // Validação extra de segurança: a ficha em disco bate com a struct na memória?
         if ( tamanho_ficha == sizeof( FichaAluno ) ) {
            FichaAluno temp_ficha;

            // Hidrata a struct local a partir do buffer cru em memória
            memcpy( &temp_ficha, conteudo_ficha, sizeof( FichaAluno ) );

            temp_ficha.cod_aluno = acesso->cod_aluno;
            temp_ficha.idx_siaep = i;
            temp_ficha.sit = acesso->sit;
            temp_ficha.ativo = acesso->ativo;
            temp_ficha.ficha_modificada = FALSE;

            if ( temp_ficha.ativo ) {
               dados->qtd_alunos_ativos++;
            }

            // Anexa a cópia da ficha direto no GArray
            g_array_append_val( ctx->fichas, temp_ficha );
         } else {
            g_printerr( "[Aviso] Tamanho corrompido para o aluno %" PRIu32 "\n", acesso->cod_aluno );
         }

         // Libera o buffer temporário alocado pelo g_file_get_contents
         g_free( conteudo_ficha );

      } else {
         g_printerr( "[Aviso] Ficha não encontrada para o aluno %" PRIu32 "\n", acesso->cod_aluno );
      }
   }

   // 6. Ordenação Alfabética nativa do GArray
   g_array_sort( ctx->fichas, alfabetica_lista_de_alunos );
}
//----------------------------------------------------------------------------------------------------






// Esta função de ordenação é exclusiva para o combo das turmas
int ordenar_turmas_novo_em( const void* a, const void* b ) {
   // 1. Cast correto para array de ponteiros (gchar **)
   // O qsort manda ponteiros para os elementos. Se o elemento é 'gchar *', recebemos 'gchar **'.
   const gchar *turma_a = *( ( const gchar ** )a );
   const gchar *turma_b = *( ( const gchar ** )b );

   // Defesa contra ponteiros nulos
   if ( !turma_a && !turma_b ) return 0;
   if ( !turma_a ) return -1;
   if ( !turma_b ) return 1;

   // 2. Primeiro Critério: A Série (100, 200, 300)
   // A função atoi lê os números no começo da string e ignora o texto que vem depois
   int serie_a = atoi( turma_a );
   int serie_b = atoi( turma_b );

   if ( serie_a != serie_b ) {
      return serie_a - serie_b;
   }

   // 3. Segundo Critério: O Turno (Matutino vs Vespertino)
   int turno_a = 3, turno_b = 3;

   if ( strstr( turma_a, "mat" ) != NULL ) turno_a = 1;
   else if ( strstr( turma_a, "vesp" ) != NULL ) turno_a = 2;

   if ( strstr( turma_b, "mat" ) != NULL ) turno_b = 1;
   else if ( strstr( turma_b, "vesp" ) != NULL ) turno_b = 2;

   if ( turno_a != turno_b ) {
      return turno_a - turno_b;
   }

   // 4. Terceiro Critério: Desempate alfabético usando o padrão UTF-8 do GLib
   return g_utf8_collate( turma_a, turma_b );
}







