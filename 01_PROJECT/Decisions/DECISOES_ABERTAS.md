# Decisões abertas — antes da Unreal

O GDD define a visão e as regras centrais. Esta lista impede que um valor de teste vire, por acidente, uma decisão oficial. **Não bloquear trabalho independente:** usar parâmetro/documentar hipótese quando possível.

| ID | Decisão a fechar | Estado em 30/09 | Impacto |
| --- | --- | --- | --- |
| D-01 | Versão exata da Unreal e toolchain C++ no novo computador | Aguardar instalação; código atual usa C++17 puro | Adaptadores, build e plugins |
| D-02 | Valores iniciais de confiança, medo, estresse, lealdade e relação de cada companheiro | Não definidos no GDD; nenhum perfil numérico oficial foi criado | Balanceamento e narrativa |
| D-03 | Planta conectada do hospital: metragem, portas, áreas acessíveis e bloqueios por ato | Grafo lógico inicial com 88 áreas, duas escadas, tubulações e quatro saídas em `02_GAME_DESIGN/World/MAPA_LOGICO_HOSPITAL.md`; metragem, geometria, portas e bloqueios por ato aguardam blockout e playtest | Blockout e navegação |
| D-04 | Frequência de save, checkpoints e consequências de carregar partida | Não especificadas | Persistência e tensão |
| D-05 | Duração e condições concretas do amanhecer por dificuldade | Metas aproximadas de 30/90/180 min; Punitivo sem vitória automática | Relógio e finais |
| D-06 | Quantidade de inventário, peso/capacidade e valores de ferimentos/stamina | Relações qualitativas no GDD; números pendentes | Balanceamento de sobrevivência |
| D-07 | Sequência exata de pistas da combinação da garagem, localização de ferramentas e rotas alternativas | Posicionamento provisório e sequência de flags no grafo testados contra softlock; dígitos, texto das pistas, descoberta pelos jogadores e falhas concretas aguardam autoria e playtest | Progressão sem softlock |
| D-08 | Como confiança do Hunter em uma área cai até permitir vitória por esconderijo | Princípio definido; limiar e duração pendentes | Final por esconderijo |
| D-09 | UI de informação, controles, acessibilidade e idiomas iniciais | Pilares definidos; fluxos e texto final pendentes | UX e produção de interface |
| D-10 | Origem, licença e uso público de personagens, trilhas e assets de terceiros | Revisão necessária antes de distribuir o jogo | Escopo de publicação |
| D-12 | Encenação exata de N34/coda, forma de J4 e parâmetros de epílogo e últimos testemunhos | Direção geral fixada em D-11; detalhes de execução em revisão próxima com Cristhian na campanha v0.2, N01–N34 | Roteiro final, flags, atuação e blockout |

## Decisões já fixadas

- Nome do projeto: **The Thug**; arquivo Unreal planejado: `TheThug.uproject`.
- Primeira pessoa, PC, Unreal Engine 5, fundação C++ e composição por Blueprint.
- O Hunter não sabe a posição exata do jogador sem percepção, não enxerga através de paredes e pode errar.
- Quatro rotas convencionais: garagem, telhado, túneis e janela; uma falha de rota não encerra necessariamente a partida.
- A dificuldade Fácil tem companheiros confiáveis; Punitivo não termina automaticamente no amanhecer.
- **D-11, 30/09/2026, confirmada por Cristhian:** Batman matou Thomas; Fácil/Normal não confirmam seu destino ao jogador; Difícil/Punitivo mostram a morte e autoria. Joe se importa com Harley, mas retribui menos seu cuidado em Fácil/Normal/Difícil, inclusive com omissões incômodas; no Punitivo demonstra maior reciprocidade, sujeita às ações do jogador. Jack tem roupa distinta, humor cruel, risadas macabras e pistas progressivas exclusivamente no Punitivo, com revelação secreta possível. O hospital conta sua história pelo ambiente e pode sugerir uma ala desativada do antigo Asilo Arkham, sem exposição direta. Harley, Bartholomeu e Jack têm dilemas e epílogos individuais com estados distintos de saída, captura, morte e paradeiro desconhecido. Justificativa: escolhas narrativas expressas pelo criador; [campanha v0.2](../../02_GAME_DESIGN/Narrative/CAMPANHA_ATOS_RASCUNHO.md) e [NarrativeCore](../../Tools/NarrativeCore/README.md) aplicam as regras. A encenação específica permanece D-12.

Ao fechar uma decisão, registrar data, justificativa, alteração no GDD quando necessário e testes/documentos afetados.
