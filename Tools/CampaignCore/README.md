# The Thug — coordenador nativo da campanha

`CampaignCore` reúne inventário, sobrevivência, três companheiros, Hunter, percepção, evidências, regras de fuga, mundo e registro narrativo. É C++17 independente da Unreal, com um único dono mutável e relógio de simulação. As consultas devolvem cópias; a ponte não deve alterar núcleos internos por fora do coordenador.

## Entrega desta etapa

- Relógio comum, pausa, fechamento de partida e interrupção exata de atualização no instante de morte por sangramento.
- Inicialização pelas capacidades, traços, equipamentos e metas temporais autorados nas quatro dificuldades.
- Coleta e devolução de recurso ao ponto de origem; tratamento/estabilização de Joe; ferramentas reutilizáveis e consumo de recurso em interação; portas, circuitos, ocupação e transferência de munição.
- Transação em cópia temporária: requisito ausente, inventário cheio ou efeito recusado preservam todos os estados e registros.
- ID de ocorrência persistente: repetir a mesma ação retorna `AlreadyApplied`, sem custo nem sinal novo; reutilizar o ID para outra ação retorna `Conflict`.
- Envelope de checkpoint que conserva pausa, ocupação de Joe e recibos de ações, além do StorySave completo. Carregamento rejeitado preserva a campanha viva.
- Diretor narrativo C++ ligado à sessão: sequência de falas/gestos, confirmação por entrada, prioridade/suspensão, diário de materiais inspecionados e epílogos condicionais.
- Entrada nativa de teclado, com tentativa real e abertura conjunta de mundo/regra; conhecimento prévio não inventa documentos lidos.
- Inventários e saúde individuais; auxílio e transferência de qualquer recurso do catálogo; seis objetivos com escolha, chegada, conclusão, reserva/cancelamento e retomada.
- Hunter com alvos individuais percebidos, agressividade por eventos testemunhados, busca por setores, passagem observada e ações físicas sobre dispositivos, com oportunidade de reação.
- Escrita/leitura de arquivo com temporário validado, sincronização, backup, substituição, recuperação explícita e bloqueio de slot.

Isso acrescenta o primeiro ciclo tático P05, entrega P01, o núcleo P03, o escritor P04 e amplia P02/P06 com recursos individuais e a primeira execução dos seis objetivos. Cooperação simultânea, medo/estresse temporal, mentiras/manipulação e planos mais ricos continuam abertos. Encenação final P09, política/validação humana P10 e restauração física no editor continuam pendentes. Ver [DirectorCore](../DirectorCore/README.md) e [contrato de arquivo](../../04_TECHNICAL/Architecture/CONTRATO_ARQUIVO_CHECKPOINT_V1.md).

## Dados de autoria

`build_data.py` exporta `CampaignData.generated.h` a partir de:

| Fonte | Uso nativo |
| --- | --- |
| `../NarrativeCore/balance_initial.json` | Capacidade, seis traços iniciais por companheiro, equipamento inicial e meta de amanhecer por modo. |
| `companion_runtime.json` | Inventários por pessoa/modo, influência médica e tempos de tarefa; exportação por `build_companion_data.py`. |
| `campaign_initial.json` | Catálogo/custos de itens, pontos de coleta, pré-requisitos e vínculos de interação. |
| `../MapCore/hospital_graph.json` | Nós lógicos, portas, circuitos, câmeras e esconderijos. |

Regenerar com `python3 Tools/CampaignCore/build_data.py`; verificar sem modificar com `python3 Tools/CampaignCore/build_data.py --check`. O CI recusa header desatualizado. Não editar o header gerado. Os números e a distribuição inicial são **hipóteses de playtest**, não equilíbrio aprovado. O loadout do Difícil conserva o GDD. Tuning de sobrevivência usa a base já existente ou a configuração explícita do construtor.

O hospital gerado é **lógico**: posições e rotações são placeholders zero. Uma `WorldDefinition` alternativa permite ao adaptador fornecer transforms reais; não existe geração de geometria, trajetória ou prova de alcance. Uma mudança nos dados/transforms/tuning altera a identidade do checkpoint; requer nova campanha ou migração explícita.

## Uso e autoridade do adaptador

