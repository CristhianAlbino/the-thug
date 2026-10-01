# Revisão pré-Unreal v1 — The Thug

**Estado:** checklist técnico e criativo para revisar com Cristhian, sem fechar o pacote como definitivo. As decisões narrativas D-11 permanecem aprovadas; falas, números, geometria, encenação e duração são versões para avaliação. Áudio e assets finais entram **depois** desta revisão. [Decisões abertas](../Decisions/DECISOES_ABERTAS.md) são a fonte do estado.

## O que está preparado para ler e testar

| Frente | Evidência versionada | Limite concreto |
| --- | --- | --- |
| Campanha | [34 cenas](../../02_GAME_DESIGN/Narrative/CAMPANHA_ATOS_RASCUNHO.md), [texto N01–N34, DOC01–16, AUD01–08 e epílogos](../../02_GAME_DESIGN/Narrative/PACOTE_TEXTO_CAMPANHA_V1.md), [bíblias](../../02_GAME_DESIGN/Narrative/BIBLIA_PERSONAGENS_V1.md) | D-12: Tom das falas, J4, N34/coda e variantes dos finais precisam de leitura do criador e encenação no editor. AUD são **roteiros**, sem áudio gerado. |
| Regras e mapa | Núcleos C++17, [grafo de 88 áreas](../../02_GAME_DESIGN/World/MAPA_LOGICO_HOSPITAL.md), ScenarioLab e quatro fugas | A topologia valida caminhos abstratos, não metragem, vista de câmera, tempo de deslocamento ou IA em paredes reais. |
| Balanceamento e pistas | [Matriz v1](../../02_GAME_DESIGN/Balance/BALANCE_INICIAL_V1.md), [`balance_initial.json`](../../Tools/NarrativeCore/balance_initial.json), [combinação/rotas](../../02_GAME_DESIGN/World/PISTAS_ROTAS_V1.md) e [`puzzle_contract.json`](../../Tools/NarrativeCore/puzzle_contract.json) | `410723`, slots e traços são hipóteses; testar descoberta sem adivinhação e 3h Difícil em playthrough. D-08 esconderijo sem limiar aprovado. |
| Interface | [Fluxos e acessibilidade](../../02_GAME_DESIGN/UX/INTERFACE_FLUXOS_V1.md), [wireframe vetorial](../../02_GAME_DESIGN/UX/WIREFRAME_HUD_CAMERAS.svg) | UMG, controles, legibilidade e idiomas adicionais aguardam editor/testes. |
| Integração | [Matriz T01–T24](../../04_TECHNICAL/Architecture/MATRIZ_TESTES_PRE_UNREAL.md), [contrato das pontes](../../04_TECHNICAL/Architecture/CONTRATO_ADAPTADORES_UNREAL_V1.md), [kit do primeiro dia](../../04_TECHNICAL/Architecture/KIT_INTEGRACAO_UNREAL_V1.md) | `SaveCore` persiste dez estados puros, mas **não** cenas, documentos, provas ou UI; o runtime do NarrativeCore ainda é Python/JSON, sem implementação Unreal. |
| Expansão futura | [Modo Extremo/Joker](../../02_GAME_DESIGN/Backlog/MODO_EXTREMO_JOKER.md) | Só após campanha Punitivo jogável, revisão, áudio/assets e teste; não ativar quinto modo agora. |

## Achados da revisão cruzada

1. **Thomas:** a verdade do mundo é morte causada por Batman. Fácil/Normal encerram sem confirmação ao jogador; Difícil/Punitivo mostram N34 ou coda. A coda não ensina Joe. DOC16/AUD08 só aparecem nos dois últimos modos. A visada real de `CAM_F4_WEST` e a prova física ainda precisam de blockout.
2. **Jack:** J1–J3 e J4 pertencem apenas ao Punitivo; J4 requer Jack fisicamente presente, pista J3 e outra testemunhada. Risada/roupa ganham força ao longo da campanha, sem virar teleporte nem posição do Hunter.
3. **Companheiros:** abandono não prova morte; estados `together`, `escaped_alone`, `captured`, `dead` e `unknown` dependem de evidências. Joe/Harley mudam o tom no Punitivo, mas a ação do jogador ainda decide ajuda e consequência. Valores numéricos não substituem encenação.
4. **Quatro fugas:** garagem exige pistas e chave; telhado depende de confirmação de rádio e prazo persistido; túnel distingue entrada de saída; janela exige ferramenta e travessia sinalizada. Falha de uma rota deixa alternativa verificável. O grafo não demonstra que um jogador achará pistas nem que uma perseguição será justa.
5. **Informação confiável:** feed ao vivo, repetição, interferência e falta de energia têm rótulos separados. Hunter e companheiros só recebem o que observaram. O futuro gás do Modo Extremo exige camada própria de percepção falsa, sem alterar agora a verdade física.
6. **Persistência é a lacuna de integração mais séria:** sem envelope narrativo versionado e restauração atômica dos atores, carregar uma partida poderia repetir cena, perder fragmento, reiniciar prazo ou inventar epílogo. O contrato de adaptadores descreve a correção, mas só a Unreal permitirá implementá-la integralmente.

## Ordem de revisão com Cristhian

1. Ler N01–N34, DOC01–16, AUD01–08 e epílogos; marcar linha por ID, sobretudo Joe/Harley, humor de Jack e prova N34/coda. Fechar D-12 após revisão, sem presumir aprovação pelo fato de o texto existir.
2. Conferir primeiro playtest de pista da garagem e quatro fugas, sem explicar solução; ajustar texto/posição do que não for legível. Aceitar números D-02/D-04/D-06/D-07 só após observar teste.
3. Revisar HUD e acessibilidade com tela real e controles; definir D-08 esconderijo por estados e limiar verificável. D-09 fica pendente até teste de UMG.
4. No notebook, instalar Unreal, clonar o Git, abrir `TheThug.uproject` e integrar o primeiro corredor seguindo o kit. Validar T01–T24 no editor; medir desempenho e duração antes de declarar “3h” ou “100% pronto”.
5. Depois da revisão geral, produzir/gravar áudio e selecionar/criar assets finais. Resolver direitos de uso público dos personagens e materiais (D-10) antes de qualquer distribuição.

**Critério de fechamento deste documento:** apontamentos do criador incorporados; contratos e testes locais/CI verdes; tarefas dependentes da Unreal e de produção identificadas por nome. Isso fecha uma **preparação**, não um jogo pronto para lançamento.
