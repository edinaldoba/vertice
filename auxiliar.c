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





int obter_foco_inicial( const int limite, const FichaAluno *ficha ) {
   int i;
   for ( i = 0; i < limite; i++ ) {
      if ( ficha[i].ativo ) {
         break;
      }
   }
   int foco = ( i < limite ) ? i : 0;
   return foco;
}
void mapear_alunos( GtkListStore *store, GtkTreeIter *iter, const void *ficha, int i ) {
   const FichaAluno *ficha_aux = ( const FichaAluno * )ficha;
   int len = calcular_len_limpo( ficha_aux[i].aluno, 30 );
   char aluno[64];
   snprintf( aluno, 64, "%.2d-%.*s", i + 1, len, ficha_aux[i].aluno );

   gtk_list_store_set( store, iter,
                       0,  aluno,
                       1,  ficha_aux[i].ativo,          // Sensibilidade
                       2, !ficha_aux[i].ativo, -1 );   // Riscar nome
}




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
      if ( strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0 || strcmp(dp->d_name,"alunos")==0 ) {
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



bool diretorio_existe( const char *caminhodir ) {
   struct stat sb;
   // stat retorna 0 se o caminho existir
   // S_ISDIR verifica se o caminho é de fato uma pasta
   return ( stat( caminhodir, &sb ) == 0 && S_ISDIR( sb.st_mode ) );
}



// Nova função de comparação usando strcoll
static int alfabetica( const void* a, const void* b ) {
   ItemCombo *arg1 = ( ItemCombo * )a;
   ItemCombo *arg2 = ( ItemCombo * )b;
   return strcoll( arg1->str, arg2->str );
}

ItemCombo *carregar_diretorios_temas( int qtd_dir, const char *diretorio, int ( *comparadora )( const void *, const void * ) ) {
   // 1. Defesa inicial: evita processamento se os parâmetros forem inválidos
   if ( diretorio == NULL || qtd_dir <= 0 ) return NULL;

   DIR *dir = opendir( diretorio );
   if ( dir == NULL ) return NULL;

   // 2. Alocação direta da estrutura FINAL (Adeus ItemOrdenavel temporário!)
   ItemCombo *tms_combo = ( ItemCombo* ) calloc( qtd_dir, sizeof( ItemCombo ) );
   if ( tms_combo == NULL ) {
      g_printerr( "Erro crítico: Falha de alocação de memória em carregar_diretorios_temas()\n" );
      closedir( dir );
      return NULL;
   }

   int i = 0;
   struct dirent *dp;
   struct stat st;
   char caminho_completo[2048];

   while ( ( dp = readdir( dir ) ) != NULL && i < qtd_dir ) {
      // 3. Filtra os seletores de diretório virtuais nativos do Linux
      if ( strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0 || strcmp(dp->d_name,"alunos")==0 ) {
         continue;
      }

      // 4. Monta o caminho completo para validar com o 'stat'
      snprintf( caminho_completo, sizeof( caminho_completo ), "%s/%s", diretorio, dp->d_name );

      if ( stat( caminho_completo, &st ) == 0 ) {
         // 5. Garante que é estritamente um DIRETÓRIO
         if ( S_ISDIR( st.st_mode ) ) {

            // 6. Grava diretamente no array final. Sem conversão mbstowcs!
            snprintf( tms_combo[i].str, sizeof( tms_combo[i].str ), "%.63s", dp->d_name );
            i++;
         }
      }
   }
   closedir( dir );

   int num_dir = i;

   // 7. Ordena o próprio array final usando a sua nova função strcoll
   if ( num_dir > 0 ) {
      if ( comparadora == NULL ) {
         qsort( tms_combo, num_dir, sizeof( ItemCombo ), alfabetica );
      } else {
         qsort( tms_combo, num_dir, sizeof( ItemCombo ), comparadora );
      }
   }

   // O retorno agora é imediato, pois os dados já estão no lugar certo
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





void caminhos_uteis_de_diretorios( const InterfaceDados *dados, CaminhoDiretorio *caminho ) {

   snprintf( caminho->base, sizeof( caminho->base ), "%s/%s/%s/%s/%s",
             dados->ano, dados->escola, dados->turma, dados->disciplina, dados->periodo );

   /* Endereço do diretório dos dados informados, onde estão os arquivos
    * lista.dat, conteúdos.dat, frequência.dat, avaliações.dat e média.dat */
   snprintf( caminho->dados, sizeof( caminho->dados ), "./dados/informados/%s", caminho->base );

   /* Endereço do diretório dos gabaristos e respostas, onde estão os arquivos gabaritos.dat, gabaritos1.dat,
    * respostas1.dat e outros arquivos gravados por ocasião da correção automática das provas */
   snprintf( caminho->gabaritos, sizeof( caminho->gabaritos ), "./dados/gabaritos/%s", caminho->base );

   /* Endereço do diretório dos relatórios e provas em geral (arquivos em PDF) */
   snprintf( caminho->relatorios, sizeof( caminho->relatorios ), "./relatorios/%s", caminho->base );

   // 1. Encontra o ponteiro para a última ocorrência da barra '/'
   const char *base = strrchr( caminho->base, '/' );
   int comp;
   if ( base != NULL ) {
      comp = ( int )( base - caminho->base );
   } else {
      comp = ( int )strlen( caminho->base );
   }
   snprintf( caminho->relatorios_final, sizeof( caminho->relatorios_final ),
             "./relatorios/%.*s", comp, caminho->base );


   /* Replica a hierarquia da pasta de relatórios e provas atraves da marcação do 'expor dados'" */
   const char* home = getenv( "HOME" );
   if ( home == NULL ) home = ".";
   snprintf( caminho->externo, sizeof( caminho->externo ), "%s/Documentos/%s", home, caminho->base );

   snprintf( caminho->externo_final, sizeof( caminho->externo_final ), "%s/Documentos/%.*s", home, comp, caminho->base );

   snprintf( caminho->externo_escola, sizeof( caminho->externo_escola ),
             "%s/Documentos/%s/%s", home, dados->ano, dados->escola );

   if ( !diretorio_existe( caminho->dados ) )            {
      g_printerr( "AVISO: Pasta ausente: %s\n", caminho->dados );
   }
   if ( !diretorio_existe( caminho->gabaritos ) )        {
      g_printerr( "AVISO: Pasta ausente: %s\n", caminho->gabaritos );
   }
   if ( !diretorio_existe( caminho->relatorios ) )       {
      g_printerr( "AVISO: Pasta ausente: %s\n", caminho->relatorios );
   }
   if ( !diretorio_existe( caminho->relatorios_final ) ) {
      g_printerr( "AVISO: Pasta ausente: %s\n", caminho->relatorios_final );
   }
   if ( !diretorio_existe( caminho->externo ) )          {
      g_printerr( "AVISO: Pasta ausente: %s\n", caminho->externo );
   }
   if ( !diretorio_existe( caminho->externo_final ) )    {
      g_printerr( "AVISO: Pasta ausente: %s\n", caminho->externo_final );
   }
   if ( !diretorio_existe( caminho->externo_escola ) )   {
      g_printerr( "AVISO: Pasta ausente: %s\n", caminho->externo_escola );
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





//====================================================================================================
static int alfabetica_lista_de_alunos( const void *a, const void *b ) {
   const FichaAluno *fa = ( const FichaAluno * )a;
   const FichaAluno *fb = ( const FichaAluno * )b;
   // strcoll respeita a localidade do sistema, ordenando acentos corretamente (Á antes de B)
   return strcoll( fa->aluno, fb->aluno );
}
//----------------------------------------------------------------------------------------------------
void acessar_e_carregar_ficha_dos_alunos_da_turma( AppContext *ctx ) {
   g_return_if_fail( ctx );

   // 1. Limpeza segura de memória (Substitui free e ctx->ficha = NULL)
   g_clear_pointer( ( gpointer * ) &ctx->ficha, g_free );

   InterfaceDados   *dados   = &ctx->dados;
   CaminhoDiretorio *caminho = &ctx->caminho;

   g_autofree char *arquivo_acesso = g_build_filename( caminho->dados, "acesso.bin", NULL );

   EstadoArquivo estado_arquivo = verificar_arquivo( arquivo_acesso );
   if ( estado_arquivo == ARQUIVO_INEXISTENTE || estado_arquivo == ARQUIVO_VAZIO ) {
      siaep_atualizar_alunos( &ctx->painel, ctx );
   }

   // 2. Leitura massiva para a RAM (Bulk Read)
   // Elimina a necessidade de 'contar_registros_binarios' e leituras sucessivas com fread.
   gsize tamanho_arquivo = 0;
   g_autofree AcessoFicha *buffer_acessos = NULL;

   dados->qtd_alunos_ativos = 0;

   if ( !g_file_get_contents( arquivo_acesso, (gchar **)&buffer_acessos, &tamanho_arquivo, NULL ) ) {
      g_printerr( "Falha ao ler o arquivo de acesso: %s\n", arquivo_acesso ); // SILÊNCIO AQUI :-)
      dados->qtd_alunos_total = dados->qtd_alunos_ativos;
      return;
   }

   // O tamanho exato do arquivo dividido pelo tamanho da struct nos dá a contagem perfeita
   dados->qtd_alunos_total = tamanho_arquivo / sizeof( AcessoFicha );


   if ( dados->qtd_alunos_total == 0 ) return;

   // 3. Alocação tipada nativa da GLib (Substitui o calloc)
   ctx->ficha = g_new0( FichaAluno, dados->qtd_alunos_total );
   FichaAluno *ficha = ctx->ficha;

   // 4. Cache do caminho base fora do loop para não recriar a mesma string dezenas de vezes
   g_autofree char *dir_base_alunos = g_build_filename(".", "dados", "informados", dados->ano, dados->escola, "alunos", NULL);

   for ( int i = 0; i < dados->qtd_alunos_total; i++ ) {

      // Pega os dados de acesso direto do buffer na memória
      AcessoFicha *acesso = &buffer_acessos[i];

      // Formata a string de forma direta com o diretório base
      g_autofree char *ficha_aluno = g_strdup_printf( "%s/%" PRIu32 ".bin", dir_base_alunos, acesso->cod_aluno );

      FILE *fa = fopen( ficha_aluno, "rb" );
      if ( !fa ) {
         g_printerr( "[Aviso] Ficha não encontrada para o aluno %" PRIu32 "\n", acesso->cod_aluno );
         continue; // Evita que o programa tente fazer fread em fa = NULL (Segmentation Fault)
      }

      if ( fread( &ficha[i], sizeof( FichaAluno ), 1, fa ) == 1 ) {
         ficha[i].cod_aluno = acesso->cod_aluno;
         ficha[i].idx = i;
         ficha[i].sit = acesso->sit;
         ficha[i].ativo = acesso->ativo;
         if ( ficha[i].ativo ) {
            dados->qtd_alunos_ativos++;
         }
      }

      fclose( fa );
   }

   // 5. Ordenação Alfabética
   qsort( ficha, dados->qtd_alunos_total, sizeof( FichaAluno ), alfabetica_lista_de_alunos );

}
//----------------------------------------------------------------------------------------------------


void mapear_datas_frequencia( GtkListStore *store, GtkTreeIter *iter, const void *dados, int i ) {
   // A função de mapeamento continua a mesma, pois o casting resolve a leitura
   // do buffer interno do GArray com perfeição.
   const RegistroDiario *registros = ( const RegistroDiario * )dados;

   gboolean aula_normal = ( registros[i].tipo_registro == TIPO_REGISTRO_AULA_NORMAL );
   gboolean aula_extra  = ( registros[i].tipo_registro == TIPO_REGISTRO_AULA_EXTRA );

   gboolean tem_chamada = ( aula_normal || aula_extra );

   gtk_list_store_set( store, iter,
                       0, registros[i].data,
                       1, ( guint )registros[i].qtd_aulas,
                       2, tem_chamada, // FALSE (risca) se for Feriado/Pedagógico
                       3, !tem_chamada,  // TRUE (trava seleção) se for Feriado/Pedagógico
                       -1 );
}




/**
 * Converte uma data no formato "AAAA/MM/DD" para "DD - mon - AAAA" (ex: "1983/10/11" -> "11 - out - 1983").
 * Retorna uma nova string alocada dinamicamente que deve ser liberada com g_free() ou usada com g_autofree.
 */
gchar *formatar_data_extenso( const gchar *data_iso ) {
   if ( !data_iso ) return NULL;

   // 1. Divide a string "AAAA/MM/DD" usando as barras como separadores
   g_auto( GStrv ) tokens = g_strsplit( data_iso, "/", 3 );

   // Valida se a string possui exatamente 3 partes (Ano, Mês, Dia)
   if ( !tokens || !tokens[0] || !tokens[1] || !tokens[2] ) {
      return NULL;
   }

   int ano = ( int )g_ascii_strtoll( tokens[0], NULL, 10 );
   int mes = ( int )g_ascii_strtoll( tokens[1], NULL, 10 );
   int dia = ( int )g_ascii_strtoll( tokens[2], NULL, 10 );

   // 2. Instancia um GDateTime local com os inteiros extraídos
   g_autoptr( GDateTime ) dt = g_date_time_new_local( ano, mes, dia, 0, 0, 0.0 );
   if ( !dt ) return NULL;

   // 3. Formata usando os especificadores de formatação (%d = dia, %b = mês abreviado, %Y = ano)
   // Por padrão, %b usa a localização do sistema (ex: "out" para outubro em pt_BR)
   return g_date_time_format( dt, "%d - %b - %Y" );
}



// Esta função de ordenação é exclusiva para o combo das turmas
int ordenar_turmas_novo_em( const void* a, const void* b ) {
   // 1. Cast idêntico à sua função padrão
   ItemCombo *arg1 = ( ItemCombo * )a;
   ItemCombo *arg2 = ( ItemCombo * )b;

   // 2. Extrai as strings do campo correto
   const char *turma_a = arg1->str;
   const char *turma_b = arg2->str;

   // 3. Primeiro Critério: A Série (100, 200, 300)
   int serie_a = atoi( turma_a );
   int serie_b = atoi( turma_b );

   if ( serie_a != serie_b ) {
      return serie_a - serie_b;
   }

   // 4. Segundo Critério: O Turno (Matutino vs Vespertino)
   int turno_a = 3, turno_b = 3;

   if ( strstr( turma_a, "mat" ) != NULL ) turno_a = 1;
   else if ( strstr( turma_a, "vesp" ) != NULL ) turno_a = 2;

   if ( strstr( turma_b, "mat" ) != NULL ) turno_b = 1;
   else if ( strstr( turma_b, "vesp" ) != NULL ) turno_b = 2;

   if ( turno_a != turno_b ) {
      return turno_a - turno_b;
   }

   // 5. Terceiro Critério: Desempate usando a sua lógica local
   return strcoll( turma_a, turma_b );
}





gint comparar_datas_diario( gconstpointer a, gconstpointer b ) {
   const RegistroDiario *d1 = (const RegistroDiario *)a;
   const RegistroDiario *d2 = (const RegistroDiario *)b;

   int dia1, mes1, ano1, dia2, mes2, ano2;
   // Converte a string "27/08/2026" para inteiros separadamente
   sscanf( d1->data, "%d/%d/%d", &dia1, &mes1, &ano1 );
   sscanf( d2->data, "%d/%d/%d", &dia2, &mes2, &ano2 );

   // Ordenação: Ano -> Mês -> Dia
   if ( ano1 != ano2 ) return ano1 - ano2;
   if ( mes1 != mes2 ) return mes1 - mes2;
   return dia1 - dia2;
}

int gravar_diario_binario( const char *caminho_arquivo, const RegistroDiario *registro, int indice_edicao ) {
   g_autoptr( GArray ) registros = g_array_new( FALSE, FALSE, sizeof( RegistroDiario ) );

   // 1. CARREGA TUDO DO DISCO
   FILE *f = fopen( caminho_arquivo, "rb" );
   if ( f ) {
      RegistroDiario temp;
      while ( fread( &temp, sizeof( RegistroDiario ), 1, f ) == 1 ) {
         g_array_append_val( registros, temp );
      }
      fclose( f );
   }

   // 2. ATUALIZA (Edição) OU ADICIONA (Novo)
   if ( indice_edicao >= 0 && indice_edicao < (int)registros->len ) {
      g_array_index( registros, RegistroDiario, indice_edicao ) = *registro;
   } else {
      g_array_append_val( registros, *registro );
   }

   // 3. MÁGICA GLIB: Ordena cronologicamente todo o arquivo
   g_array_sort( registros, comparar_datas_diario );

   // 4. DESCOBRE O NOVO ÍNDICE PARA A INTERFACE
   int novo_indice = -1;
   for ( guint i = 0; i < registros->len; i++ ) {
      RegistroDiario *atual = &g_array_index( registros, RegistroDiario, i );
      // Compara dados únicos para localizar nossa struct após a bagunça da ordenação
      if ( g_strcmp0( atual->data, registro->data ) == 0 && g_strcmp0( atual->descricao, registro->descricao ) == 0 ) {
         novo_indice = (int)i;
      }
   }

   // 5. SOBRESCREVE ORDENADO NO DISCO
   f = fopen( caminho_arquivo, "wb" );
   if ( f ) {
      if ( registros->len > 0 ) {
         fwrite( registros->data, sizeof( RegistroDiario ), registros->len, f );
      }
      fclose( f );
   }

   return novo_indice;
}



// Supondo que você tenha um enum para os temas no seu AppContext ou cabeçalho:
// typedef enum { TEMA_DARK_GREEN, TEMA_DEEP_BLUE, TEMA_LIGHT } TemaAtual;

/*
 * Retorna 1 (TRUE) se uma cor customizada foi atribuída,
 * ou 0 (FALSE) se for aula normal (devendo usar a cor padrão).
 */
int cor_texto_linha_liststore(const RegistroDiario *diario, int tema_ativo, GdkRGBA *cor_out) {
   if (!diario || !cor_out) return 0;

   // Retorna imediatamente se for Aula Normal (usa a cor padrão do tema)
   if (diario->tipo_registro == 0) return 0;

   // =====================================================================
   // 1. TIPO: Atividade Pedagógica
   // =====================================================================
   if (diario->tipo_registro == 1) {
      if (tema_ativo == 1) { // Deep Blue
         *cor_out = (GdkRGBA){ 0.50, 0.88, 1.00, 1.0 };
      } else if (tema_ativo == 2) { // Light
         *cor_out = (GdkRGBA){ 0.00, 0.00, 1.00, 1.0 };
      } else { // Dark Green (Padrão)
         *cor_out = (GdkRGBA){ 0.39, 0.71, 0.96, 1.0 };
      }
      return 1;
   }

   // =====================================================================
   // 2. TIPO: Feriado
   // =====================================================================
   if (diario->tipo_registro == 2) {
      if (tema_ativo == 1) { // Deep Blue
         *cor_out = (GdkRGBA){ 0.96, 0.40, 0.50, 1.0 };
      } else if (tema_ativo == 2) { // Light
         *cor_out = (GdkRGBA){ 0.85, 0.00, 0.00, 1.0 };
      } else { // Dark Green (Padrão)
         *cor_out = (GdkRGBA){ 0.90, 0.45, 0.45, 1.0 };
      }
      return 1;
   }

   // =====================================================================
   // 3. TIPO: Aula Extra
   // =====================================================================
   if (diario->tipo_registro == 3) {
      if (tema_ativo == 1) { // Deep Blue
         *cor_out = (GdkRGBA){ 1.00, 0.79, 0.16, 1.0 }; // #FFCA28
      } else if (tema_ativo == 2) { // Light
         *cor_out = (GdkRGBA){ 0.62, 0.49, 0.10, 1.0 }; // ##9d7c19
      } else { // Dark Green (Padrão)
         *cor_out = (GdkRGBA){ 1.00, 0.84, 0.31, 1.0 }; // #FFD54F
      }
      return 1;
   }

   return 0; // Fallback de segurança
}



int cor_texto_linha_frequencia( StatusAssiduidade status, int tema_ativo, GdkRGBA *cor_out ) {
   if ( !cor_out ) return 0;

   // =====================================================================
   // 1. STATUS COM COR PADRÃO (Retorna 0 para usar a cor nativa do tema)
   // =====================================================================
   if ( status == PRESENTE ) {
      return 0;
   }

   // =====================================================================
   // 2. SEM STATUS: Alunos inativos
   // =====================================================================
   if ( status == SEM_STATUS ) {
      if ( tema_ativo == 1 ) { // Deep Blue (Cinza azulado escuro)
         *cor_out = (GdkRGBA){ 0.40, 0.45, 0.55, 1.0 };
      } else if ( tema_ativo == 2 ) { // Light (Cinza padrão)
         *cor_out = (GdkRGBA){ 0.60, 0.60, 0.60, 1.0 };
      } else { // Dark Green - Padrão (Cinza esverdeado escuro)
         *cor_out = (GdkRGBA){ 0.45, 0.50, 0.45, 1.0 };
      }
      return 1;
   }

   // =====================================================================
   // 3. ALERTAS AVERMELHADOS: Ausente e Suspenso
   // =====================================================================
   if ( status == AUSENTE || status == SUSPENSO || status == FOI_EMBORA ) {
      if ( tema_ativo == 1 ) { // Deep Blue
         *cor_out = (GdkRGBA){ 0.96, 0.40, 0.50, 1.0 };
      } else if ( tema_ativo == 2 ) { // Light
         *cor_out = (GdkRGBA){ 0.85, 0.00, 0.00, 1.0 };
      } else { // Dark Green (Padrão)
         *cor_out = (GdkRGBA){ 0.90, 0.45, 0.45, 1.0 };
      }
      return 1;
   }

   // =====================================================================
   // 4. AVISOS AMARELADOS: Fora de sala e Foi embora
   // =====================================================================
   if ( status == FORA_DE_SALA || status == DISPENSADO ) {
      if ( tema_ativo == 1 ) { // Deep Blue
         *cor_out = (GdkRGBA){ 1.00, 0.79, 0.16, 1.0 }; // #FFCA28
      } else if ( tema_ativo == 2 ) { // Light
         *cor_out = (GdkRGBA){ 0.62, 0.49, 0.10, 1.0 }; // #9D7C19
      } else { // Dark Green (Padrão)
         *cor_out = (GdkRGBA){ 1.00, 0.84, 0.31, 1.0 }; // #FFD54F
      }
      return 1;
   }

   // =====================================================================
   // 5. DESTAQUE (ROXO/PÚRPURA): Atividade Domiciliar (Requer ação extra)
   // =====================================================================
   if ( status == ATIVIDADE_DOMICILIAR ) {
      if ( tema_ativo == 1 ) { // Deep Blue (Lilás vibrante)
         *cor_out = (GdkRGBA){ 0.80, 0.60, 0.95, 1.0 };
      } else if ( tema_ativo == 2 ) { // Light (Roxo escuro)
         *cor_out = (GdkRGBA){ 0.45, 0.15, 0.60, 1.0 };
      } else { // Dark Green - Padrão (Púrpura suave)
         *cor_out = (GdkRGBA){ 0.75, 0.55, 0.85, 1.0 };
      }
      return 1;
   }

   // =====================================================================
   // 5. DESTAQUE (ROXO/PÚRPURA): Atividade Domiciliar (Requer ação extra)
   // =====================================================================
   if ( status == FALTA_JUSTIFICADA ) {
      if ( tema_ativo == 1 ) { // Deep Blue (Verde-azulado / Turquesa vibrante)
         *cor_out = (GdkRGBA){ 0.25, 0.85, 0.75, 1.0 };
      } else if ( tema_ativo == 2 ) { // Light (Verde escuro / Floresta - alto contraste em fundo claro)
         *cor_out = (GdkRGBA){ 0.15, 0.55, 0.25, 1.0 };
      } else { // Dark Green - Padrão (Verde suave / Menta claro - legível em fundo escuro)
         *cor_out = (GdkRGBA){ 0.50, 0.85, 0.55, 1.0 };
      }
      return 1;
   }

   return 0; // Fallback de segurança
}