```cpp
thug::campaign::CampaignCore campaign(thug::rules::Difficulty::Hard, 42);
campaign.Tick(deltaSeconds, thug::survival::Activity::Walk);
// Depois de conferir interação e proximidade reais no mundo:
thug::campaign::Action action{
    occurrenceId, thug::campaign::ActionKind::Treat, "treatment",
    actualNode, campaign.Now(), 0, interactionVerified};
const auto result = campaign.Apply(action);
// Atualizar apresentação/atores somente a partir do resultado confirmado.
```

`Action::atSeconds` deve ser **exatamente** `Now()`. Avançar até o instante do evento antes de enviar a ação; usar `TickResult::advancedSeconds` quando uma atualização termina antes do delta pedido. Pausa e fim impedem novas consequências. Retentar uma ação já confirmada devolve seu recibo também nesses estados.

`physicallyVerified` é uma declaração confiada ao adaptador, não uma checagem de colisão. Para coleta, conferir presença, alcance e ação concluída; para auxílio, presença/proximidade real do companheiro; para saída ou captura, cruzamento/contato real. `SubmitKeypad` recebe os seis caracteres digitados no dispositivo alcançado e valida a combinação no núcleo; `Use("safe_code_accepted")` não é uma entrada pública permitida. Visitar um nó ou clicar na UI não basta. O coordenador verifica vínculos e requisitos lógicos; a ponte verifica o mundo físico. Consultar o [contrato de coordenação](../../04_TECHNICAL/Architecture/CONTRATO_COORDENADOR_CAMPANHA_V1.md).

Um `eventId` identifica uma ocorrência, não um botão: a retentativa reutiliza o ID e payload; a próxima ocorrência usa outro ID. A operação `Drop` atual devolve a quantidade autorada ao seu ponto de coleta, desde que vazio. Soltar pilhas em qualquer lugar, dividir quantidades e criar novos atores de pickup ainda exige extensão de autoria/persistência.

Portas e circuitos podem devolver `WorldSignal`. A ponte emite som uma vez e o encaminha a `Hear` com distância/oclusão reais. Nenhuma ação informa automaticamente a posição de Joe ao Hunter. `Observe` recebe percepção visual já verificada; `InspectEvidence` exige objeto presente, estado visível atual e visada confirmada. Amostras de sensores não usam recibos de transação; evitar emitir duas vezes a mesma amostra física.

## Execução dos companheiros

`CompanionOffers` consulta alvos conhecidos/alcançáveis; `ApplyCompanion` confirma observação, escolha, chegada e conclusão. A tarefa persiste durante viagem e interação, com serial para rejeitar callbacks antigos. Recursos ficam reservados até o commit e falhas não os consomem. `CompanionRuntime` expõe inventário/saúde/tarefa por pessoa; `Tick` aceita atividades individuais. Transferência, medicina e dispositivos usam o inventário de quem executa. Ver o [contrato de execução](../../04_TECHNICAL/Architecture/CONTRATO_EXECUCAO_COMPANHEIROS_V1.md) para campos, provas e limites.

## Execução tática do Hunter

`SenseHunter` recebe contato/perda visual individual frequente sem consumir recibos; `ApplyHunter` registra pressão testemunhada, passagem, escolha/chegada/conclusão/cancelamento. `HunterOffers` consulta somente memória/percepção e nós com navegação possível informados pela ponte. `HunterRuntime` expõe cópia do estado. Conhecer o mapa estático não autoriza consultar a posição escondida de Joe. O [contrato do Hunter](../../04_TECHNICAL/Architecture/CONTRATO_HUNTER_TATICO_V1.md) define provas, hipóteses por modo, janela de reação e limites. Interrogação, combate/captura física e armadilhas encadeadas permanecem para expansões/integração.

## Combate e respiração

`ApplyCombat` coordena recarga, disparo/impacto, preparo/arremesso/detonação, fumaça e controle voluntário da respiração. Reservas impedem concorrência com transferência/descarte; consumo ocorre somente no marco confirmado. `CombatRuntime` expõe tarefas, munição carregada, tiros, projéteis/poses, nuvens e interrupção temporária do Hunter; não existe HP de Batman. `TrackThrown` recebe física real; `SmokeOccludes` auxilia a visada sem substituir paredes/raycast.

`Tick` pode parar no primeiro fusível e devolver `AwaitingPhysics` até cada detonação ser confirmada. Respeitar `advancedSeconds`, concluir a física desse intervalo e continuar o delta restante. Usar `BreathingSound` na sessão para preservar silêncio voluntário, ferimentos e retomada ofegante; `forcedBreathRelease` comunica a fronteira. O [contrato de combate/respiração](../../04_TECHNICAL/Architecture/CONTRATO_COMBATE_RESPIRACAO_V1.md) define campos, provas, hipóteses e limites. Armas dos NPCs, física/animação, captura e aprendizado avançado de fumaça continuam pendentes.

