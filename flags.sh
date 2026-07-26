#!/bin/bash

# 1. Cria ou sobrescreve o arquivo com as flags do seu Makefile/Compilador
cat << EOF > compile_flags.txt
-Wall
-Wextra
-D_FORTIFY_SOURCE=2
-fopenmp
-O1
EOF

# 2. Puxa os caminhos de inclusão de todas as bibliotecas (GTK3, Poppler, Cairo e OpenCV 4)
pkg-config --cflags gtk+-3.0 poppler-glib cairo opencv4 | tr ' ' '\n' >> compile_flags.txt

# 3. Limpa linhas vazias resultantes da formatação
sed -i '/^$/d' compile_flags.txt
