#ifndef SIGNALS_H
#define SIGNALS_H

#include <gtk/gtk.h>

#include "interface.h"


void configurar_nomes_dos_widgets( AppContext *ctx );

void app_signals_connect( gpointer user_data );


#endif // SIGNALS_H
