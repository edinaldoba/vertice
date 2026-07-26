/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wctype.h>
#include <ctype.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <limits.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <pango/pangocairo.h>

#include "basicas.h"
#include "comum.h"
#include "interface.h"
#include "mensagens.h"





int comparar_item_texto_curto( const void* a, const void* b ) {
   const ItemTextoCurto *arg1 = ( const ItemTextoCurto * )a;
   const ItemTextoCurto *arg2 = ( const ItemTextoCurto * )b;

   // strcoll substitui o strcmp cuidando dos acentos automaticamente
   return strcoll( arg1->str, arg2->str );
}




int comparar_item_combo( const void* a, const void* b ) {
   const ItemCombo *arg1 = ( const ItemCombo * )a;
   const ItemCombo *arg2 = ( const ItemCombo * )b;

   // strcoll substitui o strcmp cuidando dos acentos automaticamente
   return strcoll( arg1->str, arg2->str );
}




int comparar_mapeamento_gabarito( const void* a, const void* b ) {
   const MapeamentoGabarito *arg1 = ( const MapeamentoGabarito * )a;
   const MapeamentoGabarito *arg2 = ( const MapeamentoGabarito * )b;

   if ( arg1->num < arg2->num ) return -1;
   if ( arg1->num > arg2->num ) return 1;
   return 0;
}




int buscar_indice_bsearch( const void *chave, const void *vetor, size_t n, size_t tamanho_elemento,
                           int ( *comparador )( const void *, const void * ) ) {
   // Defesa contra ponteiros nulos ou tamanhos inválidos
   if ( !chave || !vetor || n == 0 || tamanho_elemento == 0 || !comparador ) return -1;

   void *encontrado = bsearch( chave, vetor, n, tamanho_elemento, comparador );

   if ( encontrado != NULL ) {
      // 1. Converte os ponteiros void para char* (para trabalhar a nível de byte)
      // 2. Subtrai o endereço base do endereço encontrado
      // 3. Divide pelo tamanho do elemento para descobrir a posição exata no array
      return ( int )( ( ( const char * )encontrado - ( const char * )vetor ) / tamanho_elemento );
   }

   return -1; // Não encontrado
}







int contar_registros_binarios( const char *filepath, size_t tam ) {
   FILE *f = fopen( filepath, "rb" );
   if ( !f ) return 0;

   fseek( f, 0, SEEK_END ); // Salta para o fim do arquivo (ou usar semplesmente "ab")
   long bytes = ftell( f );
   fclose( f );

   // Assumindo que RegistroGabarito tem 32 bytes (conforme nossa refatoração anterior)
   return ( int )( bytes / tam );
}



void display_tempo( const char *descricao, GTimer *cronometro ) {
    double tempo_segundos = g_timer_elapsed( cronometro, NULL );
    if ( tempo_segundos > 60.0 ) {
        int minutos = ( int )( tempo_segundos / 60 );
        double segundos_restantes = tempo_segundos - ( minutos * 60 );
        printf( "⏱ %s concluída em %d min e %.2f seg.\n", descricao, minutos, segundos_restantes );
    } else {
        printf( "⏱ %s concluída em %.3f segundos.\n", descricao, tempo_segundos );
    }
}





