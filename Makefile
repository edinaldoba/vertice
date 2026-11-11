# =====================================================================
#                         PROJETO VÉRTICE - MAKEFILE
# =====================================================================
#
# COMANDOS PRINCIPAIS (TARGETS):
#   make [all]         - Compila o executável principal (Vértice).
#   make release       - Compila em modo de Alto Desempenho (-O3, LTO e Hardening).
#   make debug         - Compila em modo Diagnóstico (AddressSanitizer + símbolos).
#   make valgrind      - Compila preparado para o Valgrind (-O0 e sem ASan).
#
# EXECUÇÃO E DIAGNÓSTICO:
#   make run           - Compila (se necessário) e executa o binário.
#   make run-valgrind  - Compila e executa com o Valgrind, gerando relatorio_valgrind.txt.
#
# FERRAMENTAS E UTILITÁRIOS:
#   make format-ui     - Formata e simplifica a interface Glade XML (operação manual).
#   make clean         - Limpa objetos (.o), dependências (.d), binários e relatórios.
#
# INSTALAÇÃO E SISTEMA (DEBIAN 12 / LINUX):
#   make install       - Instala o executável, ícone e arquivo .desktop (requer sudo).
#   make uninstall     - Remove todos os componentes instalados no sistema.
#
# VARIÁVEIS SOBRESCREVÍVEIS NO TERMINAL:
#   PREFIX=/usr/local  - Diretório base para instalação.
#   DESTDIR=           - Raiz temporária para criação de pacotes (.deb / .rpm).
#   MODE=release       - Define o modo de compilação (release | debug | valgrind).
#
# EXEMPLOS DE USO:
#   $ make debug && make run
#   $ sudo make install PREFIX=/usr
#   $ make run-valgrind
# =====================================================================

# =====================================================================
# 1. CONFIGURAÇÕES E NOMES
# =====================================================================
APP_NAME    = vertice
CC         ?= gcc
PKG_CONFIG ?= pkg-config
RM         ?= rm -f

