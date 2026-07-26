#ifndef LAYOUT_H
#define LAYOUT_H

#include <gtk/gtk.h>

// Se a sua struct AppContext estiver definida em outro .h (ex: escola.h ou tipos.h),
// inclua ele aqui para que o compilador saiba o que é "AppContext".
#include "interface.h"

/**
 * @brief Orquestrador mestre da interface gráfica.
 * Aloca a janela principal, inicializa os containers e despacha
 * a montagem modular de cada seção do sistema.
 * * @param app Ponteiro para a GtkApplication ativa.
 * @param ctx Ponteiro para o contexto global da aplicação (AppContext).
 */
void construir_interface( GtkApplication *app, AppContext *ctx );

#endif /* LAYOUT_H */
