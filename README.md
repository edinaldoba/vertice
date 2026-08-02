# 📐 Vértice — Sistema de Gestão Educacional e Diagramação Automatizada

O **Vértice** é um ecossistema de software de alta performance desenvolvido nativamente em **C** e **GTK 3**. O projeto foi arquitetado para centralizar a gestão pedagógica de instituições de ensino, automatizar rotinas burocráticas e servir como um motor ultraveloz de diagramação e compilação de avaliações e materiais didáticos via **LaTeX** e **TikZ**.

---

## 🚀 Arquitetura e Diferenciais Técnicos

Diferente de sistemas acadêmicos convencionais, o Vértice é construído em C de baixo nível para distribuições Linux (Debian/Ubuntu), aliando máxima performance, portabilidade e zero dependências externas pesadas:

* **Multithreading e Alta Performance (OpenMP & POSIX):** Processamento paralelo massivo de imagens e compilação LaTeX em background com isolamento rigoroso de memória (Deep Copy) e sincronização via g_idle_add, mantendo a interface 100% responsiva e livre de memory leaks.

* **Otimização Estocástica e Isomorfismo Geométrico:** Localização de âncoras e ajuste de perspectiva via Algoritmo Genético Coevolutivo em C (~3 a 27 ms por folha). Avalia ortogonalidade (produto escalar) e proporção de área (Fórmula de Shoelace) em tempo real, sem a sobrecarga de interpolação de matrizes.

* **Pipeline Rápido e Sistema de Quarentena:** Leitura e correção em lote com tolerância a falhas. Isolamento automático de gabaritos rasurados, ruidosos ou desalinhados em pasta de quarentena, sem interromper o fluxo contínuo.

* **Persistência Binária e Ativos Embutidos:** Armazenamento direto em estruturas C (.bin) para leitura/escrita instantâneas. Interfaces (.glade) e recursos compilados dentro do binário final via GResource, eliminando falhas de file path.

* **Instância Única e Diagramação Editorial:** Controle de concorrência via D-Bus (GtkApplication), integração com LaTeX/TikZ para emissão de provas em nível editorial, e interface modular limpa voltada para a produtividade docente.

---

## 🎓 Modo de Demonstração (Dataset de Homologação)

Para validar o fluxo completo do sistema — da binarização das imagens até a geração dos espelhos de correção —, o repositório inclui um **dataset de demonstração** em `dados/gabaritos/2026/Escola Fictícia/`.

* **Objetivo:** Permitir que avaliadores e entusiastas experimentem a correção óptica de gabaritos sem a necessidade de configurar o banco de dados do zero.
* **Privacidade:** Todos os nomes de alunos, turmas e dados contidos nesta pasta são **estritamente fictícios** e gerados exclusivamente para fins de homologação e teste.

---

## 🛠️ Tecnologias Utilizadas

* **Linguagem C (C11):** Core do sistema, gerenciamento fino de memória e manipulação I/O de arquivos binários.
* **GTK 3 & Glade:** Construção da interface gráfica nativa e gerenciamento de eventos/sinais.
* **LaTeX & TikZ:** Renderização de alta precisão estética para fórmulas matemáticas, tabelas e gráficos.
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
