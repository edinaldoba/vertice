# 📐 Vértice — Sistema de Gestão Educacional, Educação Inclusiva e Diagramação Automatizada

O **Vértice** é um ecossistema de software de alta performance desenvolvido nativamente em **C (GNU11)**, **GLib** e **GTK 3**[cite: 13]. O projeto foi arquitetado para centralizar a gestão pedagógica e administrativa de instituições de ensino, mapear demandas de Educação Inclusiva (AEE), automatizar rotinas burocráticas docentes (conteúdos, frequência, avaliações e consolidação anual)[cite: 6, 7, 8, 9, 12, 13] e servir como um motor ultraveloz de diagramação, compilação de avaliações e correção óptica (OMR) via **LaTeX** e **TikZ**[cite: 10, 11, 13].

---

## 🚀 Arquitetura e Diferenciais Técnicos

Diferente de sistemas acadêmicos convencionais, o Vértice é construído em C de baixo nível para distribuições Linux (Debian/Ubuntu) e alinhado às **Diretrizes de Interface Humana do GNOME (GNOME HIG)**, aliando máxima performance, portabilidade e zero dependências externas pesadas[cite: 13]:

* **Módulo Escolar Completo (Diários de Classe):**
  * **Lançamento de Conteúdos:** Interface otimizada em `GtkTreeView`/`GtkListStore` para controle de aulas normais ou remotas, carga horária e temas/descrições ministrados[cite: 6, 13].
  * **Frequência e Assiduidade:** Registro rápido da presença diária (`P`, `F`, `J`), suporte a acompanhamento individual (*Por Aluno*) e sinalização visual para regimes diferenciados (como *Atividade Domiciliar*)[cite: 7, 13].
  * **Gestão de Avaliações e Notas:** Lançamento estruturado por instrumentos avaliativos (`AV-1`, `REC-1`, `AV-2`, `REC-2`), pontuação diária e controle de estudantes inativos/transferidos[cite: 8, 13].
  * **Relatório Final e Consolidação Anual:** Painel consolidado com notas dos 4 períodos letivos, soma, média anual, recuperação final, conselho de classe, filtro de pendências/exceções e status automático (*Aprov Dir*, *Rec Final*)[cite: 9].
  * **Sincronização SIAEP:** Estrutura preparada para automação e conciliação de dados pedagógicos com o sistema escolar estadual[cite: 13].

* **Mapeamento de Atipicidades e Educação Inclusiva (AEE):**
  * **Arquitetura em Máscara de Bits (32-bits):** Estrutura binária compacta (`FichaAtipicidade` de 8 bytes) capaz de mapear 24 condições clínicas, neurodivergências e metadados pedagógicos simultâneos por estudante (TEA com Níveis de Suporte 1 a 3, TDAH, Deficiência Intelectual, Transtornos de Aprendizagem, TPAC, Altas Habilidades/Superdotação, Transtorno Mental, TOD, Transtorno de Conduta, Transtornos Emocionais, Mutismo Seletivo, Síndrome de Down, Paralisia Cerebral, Deficiências Física/Auditiva/Visual/Múltipla, Surdocegueira, Epilepsia, Doença Crônica e Vulnerabilidade Social)[cite: 1, 2, 12].
  * **Dicionário Clínico de CIDs Integrado:** Rótulos e dicas de ferramentas (*tooltips*) padronizados com mapeamento cruzado entre **CID-10** e **CID-11** (incluindo subtipos clínicos reais da rotina escolar)[cite: 2].
  * **Painel Reativo em `GtkFlowBox`:** Renderização dinâmica de cartões de estudantes atípicos com destaque visual do aluno ativo, *scroll-to-view* automático (`gtk_adjustment_clamp_page`), mini-cards de diagnóstico com exclusão rápida e sincronização bidirecional com *autosave*[cite: 1, 5, 12].
  * **Relatórios Institucionais em LaTeX/TikZ:** Geração automatizada de dossiês visuais em duas colunas com cartões e mini-cards vetorizados em TikZ, categorizados por cores semânticas para apoio à coordenação pedagógica e ao AEE[cite: 3, 5, 12].

* **Interface Ergonômica, Temas Dinâmicos e Proporção Áurea ($\phi \approx 1{,}618$):**
  * **Geometria Áurea:** Janela principal dimensionada em exatos $1275 \times 788\text{ pixels}$, proporcionando harmonia estética, simetria vertical e baixo cansaço visual em longas jornadas de uso[cite: 5, 13].
  * **Três Temas Nativos em CSS:** Suporte completo aos temas **Dark Green**, **Deep Blue** e **Light**, com estilização semântica unificada[cite: 1, 5, 13].
  * **Painel de Feedback Flutuante (Overlay Toast) e Diálogos Nativos:** Sistema de notificações assíncronas com efeito *glassmorphism*, aba retrátil minimalista (*Edge Handle*) no rodapé e tratamento de erros críticos via diálogos modais nativos (`GtkMessageDialog`), eliminando dependência de terminal[cite: 6, 13].

* **Multithreading, Qualidade Estática e Gestão Segura da Heap:**
  * Processamento paralelo de imagens e compilação LaTeX assíncrona em background com **POSIX Threads** e **OpenMP**[cite: 13].
  * Uso extensivo de *smart pointers* da GLib (`g_autofree`, `g_autoptr`, `GString`, `GStrv`) e isolamento de memória (*Deep Copy*).
  * Código auditado com **Valgrind**, **AddressSanitizer (ASan/LSan)** e **Clang-Tidy** — zero vazamentos de memória (*zero memory leaks*) e zero alertas de compilação estrita (`-Wall -Wextra -Werror -Wcast-qual`)[cite: 4, 13].

