#ifndef MENSAGENS_H
#define MENSAGENS_H

#include "interface.h"

// MACRO G_GNUC_PRINTF ADICIONADA AQUI:
#include <glib.h>
#include <stdarg.h>

void atualizar_boas_vindas( InterfacePainel *painel, const InterfaceDados *dados );

bool verificar_estado_de_arquivo( const char *path, InterfacePainel *painel, const InterfaceDados *dados );

gboolean ocultar_painel_feedback_cb( gpointer user_data );
void reexibir_ultima_mensagem( InterfacePainel *painel );

void agendar_ou_exibir_mensagem_painel( MensagemTipo tipo, InterfacePainel *painel,
                                        char *titulo, char *subtitulo, char *instrucao );
void criar_mensagem_painel( MensagemTipo MENSAGEM, InterfacePainel *painel );

/* Protótipo Otimizado */
gchar* meu_gerador_variadico( const char *formato, ... )
    G_GNUC_PRINTF( 1, 2 )
    G_GNUC_MALLOC
    G_GNUC_WARN_UNUSED_RESULT;



#endif
