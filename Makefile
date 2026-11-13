# =====================================================================
#                         PROJETO VÉRTICE - MAKEFILE
# =====================================================================

# Define o shell explicitamente para garantir o funcionamento do 'echo -e',
# 'command -v' e evitar incompatibilidades com o /bin/sh (dash) do Debian.
SHELL := /bin/bash

# --- CORES PARA O TERMINAL ---
C_RESET := \033[0m
C_BOLD  := \033[1m
C_BLUE  := \033[34m
C_GREEN := \033[32m
C_WARN  := \033[33m
C_RED   := \033[31m

# =====================================================================
# 1. CONFIGURAÇÕES E ARQUIVOS DO PROJETO
# =====================================================================
APP_NAME   := vertice
CC         ?= gcc
PKG_CONFIG ?= pkg-config
RM         := rm -f

# Recursos GTK e LaTeX
RESOURCES_XML := recursos/recursos.xml
RESOURCES_C   := recursos.c
RESOURCES_O   := $(RESOURCES_C:.c=.o)
UI_FILE       := recursos/interface.glade
PROFESSOR_STY := recursos/professor.sty
CSS_FILES     := $(wildcard recursos/css/*.css)
SH_SCRIPTS    := $(wildcard *.sh)

# Fontes autorais separados do código autogerado
USER_SOURCES := \
   main.c layout.c signals.c callbacks.c auxiliar.c \
   interface.c diarios.c latex.c provas.c config.c \
   dinamica.c assincrono.c relatorios.c imagens.c \
   pds.c omr.c gas.c seduc-ma.c tex.c mensagens.c \
   basicas.c rgba.c matriz.c glib_gio.c imgcore.c

C_SOURCES := $(USER_SOURCES) $(RESOURCES_C)
OBJECTS   := $(C_SOURCES:.c=.o)
DEPFILES  := $(OBJECTS:.o=.d)

# =====================================================================
# 2. PACOTES E DEPENDÊNCIAS
# =====================================================================
PKG_DEPS   := gtk+-3.0 poppler-glib cairo
PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PKG_DEPS))
PKG_LIBS   := $(shell $(PKG_CONFIG) --libs $(PKG_DEPS))

# =====================================================================
# 3. FLAGS BASE E DIAGNÓSTICOS (Sincronizado com .clangd / Kate)
# =====================================================================
# Flags universais suportadas tanto pelo GCC quanto pelo Clang
PORTABLE_WARNINGS := \
   -Wall -Wextra -Wpedantic -Wshadow -Wcast-align -Wundef \
   -Wformat=2 -Wformat-security -Werror=implicit-function-declaration \
   -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
   -Wold-style-definition -Wredundant-decls -Wconversion \
   -Wsign-conversion -Wdouble-promotion -Wnull-dereference \
   -Wwrite-strings -Wswitch-enum -Wcast-qual -Wvla \
   -Wpointer-arith -Wfloat-equal -Wuninitialized \
   -Wno-overlength-strings

# Flags adicionais exclusivas do GCC
GCC_ONLY_WARNINGS := \
   -Wduplicated-cond -Wduplicated-branches -Wstrict-aliasing=3

BASE_WARNINGS  := $(PORTABLE_WARNINGS) $(GCC_ONLY_WARNINGS)
COMMON_CFLAGS  := -std=gnu11 $(BASE_WARNINGS) -march=native -fopenmp $(PKG_CFLAGS) -MMD -MP
COMMON_LDFLAGS := -fopenmp
LDLIBS         := $(PKG_LIBS) -lssl -lcrypto -llapacke -llapack -lblas -lm

# Flags limpas para o clang-tidy (sincronizadas com o .clangd)
CLANG_TIDY_FLAGS := -std=gnu11 $(PORTABLE_WARNINGS) -Wno-gnu-zero-variadic-macro-arguments -fopenmp $(PKG_CFLAGS)

# =====================================================================
# 4. CONTROLE DE MODO (Release vs Debug/ASan vs Valgrind)
# =====================================================================
MODE ?= release

ifeq ($(MODE),debug)
   SPECIFIC_CFLAGS  := -O1 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2
   SPECIFIC_LDFLAGS := -rdynamic -g3 -fsanitize=address,undefined
   MODE_MSG         := "$(C_BLUE)🛠️  Linkando em modo DIAGNÓSTICO (ASan + UBSan + Símbolos)...$(C_RESET)"
else ifeq ($(MODE),valgrind)
   SPECIFIC_CFLAGS  := -O0 -g3 -fno-omit-frame-pointer -U_FORTIFY_SOURCE
   SPECIFIC_LDFLAGS := -rdynamic -g3
   MODE_MSG         := "$(C_WARN)🕵️  Linkando em modo VALGRIND (Rastreamento Puro de Heap)...$(C_RESET)"
else
   SPECIFIC_CFLAGS  := -O3 -flto=auto -fno-plt -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -fPIE -fstack-protector-strong
   SPECIFIC_LDFLAGS := -flto=auto -s -pie -Wl,-z,relro,-z,now
   MODE_MSG         := "$(C_GREEN)🚀 Linkando em modo ALTO DESEMPENHO (-O3 + LTO + Hardening)...$(C_RESET)"
endif

CFLAGS  := $(COMMON_CFLAGS) $(SPECIFIC_CFLAGS)
LDFLAGS := $(COMMON_LDFLAGS) $(SPECIFIC_LDFLAGS)

# =====================================================================
# 5. REGRAS DE COMPILAÇÃO E LINKAGEM
# =====================================================================
.PHONY: default all build clean distclean run run-debug run-valgrind \
        debug release valgrind install uninstall format format-ui lint lsp fix help

default: help
all: release

debug release valgrind:
	@$(MAKE) --no-print-directory build MODE=$@

build: $(APP_NAME)

$(APP_NAME): $(OBJECTS)
	@echo -e $(MODE_MSG)
	@$(CC) $(LDFLAGS) $(OBJECTS) -o $(APP_NAME) $(LDLIBS)
	@echo -e "$(C_GREEN)✔ Binário '$(APP_NAME)' ($(MODE)) gerado com sucesso!$(C_RESET)"

# Geração do código C a partir do XML de recursos, Glade, CSS e pacote LaTeX
$(RESOURCES_C): $(RESOURCES_XML) $(UI_FILE) $(CSS_FILES) $(PROFESSOR_STY)
	@echo -e "🎨 Validando a interface XML ($(UI_FILE))..."
	@gtk-builder-tool validate $(UI_FILE)
	@echo -e "🛠️  Compilando recursos GTK (UI, CSS e LaTeX)..."
	@glib-compile-resources $(RESOURCES_XML) --sourcedir=recursos --target=$(RESOURCES_C) --generate-source

# Compilação isolada para código autogerado pela GLib (sem poluição de warnings internos)
$(RESOURCES_O): $(RESOURCES_C)
	@echo -e "📦 Compilando recursos GTK: $(C_BOLD)$<$(C_RESET)..."
	@$(CC) $(CFLAGS) -Wno-overlength-strings -Wno-missing-prototypes -Wno-missing-declarations -Wno-conversion -c $< -o $@

%.o: %.c
	@echo -e "📦 Compilando módulo: $(C_BOLD)$<$(C_RESET)..."
	@$(CC) $(CFLAGS) -c $< -o $@

# =====================================================================
# 6. FERRAMENTAS, DIAGNÓSTICOS E LINTERS
# =====================================================================
fix:
	@echo -e "$(C_BLUE)🔧 Aplicando refatoração automática e verificação com clang-tidy...$(C_RESET)"
	@if command -v clang-tidy >/dev/null 2>&1; then \
		clang-tidy --fix $(USER_SOURCES) -- $(CLANG_TIDY_FLAGS); \
		echo -e "$(C_GREEN)✅ Refatoração e limpeza semântica concluídas com sucesso!$(C_RESET)"; \
	else \
		echo -e "$(C_RED)⚠️  Erro: 'clang-tidy' não instalado. Instale com: sudo apt install clang-tidy$(C_RESET)"; \
	fi

lsp: clean
	@echo -e "$(C_BLUE)🧠 Gerando compile_commands.json para o LSP (Kate / clangd)...$(C_RESET)"
	@if command -v bear >/dev/null 2>&1; then \
		bear -- $(MAKE) --no-print-directory build MODE=debug; \
		echo -e "$(C_GREEN)✔ Arquivo compile_commands.json atualizado!$(C_RESET)"; \
	else \
		echo -e "$(C_RED)⚠️  Erro: 'bear' não encontrado. Instale com: sudo apt install bear$(C_RESET)"; \
	fi

format:
	@echo -e "$(C_BLUE)🧹 Formatando arquivos autorais C, H e Shell Scripts...$(C_RESET)"
	@if command -v astyle >/dev/null 2>&1; then \
		astyle --style=google --indent=spaces=3 --pad-oper --pad-header \
			--pad-paren-in --unpad-paren --add-braces --suffix=none $(USER_SOURCES) ./*.h; \
	else \
		echo -e "$(C_RED)⚠️  Erro: 'astyle' não instalado. Instale com: sudo apt install astyle$(C_RESET)"; \
	fi
	@if [ -n "$(SH_SCRIPTS)" ]; then \
		for f in $(SH_SCRIPTS); do \
			if [ -f "$$f" ]; then \
				expand -t 3 "$$f" > "$$f.tmp" && mv "$$f.tmp" "$$f"; \
				chmod +x "$$f"; \
			fi \
		done; \
		echo -e "$(C_GREEN)✅ Shell scripts padronizados.$(C_RESET)"; \
	fi

lint:
	@echo -e "$(C_BLUE)🔍 Executando análise estática profunda com clang-tidy...$(C_RESET)"
	@if command -v clang-tidy >/dev/null 2>&1; then \
		clang-tidy $(USER_SOURCES) -- $(CLANG_TIDY_FLAGS); \
	else \
		echo -e "$(C_RED)⚠️  Erro: 'clang-tidy' não instalado. Instale com: sudo apt install clang-tidy$(C_RESET)"; \
	fi

format-ui:
	@echo -e "$(C_BLUE)✨ Formatando e simplificando o arquivo Glade...$(C_RESET)"
	@gtk-builder-tool simplify --replace $(UI_FILE)
	@echo -e "$(C_GREEN)✅ Glade XML simplificado.$(C_RESET)"

run: release
	@echo -e "🏃 Executando o Vértice..."
	@./$(APP_NAME)

run-debug: debug
	@echo -e "$(C_WARN)🏃 Executando o Vértice (ASan + UBSan Ativos)...$(C_RESET)"
	@G_SLICE=always-malloc LSAN_OPTIONS=suppressions=lsan.supp:print_suppressions=0 ./$(APP_NAME)

run-valgrind: valgrind
	@echo -e "$(C_WARN)🕵️  Iniciando Valgrind com filtros para GTK3, Pango, Fontconfig e OpenMP...$(C_RESET)"
	@G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind \
		--leak-check=full \
		--show-leak-kinds=definite \
		--errors-for-leak-kinds=definite \
		--track-origins=yes \
		--child-silent-after-fork=yes \
		--suppressions=/usr/share/glib-2.0/valgrind/glib.supp \
		--suppressions=/usr/share/gtk-3.0/valgrind/gtk.supp \
		--suppressions=valgrind.supp \
		--log-file=relatorio_valgrind.txt \
		./$(APP_NAME)
	@echo -e "$(C_GREEN)📄 Análise concluída! Arquivo relatorio_valgrind.txt gerado.$(C_RESET)"

clean:
	@echo -e "$(C_BLUE)🧹 Limpando objetos, dependências e binário...$(C_RESET)"
	@$(RM) $(OBJECTS) $(DEPFILES) $(APP_NAME) $(RESOURCES_C)
	@echo -e "$(C_GREEN)✨ Limpeza concluída (compile_commands.json preservado).$(C_RESET)"

distclean: clean
	@echo -e "$(C_WARN)🗑️  Removendo logs de diagnóstico e base do LSP...$(C_RESET)"
	@$(RM) relatorio_valgrind.txt compile_commands.json
	@echo -e "$(C_GREEN)✨ Limpeza profunda concluída.$(C_RESET)"

# =====================================================================
# 7. INSTALAÇÃO E INTEGRAÇÃO (DEBIAN 12)
# =====================================================================
PREFIX   ?= /usr/local
BIN_DIR  := $(DESTDIR)$(PREFIX)/bin
APP_DIR  := $(DESTDIR)$(PREFIX)/share/applications
ICON_DIR := $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps

install: release
	@echo -e "$(C_BLUE)📦 Instalando o Vértice no sistema (requer sudo)...$(C_RESET)"
	@install -d $(BIN_DIR) $(APP_DIR) $(ICON_DIR)
	@install -m 755 $(APP_NAME) $(BIN_DIR)/vertice
	@install -m 644 recursos/vertice.desktop $(APP_DIR)/
	@install -m 644 recursos/imagens/vertice.svg $(ICON_DIR)/vertice.svg
	@echo -e "🔄 Atualizando banco de dados de atalhos..."
	@-update-desktop-database $(APP_DIR) 2>/dev/null || true
	@-gtk-update-icon-cache -f -t $(DESTDIR)$(PREFIX)/share/icons/hicolor 2>/dev/null || true
	@echo -e "$(C_GREEN)✅ Instalação concluída! Disponível no menu Educativo.$(C_RESET)"

uninstall:
	@echo -e "$(C_WARN)🗑️  Removendo o Vértice do sistema...$(C_RESET)"
	@$(RM) $(BIN_DIR)/vertice $(APP_DIR)/vertice.desktop $(ICON_DIR)/vertice.svg
	@-update-desktop-database $(APP_DIR) 2>/dev/null || true
	@-gtk-update-icon-cache -f -t $(DESTDIR)$(PREFIX)/share/icons/hicolor 2>/dev/null || true
	@echo -e "$(C_GREEN)✨ Remoção concluída.$(C_RESET)"

# =====================================================================
# 8. PAINEL DE AJUDA
# =====================================================================
help:
	@echo -e "$(C_BOLD)$(C_BLUE)=====================================================================$(C_RESET)"
	@echo -e "$(C_BOLD)                      PROJETO VÉRTICE - MAKEFILE                     $(C_RESET)"
	@echo -e "$(C_BOLD)$(C_BLUE)=====================================================================$(C_RESET)"
	@echo -e ""
	@echo -e "$(C_BOLD)🎯 COMANDOS PRINCIPAIS:$(C_RESET)"
	@echo -e "   $(C_GREEN)make release$(C_RESET)      - Compila em modo Alto Desempenho (-O3, LTO, Hardening)."
	@echo -e "   $(C_GREEN)make debug$(C_RESET)        - Compila em modo Diagnóstico (ASan + UBSan + -g3)."
	@echo -e "   $(C_GREEN)make valgrind$(C_RESET)     - Compila preparado para o Valgrind (-O0, sem ASan)."
	@echo -e ""
	@echo -e "$(C_BOLD)🏃 EXECUÇÃO:$(C_RESET)"
	@echo -e "   $(C_GREEN)make run$(C_RESET)          - Compila (release) e executa o binário."
	@echo -e "   $(C_GREEN)make run-debug$(C_RESET)    - Compila (debug) e executa o binário."
	@echo -e "   $(C_GREEN)make run-valgrind$(C_RESET) - Compila e executa com o Valgrind (gera log)."
	@echo -e ""
	@echo -e "$(C_BOLD)🛠️  FERRAMENTAS E EDITOR (KATE/LSP):$(C_RESET)"
	@echo -e "   $(C_GREEN)make fix$(C_RESET)          - Aplica correções automáticas (Casts, Divisão Inteira, Chaves)."
	@echo -e "   $(C_GREEN)make lsp$(C_RESET)          - Gera compile_commands.json para o Kate via 'bear'."
	@echo -e "   $(C_GREEN)make format$(C_RESET)       - Formata códigos C/H autorais (AStyle) e Shell Scripts."
	@echo -e "   $(C_GREEN)make lint$(C_RESET)         - Executa análise estática severa (Clang-Tidy)."
	@echo -e "   $(C_GREEN)make format-ui$(C_RESET)    - Simplifica e formata a interface Glade XML."
	@echo -e "   $(C_GREEN)make clean$(C_RESET)        - Limpa objetos (.o/.d) e binário (preserva LSP)."
	@echo -e "   $(C_GREEN)make distclean$(C_RESET)    - Limpeza total (remove também compile_commands.json e logs)."
	@echo -e ""
	@echo -e "$(C_BOLD)📦 SISTEMA (Debian 12):$(C_RESET)"
	@echo -e "   $(C_GREEN)make install$(C_RESET)      - Instala binário, ícone e .desktop (requer sudo)."
	@echo -e "   $(C_GREEN)make uninstall$(C_RESET)    - Remove os componentes do sistema."
	@echo -e ""

-include $(DEPFILES)
