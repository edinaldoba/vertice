#!/bin/bash

sudo apt install build-essential gcc gdb make libgtk-3-dev libssl-dev texlive-full imagemagick gnuplot kate gedit gnumeric astyle shfmt shellcheck parallel -y

sudo sed -i 's/domain="coder" rights="none" pattern="PDF"/domain="coder" rights="read|write" pattern="PDF"/' /etc/ImageMagick-6/policy.xml

