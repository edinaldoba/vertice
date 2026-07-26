#ifndef MENSAGENS_H
#define MENSAGENS_H

#include "interface.h"


// void atualizar_boas_vindas( GtkWidget *label, const InterfaceDados *dados );
void atualizar_boas_vindas( InterfacePainel *painel, const InterfaceDados *dados );

bool verificar_estado_de_arquivo( const char *path, InterfacePainel *painel, const InterfaceDados *dados );

void criar_mensagem_painel( MensagemTipo MENSAGEM, InterfacePainel *painel );

gchar* meu_gerador_variadico( const char *formato, ... );

#endif
