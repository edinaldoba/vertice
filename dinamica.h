#ifndef DINAMICA_H
#define DINAMICA_H

#include "interface.h"



//==================== ESTRUTURAS DE DADOS AUXILIARES ========================
typedef struct {
   GtkWidget *tag_box;
   GtkWidget *label_qtd;
   GtkWidget *btn_menos;
   int valor_atual;
   int limite_max;
} EstadoStepper;


typedef struct {
   const char *nome_alvo;
   GtkWidget *encontrado;
} BuscaWidgetData;
//==================================================================


void limpar_container( GtkWidget *container );

void atualizar_listbox_subtemas( AppContext *ctx );

void logica_stepper_mais( GtkWidget *button, AppContext *ctx );

void logica_stepper_menos( GtkWidget *button, AppContext *ctx );

void rolar_para_o_fim( GtkScrolledWindow *scrolled_window );

void logica_subtema_toggled( GtkWidget *widget, AppContext *ctx );

void alimentar_interface_temporaria( AppContext *ctx, const DadosRascunho *rascunho, const FocoCoordenadas *foco_cache );

void professor_da_disciplina( const char *diretorio, GtkWidget *entry_professor, char *professor );

void gestor_da_escola( const char *diretorio, GtkWidget *entry_gestor, char *gestor );



#endif
