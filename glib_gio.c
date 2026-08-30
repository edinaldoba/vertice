#include <gio/gio.h>
#include <glib/gstdio.h>

#include "comum.h"
#include "glib_gio.h"
#include "interface.h"
#include "basicas.h"
#include "mensagens.h"




gboolean gio_copiar_arquivo( const gchar *caminho_origem, const gchar *caminho_destino ) {
   // 1. Validação defensiva padronizada da GLib (substitui o if inicial)
   g_return_val_if_fail( caminho_origem != NULL, FALSE );
   g_return_val_if_fail( caminho_destino != NULL, FALSE );

   // 2. Extrai a rota da pasta destino e garante que toda a árvore de diretórios exista
   // g_autofree gchar *diretorio_pai = g_path_get_dirname(caminho_destino);
   // if (g_mkdir_with_parents(diretorio_pai, 0755) != 0) {
   //     g_printerr("[ERRO] Nao foi possivel criar/acessar a pasta de destino: %s\n", diretorio_pai);
   //     return FALSE;
   // }

   // 3. Instancia os objetos GFile aproveitando o excelente uso do g_autoptr
   g_autoptr( GFile ) origem = g_file_new_for_path( caminho_origem );
   g_autoptr( GFile ) destino = g_file_new_for_path( caminho_destino );
   g_autoptr( GError ) erro = NULL;

   // 4. Executa a cópia com sobrescrita e tratamento de erro
   if ( !g_file_copy( origem, destino, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &erro ) ) {
      g_printerr( "[ERRO] Falha ao copiar arquivo de '%s' para '%s': %s\n",
                  caminho_origem, caminho_destino, erro->message );
      return FALSE;
   }

   return TRUE;
}



gboolean gio_mover_arquivo( const gchar *caminho_origem, const gchar *caminho_destino ) {
   // 1. Validação defensiva usando as macros nativas da GLib
   g_return_val_if_fail( caminho_origem != NULL, FALSE );
   g_return_val_if_fail( caminho_destino != NULL, FALSE );

   // 2. Extrai a rota da pasta onde o arquivo vai cair e garante que ela exista
   // g_autofree gchar *diretorio_pai = g_path_get_dirname(caminho_destino);
   // if (g_mkdir_with_parents(diretorio_pai, 0755) != 0) {
   //     g_printerr("[ERRO] Nao foi possivel criar/acessar a pasta de destino: %s\n", diretorio_pai);
   //     return FALSE;
   // }

   // 3. Instancia os objetos GFile com limpeza automática de memória
   g_autoptr( GFile ) origem = g_file_new_for_path( caminho_origem );
   g_autoptr( GFile ) destino = g_file_new_for_path( caminho_destino );
   g_autoptr( GError ) erro = NULL;

   // 4. Executa a movimentação sobrescrevendo se já existir
   if ( !g_file_move( origem, destino, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &erro ) ) {
      g_printerr( "[ERRO] Falha ao mover arquivo de '%s' para '%s': %s\n",
                  caminho_origem, caminho_destino, erro->message );
      return FALSE;
   }

   return TRUE;
}






void g_xdg_open( const char *caminho ) {
   g_return_if_fail( caminho != NULL );

   if ( g_file_test( caminho, G_FILE_TEST_EXISTS ) ) {

      GError *erro = NULL;

      // Usa GFile para aceitar caminhos relativos ou absolutos com segurança total
      g_autoptr( GFile ) arquivo = g_file_new_for_path( caminho );
      g_autofree gchar *uri = g_file_get_uri( arquivo );

      if ( !uri ) {
         g_printerr( "Erro ao criar URI para o caminho: %s\n", caminho );
         return;
      }

      if ( !g_app_info_launch_default_for_uri( uri, NULL, &erro ) ) {
         g_printerr( "Erro ao abrir o arquivo: %s\n", erro->message );
         g_clear_error( &erro );
      }

   } else {
      g_printerr( "[AVISO] Arquivo nao encontrado: %s\n", caminho );
   }
}




void g_system_async( const char *comando ) {
   g_return_if_fail( comando != NULL );

   GError *erro = NULL;

   // Passa o comando diretamente, permitindo que o shell interprete os argumentos e binários
   if ( !g_spawn_command_line_async( comando, &erro ) ) {
      g_printerr( "Erro fatal ao tentar executar: %s\n", erro->message );
      g_clear_error( &erro );
   }
}





