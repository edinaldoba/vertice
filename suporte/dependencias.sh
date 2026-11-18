#!/bin/bash
set -euo pipefail

# 1. Atualização do índice de pacotes
sudo apt update

# 2. Compilação C, GTK3, GLib (GResource) e ferramentas de análise/depuração
DEV_PKGS=(
   build-essential
   gcc
   clang
   clang-tidy
   bear
   gdb
   valgrind
   make
   pkg-config
   libgtk-3-dev
   libglib2.0-dev
   libglib2.0-dev-bin
   libssl-dev
   libpoppler-glib-dev
   libopencv-dev
   libblas-dev
   liblapacke-dev
   libomp-dev
   zathura
   zathura-pdf-poppler
)

sudo apt update && sudo apt install -y

# 3. Motor LaTeX, manipulação de PDFs (pdfunite) e processamento gráfico/OMR
LATEX_MEDIA_PKGS=(
   texlive-full
   poppler-utils
   imagemagick
   gnuplot
)

# 4. Editor e utilitários de linha de comando / formatação
TOOLS_PKGS=(
   kate
   astyle
   shfmt
   shellcheck
   parallel
)

sudo apt install -y "${DEV_PKGS[@]}" "${LATEX_MEDIA_PKGS[@]}" "${TOOLS_PKGS[@]}"

# 5. Libera leitura/escrita de PDF no ImageMagick (detecta v6 ou v7 automaticamente)
for POLICY_XML in /etc/ImageMagick-7/policy.xml /etc/ImageMagick-6/policy.xml; do
   if [[ -f "$POLICY_XML" ]]; then
      sudo sed -i 's/domain="coder" rights="none" pattern="PDF"/domain="coder" rights="read|write" pattern="PDF"/g' "$POLICY_XML"
      echo "Permissão PDF atualizada em: $POLICY_XML"
   fi
done
