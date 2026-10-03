# Execução nativa dos companheiros — v1

**Estado:** primeira execução dos seis objetivos implementada em CampaignCore, sem dependência da Unreal. Este contrato não revela soluções das rotas. Inventários, tempos de interação e deltas médicos são hipóteses de playtest em `Tools/CampaignCore/companion_runtime.json`. Thomas permanece na abertura narrativa; os corpos sistêmicos são Harley, Bartholomeu e Jack.

## Autoridade e percepção

A sessão possui o inventário, a sobrevivência, o conhecimento e a tarefa de cada companheiro. Consultar `CompanionRuntime(Character)`; não escrever diretamente nos núcleos inferiores. `Companion(Character)` continua mostrando traços/rotas/RNG, e sua munição/ferimento devem concordar com o corpo individual.

`ApplyCompanion(command, context)` usa o mesmo relógio, pausa e registro de ocorrências das outras APIs. `eventId` identifica a ocorrência física; repetição retorna `AlreadyApplied` sem novos custos ou sons, inclusive após load. Payload diferente com o mesmo ID retorna `Conflict`. O tempo precisa ser exatamente `Now()`. Uma rejeição preserva os estados, inclusive RNG.

A ponte fornece `physicallyVerified` somente depois da verificação correspondente. `CompanionContext.actors` contém o próprio ator e apenas pessoas efetivamente percebidas por ele, com nó e disponibilidade atuais. `reachableNodes` contém destinos com caminho fisicamente verificado. Uma consulta de opções não permite conhecer o inventário de outra pessoa, um pickup remoto desaparecido ou a posição global de Joe/Hunter. A memória de um recurso pode ficar desatualizada: a presença é conferida ao observar e ao concluir a coleta. Quem conclui a coleta já retira esse alvo da própria lista de recursos disponíveis. Depois de cancelar uma busca falha, `ObserveMissingPickup` permite registrar a ausência fisicamente conferida no local autorado. A memória só muda com essa prova; um recurso devolvido pode ser observado novamente.

O nó é uma âncora lógica, não posição contínua. A ponte continua responsável por visada, distância dentro da sala, colisão, postura, navegação, bloqueio de caminho, animação e comunicação realmente ouvida. `lastNode` registra a última ação confirmada; não fornece um sistema de localização dos NPCs para o jogador.

## Ciclo de uma tarefa

| Etapa | Entrada | Consequência confirmada |
| --- | --- | --- |
| Observar | `ObservePickup`, `ObserveHide`, `ObserveRoute` com prova no local autorado | Memória individual; não concede documentos lidos nem conhecimento aos colegas. |
| Consultar | `CompanionOffers(actor, actualNode, context)` | Objetivos executáveis com alvos/destinos e pesos; não muda estado nem RNG. |
| Escolher | `ChooseTask`, alvo `auto`, ou `BeginTask`, alvo oferecido e `argument = Goal` | Tarefa persistente com serial novo, ocorrência de origem, fase `Travelling` e reserva. Escolha automática usa a política e RNG próprios. |
| Chegar | `Arrive`, alvo/serial da tarefa, nó do destino e `movementCompleted` | Fase `Ready` e instante inicial da interação. Não conclui a ação. |
| Concluir | `Complete`, alvo/serial, `argument = Goal`, nó do destino e `actionCompleted` | Após o tempo mínimo, revalida requisitos e aplica os efeitos juntos. |
| Interromper | `Cancel` com alvo/serial e localização atual verificada | Libera a reserva, conserva recursos e não aplica influência. Ferimento, colapso, resultado individual ou fim da campanha também interrompem. |

O serial precisa ser o da tarefa atual. Um callback de tarefa cancelada não conclui uma tarefa nova. A tarefa não termina automaticamente com `Tick`, chegada ao nó, fim de um timer ou load. A ponte deve renovar a prova física depois da retomada.

Durante a tarefa, o inventário do executor fica indisponível para outras transferências/operações. Outro ator também não pode transferir itens para esse inventário. Coleta e esconderijo reservam um alvo contra tarefas concorrentes e ações de Joe. Os itens permanecem no mundo/no inventário até o commit final. Falha na conclusão conserva a tarefa em `Ready`: a ponte cancela, corrige o contexto ou retenta com a ocorrência apropriada. Cura recebida de outro ator permanece possível.

## Efeitos dos objetivos

| Objetivo | Efeito na conclusão |
| --- | --- |
| Esconder-se | Registra o dono e ocupa um esconderijo conhecido, livre e alcançado. Perigo sozinho não impede a entrada fisicamente validada. |
| Procurar recurso | Adiciona ao inventário próprio, retira o pickup e confirma preparação de mundo prevista na autoria. Capacidade/requisitos são revalidados. |
| Ajudar Joe | Consome tratamento do companheiro e trata Joe, somente com Joe presente/alcançável e ação concluída. |
| Compartilhar rota | Comunica a rota conhecida a Joe; registra `JoeKnownRoutes()` e descoberta de mundo aplicável. Não inventa inspeções, pistas de teclado ou leitura de documentos. |
| Fugir sozinho | Exige conhecimento, caminho, rota pronta e travessia concluída. Registra apenas `escapedAlone` daquele NPC; não encerra a campanha de Joe. Indisponível no Fácil. |
| Usar Joe como distração | Revalida condições percebidas e traços; gera `CompanionSignal` na fonte real do executor. Prova testemunhada por Joe pode registrar a ação concreta para o epílogo. Disponível apenas nos dois modos superiores sob os requisitos da política. |
| Seguir sinal | Ordem percebida/aceita ainda válida, viagem concluída e Joe presente no destino. Não transfere inventário nem conhecimento. |
| Investigar área sinalizada | Ordem percebida/aceita ainda válida, viagem e inspeção física concluídas. Não encontra recurso ou rota automaticamente. |

