/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include "rgba.h"
#include "comum.h"

//===================================================================================================
// MOTOR CENTRAL DE CORES DO VÉRTICE
//===================================================================================================

/**
 * @brief Enumeração das Cores Semânticas do Vértice.
 * Define o significado da cor independente de onde ela é usada.
 */
typedef enum {
   COR_NATIVA = 0,     // Retorna 0 (Usa a cor padrão do GTK)
   COR_INATIVO,        // Cinzas/Opacos para itens inativos ou sem status
   COR_PERIGO,         // Vermelhos para Reprovação, Falta, Feriado
   COR_ALERTA,         // Amarelos para Aulas Extras, Dispensa, Fora de Sala
   COR_INFORMACAO,     // Azuis/Cianos para Dias Pedagógicos
   COR_DESTAQUE,       // Roxos/Púrpuras para Atividades Domiciliares
   COR_SUCESSO,        // Verdes para Faltas Justificadas
   COR_FUNDO_PENDENTE  // Tons pastéis/mutados para fundo de células editáveis
} CorSemantica;

/**
 * @brief Função mestre de cores do Vértice.
 * Retorna 1 e preenche `cor_out` com o RGBA correto de acordo com a semântica e o tema.
 * Retorna 0 se a semântica for COR_NATIVA (deixando o GTK pintar sozinho).
 */
static int obter_cor_semantica( CorSemantica cor, int tema_ativo, GdkRGBA *cor_out ) {
   if ( !cor_out || cor == COR_NATIVA ) return 0;

   switch ( cor ) {

   case COR_INATIVO: // Cinzas
      if ( tema_ativo == STYLE_DEEP_BLUE )  *cor_out = ( GdkRGBA ) {
         0.40, 0.45, 0.55, 1.0
      }; // Deep Blue
      else if ( tema_ativo == STYLE_LIGHT ) *cor_out = ( GdkRGBA ) {
         0.60, 0.60, 0.60, 1.0
      }; // Light
      else                                  *cor_out = ( GdkRGBA ) {
         0.45, 0.50, 0.45, 1.0
      }; // Dark Green (Padrão)
      break;

   case COR_PERIGO: // Vermelhos
      if ( tema_ativo == STYLE_DEEP_BLUE )  *cor_out = ( GdkRGBA ) {
         1.00, 0.36, 0.45, 1.0
      };
      else if ( tema_ativo == STYLE_LIGHT ) *cor_out = ( GdkRGBA ) {
         0.90, 0.00, 0.00, 1.0
      };
      else                                  *cor_out = ( GdkRGBA ) {
         0.95, 0.40, 0.40, 1.0
      };
      break;

   case COR_ALERTA: // Amarelos / Marrons para o Tema Light
      if ( tema_ativo == STYLE_DEEP_BLUE )  *cor_out = ( GdkRGBA ) {
         0.95, 0.79, 0.16, 1.0
      }; // Deep Blue
      else if ( tema_ativo == STYLE_LIGHT ) *cor_out = ( GdkRGBA ) {
         0.70, 0.40, 0.05, 1.0
      }; // Light
      else                                  *cor_out = ( GdkRGBA ) {
         1.00, 0.84, 0.31, 1.0
      }; // Dark Green (Padrão)
      break;

   case COR_INFORMACAO: // Azuis / Cianos
      if ( tema_ativo == STYLE_DEEP_BLUE )  *cor_out = ( GdkRGBA ) {
         0.50, 0.88, 1.00, 1.0
      };
      else if ( tema_ativo == STYLE_LIGHT ) *cor_out = ( GdkRGBA ) {
         0.00, 0.00, 0.85, 1.0
      };
      else                                  *cor_out = ( GdkRGBA ) {
         0.39, 0.71, 0.96, 1.0
      };
      break;

   case COR_DESTAQUE: // Roxos / Púrpuras
      if ( tema_ativo == STYLE_DEEP_BLUE )  *cor_out = ( GdkRGBA ) {
         0.80, 0.60, 0.95, 1.0
      };
      else if ( tema_ativo == STYLE_LIGHT ) *cor_out = ( GdkRGBA ) {
         0.45, 0.15, 0.60, 1.0
      };
      else                                  *cor_out = ( GdkRGBA ) {
         0.75, 0.55, 0.85, 1.0
      };
      break;

   case COR_SUCESSO: // Verdes Menta / Floresta
      if ( tema_ativo == STYLE_DEEP_BLUE )  *cor_out = ( GdkRGBA ) {
         0.25, 0.85, 0.75, 1.0
      };
      else if ( tema_ativo == STYLE_LIGHT ) *cor_out = ( GdkRGBA ) {
         0.15, 0.55, 0.25, 1.0
      };
      else                                  *cor_out = ( GdkRGBA ) {
         0.50, 0.85, 0.55, 1.0
      };
      break;

   case COR_FUNDO_PENDENTE: // Fundos pastéis/mutados (Backgrounds)
      if ( tema_ativo == STYLE_DEEP_BLUE )  *cor_out = ( GdkRGBA ) {
         0.18, 0.25, 0.38, 1.0
      };
      else if ( tema_ativo == STYLE_LIGHT ) *cor_out = ( GdkRGBA ) {
         0.95, 0.91, 0.78, 1.0
      };
      else                                  *cor_out = ( GdkRGBA ) {
         0.20, 0.30, 0.20, 1.0
      };
      break;

   default:
      return 0;
   }
   return 1;
}

