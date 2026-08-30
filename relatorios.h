#ifndef RELATORIOS_H
#define RELATORIOS_H

#include "comum.h"
#include "interface.h"


#define MAX_HORAS 4
#define QTD_GRUPOS 9

typedef struct {
   long int ids[5][MAX_HORAS];
   char descricao[40]; // Opcional, para ajudar no debug
} GrupoHorario;


void relatorio_de_avaliacoes( InterfacePainel *painel, const AppContext *ctx );
void relatorio_final( InterfacePainel *painel, const AppContext *ctx );
void relatorio_de_frequencia( InterfacePainel *painel, const AppContext *ctx );
void relatorio_de_conteudos( InterfacePainel *painel, const AppContext *ctx );
void abrir_arquivos_de_dados( InterfacePainel *painel, const AppContext *ctx );


void atividades( const InterfaceDados *dados, const CaminhoDiretorio *caminho );
void atividadesQ( const InterfaceDados *dados, const CaminhoDiretorio *caminho );
void atividadesQT( const InterfaceDados *dados, const CaminhoDiretorio *caminho );

void expor_relatorio_escola( const char *path_pdf_origem, const char *nome_arquivo_base,
                             const InterfaceDados *dados, const CaminhoDiretorio *caminho );

void disparar_latex( const char *nome_base, const char *pasta_destino,
                     const InterfaceDados *dados, const CaminhoDiretorio *caminho );



#endif