void g_pdfunite( const char *diretorio, const char **arquivos, const int qtd_arquivos, const char *saida ) {
   g_return_if_fail( diretorio != NULL );
   g_return_if_fail( arquivos != NULL );
   g_return_if_fail( saida != NULL );
   g_return_if_fail( qtd_arquivos > 0 );

   // g_auto(GStrv) aplica g_strfreev automaticamente ao final do escopo.
   g_auto( GStrv ) argv = g_new0( char *, qtd_arquivos + 3 );

   int pos = 0;
   argv[pos++] = g_strdup( "pdfunite" );

   // 1. Os arquivos de entrada mantêm-se relativos, pois queremos
   // que o pdfunite os procure dentro do 'diretorio' temporário.
   for ( int i = 0; i < qtd_arquivos; i++ ) {
      argv[pos++] = g_strdup( arquivos[i] );
   }

   // 2. A MÁGICA AQUI: Converte a saída para um caminho absoluto baseado no
   // diretório raiz do Vértice, ignorando a mudança de diretório do filho!
   argv[pos++] = g_canonicalize_filename( saida, NULL );

   // Terminador NULO obrigatório para vetores da família g_spawn
   argv[pos] = NULL;

   GError *erro = NULL;
   int status_saida = 0;

   // Executamos o binário DIRETAMENTE, sem passar por um terminal "sh -c".
   if ( !g_spawn_sync( diretorio, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL, &status_saida, &erro ) ) {
      g_printerr( "[ERRO FATAL] Falha ao executar pdfunite: %s\n", erro->message );
      g_clear_error( &erro );
   } else if ( status_saida != 0 ) {
      // Se o pdfunite retornar um aviso, logamos o status, mas a interface permanece estável.
      g_printerr( "[AVISO] União de PDFs em '%s' concluída com status %d.\n", diretorio, status_saida );
   }
}





void g_pdflatex_parallel( const char *dir_compile ) {
   g_return_if_fail( dir_compile != NULL );

   int num_cores = ( int ) g_get_num_processors();
   GError *erro = NULL;
   int status_saida = 0;

   // 1. O comando volta a ser puro e limpo.
   // Sem caminhos complexos, sem -output-directory. Apenas compila o que está na pasta.
   g_autofree char *comando_interno = g_strdup_printf(
                                         "parallel -j %d nice -n 5 pdflatex -synctex=1 -interaction=nonstopmode ::: *.tex", num_cores );

   char *argv[] = { ( char * )"sh", ( char * )"-c", comando_interno, NULL };

   // 2. A MÁGICA: O 3º parâmetro (dir_compile) diz à GLib para fazer um "cd"
   // silencioso e seguro para essa pasta ANTES de executar o argv.
   if ( !g_spawn_sync( dir_compile, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL, &status_saida, &erro ) ) {
      g_printerr( "[ERRO FATAL] Falha ao iniciar a compilação paralela: %s\n", erro->message );
      g_clear_error( &erro );
   } else if ( status_saida != 0 ) {
      // Isso substitui o seu antigo "|| true". Se o pdflatex retornar um aviso,
      // nós apenas logamos, mas o programa continua rodando normalmente.
      g_printerr( "[AVISO] Compilação paralela em '%s' concluída com status %d.\n", dir_compile, status_saida );
   }
}





/**
 * Mapeia recursivamente todas as subpastas a partir de 'caminho_pai'.
 * Adiciona cada caminho encontrado dentro do GPtrArray retornado.
 */
GPtrArray *listar_subpastas_recursivo( const char *caminho_pai, GPtrArray *lista_caminhos ) {
   if ( !caminho_pai ) return lista_caminhos;

   if ( !lista_caminhos ) {
      lista_caminhos = g_ptr_array_new_with_free_func( g_free );
   }

   g_autoptr( GFile ) dir = g_file_new_for_path( caminho_pai );
   g_autoptr( GError ) erro = NULL;

   g_autoptr( GFileEnumerator ) enumerator = g_file_enumerate_children(
       dir,
       G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_TYPE,
       G_FILE_QUERY_INFO_NONE,
       NULL,
       &erro );

   if ( erro ) {
      g_printerr( "Erro ao abrir diretório '%s': %s\n", caminho_pai, erro->message );
      return lista_caminhos;
   }

   // Laço infinito controlado com break para capturar erros de leitura do GIO
   while ( TRUE ) {
      GFileInfo *info = g_file_enumerator_next_file( enumerator, NULL, &erro );

      // Proteção contra arquivos corrompidos ou sem permissão de leitura durante o ciclo
      if ( erro ) {
         g_printerr( "Erro ao ler item em '%s': %s\n", caminho_pai, erro->message );
         g_clear_error( &erro );
         break;
      }

      if ( !info ) break; // Atingiu o fim do diretório com sucesso

      g_autoptr( GFileInfo ) info_ptr = info;
      const char *nome = g_file_info_get_name( info_ptr );

      // Barreira: Ignora pastas ocultas do sistema (ex: .git, .vscode, .config)
      if ( nome[0] == '.' ) {
         continue;
      }

      if ( g_file_info_get_file_type( info_ptr ) == G_FILE_TYPE_DIRECTORY ) {
         gchar *caminho_subpasta = g_build_filename( caminho_pai, nome, NULL );

         g_ptr_array_add( lista_caminhos, caminho_subpasta );
         listar_subpastas_recursivo( caminho_subpasta, lista_caminhos );
      }
   }

   return lista_caminhos;
}