* **Leitura Óptica e Otimização Estocástica (Visão Computacional em C):**
  * Localização de âncoras e ajuste de perspectiva via Algoritmo Genético Coevolutivo em C ($\sim 3$ a $27\text{ ms}$ por folha)[cite: 13].
  * Avalia ortogonalidade (produto escalar) e proporção de área (Fórmula de Shoelace) em tempo real, sem sobrecarga de interpolação de matrizes[cite: 13].
  * **Sistema de Quarentena:** Leitura e correção em lote com isolamento automático de gabaritos rasurados ou desalinhados sem interromper o fluxo contínuo[cite: 13].

* **Persistência Binária e Ativos Embutidos:**
  * Armazenamento direto em estruturas C blindadas (`.bin`) para leitura e escrita instantâneas[cite: 13].
  * Interfaces (`.glade`) e folhas de estilo (`.css`) compiladas diretamente dentro do binário final via `GResource`[cite: 13].

---

## 📋 Módulos do Sistema

1. **`DIÁRIOS` — Gestão Diária de Sala de Aula:**
   * Sub-abas dedicadas para **Conteúdos**[cite: 6], **Frequência**[cite: 7], **Avaliações**[cite: 8] e **Relatório Final**[cite: 9].
   * Controle de modo de edição, exposição controlada de dados e consolidação automática de médias e situação final[cite: 6, 7, 8, 9].

2. **`LATEX` — Diagramação Parametrizada:**
   * Seleção de temas e subtemas curriculares com personalização completa de layout: tipografia (`CMB Right` / `CMU Serif`), número de páginas (1 ou 2), separadores (pontilhados ou contínuos), colunas (2 ou 3), posição de cabeçalho, decoração geométrica e paleta de cores por série/ano integrada ao pacote próprio `professor.sty`[cite: 3, 10].

3. **`PROVAS` — Montagem, Compilação em Lote e Correção Óptica (OMR):**
   * Montagem de avaliações por período e modalidade (presencial ou não presencial), validação de ciclos, geração automatizada de cadernos de prova individualizados, processamento de imagens e importação direta de notas para o diário[cite: 11, 13].

4. **`CONFIG` — Parâmetros Gerais, Atipicidades e Acervo:**
   * Sub-aba **Geral** para diretrizes da turma e instituição[cite: 12].
   * Sub-aba **Atipicidades** para ficha clínica/pedagógica AEE, gestão de provas adaptadas (`ADAPT`), controle de laudos (`LAUDO`) e geração de relatórios em PDF/TikZ[cite: 1, 5, 12].
   * Sub-aba **Acervo** para administração do banco de questões parametrizadas em TeX[cite: 12].

---

## 🎓 Modo de Demonstração (Dataset de Homologação)

Para validar o fluxo completo do sistema — da binarização das imagens até a geração dos espelhos de correção —, o repositório inclui um **dataset de demonstração** em `dados/gabaritos/2026/Escola Fictícia/`[cite: 13].

* **Objetivo:** Permitir que avaliadores e entusiastas experimentem a correção óptica de gabaritos sem a necessidade de configurar o banco de dados do zero[cite: 13].
* **Privacidade:** Todos os nomes de alunos, turmas e dados contidos nesta pasta são **estritamente fictícios** e gerados exclusivamente para fins de homologação e teste[cite: 13].

---

## 🛠️ Tecnologias Utilizadas

* **Linguagem C (GNU11) & GLib / GIO:** Core do sistema, manipulação I/O binária, estruturas de dados e gerenciamento automático de memória (`g_autoptr`/`g_autofree`)[cite: 13].
* **GTK 3, Glade & CSS3:** Construção da interface gráfica nativa (`GtkStack`, `GtkFlowBox`, `GtkListBox`, `GtkTreeView`, `GtkOverlay`) seguindo o GNOME HIG[cite: 1, 6, 12, 13].
* **LaTeX, TikZ & `professor.sty`:** Renderização tipográfica e vetorial de alta precisão para provas, cartões AEE, tabelas e relatórios[cite: 3, 10, 13].
* **OpenMP & POSIX Threads:** Paralelismo e concorrência segura para compilação em lote e processamento óptico[cite: 13].
* **Valgrind, ASan/LSan & Clang-Tidy:** Ferramental de auditoria contínua de memória e análise estática[cite: 4].

---

## ⚖️ Licença e Termos de Uso (Licenciamento Dual)

Este projeto adota o modelo de **Licenciamento Dual** para proteger a autoria do código, incentivar a comunidade de software livre e resguardar a exploração comercial[cite: 13].

### 1. Uso Gratuito e Comunitário (Open Source)
Para professores, estudantes e entusiastas, este software é distribuído sob a licença **GNU General Public License v3.0 (GPLv3)**[cite: 3, 13].
* Você pode usar, estudar, modificar e redistribuir este software gratuitamente[cite: 3, 13].
* **Reciprocidade Copyleft:** Qualquer trabalho derivado deste código deve, obrigatoriamente, manter seu código-fonte aberto sob a mesma licença GPLv3[cite: 13].

### 2. Uso e Exploração Comercial
Os termos da licença GPLv3 aplicam-se estritamente à comunidade de software livre. Empresas, redes privadas de ensino ou instituições que desejem integrar este sistema em produtos proprietários/fechados **não estão autorizados** sob a licença GPLv3[cite: 13].

> 📩 **Contato para Licenciamento Proprietário:** Para parcerias, suporte corporativo ou aquisição de licença comercial (que isenta as obrigações da GPLv3), entre em contato com o autor:
> **Prof. Edinaldo Barbosa de Alencar**[cite: 13]

---
*Copyright © 2026 Edinaldo Barbosa de Alencar. Todos os direitos reservados.*[cite: 3, 13]