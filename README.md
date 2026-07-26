# Vértice - Sistema de Gestão Educacional e Geração de Documentos

O **Vértice** é um ecossistema de software de alta performance desenvolvido nativamente em **C** e **GTK 3**. O projeto foi arquitetado para centralizar a gestão pedagógica de instituições de ensino, automatizar rotinas burocráticas escolares e servir como um motor ultraveloz de diagramação e compilação de avaliações e materiais didáticos via **LuaLaTeX** e **TikZ**.

---

## 🚀 Diferenciais e Arquitetura Técnica

Diferente de sistemas acadêmicos convencionais, o Vértice foi projetado sob conceitos de baixo nível para garantir máxima performance, portabilidade no ecossistema Linux (Debian/Ubuntu) e blindagem de dados:

* **Instância Única Nativa:** Controle de concorrência via D-Bus (GtkApplication), otimizando o uso da memória e tempo de resposta.
* **Processamento Assíncrono e Concorrência:** Arquitetura multithread robusta utilizando POSIX Threads (pthreads) e OpenMP. O isolamento rigoroso de memória (Deep Copy) e a sincronização nativa com o Main Loop (g_idle_add) permitem a execução massiva de Visão Computacional e compilação LaTeX em background, garantindo uma interface gráfica 100% responsiva e à prova de vazamentos.
* **Visão Computacional e Correção Óptica:** Pipeline nativo de processamento em lote com alta tolerância a falhas. Realiza detecção de âncoras geométricas e extração de payload via matrizes, contando com um mecanismo de Quarentena automatizado que isola provas danificadas ou com ruído, sem interromper o fluxo de correção do sistema.
* **Recursos Embutidos (GResource):** Interfaces (.glade) e ativos gráficos compilados diretamente dentro do binário, eliminando falhas de caminho de arquivo.
* **Persistência Binária Pura:** Dados armazenados em estruturas C gravadas em blocos binários (.bin), garantindo performance de leitura instantânea e proteção contra corrupção externa.
* **Motor de Geração:** Compilação de PDFs via LuaLaTeX, conferindo estética matemática de nível editorial aos documentos produzidos.
* **Interface Harmoniosa:** Painel moderno com arquitetura modular de abas e suporte a temas customizados.

---

## 🎓 Modo de Demonstração (Dataset de Homologação)

Para validar o fluxo completo do sistema — da binarização das imagens até a geração dos espelhos de correção — o repositório inclui um **dataset de demonstração** localizado em `dados/gabaritos/2026/Escola Fictícia/`.

* **Objetivo:** Permitir que novos usuários experimentem a correção de gabaritos sem a necessidade de configurar todo o banco de dados do zero.
* **Privacidade:** Todos os nomes de alunos e dados contidos nesta pasta são **estritamente fictícios** e gerados para fins puramente educacionais e de teste.

---

## 🛠️ Tecnologias Utilizadas

* **Linguagem C (C11):** Base do core do sistema, gerenciamento de memória e manipulação de arquivos.
* **GTK 3 & Glade:** Construção da interface gráfica e gerenciamento de sinais e eventos.
* **LuaLaTeX & TikZ:** Mecanismo de renderização de alta precisão estética para fórmulas e gráficos.
* **OpenMP:** Processamento paralelo para otimização de rotinas pesadas de compilação.

---

## ⚖️ Licença e Termos de Uso (Licenciamento Dual)

Este projeto adota o modelo de **Licenciamento Dual** para proteger a autoria do código, incentivar a comunidade de software livre e manter o controle sobre o uso comercial.

### 1. Uso Gratuito e Comunitário (Open Source)
Para professores, estudantes e entusiastas, este software é distribuído sob a licença **GNU GPL v3**. 
* Você pode usar, estudar, modificar e compartilhar este software gratuitamente.
* **Reciprocidade Copyleft:** Qualquer trabalho derivado deste código deve obrigatoriamente ter seu código fonte aberto sob a mesma licença GPL v3.

### 2. Uso e Exploração Comercial por Terceiros
Os termos da licença GPL v3 aplicam-se estritamente à comunidade de software livre. Se você for uma empresa ou instituição privada com interesse em integrar este sistema em um produto comercial fechado ou proprietário, você **NÃO** está autorizado sob a licença GPL v3.

> 📩 **Contato para Licenciamento Comercial:** Para parcerias, suporte corporativo ou aquisição de licença proprietária (que dispensa as obrigações da GPL v3), entre em contato com o autor:
> **Prof. Edinaldo Barbosa de Alencar**

---
*Copyright © 2026 Edinaldo Barbosa de Alencar. Todos os direitos reservados.*
