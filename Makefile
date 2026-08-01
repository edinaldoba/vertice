# =====================================================================
# 1. CONFIGURAÇÕES E NOMES
# =====================================================================
APP_NAME = app
CC       = gcc

# Regra para gerar o fonte dos recursos
RESOURCES_XML = recursos/recursos.xml
RESOURCES_C   = recursos.c

$(RESOURCES_C): $(RESOURCES_XML)
	glib-compile-resources $(RESOURCES_XML) --sourcedir=recursos --target=$(RESOURCES_C) --generate-source

# Módulos do sistema escritos em C puro
C_SOURCES = main.c interface.c dinamica.c assincrono.c relatorios.c imagens.c pds.c \
            glibrary.c imgcore.c callbacks.c basicas.c provas.c gabaritos.c gas.c \
            latex.c mensagens.c layout.c signals.c acervo.c $(RESOURCES_C)

# Consolidação dos objetos gerados
OBJECTS = $(C_SOURCES:.c=.o)

# =====================================================================
# 2. PACOTES E DEPENDÊNCIAS (GLib, GTK3, Poppler, Cairo)
# =====================================================================
# Agrupamos todos os pacotes aqui para o pkg-config fazer o trabalho pesado de uma vez
PKG_DEPS = gtk+-3.0 poppler-glib cairo

PKG_CFLAGS = $(shell pkg-config --cflags $(PKG_DEPS))
PKG_LIBS   = $(shell pkg-config --libs $(PKG_DEPS))

# =====================================================================
# 3. FLAGS DE ALTO DESEMPENHO E SEGURANÇA
# =====================================================================
# Otimizações extremas (OpenMP, LTO, CPU nativa)
PERF_FLAGS = -O3 -march=native -flto=auto -fno-plt -fopenmp

# Warnings rigorosos compartilhados
BASE_WARNINGS = -Wall -Wextra -Wunused-result -Wpedantic -Wshadow \
                -Wcast-align -Wunreachable-code -Wpointer-arith \
                -Wwrite-strings -Wno-overlength-strings

# CFLAGS: Flags de compilação em C purista
CFLAGS = $(BASE_WARNINGS) -Wmissing-prototypes -D_FORTIFY_SOURCE=2 $(PKG_CFLAGS) $(PERF_FLAGS)

# LDFLAGS: A linkagem final precisa do LTO e do OpenMP
LDFLAGS = -fopenmp -flto=auto -s

# LDLIBS: Bibliotecas do sistema (OpenSSL, Math) + as capturadas pelo pkg-config
LDLIBS = $(PKG_LIBS) -lssl -lcrypto -lm

# =====================================================================
# 4. REGRAS DE COMPILAÇÃO E LINKAGEM
# =====================================================================
.PHONY: all clean run

all: $(APP_NAME)

# Linkagem final (Usamos $(CC) puramente)
$(APP_NAME): $(OBJECTS)
	@echo "🚀 Linkando objetos com otimizações globais (-O3 -flto) via GCC..."
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(APP_NAME) $(LDLIBS)
	@echo "✔ Compilação purista de alto desempenho concluída com sucesso!"

# Compilação dos arquivos em C
%.o: %.c
	@echo "📦 Compilando módulo C: $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# =====================================================================
# 5. COMANDOS UTILITÁRIOS
# =====================================================================
run: all
	@echo "🏃 Executando o aplicativo em modo de alto desempenho..."
	./$(APP_NAME)

clean:
	@echo "🧹 Limpando arquivos de objeto e binários antigos..."
	rm -f $(OBJECTS) $(APP_NAME) $(RESOURCES_C)
	@echo "✨ Limpeza concluída."
