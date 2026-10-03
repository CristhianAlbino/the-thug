# The Thug — combate, consumíveis e respiração v1

**Estado:** primeiro ciclo P07 em C++17, ligado a CampaignCore e ao checkpoint runtime **v5**. São regras e transações testáveis sem engine. Raycast, física, animações, efeitos visuais, atuação e balanceamento jogável exigem a ponte Unreal. Áudio/assets finais e Extremo/Joker permanecem para etapas posteriores. Este contrato não contém soluções das fugas.

## Autoridade e ciclo de uso

`ApplyCombat(CombatCommand, CombatContext)` é a entrada transacional. `atSeconds` deve coincidir com `Now()`; nó de Joe, interação física, animação e colisão são provas do adaptador. Uma solicitação de UI não é uma prova. Retentativa usa o mesmo `eventId` e payload; `AlreadyApplied` não retorna novo projétil, som ou efeito. Payload diferente com o mesmo ID retorna `Conflict`. Commands aceitos entram no mesmo registro da campanha.

| Operação | Comportamento confirmado | Prova adicional |
| --- | --- | --- |
| `BeginReload` | Reserva um `magazine`, cria tarefa/serial e prazo; não consome. | Joe possui `rifle`, pode agir e tem um carregador livre. |
| `CompleteReload` | Consome um carregador e instala 30 disparos. Carregador parcial antigo é descartado. | Serial corrente, prazo cumprido e `animationCompleted`. |
| `CancelReload` | Libera a reserva sem custo. | Serial corrente. Ferimento confirmado, entrada em esconderijo, colapso e fim também interrompem. |
| `Fire` | Consome um disparo, cria ocorrência/serial de tiro e um `SoundEvent::Gunshot`. | Muzzle/direção reais, direção normalizada, intervalo, arma e munição. Não dispara dentro do esconderijo ou durante tarefa. |
| `ResolveShot` | Confirma erro, contato no Hunter ou alteração de destrutível autorizado. | Serial de tiro ainda vivo e `collisionVerified`; raycast pertence ao mundo. |
| `BeginThrow` | Reserva granada/fumaça e cria preparo cancelável. | Item livre, Joe capaz e prazo de preparo. O item ainda não está armado. |
| `ReleaseThrow` | Consome o item e cria projétil com posição, velocidade e fusível. | Serial corrente e animação concluída. Fuzil/medicina/coleta não são fontes alternativas de granadas. |
| `CancelThrow` | Cancela apenas o preparo, sem custo ou explosão. | Serial corrente. Item já lançado não pode ser cancelado nem devolvido ao estoque. |
| `Detonate` | Troca fumaça lançada por nuvem temporária, ou aplica explosão e contatos juntos. | Fusível no instante exato, pose/nó rastreados e consulta física completa de colisão/oclusão. |
| `HoldBreath` / `ReleaseBreath` | Controla silêncio respiratório com reserva finita e recuperação. | Joe capaz, oxigênio mínimo e ausência de recuperação ofegante. Não há morte por segurar a respiração. |

O fuzil começa sem carregador instalado: os carregadores do perfil inicial estão no inventário. Essa hipótese torna o custo explícito e não acrescenta um carregador gratuito. Tutorial/apresentação deverão indicar a primeira recarga. Carregador é uma unidade selada de estoque; munição parcialmente retirada não vira um carregador cheio transferível. Arma carregada ou reservada não pode ser transferida a outro personagem. Armas próprias/disparos dos companheiros permanecem uma extensão; os inventários deles já recebem recursos.

Compartilhamento, transferência, descarte e consumo do inventário de Joe respeitam a reserva corrente. Operações rejeitadas não alteram inventário, ferimentos, mundo, RNG ou recibos. O limite de 10.000 recibos reserva espaço para concluir/cancelar tarefas e resolver granadas já lançadas; operações adicionais podem retornar `LogFull`. Isso não aumenta o orçamento global nem prova suficiência para a campanha inteira.

## Tiro, impacto e Hunter

Tiro confirmado retorna somente uma ocorrência de raycast/projétil e sua fonte sonora. Encaminhar o som uma vez a `Hear` com distância/oclusão reais. Um disparo não concede visada nem identidade ao Hunter. O `Shot` expira após dois segundos se a ponte não confirmar o impacto; expiração não causa dano. A ponte pode resolver um tiro instantaneamente, no mesmo relógio, usando o serial retornado.

Hunter não possui HP ou resultado de morte neste ciclo. Contato físico confirmado abre uma interrupção curta, cancela a tarefa tática atual e conserva percepção/memória. `HunterOffers` e confirmação de escolha/chegada/conclusão recusam avanço durante essa janela. Existe imunidade temporária a novas interrupções para evitar travamento por tiros contínuos. A ponte também precisa suspender movimento/ataques físicos durante a janela: impedir lógica não para uma animação já rodando. Captura/ataques do Hunter, resistência visual, ricochetes e retirada tática ainda exigem integração/expansão.

`WorldDefinition::destructibles` autoriza objetos opcionais por ID, nó, estados anterior/posterior e fontes aceitas. Exige archetype `destructible`; não substituir portas, circuitos, pickups ou interações obrigatórias das rotas. O hospital atual não recebe obstáculos novos automaticamente; o designer deverá autorar os alvos físicos no blockout. Impacto em objeto já quebrado resolve sem bloquear a sessão. A identidade do conteúdo inclui esses vínculos, e o loader exige prova de alteração no registro de combate.

## Granadas, física e atualização do relógio