# Arquivos de recursos, interface e estilos
RESOURCES_XML = recursos/recursos.xml
RESOURCES_C   = recursos.c
UI_FILE       = recursos/interface.glade
PROFESSOR_STY = recursos/professor.sty
CSS_FILES     = $(wildcard recursos/css/*.css)

# Módulos do sistema escritos em C puro
C_SOURCES = main.c layout.c signals.c callbacks.c auxiliar.c interface.c diarios.c latex.c provas.c config.c \
	    dinamica.c assincrono.c relatorios.c imagens.c pds.c omr.c gas.c seduc-ma.c tex.c mensagens.c \
	    basicas.c rgba.c matriz.c glib_gio.c imgcore.c $(RESOURCES_C)

# Consolidação dos objetos e dependências dinâmicas
OBJECTS  = $(C_SOURCES:.c=.o)
DEPFILES = $(OBJECTS:.o=.d)

# =====================================================================
# 2. PACOTES E DEPENDÊNCIAS
# =====================================================================
PKG_DEPS   = gtk+-3.0 poppler-glib cairo
PKG_CFLAGS = $(shell $(PKG_CONFIG) --cflags $(PKG_DEPS))
PKG_LIBS   = $(shell $(PKG_CONFIG) --libs $(PKG_DEPS))

# =====================================================================
# 3. FLAGS BASE (Comuns a ambos os modos)
# =====================================================================
BASE_WARNINGS = -Wall -Wextra -Wunused-result -Wpedantic -Wshadow \
                -Wcast-align -Wunreachable-code -Wpointer-arith \
                -Wwrite-strings -Wno-overlength-strings

# -MMD e -MP geram os arquivos .d (dependências de headers) automaticamente
COMMON_CFLAGS  = $(BASE_WARNINGS) -Wmissing-prototypes -march=native -fopenmp $(PKG_CFLAGS) -MMD -MP
COMMON_LDFLAGS = -fopenmp
LDLIBS         = $(PKG_LIBS) -lssl -lcrypto -llapacke -llapack -lblas -lm

# =====================================================================
# 4. CONTROLE DE MODO (Desempenho vs Depuração vs Valgrind)
# =====================================================================
ifeq ($(MODE),debug)
    # Diagnóstico: AddressSanitizer e _FORTIFY_SOURCE=2 (requer otimização > O0)
    SPECIFIC_CFLAGS  = -O1 -g -fsanitize=address -fno-omit-frame-pointer -D_FORTIFY_SOURCE=2
    SPECIFIC_LDFLAGS = -rdynamic -g -fsanitize=address
    MODE_MSG = "🛠️ Compilando em modo DIAGNÓSTICO (AddressSanitizer)..."

else ifeq ($(MODE),valgrind)
    # Valgrind: Sem otimização (-O0) para precisão máxima de linha, e sem FORTIFY_SOURCE
    SPECIFIC_CFLAGS  = -O0 -g -fno-omit-frame-pointer
    SPECIFIC_LDFLAGS = -rdynamic -g
    MODE_MSG = "🕵️ Compilando em modo VALGRIND (Rastreamento rigoroso de Heap)..."

else
    # Release: Flags de Alto Desempenho e Hardening (Segurança) para binário de produção
    SPECIFIC_CFLAGS  = -O3 -flto=auto -fno-plt -D_FORTIFY_SOURCE=3 -fPIE
    SPECIFIC_LDFLAGS = -flto=auto -s -pie -Wl,-z,relro,-z,now
    MODE_MSG = "🚀 Linkando objetos com otimizações globais (-O3 -flto) e Hardening..."
endif

CFLAGS  = $(COMMON_CFLAGS) $(SPECIFIC_CFLAGS)
LDFLAGS = $(COMMON_LDFLAGS) $(SPECIFIC_LDFLAGS)

# =====================================================================
# 5. REGRAS DE COMPILAÇÃO E LINKAGEM
# =====================================================================
.PHONY: all clean run debug release install uninstall valgrind format-ui

all: $(APP_NAME)

debug release valgrind:
	@$(MAKE) MODE=$@

$(APP_NAME): $(OBJECTS)
	@echo $(MODE_MSG)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(APP_NAME) $(LDLIBS)
	@echo "✔ Processo concluído com sucesso!"

%.o: %.c
	@echo "📦 Compilando módulo C: $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para gerar os recursos
# Removido o --replace daqui para evitar alterações não esperadas no código-fonte durante a build
$(RESOURCES_C): $(RESOURCES_XML) $(UI_FILE) $(CSS_FILES) $(PROFESSOR_STY)
	@echo "🎨 Validando a interface XML..."
	@gtk-builder-tool validate $(UI_FILE)
	@echo "🛠️ Compilando recursos GTK (UI e CSS)..."
	@glib-compile-resources $(RESOURCES_XML) --sourcedir=recursos --target=$(RESOURCES_C) --generate-source

# Nova regra específica para formatar e simplificar a UI (executar manualmente quando editar o XML)
format-ui:
	@echo "✨ Formatando e simplificando o arquivo Glade..."
	gtk-builder-tool simplify --replace $(UI_FILE)

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

# Regra do Valgrind com dependência da tag "valgrind" (força o rebuild correto)
run-valgrind: valgrind
	@echo "🕵️ Iniciando Valgrind com filtros para GTK3 e GLib..."
	G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind \
		--leak-check=full \
		--show-leak-kinds=definite \
		--errors-for-leak-kinds=definite \
		--track-origins=yes \
		--suppressions=/usr/share/glib-2.0/valgrind/glib.supp \
		--suppressions=/usr/share/gtk-3.0/valgrind/gtk.supp \
		--log-file=relatorio_valgrind.txt \
		./$(APP_NAME)
	@echo "📄 Análise concluída! Arquivo relatorio_valgrind.txt gerado."

clean:
	@echo "🧹 Limpando arquivos temporários e binários..."
	$(RM) $(OBJECTS) $(DEPFILES) $(APP_NAME) $(RESOURCES_C) relatorio_valgrind.txt
	@echo "✨ Limpeza concluída."

# =====================================================================
# 7. INSTALAÇÃO E INTEGRAÇÃO COM O DESKTOP (DEBIAN 12 / LINUX)
# =====================================================================
# Adicionado suporte ao padrão DESTDIR (essencial para criação de pacotes .deb/.rpm)
PREFIX   ?= /usr/local
BIN_DIR   = $(DESTDIR)$(PREFIX)/bin
APP_DIR   = $(DESTDIR)$(PREFIX)/share/applications
ICON_DIR  = $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps

install: release
	@echo "📦 Instalando o Vértice no sistema (requer privilégios)..."
	@install -d $(BIN_DIR)
	@install -d $(APP_DIR)
	@install -d $(ICON_DIR)
	@install -m 755 $(APP_NAME) $(BIN_DIR)/vertice
	@install -m 644 recursos/vertice.desktop $(APP_DIR)/
	@install -m 644 recursos/imagens/vertice.svg $(ICON_DIR)/vertice.svg
	@echo "🔄 Atualizando banco de dados de atalhos e ícones..."
	@-update-desktop-database $(APP_DIR) 2>/dev/null || true
	@-gtk-update-icon-cache -f -t $(DESTDIR)$(PREFIX)/share/icons/hicolor 2>/dev/null || true
	@echo "✅ Instalação concluída! O Vértice está disponível no menu Educativo."

uninstall:
	@echo "🗑️ Removendo o Vértice do sistema..."
	$(RM) $(BIN_DIR)/vertice
	$(RM) $(APP_DIR)/vertice.desktop
	$(RM) $(ICON_DIR)/vertice.svg
	@-update-desktop-database $(APP_DIR) 2>/dev/null || true
	@-gtk-update-icon-cache -f -t $(DESTDIR)$(PREFIX)/share/icons/hicolor 2>/dev/null || true
	@echo "✨ Remoção concluída."

# Inclui os arquivos de dependência gerados pelo compilador
-include $(DEPFILES)