O runtime v7 amplia os seis objetivos originais com estes dois planos e acrescenta silêncio, gestos e pressão temporal. Ver [contrato das interações](CONTRATO_INTERACOES_REGISTRO_V1.md) para autoria, prazos, provas e registro pessoal.

Sinais de distração, porta e circuito não chamam `Hear` automaticamente. A ponte emite uma vez e envia à percepção com fonte, distância e oclusão reais. Uma intenção de traição não equivale a revelar, capturar ou matar Joe.

## Operações individuais

`Transfer` move qualquer item do catálogo entre Joe e companheiros ou entre dois companheiros, depois de conferir presença, quantidade e capacidade. A munição dos traços acompanha o inventário. Entrega de munição por Joe aplica confiança/lealdade uma vez por ocorrência; `ActionKind::ShareAmmo` preserva a operação anterior de uma unidade e agora também preenche o inventário individual.

`Treat` e `Stabilize` consomem medicina do executor para o alvo presente. Tratamento melhora um nível e pode recuperar sangue; estabilização interrompe sangramento sem apagar a gravidade. Não ressuscitam mortos. Joe pode ajudar um companheiro colapsado; o ferido não consegue executar sozinho uma ação enquanto incapacitado. Influência médica por pessoa é hipótese autorada, não um terceiro delta aprovado pelo GDD.

`Injury` recebe a gravidade física confirmada; sincroniza os traços e cancela a tarefa. `Tick(seconds, joeActivity, companionActivities)` avança saúde/stamina independentes. Omitir uma atividade significa repouso; a ponte deve informar movimento real, inclusive durante viagem. Sangramento de um NPC pode causar colapso/morte sem encerrar Joe. Morte é limitada ao instante fatal daquele corpo; uma atualização longa não gera sangue além desse limite. A morte de Joe interrompe o delta compartilhado conforme o contrato anterior.

`Operate` executa uso, destrancamento, abertura/fechamento de porta ou circuito com os recursos do companheiro e os vínculos de mundo. O teclado permanece na API específica de Joe; não existe bypass por operação genérica.

`Abandon` representa abandono real observado, não uma falha de navegação ou ausência genérica. Aplica os dois efeitos pertinentes do GDD uma vez; não deduz captura/morte. `Outcome` registra captura, morte física ou fuga junto com Joe. Fuga conjunta exige Joe já confirmado como fugitivo e presença física no mesmo nó. Destino sem prova continua desconhecido. Esses resultados não são inferidos de um objetivo escolhido, conversa ou fim da campanha.

O marcador de morte física usa o estado terminal fatal do SurvivalCore atual; causas específicas de morte ainda não têm um modelo próprio. A ponte não deve produzir sangue/dano visual a partir desse marcador abstrato.

Uma morte dentro de esconderijo conserva a ocupação pelo corpo. Captura/extração confirmada pode liberá-lo. Remoção de cadáver e drops de inventário ainda exigem extensão física/persistente; não apagar a ocupação ao carregar.

## Retomada e verificação

Runtime v5 acrescenta os três inventários, saúde/stamina, memórias, ocupação, serial/tarefa/fase/instantes, motivo de interrupção e rotas comunicadas a Joe. O loader valida catálogo/capacidade, saúde contra resultado e ocorrências de captura/fuga/morte, munição/ferimento contra traços, memórias contra ocorrências, reservas únicas, dono de esconderijo e tarefa contra escolha/chegada registradas. Um estado impossível não muda a sessão viva.

Runtime v1/v2 e serializadores inferiores não são checkpoints completos desta sessão. Não há migração automática. Identidade inclui esta autoria e tuning; um ajuste pode requerer nova campanha ou migração explícita. Usar o escritor de arquivo existente para durabilidade, backup e recuperação.

`companion_tests.cpp` cobre os seis objetivos, quatro modos, conservação/capacidade, provas ausentes, escolha reproduzível após load, disputa, cancelamento e callbacks antigos, viagem/interação retomadas, medicina, colapso/morte, ocupação, resultados e operação de dispositivo. Não comprova navmesh, encenação, sensação de três horas ou balanceamento final.

## Próximas extensões

Medo/estresse com evolução temporal, mentiras/manipulação e objetivos adicionais, cooperação simultânea em obstáculos, planos com múltiplas etapas, inventário derrubado/corpos, restauração física e atuação continuam abertos. As tarefas atuais não fornecem Animation Blueprint, Behavior Tree, navmesh nem personagens prontos para importar.

O runtime v8 acrescenta [captura temporária e soltura conjunta/própria](CONTRATO_CAPTURA_RESGATE_V1.md). Contidos/helpers não executam objetivos habituais até libertação/cancelamento. Recursos continuam individuais; soltar não concede rota ou deslocamento.