//===================================================================================================
// WRAPPERS DE NEGÓCIO (Conectam a lógica da UI com a paleta semântica)
//===================================================================================================

int cor_fundo_celula_pendente( int tema_ativo, GdkRGBA *cor_out ) {
   return obter_cor_semantica( COR_FUNDO_PENDENTE, tema_ativo, cor_out );
}

gboolean cor_texto_observacao( gboolean eh_inativo, int tema_ativo, int pendencias_e_status, GdkRGBA *cor_out ) {
   // Blindagem de ponteiro
   g_return_val_if_fail( cor_out != NULL, FALSE );

   // 1. Tratamento prioritário para alunos inativos
   if ( eh_inativo ) {
      return obter_cor_semantica( COR_INATIVO, tema_ativo, cor_out );
   }

   // 2. Agrupamentos lógicos de estados (Máscaras combinadas)
   int mascara_sucesso = OBS_ALUNO_APROVADO_DIRETO |
                         OBS_ALUNO_APROVADO_NA_FINAL |
                         OBS_ALUNO_APROVADO_NO_CONSELHO |
                         OBS_ALUNO_APROVADO_PENDENCIA;

   int mascara_alerta  = OBS_ALUNO_EM_RECUPERACAO_FINAL |
                         OBS_ALUNO_EM_CONSELHO_CLASSE;

   // 3. Resolução Matemática O(1) via Bitwise AND (&)
   if ( pendencias_e_status & mascara_sucesso ) {
      return obter_cor_semantica( COR_SUCESSO, tema_ativo, cor_out ); // Verde
   }

   if ( pendencias_e_status & OBS_ALUNO_REPROVADO ) {
      return obter_cor_semantica( COR_PERIGO, tema_ativo, cor_out ); // Vermelho
   }

   if ( pendencias_e_status & mascara_alerta ) {
      return obter_cor_semantica( COR_ALERTA, tema_ativo, cor_out ); // Amarelo/Laranja
   }

   // 4. Fallback (Ex: OBS_ALUNO_CURSANDO) manterá a cor nativa do tema GTK
   return FALSE;
}

int cor_texto_aluno_inativo( int eh_inativo, int tema_ativo, GdkRGBA *cor_out ) {
   if ( eh_inativo ) return obter_cor_semantica( COR_INATIVO, tema_ativo, cor_out );
   return 0; // Nota normal, usa cor nativa
}

int cor_texto_nota_vermelha( double nota, int tema_ativo, GdkRGBA *cor_out ) {
   if ( nota >= 0.0 && nota < 6.0 ) return obter_cor_semantica( COR_PERIGO, tema_ativo, cor_out );
   return 0; // Nota normal, usa cor nativa
}

int cor_texto_tipo_registro( int tipo_registro, int tema_ativo, GdkRGBA *cor_out ) {
   switch ( tipo_registro ) {
   case TIPO_REGISTRO_PEDAGOGICO:
      return obter_cor_semantica( COR_INFORMACAO, tema_ativo, cor_out );
   case TIPO_REGISTRO_FERIADO:
      return obter_cor_semantica( COR_PERIGO, tema_ativo, cor_out );
   case TIPO_REGISTRO_AULA_EXTRA:
      return obter_cor_semantica( COR_ALERTA, tema_ativo, cor_out );
   default:
      return 0; // Aula normal
   }
}

int cor_texto_linha_frequencia( StatusAssiduidade status, int tema_ativo, GdkRGBA *cor_out ) {
   switch ( status ) {
   case SEM_STATUS:
      return obter_cor_semantica( COR_INATIVO, tema_ativo, cor_out );

   case AUSENTE:
   case SUSPENSO:
   case CABULOU_AULA:
      return obter_cor_semantica( COR_PERIGO, tema_ativo, cor_out );

   case FORA_DE_SALA:
   case DISPENSADO:
   case DORMINDO:
      return obter_cor_semantica( COR_ALERTA, tema_ativo, cor_out );

   case ATIVIDADE_DOMICILIAR:
      return obter_cor_semantica( COR_DESTAQUE, tema_ativo, cor_out );

   case FALTA_JUSTIFICADA:
      return obter_cor_semantica( COR_SUCESSO, tema_ativo, cor_out );

   default:
      return 0; // Presente (usa cor nativa)
   }
}

