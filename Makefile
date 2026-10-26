# =====================================================================
# 1. CONFIGURAÇÕES E NOMES
# =====================================================================
APP_NAME = app
CC       = gcc

# Arquivos de recursos, interface e estilos
RESOURCES_XML = recursos/recursos.xml
RESOURCES_C   = recursos.c
UI_FILE       = recursos/interface.glade
PROFESSOR_STY = recursos/professor.sty
CSS_FILES     = $(wildcard recursos/css/*.css)

# Regra para validar, limpar a UI e gerar o fonte dos recursos
$(RESOURCES_C): $(RESOURCES_XML) $(UI_FILE) $(CSS_FILES) $(PROFESSOR_STY)
	@echo "🎨 Validando e simplificando a interface XML..."
	@gtk-builder-tool validate $(UI_FILE)
	@gtk-builder-tool simplify --replace $(UI_FILE)
	@echo "🛠️ Compilando recursos GTK (UI e CSS)..."
	glib-compile-resources $(RESOURCES_XML) --sourcedir=recursos --target=$(RESOURCES_C) --generate-source

# Módulos do sistema escritos em C puro
C_SOURCES = main.c layout.c signals.c callbacks.c ui_diario.c auxiliar.c interface.c dinamica.c \
            assincrono.c relatorios.c imagens.c pds.c provas.c omr.c gas.c seduc-ma.c latex.c \
            mensagens.c acervo.c basicas.c rgba.c matriz.c glib_gio.c imgcore.c \
            acervo/estatistica.c \
            $(RESOURCES_C)

# Consolidação dos objetos
OBJECTS = $(C_SOURCES:.c=.o)

# =====================================================================
# 2. PACOTES E DEPENDÊNCIAS
# =====================================================================
PKG_DEPS = gtk+-3.0 poppler-glib cairo

PKG_CFLAGS = $(shell pkg-config --cflags $(PKG_DEPS))
PKG_LIBS   = $(shell pkg-config --libs $(PKG_DEPS))

# =====================================================================
# 3. FLAGS BASE (Comuns a ambos os modos)
# =====================================================================
BASE_WARNINGS = -Wall -Wextra -Wunused-result -D_FORTIFY_SOURCE=2 \
                -Wpedantic -Wshadow -Wcast-align -Wunreachable-code \
                -Wpointer-arith -Wwrite-strings -Wno-overlength-strings

COMMON_CFLAGS = $(BASE_WARNINGS) -Wmissing-prototypes -march=native -fopenmp $(PKG_CFLAGS)
COMMON_LDFLAGS = -fopenmp
LDLIBS = $(PKG_LIBS) -lssl -lcrypto -llapacke -llapack -lblas -lm

# =====================================================================
# 4. CONTROLE DE MODO (Desempenho vs Depuração)
# =====================================================================
# Para compilar em depuração, execute: make MODE=debug
# O padrão é o modo de desempenho.
ifeq ($(MODE),debug)
    # Flags de Diagnóstico e AddressSanitizer
    SPECIFIC_CFLAGS = -O1 -g -fsanitize=address -fno-omit-frame-pointer
    SPECIFIC_LDFLAGS = -rdynamic -g -fsanitize=address
    MODE_MSG = "🛠️ Compilando em modo DIAGNÓSTICO (AddressSanitizer)..."
else
    # Flags de Alto Desempenho e Segurança
    SPECIFIC_CFLAGS = -O3 -flto=auto -fno-plt
    SPECIFIC_LDFLAGS = -flto=auto -s
    MODE_MSG = "🚀 Linkando objetos com otimizações globais (-O3 -flto) via GCC..."
endif

CFLAGS = $(COMMON_CFLAGS) $(SPECIFIC_CFLAGS)
LDFLAGS = $(COMMON_LDFLAGS) $(SPECIFIC_LDFLAGS)

# =====================================================================
# 5. REGRAS DE COMPILAÇÃO E LINKAGEM
# =====================================================================
.PHONY: all clean run debug release

all: $(APP_NAME)

debug:
	@$(MAKE) MODE=debug

release:
	@$(MAKE) MODE=release

$(APP_NAME): $(OBJECTS)
	@echo $(MODE_MSG)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(APP_NAME) $(LDLIBS)
	@echo "✔ Processo concluído com sucesso!"

%.o: %.c
	@echo "📦 Compilando módulo C: $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# =====================================================================
# 6. COMANDOS UTILITÁRIOS
# =====================================================================
run: all
ifeq ($(MODE),debug)
	@echo "🏃 Executando o aplicativo em modo de diagnóstico (ASan ativo)..."
	LSAN_OPTIONS=suppressions=lsan.supp ./$(APP_NAME)
else
	@echo "🏃 Executando o aplicativo em modo de alto desempenho..."
	./$(APP_NAME)
endif

clean:
	@echo "🧹 Limpando arquivos de objeto e binários antigos..."
	rm -f $(OBJECTS) $(APP_NAME) $(RESOURCES_C)
	@echo "✨ Limpeza concluída."
