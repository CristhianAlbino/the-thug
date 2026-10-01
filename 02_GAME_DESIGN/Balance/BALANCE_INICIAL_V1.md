# The Thug — hipóteses iniciais de balanceamento v1

**Estado:** números de autoria para o primeiro playtest, **não aprovados como equilíbrio final**. Os núcleos C++ usam escala 0–100 para traços; os valores abaixo passam a ser dados sugeridos em [`balance_initial.json`](../../Tools/NarrativeCore/balance_initial.json). Mudanças após Unreal devem registrar dificuldade, cenário, resultado observado e versão de save. O GDD é fonte das metas 30/90/180 minutos e loadout do Difícil; restante é hipótese explicitamente rotulada.

## Traços iniciais dos três companheiros

| Dificuldade / pessoa | Confiança | Medo | Estresse | Lealdade | Autopreservação | Relação com Joe |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Fácil · Harley | 85 | 15 | 15 | 85 | 45 | 85 |
| Fácil · Bartholomeu | 80 | 35 | 25 | 75 | 45 | 40 |
| Fácil · Jack | 70 | 20 | 20 | 70 | 55 | 30 |
| Normal · Harley | 75 | 25 | 25 | 75 | 50 | 85 |
| Normal · Bartholomeu | 65 | 50 | 40 | 65 | 55 | 35 |
| Normal · Jack | 55 | 30 | 30 | 50 | 65 | 25 |
| Difícil · Harley | 65 | 35 | 40 | 70 | 55 | 85 |
| Difícil · Bartholomeu | 55 | 60 | 45 | 55 | 65 | 35 |
| Difícil · Jack | 40 | 35 | 30 | 35 | 80 | 25 |
| Punitivo · Harley | 65 | 45 | 50 | 70 | 60 | 90 |
| Punitivo · Bartholomeu | 45 | 75 | 60 | 45 | 80 | 30 |
| Punitivo · Jack | 35 | 20 | 25 | 25 | 85 | 20 |

**Leitura:** a reciprocidade de **Joe** no Punitivo é texto/ação, não aumento automático de confiança de Harley; 90 em relação representa vínculo inicial, não perdão de abandono. Jack Punitivo pouco amedrontado é pista de atuação, não imunidade ao Hunter. Bartholomeu começa cooperativo no Fácil e pode perder confiança por ato real nos demais. `JoeSharedAmmo` aplica +20 confiança/+10 lealdade e transfere uma unidade pelo IntegrationCore; `JoeAbandonedCompanion` aplica −40 confiança/+20 medo/+30 autopreservação **uma vez por ocorrência real**, como no GDD. Não inventar deltas adicionais sem cena/IDs.

**Teste dirigido:** após abandono, Bartholomeu no Difícil cai de 55 para 15 de confiança, sobe de 60 para 80 de medo e de 65 para 95 de autopreservação; `UseJoeAsDistraction` ainda exige lealdade <50, Hunter percebido e Joe presente. Ele inicia em 55, então só abandono não basta para traição imediata. Isso preserva uma história causal. Depois de dividir munição, confiança vai a 75 e lealdade a 65, tornando a ajuda plausível. Peso aleatório ainda exige ensaio por sementes e playtest.

## Inventário físico e recursos

| Dificuldade | Capacidade proposta | Oferta relativa | Ponto de partida |
| --- | ---: | --- | --- |
| Fácil | 16 slots | Abundante | Ferramentas e tratamento comuns, sem porta final gratuita. |
| Normal | 14 slots | Moderada | Algumas escolhas de carregar ou voltar. |
| Difícil | 12 slots | Limitada | Loadout GDD: fuzil, 2 carregadores, 1 granada, 2 fumaças; custos provisórios 3+2+1+2 = **8 slots**, deixando 4. |
| Punitivo | 10 slots | Muito escassa | Equipamento inicial a definir em teste; não copiar a abundância do futuro Modo Extremo. |

**Custos sugeridos:** fuzil 3, carregador 1, granada 1, fumaça 1, tratamento 2, bateria/pilhas 1, ferramenta da janela 2, chave da garagem 1. Documentos e códigos são consultáveis no diário após inspeção física; não são `ItemDefinition` com custo zero, que o InventoryCore atual rejeita. Objetos de missão têm ID persistente e origem, e nunca podem ser duplicados por save/load. O jogador pode deixar recurso não essencial e retornar; a chave/ferramenta de rota não deve desaparecer ao cair por falta de espaço. Capacidade e catálogo entram no save, por isso alterar ambos exige migração ou nova campanha.

**Sobrevivência:** manter inicialmente os valores já codificados em `SurvivalCore::Tuning` como hipóteses comuns (100 stamina, sprint 18/s, recuperação parado 12/s, sangramento 0,35/s, tratamento 20 unidades de perda sanguínea). Não declarar ferida “leve/moderada/grave” por porcentagem inventada: `ApplyInjury` recebe severidade autorada do evento físico. Avaliar distância de sprint/ruído e frequência de tratamento no editor antes de endurecer números por modo.

## Relógio, saves e esconderijo

| Sistema | Hipótese para primeiro teste | O que pode invalidá-la |
| --- | --- | --- |
| Amanhecer | Fácil 30 min, Normal 90 min, Difícil 180 min de tempo de jogo conforme GDD; evento `DawnReached` só após relógio/ambiente consistentes. Punitivo não vence por amanhecer. | Geometria, exploração e rotas podem permitir fugir antes; duração não é obrigação de segurar o jogador até o marco. |
| Checkpoint | Autosave após N03 e marco irreversível com mundo/atores/narrativa em estado coerente. Política de manual/autosave por modo e arquivo temporário no [contrato de checkpoint](../../04_TECHNICAL/Architecture/CONTRATO_CHECKPOINT_ESCONDERIJO_V1.md). | Frequência real e custo de replay precisam de playtest; salvar antes de cada risco pode anular tensão. |
| Carregar save | Restaurar Hunter, companheiros, inventário, relógio de helicóptero, portas/câmeras/circuitos e flags narrativas de modo atômico. A mesma semente/estado dá continuação reprodutível. | Adaptação Unreal ainda não salva cenas/áudios em andamento; política final aguarda editor. |
| Vitória escondido | Hunter abandonou a busca, saiu fisicamente da área e não recebeu estímulo novo; Joe permanece em esconderijo válido durante janela estável proposta de 20/30/45/60 s por modo **depois** da saída verificada. Ver [sequência de aceitação](../../04_TECHNICAL/Architecture/CONTRATO_CHECKPOINT_ESCONDERIJO_V1.md). | `HideSpotSuspicion`, geometria e navegação reais podem exigir revisão; limiares D-08 pendentes de playtest. |

## Critérios antes de fixar números

1. Por dificuldade e semente, registrar tempo até primeira pista, primeiro encontro, primeira rota preparada e saída/derrota.
2. Verificar que jogador atento consegue ao menos uma opção de recuperação após falha de fuga; o Hunter não recebe posição por gatilho narrativo.
3. Medir para cada companheiro quantas decisões reais de ajudar/partir ocorreram e se epílogo distingue `captured`, `dead` e `unknown`.
4. Verificar que o código da garagem pode ser descoberto sem adivinhação e que o prazo do telhado comporta travessia sob perseguição plausível.
5. Ajustar **uma família de variáveis por vez** e repetir sementes comparáveis. Três horas no Difícil são alvo de experiência, não promessa comprovada pelo grafo.
