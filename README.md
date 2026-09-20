# 📐 Vértice — Sistema de Gestão Educacional e Diagramação Automatizada

O **Vértice** é um ecossistema de software de alta performance desenvolvido nativamente em **C** e **GTK 3**. O projeto foi arquitetado para centralizar a gestão pedagógica e administrativa de instituições de ensino, automatizar rotinas burocráticas docentes (conteúdos, frequência e notas), e servir como um motor ultraveloz de diagramação e compilação de avaliações e materiais didáticos via **LaTeX** e **TikZ**.

---

## 🚀 Arquitetura e Diferenciais Técnicos

Diferente de sistemas acadêmicos convencionais, o Vértice é construído em C de baixo nível para distribuições Linux (Debian/Ubuntu), aliando máxima performance, portabilidade e zero dependências externas pesadas:

* **Módulo Escolar Completo (Conteúdos, Frequência e Notas):**
  * **Lançamento de Conteúdos:** Interface otimizada com visualização expandida em `GtkListStore` para controle de aulas, carga horária e temas ministrados.
  * **Frequência e Assiduidade:** Registro rápido da presença diária dos alunos com suporte a alternância em lote e status detalhados.
  * **Gestão de Avaliações e Notas:** Organização por períodos letivos com cálculo automático de médias e mapeamento contínuo do desempenho da turma.
  * **Sincronização SIAEP:** Estrutura preparada para automação e conciliação de dados pedagógicos com o sistema escolar estadual.

* **Interface Ergonômica e Proporção Áurea ($\phi \approx 1{,}618$):**
  * **Geometria Áurea:** Janela principal dimensionada em exatos $1252 \times 774\text{ pixels}$, proporcionando harmonia estética, equilíbrio visual e baixo cansaço mental em longas jornadas de uso.
  * **Painel de Feedback Flutuante (Overlay Toast):** Sistema de notificações assíncronas com efeito *glassmorphism*, barra lateral viva por status (Dark Green, Deep Blue e Light) e controle de congelamento inteligente por *hover* do mouse.
  * **Resgate Sutil (Orelhinha Edge Handle):** Aba retrátil minimalista estilo *Samsung Edge* no rodapé, permitindo reexibir a última notificação a qualquer momento com um único clique sem poluir a área de trabalho.

* **Multithreading e Gestão Segura da Heap (POSIX Threads & OpenMP):**
  * Processamento paralelo de imagens e compilação LaTeX assíncrona em background.
  * Isolamento rigoroso de memória (snapshot com Deep Copy) e gerenciamento de ciclo de vida com encerramento diferido (*deferred cleanup*), garantindo $100\%$ de segurança na memória, zero *memory leaks* e sem travamentos na interface gráfica.

* **Leitura Óptica e Otimização Estocástica (Visão Computacional em C):**
  * Localização de âncoras e ajuste de perspectiva via Algoritmo Genético Coevolutivo em C ($\sim 3$ a $27\text{ ms}$ por folha).
  * Avalia ortogonalidade (produto escalar) e proporção de área (Fórmula de Shoelace) em tempo real, sem a sobrecarga de interpolação de matrizes.
  * **Sistema de Quarentena:** Leitura e correção em lote com isolamento automático de gabaritos rasurados ou desalinhados sem interromper o fluxo contínuo.

* **Persistência Binária e Ativos Embutidos:**
  * Armazenamento direto em estruturas C (`.bin`) para leitura/escrita instantâneas.
  * Interfaces (`.glade`) e recursos compilados dentro do binário final via `GResource`, eliminando falhas de *filepath*.

---

## 📋 Módulos do Sistema

1. **Gestão Diária de Sala de Aula:**
   * Diário de classe digital para registro imediato de conteúdos e horas-aula.
   * Controle contínuo de frequência e chamadas.
   * Lançamento estruturado de notas por avaliações parciais e bimestrais.

2. **Diagramação e Compilação TeX:**
   * Motor de geração automática de provas e gabaritos em nível editorial via LuaLaTeX/TikZ.
   * Suporte a variações dinâmicas de questões, cabeçalhos personalizados e gráficos vetoriais.

3. **Correção Óptica e Relatórios:**
   * Processamento e correção automatizada de folhas de resposta digitalizadas.
   * Relatórios consolidados de desempenho individual e por turma.

---

## 🎓 Modo de Demonstração (Dataset de Homologação)

Para validar o fluxo completo do sistema — da binarização das imagens até a geração dos espelhos de correção —, o repositório inclui um **dataset de demonstração** em `dados/gabaritos/2026/Escola Fictícia/`.

* **Objetivo:** Permitir que avaliadores e entusiastas experimentem a correção óptica de gabaritos sem a necessidade de configurar o banco de dados do zero.
* **Privacidade:** Todos os nomes de alunos, turmas e dados contidos nesta pasta são **estritamente fictícios** e gerados exclusivamente para fins de homologação e teste.

---

## 🛠️ Tecnologias Utilizadas

* **Linguagem C (C11):** Core do sistema, manipulação I/O binária e gerenciamento fino de memória.
* **GTK 3 & Glade:** Construção da interface gráfica nativa, leiaute responsivo em `GtkOverlay` e gerenciamento de eventos de ponteiro.
* **LaTeX & TikZ:** Renderização de alta precisão estética para fórmulas matemáticas, tabelas e gráficos.
* **OpenMP & POSIX Threads:** Paralelismo e concorrência segura para tarefas assíncronas pesadas.

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