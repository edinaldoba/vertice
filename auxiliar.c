/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <sys/stat.h>

#include "auxiliar.h"
#include "basicas.h"

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



int obter_foco_inicial( const int limite, const FichaAluno *diario ) {
   int i;
   for ( i = 0; i < limite; i++ ) {
      if ( diario[i].ativo ) {
         break;
      }
   }
   int foco = ( i < limite ) ? i : 0;
   return foco;
}

void mapear_alunos( GtkListStore *store, GtkTreeIter *iter, const void *ficha, int i ) {
   const FichaAluno *diario = ( const FichaAluno * )ficha;
   int len = calcular_len_limpo( diario[i].aluno, 30 );
   char aluno[64];
   snprintf( aluno, 64, "%.2d-%.*s", i + 1, len, diario[i].aluno );
   gtk_list_store_set( store, iter, 0, aluno, 1, diario[i].ativo, -1 );
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
      if ( strcmp( dp->d_name, "." ) == 0 || strcmp( dp->d_name, ".." ) == 0 ) {
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
      if ( strcmp( dp->d_name, "." ) == 0 || strcmp( dp->d_name, ".." ) == 0 ) {
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
static int find_alpha_utf8( const char *str ) {
   const char *p = str;
   while ( *p != '\0' ) {
      gunichar c = g_utf8_get_char( p );
      if ( g_unichar_isalpha( c ) ) {
         return p - str; // Retorna o tamanho do salto exato em bytes
      }
      p = g_utf8_next_char( p ); // Avança de forma segura pelo UTF-8
   }
   return -1;
}
//----------------------------------------------------------------------------------------------------
static gboolean eh_preposicao( const gchar *str ) {
   g_return_val_if_fail( str, FALSE );
   // Ex: retorna TRUE se for "de", "da", "dos", etc.
   // Certifique-se de que sua função compare em minúsculas!
   if ( g_ascii_strcasecmp( str, "de" ) == 0 ||
         g_ascii_strcasecmp( str, "da" ) == 0 ||
         g_ascii_strcasecmp( str, "dos" ) == 0 ||
         g_ascii_strcasecmp( str, "das" ) == 0 ||
         g_ascii_strcasecmp( str, "e" ) == 0 ) {
      return TRUE;
   }
   return FALSE;
}
//----------------------------------------------------------------------------------------------------
static int contar_linhas_arquivo( const char *arquivo ) {
   FILE *p = fopen( arquivo, "r" );
   if ( p == NULL ) {
      return -1; // Retorna -1 se o arquivo não existir ou não puder ser aberto
   }

   int contador = 0;
   int ch; // ⚠️ IMPORTANTE: Deve ser 'int' e não 'char' para capturar corretamente o EOF

   // Varre o arquivo caractere por caractere até o Fim do Arquivo (EOF)
   while ( ( ch = fgetc( p ) ) != EOF ) {
      if ( ch == '\n' ) {
         contador++;
      }
   }

   fclose( p );

   return contador;
}
//----------------------------------------------------------------------------------------------------
static double obter_largura_nome_mm( const char *texto, const char *fonte_desc ) {
   PangoFontMap *font_map = pango_cairo_font_map_get_default();
   PangoContext *context = pango_font_map_create_context( font_map );
   PangoLayout *layout = pango_layout_new( context );

   pango_layout_set_text( layout, texto, -1 );

   PangoFontDescription *desc = pango_font_description_from_string( fonte_desc ); // Ex: "Arial 11"
   pango_layout_set_font_description( layout, desc );

   int largura_pango;
   pango_layout_get_size( layout, &largura_pango, NULL );

   pango_font_description_free( desc );
   g_object_unref( layout );
   g_object_unref( context );

   // Converte Pango Units para Milímetros
   return ( double )largura_pango / ( PANGO_SCALE * 2.83465 );
   // return (double)largura_pango / (PANGO_SCALE * 3.78);
}
//----------------------------------------------------------------------------------------------------
static void ajustar_nomes_tabelas( FichaAluno *diario, const InterfaceDados *dados ) {
   char nome[64];

   // Definimos a fonte uma única vez fora do loop para deixar o código mais limpo
   const char *fonte = dados->fonte_latex == 1 ? "CMU Bright 11" : "CMU Serif 11";

   for ( int i = 0; i < dados->qtd_alunos_total; i++ ) {

      // 1. Descobrimos a quantidade de caracteres reais (não bytes) do nome original
      int max_chars = g_utf8_strlen( diario[i].aluno, -1 );

      // 2. Usamos sua abstração para pegar os bytes exatos, já limpos de sujeiras finais
      diario[i].limite_corte = calcular_len_limpo( diario[i].aluno, max_chars );

      // Monta o buffer seguro
      snprintf( nome, sizeof( nome ), "%.*s", diario[i].limite_corte, diario[i].aluno );

      // Mede o tamanho físico em milímetros
      double largura = obter_largura_nome_mm( nome, fonte );

      // 3. O Loop de Ajuste: Enquanto a largura estourar os 69.0 mm
      while ( largura > 69.0 && max_chars > 0 ) {

         max_chars--; // Avisamos: "Eu quero um caractere visual a menos"

         // A calcular_len_limpo converte essa nossa vontade visual em bytes seguros
         diario[i].limite_corte = calcular_len_limpo( diario[i].aluno, max_chars );

         // Remonta o buffer com o novo limite
         snprintf( nome, sizeof( nome ), "%.*s", diario[i].limite_corte, diario[i].aluno );

         // Mede novamente
         largura = obter_largura_nome_mm( nome, fonte );
      }
   }
}
//----------------------------------------------------------------------------------------------------
static gchar* converter_nome_proprio( const gchar *nome_completo ) {
   if ( !nome_completo || *nome_completo == '\0' ) return NULL;

   // 1. Converte o nome inteiro para minúsculas primeiro para padronizar
   gchar *nome_minusculo = g_utf8_strdown( nome_completo, -1 );

   // 2. Divide a string em um array de palavras usando o espaço como delimitador
   gchar **palavras = g_strsplit( nome_minusculo, " ", -1 );
   g_free( nome_minusculo ); // Não precisamos mais da string inteira minúscula

   // 3. Percorre cada palavra aplicando a regra
   for ( int i = 0; palavras[i] != NULL; i++ ) {
      // Pula espaços duplos (palavras vazias)
      if ( palavras[i][0] == '\0' ) continue;

      // Se for preposição, mantém minúscula (já está minúscula devido ao g_utf8_strdown)
      if ( eh_preposicao( palavras[i] ) ) {
         continue;
      }

      // Se NÃO for preposição, torna a primeira letra maiúscula
      // Pegamos a primeira letra (lidando corretamente com UTF-8)
      gchar *primeira_letra = g_utf8_strup( palavras[i], g_utf8_next_char( palavras[i] ) - palavras[i] );

      // Pegamos o resto da palavra
      const gchar *resto_palavra = g_utf8_next_char( palavras[i] );

      // Juntamos a primeira letra maiúscula com o resto
      gchar *palavra_capitalizada = g_strconcat( primeira_letra, resto_palavra, NULL );

      // Substituímos no array de palavras e liberamos a memória temporária
      g_free( palavras[i] );
      palavras[i] = palavra_capitalizada;
      g_free( primeira_letra );
   }

   // 4. Junta todas as palavras de volta em uma única string separada por espaços
   gchar *resultado = g_strjoinv( " ", palavras );

   // 5. Limpa o array da memória (muito importante na GLib!)
   g_strfreev( palavras );

   return resultado;
}
//----------------------------------------------------------------------------------------------------
void ajustar_nomes( const char *arquivo, AppContext *ctx ) {
   if ( !ctx ) return;

   free( ctx->diario );
   ctx->diario = NULL;

   InterfaceDados *dados = &( ctx->dados );
   FocoCoordenadas *foco = &( ctx->cascata.foco );

   dados->qtd_alunos_total = contar_linhas_arquivo( arquivo );

   if ( dados->qtd_alunos_total <= 0 ) {
      dados->qtd_alunos_ativos = dados->qtd_alunos_total;
      return;
   }

   FILE *p = fopen( arquivo, "r" );
   if ( p == NULL ) {
      return;
   }

   ctx->diario = ( FichaAluno* ) calloc( dados->qtd_alunos_total, sizeof( FichaAluno ) );
   FichaAluno *diario = ctx->diario;

   int n1 = 0, n2 = 0;

   for ( int i = 0; i < dados->qtd_alunos_total; i++ ) {
      diario[i].idx = i;

      // 1. Lê diretamente para a string char padrão (aluno) usando fgets
      if ( fgets( diario[i].aluno, sizeof( diario[i].aluno ), p ) == NULL ) break;

      int len = strlen( diario[i].aluno );
      if ( len > 0 && diario[i].aluno[len - 1] == '\n' ) {
         diario[i].aluno[--len] = '\0';
      }

      // 2. Os prefixos '*' e '>' ocupam 1 byte em UTF-8, lógica padrão funciona
      if ( diario[i].aluno[0] == '*' ) n1++;
      else if ( diario[i].aluno[0] == '>' ) n2++;

      diario[i].ativo = ( foco->periodo < 4 ) ? ( diario[i].aluno[0] != '*' ) : ( diario[i].aluno[0] == '>' );

      // 3. Remove os marcadores copiando a string para o início (memmove nativo)
      int salto = find_alpha_utf8( diario[i].aluno );
      if ( salto > 0 ) {
         memmove( diario[i].aluno, diario[i].aluno + salto, len - salto + 1 );
      } else if ( salto == -1 && len > 0 ) {
         diario[i].aluno[0] = '\0';
      }

      // 4. Aplica a conversão de capitalização (GLib)
      gchar *nome_formatado = converter_nome_proprio( diario[i].aluno );
      if ( nome_formatado ) {
         // Copia de volta para a struct e libera a memória alocada pela GLib
         g_strlcpy( diario[i].aluno, nome_formatado, sizeof( diario[i].aluno ) );
         g_free( nome_formatado );
      }
   }
   fclose( p );

   dados->qtd_alunos_ativos = ( ( n2 != 0 ) || ( foco->periodo == 4 ) ) ? n2 : ( dados->qtd_alunos_total - n1 );

   qsort( diario, dados->qtd_alunos_total, sizeof( FichaAluno ), alfabetica_lista_de_alunos );

   ajustar_nomes_tabelas( diario, dados );
}
//====================================================================================================







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
