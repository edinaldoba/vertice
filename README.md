# 📐 Vértice — Sistema de Gestão Educacional e Diagramação Automatizada

O **Vértice** é um ecossistema de software de alta performance desenvolvido nativamente em **C** e **GTK 3**. O projeto foi arquitetado para centralizar a gestão pedagógica de instituições de ensino, automatizar rotinas burocráticas e servir como um motor ultraveloz de diagramação e compilação de avaliações e materiais didáticos via **LuaLaTeX** e **TikZ**.

---

## 🚀 Diferenciais e Arquitetura Técnica

Diferente de sistemas acadêmicos convencionais, o Vértice foi projetado em baixo nível para garantir máxima performance, portabilidade em distribuições Linux (Debian/Ubuntu) e total blindagem de dados:

* **Instância Única Nativa:** Controle de concorrência via D-Bus (`GtkApplication`), otimizando o uso de memória e garantindo tempo de resposta imediato.
* **Processamento Assíncrono e Concorrência:** Arquitetura multithread robusta utilizando POSIX Threads (`pthreads`) e **OpenMP**. O isolamento rigoroso de memória (*Deep Copy*) e a sincronização nativa com o *Main Loop* (`g_idle_add`) permitem a execução massiva de Visão Computacional e compilação LaTeX em background, mantendo a interface gráfica 100% responsiva e isenta de *memory leaks*.
* **Pipeline de Alta Performance com Algoritmo Genético Coevolutivo (GA):** Substitui as abordagens clássicas de detecção e rotação por uma busca estocástica multiobjetivo em C. O motor localiza e ajusta as âncoras da prova diretamente no espaço da imagem em tempo recorde (~3 a 30 ms por folha).
* **isomorfismo Geométrico Implícito:** Avalia a ortogonalidade (via produto escalar) e a proporção de área (via Fórmula de Gauss/Shoelace) em tempo real, absorvendo rotações e distorções de perspectiva sem a necessidade de interpolação de matrizes pesadas.
* **Resiliência e Sistema de Quarentena Automatizado:** Pipeline nativo de processamento em lote com alta tolerância a falhas. Identifica e isola automaticamente em um diretório de Quarentena as provas rasuradas, com ruído severo ou desalinhadas, garantindo que o fluxo contínuo de correção em massa não seja interrompido.
* **Visão Computacional e Correção Óptica:** Pipeline nativo de processamento em lote com alta tolerância a falhas. Realiza detecção de âncoras geométricas e extração de *payload* via matrizes, contando com um mecanismo automatizado de **Quarentena** que isola provas danificadas ou com ruído sem interromper o fluxo contínuo de correção.
* **Recursos Embutidos (`GResource`):** Interfaces (`.glade`) e ativos gráficos compilados diretamente dentro do binário final, eliminando dependências externas e falhas de caminho de arquivo (*file path*).
* **Persistência Binária Pura:** Armazenamento em estruturas C gravadas em blocos binários (`.bin`), garantindo leitura e escrita instantâneas com proteção contra corrupção externa.
* **Motor de Diagramação Editorial:** Compilação de PDFs via LuaLaTeX e TikZ, conferindo estética matemática e visual de nível editorial a todos os documentos gerados.
* **Interface Modular e Harmoniosa:** Painel moderno com arquitetura modular de abas, suporte a temas customizados e foco na experiência de uso do docente.

---

## 🎓 Modo de Demonstração (Dataset de Homologação)

Para validar o fluxo completo do sistema — da binarização das imagens até a geração dos espelhos de correção —, o repositório inclui um **dataset de demonstração** em `dados/gabaritos/2026/Escola Fictícia/`.

* **Objetivo:** Permitir que avaliadores e entusiastas experimentem a correção óptica de gabaritos sem a necessidade de configurar o banco de dados do zero.
* **Privacidade:** Todos os nomes de alunos, turmas e dados contidos nesta pasta são **estritamente fictícios** e gerados exclusivamente para fins de homologação e teste.

---

## 🛠️ Tecnologias Utilizadas

* **Linguagem C (C11):** Core do sistema, gerenciamento fino de memória e manipulação I/O de arquivos binários.
* **GTK 3 & Glade:** Construção da interface gráfica nativa e gerenciamento de eventos/sinais.
* **LuaLaTeX & TikZ:** Renderização de alta precisão estética para fórmulas matemáticas, tabelas e gráficos.
* **OpenMP & POSIX Threads:** Paralelismo e concorrência para otimização de rotinas pesadas.

---

## ⚖️ Licença e Termos de Uso (Licenciamento Dual)

Este projeto adota o modelo de **Licenciamento Dual** para proteger a autoria do código, incentivar a comunidade de software livre e resguardar a exploração comercial.

### 1. Uso Gratuito e Comunitário (Open Source)
Para professores, estudantes e entusiastas, este software é distribuído sob a licença **GNU General Public License v3.0 (GPLv3)**.
* Você pode usar, estudar, modificar e redistribuir este software gratuitamente.
* **Reciprocidade Copyleft:** Qualquer trabalho derivado deste código deve, obrigatoriamente, manter seu código-fonte aberto sob a mesma licença GPLv3.

### 2. Uso e Exploração Comercial
Os termos da licença GPLv3 aplicam-se estritamente à comunidade de software livre. Empresas, redes privadas de ensino ou instituições que desejem integrar este sistema em produtos proprietários/fechados **não estão autorizados** sob a licença GPLv3.

> 📩 **Contato para Licenciamento Proprietário:** Para parcerias, suporte corporativo ou aquisição de licença comercial (que isenta as obrigações da GPLv3), entre em contato com o autor:
> **Prof. Edinaldo Barbosa de Alencar**

---
*Copyright © 2026 Edinaldo Barbosa de Alencar. Todos os direitos reservados.*