`TrackThrown(serial, actualNode, position, velocity, physicallyVerified)` atualiza pose/velocidade sem consumir recibos. A ponte simula o trajeto e colisões; o núcleo não aplica gravidade ou decide onde uma granada caiu. A atualização final pode ocorrer **no** instante do fusível, antes de `Detonate`, e não depois dele. Pose e velocidade salvas permitem reconstruir o ator lançado sem relançar/consumir o item.

`Tick` devolve `advancedSeconds` limitado ao primeiro fusível ou fronteira fatal/respiratória. Simular o mundo físico somente por esse tempo; rastrear a pose final; resolver **cada** granada vencendo nesse instante; continuar o restante do delta. `AwaitingPhysics` com zero avanço indica uma detonação pendente: não ignorar, não avançar outro relógio e não criar explosão sem consulta física. Um save nessa fronteira preserva a pendência. Erro de prova é reversível, permitindo corrigir/rastrear/reconsultar. Se Joe morrer primeiro, o fim cancela o trabalho físico pendente da sessão.

Explosão recebe `BlastContact` somente para alvos realmente expostos, com posição/nó e nível de ferimento autorado. Raio máximo é verificado no núcleo; paredes, cobertura, alcance exato e lista completa pertencem ao solver físico. Nó informado de Joe/NPC ocultos deve coincidir com a ocupação registrada; inconsistência é recusada sem mutação. Para Joe/NPCs, aplicar ferimento/sangramento individual; ferimento já igual/mais grave não invalida o impacto. NPCs podem morrer posteriormente por sangramento sem encerrar Joe. Para Hunter/objetos, `injury=None`; Hunter recebe interrupção, objetos autorizados recebem alteração. O som de explosão usa a fonte da granada. O adaptador não deve incluir NPCs já mortos/capturados/saídos como pessoas ativas.

`SmokeOccludes(from,to)` verifica interseção do segmento com esferas de fumaça ativas; não substitui raycast de paredes, ventilação nem geometria volumétrica. Usar junto à visada física e enviar `LoseActor` quando o sensor realmente perder a pessoa. Criar fumaça não apaga automaticamente memória nem revela onde Joe se reposicionou. Padrões aprendidos de uso, dispersão por ambiente e efeitos sobre câmeras/sensores especiais permanecem extensões.

## Respiração

`Breathing()` expõe reserva, capacidade, estado e energia sonora. `BreathingSound(actualSource)` retorna evento no relógio atual, ou nenhum quando pausado/encerrado/segurando o ar. Usar essa API na campanha: chamar diretamente `MakeBreathingEvent` com o efeito do SurvivalCore ignoraria o controle voluntário. Ferimento e fadiga continuam amplificando o som quando Joe respira. A ponte escolhe cadência e propagação espacial e não cria barra explícita de medo.

O oxigênio aqui é a reserva de controle respiratório; não simula afogamento, gás ou ambientes sem ar. Segurar consome reserva, ferimento/fadiga aceleram o gasto, soltar recupera. Ao esgotar, `Tick` para na fronteira, retorna `forcedBreathRelease` e abre período ofegante audível. Tentar correr solta o ar com zero avanço nessa primeira chamada; apresentar o aviso/som e reenviar o restante do delta. Gasp e reserva mínima impedem o ciclo de pressionar/soltar para manter silêncio infinito. Não existe regeneração ou consumo durante pausa. O gasto usa o estado físico observado no início de cada intervalo; sensação, cadência e calibração no editor continuam hipóteses.

## Hipóteses numéricas

Fonte executável: `CombatProfile` em `CampaignCombat.cpp`; `CombatRuntimeRevision` participa da identidade. Modificar valores exige revisão/migração explícita.

| Parâmetro | Fácil | Normal | Difícil | Punitivo |
| --- | --- | --- | --- | --- |
| Recarga (s) | 2 | 2,5 | 2,5 | 3 |
| Duração da fumaça (s) | 24 | 20 | 18 | 15 |
| Gasto básico de oxigênio por segundo | 5 | 6 | 7 | 8 |

Comum aos modos: 30 disparos por carregador; intervalo 0,12 s; preparo 0,5 s; fusível granada 3 s/fumaça 1 s; raio explosão 5 m/fumaça 4 m; interrupção tiro 0,6 s/explosão 1,5 s; imunidade adicional 4 s; oxigênio 100, limiar de novo controle 50, recuperação 10/s, período ofegante 3 s. Estes números não são tempos/alcances medidos em uma build.

## Persistência e teste

Runtime v5 introduziu os campos de combate; runtime v9 mantém StorySave v1 e acrescenta disparos carregados, sequência de comandos/criações, reserva/tarefa, tiros pendentes, granadas/pose/velocidade/fusível, nuvens, interrupção/imunidade e oxigênio/controle/gasp. O loader valida limites, origem/ordem/seriais, consumo lógico, munição reconstruída, ocorrências de impacto, destruição, tempos e consistência com fim da campanha antes de trocar a sessão. Runtime v1–v8 exige nova campanha ou migração específica; não descartar campos silenciosamente.

Compilar `CampaignCombat.cpp` e `CampaignCombatSave.cpp` com os demais arquivos de CampaignCore. `combat_tests.cpp` e CMake `runtime_combat` cobrem quatro modos, reserva disputada, retentativa, recarga interrompida, limite de munição, provas rejeitadas, impactos, imunidade, fumaça, ferimentos individuais, fusíveis simultâneos, save em fases distintas, fronteira fatal, oxigênio, corrupção/incompatibilidade e limite de recibos. Esses ensaios não são partidas humanas, física Unreal, medição de desempenho ou teste de três horas.

**Atualização v9:** pilhas/informação/interrogação/usos conjuntos, armas dos NPCs e planos observados usam o [contrato complementar](CONTRATO_RECURSOS_INFORMACAO_PLANOS_V1.md). Os limites físicos deste documento permanecem.