// Sua função existente de verificação (exemplo ilustrativo)
static gboolean eh_preposicao( const gchar *str ) {
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

// Nova função para converter o nome completo
gchar* converter_nome_proprio( const gchar *nome_completo ) {
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






//=========================================================================================================//
int quantidade_diretorios( const char *diretorio ) {
   // 1. O "Guardião" inicial
   if ( diretorio == NULL ) return 0;

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
//=========================================================================================================//





// Função comparadora para o qsort
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







//=========================================================================================================//
int quantidade_arquivos_por_extensao( const char *diretorio, const char *ext ) {
   if ( diretorio == NULL || ext == NULL ) return 0;

   DIR *dir = opendir( diretorio );
   if ( dir == NULL ) {
      // Evita que o programa quebre se o diretório não existir
      return 0;
   }

   int cont = 0;
   struct dirent *dp;
   struct stat st;
   char caminho_completo[2048];

   while ( ( dp = readdir( dir ) ) != NULL ) {
      // 1. Ignora os seletores de diretório pai/atual "." e ".."
      if ( strcmp( dp->d_name, "." ) == 0 || strcmp( dp->d_name, ".." ) == 0 ) {
         continue;
      }

      // 2. Monta o caminho completo necessário para o 'stat' funcionar corretamente
      snprintf( caminho_completo, sizeof( caminho_completo ), "%s/%s", diretorio, dp->d_name );

      // 3. Lê os metadados do arquivo/diretório
      if ( stat( caminho_completo, &st ) == 0 ) {
         // Verifica se é um arquivo regular (ignora pastas, links, etc.)
         if ( S_ISREG( st.st_mode ) ) {

            // 4. Verifica a extensão
            const char *str = strrchr( dp->d_name, '.' );
            if ( str != NULL && strcmp( str, ext ) == 0 ) {
               cont++;
            }
         }
      }
   }

   closedir( dir );
   return cont;
}
//=========================================================================================================//





//=========================================================================================================//
/**
 * @brief Escaneia o diretório e carrega uma lista com os nomes dos arquivos regulares
 * que possuem a extensão informada, ordenados ou limitados a 'n' elementos.
 */
ItemTextoCurto *carregar_arquivos_por_extensao( const char *diretorio, const char *ext, int n ) {
   // 1. Defesa inicial: evita processamento se os parâmetros forem inválidos ou se n for inválido
   if ( diretorio == NULL || ext == NULL || n <= 0 ) return NULL;

   DIR *dir = opendir( diretorio );
   if ( dir == NULL ) return NULL;

   // 2. Alocação dinâmica segura com calloc (zera toda a memória automaticamente)
   ItemTextoCurto *img = ( ItemTextoCurto* ) calloc( n, sizeof( ItemTextoCurto ) );
   if ( img == NULL ) {
      g_printerr( "Erro crítico: Falha de alocação de memória em carregar_arquivos_por_extensao()\n" );
      closedir( dir );
      return NULL;
   }

   int i = 0;
   struct dirent *dp;
   struct stat st;
   char caminho_completo[1024];

   // 3. Loop de leitura respeitando estritamente o limite máximo 'n' alocado
   while ( ( dp = readdir( dir ) ) != NULL && i < n ) {

      // 4. Ignora os seletores de diretório pai/atual "." e ".."
      if ( strcmp( dp->d_name, "." ) == 0 || strcmp( dp->d_name, ".." ) == 0 ) {
         continue;
      }

      // 5. Monta o caminho completo necessário para o 'stat' funcionar corretamente
      snprintf( caminho_completo, sizeof( caminho_completo ), "%s/%s", diretorio, dp->d_name );

      // 6. Lê os metadados do arquivo/diretório
      if ( stat( caminho_completo, &st ) == 0 ) {

         // Garante que é estritamente um ARQUIVO REGULAR (ignora pastas, links, etc.)
         if ( S_ISREG( st.st_mode ) ) {

            // 7. Busca segura pela extensão real (de trás para frente)
            const char *str = strrchr( dp->d_name, '.' );

            if ( str != NULL && strcmp( str, ext ) == 0 ) {
               // Copia o nome do arquivo com segurança para o buffer da estrutura
               snprintf( img[i].str, sizeof( img[i].str ), "%.31s", dp->d_name );
               i++;
            }
         }
      }
   }

   closedir( dir );
   return img;
}
//=========================================================================================================//










int extrair_id_widget( const char *string_id ) {
   // Encontra o último sublinhado (garante suporte a múltiplos underscores anteriores)
   const char *ultimo_sublinhado = strrchr( string_id, '_' );
   if ( !ultimo_sublinhado ) {
      g_warning( "Erro: O widget '%s' não possui um caractere sublinhado de separação!", string_id );
      return -1;
   }

   // Ponteiro apontando exatamente para o início do fragmento que deve ser o ID
   const char *sufixo_id = ultimo_sublinhado + 1;

   // 2. VALIDAÇÃO DEFENSIVA: Verifica se o sufixo é composto APENAS por números digitais
   // Se a string for vazia (ex: "botao_") ou o primeiro caractere não for número, rejeita.
   if ( *sufixo_id == '\0' ) return -1;

   for ( const char *p = sufixo_id; *p != '\0'; p++ ) {
      if ( !isdigit( ( unsigned char )*p ) ) {
         g_warning( "Erro de Sintaxe: O sufixo '%s' no widget '%s' não é um ID numérico válido!", sufixo_id, string_id );
         return -1; // Aborta a operação com segurança sem quebrar o app
      }
   }

   // --- Abordagem Avançada com Ponteiro e Loop While ---
   // const char *p = sufixo_id;
   // while ( *p != '\0' ) {
   //    if ( !isdigit( ( unsigned char )*p ) ) {
   //       g_warning( "Erro de Sintaxe: O sufixo '%s' no widget '%s' não é um ID numérico válido!", sufixo_id, string_id );
   //       return -1; // Aborta a operação com segurança
   //    }
   //    p++; // Avança o ponteiro diretamente na memória RAM
   // }

   return atoi( sufixo_id );
}





int atoi_seguro( const char *num_string ) {
   // 1. Defesa básica: se o ponteiro for nulo ou a string for vazia, retorna 0
   if ( !num_string || num_string[0] == '\0' ) {
      return 0;
   }

   char *fim_da_leitura;

   // 2. Faz a leitura usando a função segura da GLib
   guint64 numero_extraido = g_ascii_strtoull( num_string, &fim_da_leitura, 10 );

   // 3. Validação: se o fim for igual ao início, significa que NENHUM número foi lido (ex: "texto")
   if ( num_string == fim_da_leitura ) {
      return 0;
   }

   // 4. Proteção contra estouro (Overflow): garante que o valor cabe em um 'int'
   if ( numero_extraido > INT_MAX ) {
      return INT_MAX; // Limita ao valor máximo que um int suporta
   }

   return ( int )numero_extraido;
}






int contar_linhas_arquivo( const char *arquivo ) {
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







//====================================================================================================
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
//====================================================================================================
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
//====================================================================================================
static int alfabetica_lista_de_alunos( const void *a, const void *b ) {
   const FichaAluno *fa = ( const FichaAluno * )a;
   const FichaAluno *fb = ( const FichaAluno * )b;
   // strcoll respeita a localidade do sistema, ordenando acentos corretamente (Á antes de B)
   return strcoll( fa->aluno, fb->aluno );
}
//====================================================================================================
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
//====================================================================================================
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






//=========================================================================================================//
bool ismember( int num, int *v, int n ) {
   int i;
   for ( i = 0; i < n; i++ ) {
      if ( num == v[i] )
         return true;
   }
   return false;
}
//=========================================================================================================//





//=========================================================================================================//
bool XOR( bool a, bool b ) {
   return ( a || b ) && !( a && b );
}
//=========================================================================================================//





//=========================================================================================================//
int datefind( int a, int *v, int n ) {
   int i;
   for ( i = 0; i < n; i++ ) {
      if ( v[i] == a )
         return i;
   }
   return -1;
}
//=========================================================================================================//





//=========================================================================================================//
int sdatefind( char a, char *v, int n ) {
   char *ptr = memchr( v, a, n );
   if ( ptr != NULL ) {
      return ( int )( ptr - v ); // Aritmética de ponteiros para achar o índice
   }
   return -1;
}
//=========================================================================================================//





//=========================================================================================================//
void intperm( int *P, int n ) {
   int i, j, aux;
   for ( i = 0; i < n; i++ ) {
      j = i + rand() % ( n - i ); // Dica da GG :-) (Apenas para elementos que ainda não foram visitados)
      // j = rand()%n;
      aux = P[j];
      P[j] = P[i];
      P[i] = aux;
   }
}
//=========================================================================================================//





//=========================================================================================================//
int *randperm( int n ) {
   int *N = ( int* ) calloc( n, sizeof( int ) );
   int i;
   for ( i = 0; i < n; i++ ) {
      N[i] = i;
   }
   intperm( N, n );
   return N;
}
//=========================================================================================================//





//=========================================================================================================//
float fMax( float *A, int n ) {
   int i;
   float max;
   max = A[0];
   for ( i = 1; i < n; i++ ) {
      if ( max < A[i] ) {
         max = A[i];
      }
   }
   return max;
}
//=========================================================================================================//





//=========================================================================================================//
IndexacaoValor maximo_f( const float *A, int n ) {
   IndexacaoValor maximo = { 0.0f, -1 }; // Inicialização de segurança padrão

   // Prevenção contra ponteiros nulos ou vetores vazios
   if ( A == NULL || n <= 0 ) {
      return maximo;
   }

   maximo.num = A[0];
   maximo.idx = 0; // Correção crucial: garante o índice 0 caso A[0] seja o maior

   // Declaração do iterador dentro do for (padrão C99+)
   for ( int i = 1; i < n; i++ ) {
      if ( A[i] > maximo.num ) {
         maximo.num = A[i];
         maximo.idx = i;
      }
   }

   return maximo;
}
//=========================================================================================================//





//=========================================================================================================//
void file_permute( FILE **pb, int nquest ) {
   int i, j;
   FILE *aux;
   for ( i = 0; i < nquest; i++ ) {
      j = rand() % nquest;
      aux = pb[j];
      pb[j] = pb[i];
      pb[i] = aux;
   }
}
//=========================================================================================================//





//=========================================================================================================//
void chrperm( char *str, int n ) {
   int i, j;
   char aux;
   for ( i = 0; i < n; i++ ) {
      j = i + rand() % ( n - i );
      aux = str[j];
      str[j] = str[i];
      str[i] = aux;
   }
}
//=========================================================================================================//





//=========================================================================================================//
bool strdist( char *str, int nn, ItemTextoCurto *S ) {
   int i;
   for ( i = 0; i < nn; i++ ) {
      if ( strcmp( str, S[i].str ) == 0 ) {
         return true;
      }
   }
   return false;
}
//=========================================================================================================//








// Cada aluno ocupará exatamente 32 bytes no disco, independente de quantas questões a prova tiver.

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




//=========================================================================================================//
void GAB( const char *arquivo, ItemTextoCurto *G, const int N, const int total_questoes ) {
   FILE *p = fopen( arquivo, "w+" );
   int i;
   for ( i = 0; i < N; i++ ) {
      snprintf( G[i].str, sizeof( G[i].str ), "%s\n", "AABBCCDDEE" );
      do {
         chrperm( G[i].str, total_questoes );
      } while ( strdist( G[i].str, i, G ) );
      fputs( G[i].str, p );
   }
   fclose( p );
}
//=========================================================================================================//





//=========================================================================================================//
DataHoje data_de_hoje( void ) {
   time_t mytime;
   mytime = time( NULL );
   struct tm tm = *localtime( &mytime );
   DataHoje data;
   data.dia = tm.tm_mday;
   data.mes = tm.tm_mon + 1;
   data.ano = tm.tm_year + 1900;
   return data;
}
//=========================================================================================================//




// Verifica qual o dia da semana
int dia_da_semana( int dia, int mes, int ano ) {
   if ( mes < 3 ) {
      mes += 12;
      ano -= 1;
   }

   int k = ano % 100; // Ano do século
   int j = ano / 100; // Século (zero-based)

   // Fórmula de Zeller
   int h = ( dia + ( 13 * ( mes + 1 ) ) / 5 + k + ( k / 4 ) + ( j / 4 ) - 2 * j ) % 7;

   // Ajuste para garantir que o resultado não seja negativo
   return ( h + 7 ) % 7;
}






// =====================================================================
// FUNÇÕES UTILITÁRIAS E DA SUA BIBLIOTECA
// =====================================================================
EstadoArquivo verificar_arquivo( const char *nome_arquivo ) {
   // 1. Blindagem contra ponteiros nulos. Se por acaso a string não existir,
   // a função aborta em segurança retornando que o arquivo é inexistente,
   // evitando um Segmentation Fault clássico do C.
   g_return_val_if_fail( nome_arquivo != NULL, ARQUIVO_INEXISTENTE );

   GStatBuf st;

   // 2. A GLib assume a operação com o sistema de arquivos
   if ( g_stat( nome_arquivo, &st ) != 0 ) {
      return ARQUIVO_INEXISTENTE;
   }

   // 3. Verificação de tamanho
   if ( st.st_size == 0 ) {
      return ARQUIVO_VAZIO;
   }

   return ARQUIVO_PRONTO;
}

int copiar_arquivo( const char *origem, const char *destino ) {
   FILE *src = fopen( origem, "rb" );
   if ( src == NULL ) return -1;

   FILE *dst = fopen( destino, "wb" );
   if ( dst == NULL ) {
      fclose( src );
      return -1;
   }

   char cp_buffer[65536];
   size_t bloco;
   while ( ( bloco = fread( cp_buffer, 1, sizeof( cp_buffer ), src ) ) > 0 ) {
      if ( fwrite( cp_buffer, 1, bloco, dst ) != bloco ) {
         fclose( src );
         fclose( dst );
         return -1;
      }
   }
   fclose( src );
   fclose( dst );
   return 0;
}

// Emula o `mkdir -p` do Bash
int criar_diretorios_cascata( const char *caminho ) {
   char tmp[512];
   snprintf( tmp, sizeof( tmp ), "%s", caminho );
   size_t len = strlen( tmp );
   if ( tmp[len - 1] == '/' ) tmp[len - 1] = 0;

   for ( char *p = tmp + 1; *p; p++ ) {
      if ( *p == '/' ) {
         *p = 0;
         mkdir( tmp, 0755 ); // Cria o nível atual se não existir
         *p = '/';
      }
   }
   return mkdir( tmp, 0755 );
}

// Emula o `cp -u` do Bash (compara qual arquivo é mais novo)
int origem_mais_recente( const char *origem, const char *destino ) {
   struct stat st_orig, st_dest;
   if ( stat( origem, &st_orig ) != 0 ) return 0;
   if ( stat( destino, &st_dest ) != 0 ) return 1; // Se o destino sumiu, atualiza

   // Retorna 1 (verdadeiro) se a origem foi modificada depois do destino
   return st_orig.st_mtime > st_dest.st_mtime;
}




// =====================================================================
// FUNÇÕES PARA MANIPULAÇÃO DE RECURSOS (GLIB) PARA O DISCO FÍSICO
// =====================================================================

// Extrai o arquivo da memória do executável e grava no HD
static int copiar_recurso_para_disco( const char *caminho_recurso, const char *destino ) {
   GError *error = NULL;

   // 1. Puxa os dados diretamente da memória do executável (GResource)
   GBytes *bytes = g_resources_lookup_data( caminho_recurso, G_RESOURCE_LOOKUP_FLAGS_NONE, &error );
   if ( bytes == NULL ) {
      g_printerr( "🚨 Erro interno no GResource: %s\n", error->message );
      g_clear_error( &error );
      return -1;
   }

   // 2. Extrai o ponteiro e o tamanho dos dados
   gsize tamanho;
   gconstpointer buffer = g_bytes_get_data( bytes, &tamanho );

   // 3. Grava no disco rígido físico com as funções padrão do C
   FILE *dst = fopen( destino, "wb" );
   if ( dst == NULL ) {
      g_bytes_unref( bytes ); // Evita vazamento de memória
      return -1;
   }

   size_t gravados = fwrite( buffer, 1, tamanho, dst );
   fclose( dst );

   // Libera o recurso da memória RAM
   g_bytes_unref( bytes );

   return ( gravados == tamanho ) ? 0 : -1;
}

// Como recursos embutidos não tem "data de modificação", comparamos o tamanho do arquivo
static int recurso_foi_modificado( const char *caminho_recurso, const char *caminho_destino ) {
   gsize tamanho_recurso;

   // Pega o tamanho do arquivo embutido
   if ( !g_resources_get_info( caminho_recurso, G_RESOURCE_LOOKUP_FLAGS_NONE, &tamanho_recurso, NULL, NULL ) ) {
      return 0; // Recurso não existe
   }

   struct stat st_dest;
   // Pega o tamanho do arquivo físico no disco
   if ( stat( caminho_destino, &st_dest ) == 0 ) {
      // Se os tamanhos forem diferentes, você alterou o .sty e recompilou!
      if ( ( gsize )st_dest.st_size != tamanho_recurso ) {
         return 1;
      }
   }
   return 0; // Estão iguais
}


// =====================================================================
// FUNÇÃO PRINCIPAL: ORQUESTRADOR DO AMBIENTE LATEX
// =====================================================================
void preparar_ambiente_latex( const char *recursos_prefix ) {
   const char *home = getenv( "HOME" );
   if ( home == NULL ) {
      fprintf( stderr, "🚨 Erro: Variável de ambiente HOME não encontrada.\n" );
      return;
   }

   char pasta_destino[512];
   char caminho_destino_final[1014];
   char origem_completa[512];

   snprintf( pasta_destino, sizeof( pasta_destino ), "%s/texmf/tex/latex/professor", home );
   snprintf( caminho_destino_final, sizeof( caminho_destino_final ), "%s/professor.sty", pasta_destino );
   snprintf( origem_completa, sizeof( origem_completa ), "%s/professor.sty", recursos_prefix );

   printf( "🔍 Verificando ambiente LaTeX...\n" );

   // 1. Verifica e cria o diretório físico texmf
   struct stat st;
   if ( stat( pasta_destino, &st ) != 0 ) {
      printf( "📁 Criando estrutura de diretórios no texmf...\n" );
      criar_diretorios_cascata( pasta_destino );
   }

   // 2. Verifica se o pacote JÁ EXISTE NO DISCO físico
   EstadoArquivo estado = verificar_arquivo( caminho_destino_final );

   if ( estado & (ARQUIVO_INEXISTENTE | ARQUIVO_VAZIO) ) {
      printf( "⚠️ O pacote professor.sty não foi encontrado no sistema TeX.\n" );

      // 3. Verifica se o RECURSO existe na MEMÓRIA (Usando GLib ao invés de stat)
      if ( g_resources_get_info( origem_completa, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL, NULL, NULL ) ) {

         if ( copiar_recurso_para_disco( origem_completa, caminho_destino_final ) == 0 ) {
            printf( "✅ Sucesso: professor.sty instalado em %s\n", pasta_destino );

            // 4. Atualiza o banco do TeX
            char cmd_texhash[512];
            snprintf( cmd_texhash, sizeof( cmd_texhash ), "texhash %s/texmf", home );
            if ( system( cmd_texhash ) == -1 ) {
               fprintf( stderr, "Erro crítico: Não foi possível atualizar /texmf\n" );
            }
         } else {
            fprintf( stderr, "🚨 Erro de I/O ao gravar no disco.\n" );
         }
      } else {
         fprintf( stderr, "🚨 Erro: O recurso interno não foi encontrado em: %s\n", origem_completa );
      }

   } else {
      printf( "✨ Ambiente pronto: professor.sty já está instalado no sistema.\n" );

      // 5. O update invisível (checa diferença de tamanho entre a memória e o disco)
      if ( recurso_foi_modificado( origem_completa, caminho_destino_final ) ) {
         printf( "🔄 Nova versão do .sty detectada no binário! Atualizando o texmf...\n" );
         if ( copiar_recurso_para_disco( origem_completa, caminho_destino_final ) == 0 ) {
            printf( "✅ Pacote atualizado com sucesso.\n" );
         }
      }
   }
}

void preparar_ambiente_latex_no_disco( void ) {
   const char *home = getenv( "HOME" );
   if ( home == NULL ) {
      fprintf( stderr, "🚨 Erro: Variável de ambiente HOME não encontrada.\n" );
      return;
   }

   // Alocação estática dos caminhos usando o HOME dinâmico
   char pasta_destino[512];
   char caminho_destino_final[1014];
   char origem_completa[512];

   snprintf( pasta_destino, sizeof( pasta_destino ), "%s/texmf/tex/latex/professor", home );
   snprintf( caminho_destino_final, sizeof( caminho_destino_final ), "%s/professor.sty", pasta_destino );
   snprintf( origem_completa, sizeof( origem_completa ), "%s/Documentos/LATEX/SEDUC/recursos/professor.sty", home );

   printf( "🔍 Verificando ambiente LaTeX...\n" );

   // 1. Verifica e cria o diretório texmf se necessário
   struct stat st;
   if ( stat( pasta_destino, &st ) != 0 ) {
      printf( "📁 Criando estrutura de diretórios no texmf...\n" );
      criar_diretorios_cascata( pasta_destino );
   }

   // 2. Verifica o status do arquivo no destino
   EstadoArquivo estado = verificar_arquivo( caminho_destino_final );

   if ( (estado & ARQUIVO_INEXISTENTE) || (estado & ARQUIVO_VAZIO) ) {
      printf( "⚠️ O pacote professor.sty não foi encontrado no sistema TeX.\n" );

      // 3. Tenta instalar da origem
      if ( verificar_arquivo( origem_completa ) & ARQUIVO_PRONTO ) {
         if ( copiar_arquivo( origem_completa, caminho_destino_final ) == 0 ) {
            printf( "✅ Sucesso: professor.sty instalado em %s\n", pasta_destino );

            // 4. Atualiza o banco do TeX (usando caminho seguro em vez de ~)
            char cmd_texhash[512];
            snprintf( cmd_texhash, sizeof( cmd_texhash ), "texhash %s/texmf", home );
            if ( system( cmd_texhash ) == -1 ) {
               fprintf( stderr, "Erro crítico: Não foi possível atualizar /texmf\n" );
            }
         } else {
            fprintf( stderr, "🚨 Erro de I/O ao tentar copiar o arquivo.\n" );
         }
      } else {
         fprintf( stderr, "🚨 Erro: O arquivo de origem não está pronto em: %s\n", origem_completa );
      }

   } else {
      printf( "✨ Ambiente pronto: professor.sty já está instalado no sistema.\n" );

      // 5. O cp -u invisível: checa se você editou o .sty no projeto
      if ( origem_mais_recente( origem_completa, caminho_destino_final ) ) {
         printf( "🔄 Nova versão detectada na origem! Atualizando o texmf...\n" );
         if ( copiar_arquivo( origem_completa, caminho_destino_final ) == 0 ) {
            printf( "✅ Pacote atualizado com sucesso.\n" );
         }
      }
   }
}







void apagar_arquivos_temporarios_latex( void ) {
   if ( system( "rm -f   ./dados/temporarios/*.aux          ./dados/temporarios/*.log   \
                    ./dados/temporarios/*.synctex.gz   ./dados/temporarios/*.tex    ./dados/temporarios/*.pdf" ) == -1 ) {
      fprintf( stderr, "Erro crítico: Não foi possível remover arquivos latex\n" );
   }
}




void apagar_arquivos_temporarios_latex_nativamente( const char *diretorio, const char *nome_base, int n_tipos ) {
   // 1. Validação defensiva dos ponteiros
   g_return_if_fail( diretorio != NULL );
   g_return_if_fail( nome_base != NULL );

   const char *extensoes[] = {".aux", ".log", ".tex", ".synctex.gz", ".pdf"};

   // 2. Macro da GLib para descobrir dinamicamente o tamanho do array
   int max_tipos = G_N_ELEMENTS( extensoes );

   // 3. Trava de segurança contra Buffer Over-read
   if ( n_tipos > max_tipos ) {
      n_tipos = max_tipos;
   }

   for ( int i = 0; i < n_tipos; i++ ) {
      // 4. Concatena o nome base com a extensão
      g_autofree char *nome_arquivo = g_strdup_printf( "%s%s", nome_base, extensoes[i] );

      // 5. Monta o caminho seguro resolvendo as barras do sistema operacional automaticamente
      g_autofree char *caminho_completo = g_build_filename( diretorio, nome_arquivo, NULL );

      // 6. g_remove é o wrapper da GLib para o remove() do C
      g_remove( caminho_completo );
   }
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







//=========================================================================================================//
bool detectar_ubuntu( void ) {
   FILE *fp = fopen( "/etc/os-release", "r" );
   if ( fp == NULL ) {
      // Se o arquivo não existir, provavelmente não é um Linux moderno
      return false;
   }

   char linha[256];
   bool eh_ubuntu = false;

   // Lê o arquivo linha por linha buscando o ID da distribuição
   while ( fgets( linha, sizeof( linha ), fp ) ) {
      // O arquivo costuma ter uma linha como: ID=ubuntu
      if ( strncmp( linha, "ID=", 3 ) == 0 ) {
         if ( strstr( linha, "ubuntu" ) != NULL ) {
            eh_ubuntu = true;
            break;
         }
      }
   }

   fclose( fp );
   return eh_ubuntu;
}
//=========================================================================================================//





int obter_quantidade_nucleos( void ) {
   // _SC_NPROCESSORS_ONLN retorna o número de processadores online
   long num_cores = sysconf( _SC_NPROCESSORS_ONLN );

   // Uma salvaguarda caso o sistema falhe em responder
   if ( num_cores < 1 ) {
      return 1;
   }

   return ( int )num_cores;
}




void obter_resolucao_tela( GtkWidget *widget, int *largura, int *altura ) {
   // 1. Pega a estrutura do display padrão do sistema
   GdkDisplay *display = gtk_widget_get_display( widget );

   // 2. Pega o monitor onde a sua janela está renderizada atualmente
   GdkMonitor *monitor = gdk_display_get_monitor_at_window( display, gtk_widget_get_window( widget ) );

   // 3. Estrutura nativa do GDK que guardará os dados de geometria
   GdkRectangle geometria;
   gdk_monitor_get_geometry( monitor, &geometria );

   // 4. Retorna os valores exatos de pixels pelos ponteiros
   *largura = geometria.width;
   *altura  = geometria.height;
}




// // Provavelmente GTK4
// void obter_resolucao_global(int *largura, int *altura) {
//     GdkScreen *screen = gdk_screen_get_default();
//     if (screen != NULL) {
//         *largura = gdk_screen_get_width(screen);
//         *altura  = gdk_screen_get_height(screen);
//     } else {
//         *largura = 1920; // Fallback de segurança (padrão Full HD)
//         *altura  = 1080;
//     }
// }


int tamanho_fonte( GtkWidget *window ) {
   PangoContext *context = gtk_widget_get_pango_context( window );
   PangoFontDescription *desc = pango_context_get_font_description( context );
   int font_size = pango_font_description_get_size( desc );
   if ( !pango_font_description_get_size_is_absolute( desc ) ) {
      font_size = font_size / PANGO_SCALE; // Converte para pixels se estiver em pontos
   }
   return font_size;
}

int tamanho_fonte_px( GtkWidget *window ) {
   if ( !window ) return 0;

   // 1. Obtém o contexto tipográfico do widget
   PangoContext *context = gtk_widget_get_pango_context( window );
   if ( !context ) return 0;

   // 2. Captura a descrição da fonte ativa herdada pelo contexto
   PangoFontDescription *desc = pango_context_get_font_description( context );
   if ( !desc ) return 0;

   // 3. Captura o tamanho bruto nas unidades internas do Pango
   int raw_size = pango_font_description_get_size( desc );

   /* ==========================================================================
      📐 CONVERSÃO DE UNIDADES DO PANGO
      --------------------------------------------------------------------------
      - Se for ABSOLUTO: O tamanho já está mapeado diretamente em Pixels (px).
      - Se NÃO for absoluto: O tamanho está mapeado em Pontos (pt). Para extrair
        o valor real em pontos, dividimos pela constante de escala PANGO_SCALE
        (que equivale a 1024).
      ========================================================================== */
   if ( !pango_font_description_get_size_is_absolute( desc ) ) {
      return raw_size / PANGO_SCALE; // Retorna o valor limpo em Pontos (ex: 11)
   }

   return raw_size; // Retorna o valor direto em Pixels
}







int calcular_len_limpo( const char *str, int len_maximo_caracteres ) {
   if ( !str || str[0] == '\0' || len_maximo_caracteres <= 0 ) return 0;

   const char *ptr = str;
   int char_count = 0;

   // 1. Avança na string contando caracteres lógicos (não bytes)
   while ( *ptr != '\0' && char_count < len_maximo_caracteres ) {
      ptr = g_utf8_next_char( ptr );
      char_count++;
   }

   // byte_len agora descobre sozinho quantos bytes foram necessários
   // para formar esses 'char_count' caracteres.
   // Se tinha 25 letras com acentos, pode dar 28, 30 bytes... você não precisa saber!
   int byte_len = ptr - str;

   // 2. Retrocede limpando os espaços e lixos do final
   while ( byte_len > 0 ) {
      const char *prev_ptr = g_utf8_prev_char( str + byte_len );
      gunichar ch = g_utf8_get_char( prev_ptr );

      if ( g_unichar_isspace( ch ) || !g_unichar_isprint( ch ) ) {
         byte_len = prev_ptr - str;
      } else {
         break;
      }
   }

   // 3. Retorna o tamanho em BYTES, pronto para alimentar o %.*s do snprintf
   return byte_len;
}




