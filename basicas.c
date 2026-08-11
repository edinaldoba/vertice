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
#include "glibrary.h"




void gerar_sementes( guint32 *sementes ) {
   g_return_if_fail( sementes );

   // 1. Pega os ciclos/tempo monotônico do processador em alta precisão (64 bits)
   gint64 ciclos_cpu = g_get_monotonic_time();

   // 2. Separa a parte alta e baixa do inteiro de 64 bits
   guint32 baixa = ( guint32 )( ciclos_cpu & 0xFFFFFFFF );
   guint32 alta  = ( guint32 )( ciclos_cpu >> 32 );

   // Imprime para manter a rastreabilidade se precisar reproduzir a execução
   // g_print( "Semente Monotonica (Ciclos): %" G_GINT64_FORMAT "\n", ciclos_cpu );

   // 3. Monta o array de sementes multiplicando por constantes de dispersão
   sementes[0] = baixa;
   sementes[1] = alta ^ 0x9E3779B9; // Proporção Áurea de 32-bit
   sementes[2] = baixa ^ 0x6789A;
   sementes[3] = ( baixa + alta ) ^ 0xBCDEF;
}

int* randperm( int n ) {
   g_return_val_if_fail( n > 0, NULL );
   // Alocação tipo-segura da GLib (equivale a (int*) calloc(n, sizeof(int)))
   int *N = g_new0( int, n );

   for ( int i = 0; i < n; i++ ) {
      N[i] = i;
   }

   // Embaralhamento Fisher-Yates
   for ( int i = 0; i < n - 1; i++ ) {
      int j = g_random_int_range( i, n );
      int aux = N[i];
      N[i] = N[j];
      N[j] = aux;
   }

   return N; // Libera-se depois com g_free(N);
}

void file_permute( FILE **pb, int nquest ) {
   g_return_if_fail( pb && nquest > 0 );

   for ( int i = 0; i < nquest-1; i++ ) {
      int j = g_random_int_range( i, nquest );
      FILE *aux = pb[j];
      pb[j] = pb[i];
      pb[i] = aux;
   }
}

void chrperm( char *str, int n ) {
   g_return_if_fail( str && n > 0 );

   for ( int i = 0; i < n; i++ ) {
      int j = g_random_int_range( i, n );
      char c = str[j];
      str[j] = str[i];
      str[i] = c;
   }
}

void nome_base_gabaritos_bin( char *nome, size_t tam, int turma, int disciplina, int periodo, int prova ) {
   if ( !nome || tam < 16 ) return;
   snprintf( nome, tam, "%.3d%.2d%d%d.bin", turma, disciplina, periodo, prova );
}






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
   g_return_val_if_fail( chave && vetor && comparador && n>0 && tamanho_elemento>0, -1 );

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
   g_return_val_if_fail( filepath && tam>0, 0 );

   FILE *f = fopen( filepath, "rb" );
   if ( !f ) return 0;

   fseek( f, 0, SEEK_END ); // Salta para o fim do arquivo (ou usar semplesmente "ab")
   long bytes = ftell( f );
   fclose( f );

   // Assumindo que RegistroGabarito tem 32 bytes (conforme nossa refatoração anterior)
   return ( int )( bytes / tam );
}



void display_tempo( const char *descricao, GTimer *cronometro ) {
   g_return_if_fail( descricao && cronometro );

    double tempo_segundos = g_timer_elapsed( cronometro, NULL );
    if ( tempo_segundos > 60.0 ) {
        int minutos = ( int )( tempo_segundos / 60 );
        double segundos_restantes = tempo_segundos - ( minutos * 60 );
        printf( "⏱ %s concluída em %d min e %.2f seg.\n", descricao, minutos, segundos_restantes );
    } else {
        printf( "⏱ %s concluída em %.3f segundos.\n", descricao, tempo_segundos );
    }
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




