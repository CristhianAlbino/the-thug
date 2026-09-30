# The Thug — preparação antes da Unreal (30/09 a 23/10/2026)

**Meta de 24 dias:** deixar o trabalho independente da engine especificado, versionado e verificável para iniciar a produção na Unreal assim que o computador estiver disponível. **Não equivale a jogo 100% pronto:** mapas, assets importados, Blueprints, animações, navegação, iluminação, áudio espacial, desempenho e build jogável dependem do editor.

O [GDD Mestre](../../02_GAME_DESIGN/GDD/GDD_MASTER.md) é a fonte do design. Divergências vão para um registro de decisões; números ainda não fixados no GDD são *hipóteses de balanceamento*, nunca fatos aprovados.

Registro atual: [Decisões abertas](../Decisions/DECISOES_ABERTAS.md).

## Estado inicial em 30/09

- [x] GDD consolidado e estrutura do repositório.
- [x] HunterCore C++17: percepções, memória limitada, dificuldade e escolha falível de rota.
- [x] RunRules C++17: quatro rotas convencionais, falhas parciais e regras de amanhecer.
- [x] CompanionCore C++17: estado individual, consequências numéricas do GDD e objetivos condicionais.
- [x] SurvivalCore C++17: ferimentos, sangramento, stamina e inventário com capacidade configurável (base inicial dos dias 03–04 adiantada).
- [x] PerceptionCore C++17: propagação abstrata de som, incerteza e evidências vistas pelo Hunter (base inicial do dia 05 adiantada).
- [x] Testes locais e GitHub Actions para HunterCore, RunRules e CompanionCore.
- [ ] Projeto `.uproject`, níveis e execução no editor: aguardam o computador/Unreal.

## Calendário de entregas

| Dia | Data | Entrega independente da Unreal | Evidência de conclusão |
| --- | --- | --- | --- |
| 01 | 30/09 | Plano, limites, inventário de decisões e início dos companheiros | Roadmap versionado; contratos claros |
| 02 | 01/10 | Companheiros: confiança, medo, recursos, conhecimento e decisões | Casos de ajuda, abandono e traição testados |
| 03 | 02/10 | Ferimentos, sangramento, stamina e recuperação | Regras e testes de limites |
| 04 | 03/10 | Inventário físico e consumo de recursos | Capacidade, categorias e persistência especificadas |
| 05 | 04/10 | Ruído, distrações, evidências e linha entre sensor e decisão | Eventos com origem e incerteza definidos |
| 06 | 05/10 | Relógio, dificuldades, objetivos e estados de campanha | Contratos alinhados ao GDD |
| 07 | 06/10 | Save/load versionado dos sistemas puros | Round trip e migração/erro testados |
| 08 | 07/10 | Simulador de cenários fora da engine | Sequências completas reproduzíveis por semente |
| 09 | 08/10 | Grafo lógico do hospital por andar | Conexões, travas e atalhos rastreáveis |
| 10 | 09/10 | Rotas de fuga e dependências de pistas | Nenhuma rota depende de pista impossível |
| 11 | 10/10 | Sala de câmeras, energia, portas e esconderijos | Contratos de interação e estados |
| 12 | 11/10 | Bíblia dos cinco personagens | Motivações, voz, arcos e limites do GDD |
| 13 | 12/10 | Linha de acontecimentos dos cinco atos | Eventos obrigatórios vs. emergentes separados |
| 14 | 13/10 | Matriz de consequências dos companheiros | Gatilhos, condições e efeitos auditáveis |
| 15 | 14/10 | Diálogos e gravações da campanha | Rascunhos indexados por evento e personagem |
| 16 | 15/10 | Direção de áudio e música adaptativa | Lista de cues, estados e prioridade de mix |
| 17 | 16/10 | Bíblia visual e lista de assets | Referências, escala, placeholders e origem/licença |
| 18 | 17/10 | HUD, menus, acessibilidade e fluxos de informação | Wireframes e critérios de leitura |
| 19 | 18/10 | Tabelas de dificuldade e balanceamento inicial | Variáveis em dados, com hipóteses identificadas |
| 20 | 19/10 | Matriz de testes da campanha completa | Vitória, derrota, saves, IA, rotas e regressão |
| 21 | 20/10 | Orçamento técnico e estratégia de conteúdo | Metas provisórias de desempenho e Git LFS |
| 22 | 21/10 | Contratos dos adaptadores Unreal | Interfaces, eventos, conversão de unidades e logs |
| 23 | 22/10 | Kit de importação para o primeiro dia no editor | Ordem de integração, pastas e checklist executável |
| 24 | 23/10 | Revisão cruzada e congelamento do pacote | Links válidos, testes verdes, pendências explícitas |

As datas são **ordem de produção**, não execução automática em segundo plano. Replanejar se um sistema revelar uma dependência maior. Cada entrega deve ter: especificação, implementação/dados quando fizer sentido, teste relevante, instruções de integração e estado honesto no README.

## Critérios de pronto em 23/10

1. Todo sistema que não depende de rendering/física/editor tem contrato e casos de teste; código compilável passa no CI.
2. Conteúdo narrativo, fluxos e mapas lógicos possuem identificadores estáveis e referências cruzadas.
3. Nenhuma informação é concedida ao Hunter ou a companheiros sem um evento observável que a justifique.
4. Quatro rotas convencionais e seus estados de falha são rastreáveis em design, dados e testes.
5. Cada valor de balanceamento possui origem: **GDD**, **hipótese a testar**, ou **medido na Unreal**.
6. O kit explica a sequência exata de integração no editor e lista o que só poderá ser validado nele.

## Dependências inevitáveis do editor

Criação do projeto C++, importação de `.uasset`/`.umap`, blockout jogável, AI Controller/Behavior Tree, percepção com geometria real, animação, som espacial, UI final, cinematics, performance e build PC. Nenhum documento ou teste de lógica substitui essas verificações.