## Checkpoint e limites

`Save(std::ostream&)` / `Load(std::istream&)` usam `THE_THUG_RUNTIME_CAMPAIGN 6`: StorySave v1 aninhado, identidade de conteúdo, pausa, ocupação de Joe, planos/seriais de apresentação, corpos/recursos/tarefas individuais, memória/agressividade/tática/RNG do Hunter, armas/tiros/projéteis/fumaça/oxigênio, fases/instalações/encontros especiais e até 10.000 recibos. Limite de 4 MiB; checksum FNV-1a 64 identifica corrupção acidental, sem proteção contra adulteração intencional. Falha de formato, versão, configuração ou coerência rejeita o arquivo inteiro. Runtime v1/v2/v3/v4/v5 e formatos inferiores não são convertidos silenciosamente. RNG prossegue do estado salvo; identidade inclui autoria narrativa e não depende da semente do construtor.

Para disco, usar `checkpoint::SaveFile/LoadFile` de `CheckpointFile.h`; o stream sozinho não oferece garantia de arquivo. Recuperação retorna `RecoveredBackup`, indicando progresso anterior. Uma falha posterior à substituição retorna `CommittedUnconfirmed`. Restauração de atores/animações e integração da política de checkpoint ficam para a engine. Não usar serializadores inferiores como checkpoint completo da sessão.

`ApplyNarrative` usa prova/contexto físico e o mesmo registro de ocorrências para cenas, conversas, leitura e fatos testemunhados. `CurrentPresentation` fornece a entrada a apresentar; a ponte confirma seu serial/índice/ID depois de concluir. `JournalText` expõe somente material inspecionado e `EpilogueText` usa evidências individuais da campanha encerrada. Condições temporárias não viram conhecimento. O núcleo não decide visada, não produz voz/animação e não confirma uma ação só por estar elegível. N33 deriva dos resultados especiais confirmados; falas dependem de sobreviventes presentes. `ApplySpecial` cobre preparação, custos, resolução física e encontros pontuais; veja o [contrato P08](../../04_TECHNICAL/Architecture/CONTRATO_FINAIS_PARTICIPACOES_V1.md). Robin não é uma segunda IA pronta.

Cada ação/atualização/sensor aceito prepara uma cópia dos estados. A correção transacional está ensaiada, mas custo com campanha longa e frequência de sensores precisa de benchmark antes da integração definitiva. Uso em uma thread de simulação; sem promessa de 60 Hz ou duração de campanha comprovada.

## Verificação

```sh
bash Tools/CampaignCore/run-tests.sh
cmake -S Tools/CampaignCore -B Build/CampaignCore
cmake --build Build/CampaignCore
ctest --test-dir Build/CampaignCore --output-on-failure
```

A suíte `special_tests` cobre finais raros/encontros, recursos, prova física, consequências individuais, retomada e orçamento reservado. A suíte `combat_tests` cobre quatro modos, recarga/itens reservados, disparos, impactos, interrupção sem travamento do Hunter, fumaça/explosões, fronteiras físicas, ferimentos, retomada, respiração e limite de recibos. A suíte `hunter_tests` cobre quatro modos, seleção de indivíduos, memória sem identidade mágica, agressividade, setores vazios, emboscada/ambiente, aviso/reação, retomada, RNG e sensores frequentes sem esgotar recibos. A suíte `companion_tests` acrescenta recursos/saúde individuais, os seis objetivos, reservas, interrupções, retomada, destinos e ausência de informação mágica. O script verifica geração e compila com C++17, `-Wall -Wextra -Wpedantic -Werror`. Os cenários cobrem pausa/carregamento nos quatro modos, conservação de recursos, falhas sem mutação, repetição após load, portas/energia/percepção, resultados convencionais, limite fatal, retomada de falas, fatos restritos e diário. O escritor é ensaiado com corrupção, erro/cancelamento, Unicode, bloqueio e processos encerrados durante a escrita. CMake inclui paridade narrativa e o CI cobre Linux e Windows/MSVC. São testes de regras e arquivos, não sessões jogáveis nem ensaio de perda elétrica.
