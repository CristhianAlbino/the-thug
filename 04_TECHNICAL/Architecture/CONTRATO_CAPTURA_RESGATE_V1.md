# Captura temporária e soltura conjunta — v1

**Estado:** primeiro ciclo nativo C++17 de P02/P05/P06, integrado a CampaignCore/runtime v8 em 03/10/2026. Sustenta o [Resgate no escuro](../../02_GAME_DESIGN/Narrative/IDEIA_RESGATE_NO_ESCURO_RASCUNHO.md). O encontro ainda precisa de atores, navegação, colisão, visada, som, animação, interface e teste humano na Unreal. Interrogação e armadilhas encadeadas continuam abertas.

## Contenção e destino

Captura temporária não marca o destino narrativo `captured`. Harley, Bartholomeu e Jack podem estar em `Restraining`, `Contained`, `Released` ou `Finalized`. Thomas não participa deste resgate; Joe conserva seu fluxo próprio de derrota.

`ApplyRescue(command, context)` usa sessão/registro compartilhados e valida em cópia temporária. Falha não altera relógio, inventário, memória, serial ou destino. Mesmo ID/payload retorna `AlreadyApplied`; outro payload retorna `Conflict`. `RescueRuntime/RescueOffers` não executam nem sorteiam nada. A integração escolhe uma oferta e inicia a ação física; elegibilidade não obriga o NPC a ajudar.

| Operação | Prova necessária | Consequência |
| --- | --- | --- |
| BeginContainment | Hunter e vítima em contato no mesmo nó; identidade atualmente vista por SenseHunter; vítima ativa/viva e fora de esconderijo; Hunter sem interrupção de combate/confronto. | Restringe movimento e cancela intenções anteriores da vítima/Hunter, inclusive sua ajuda num resgate. Só uma contenção em início por Hunter. |
| FinishContainment | Mesmo serial, vítima viva, visada e contato contínuo, ação concluída e pelo menos 2 s. | Contido no local, ainda recuperável. |
| CancelContainment | Mesmo serial e perda física de contato ou interrupção ativa do Hunter; permite limpar contato após morte real. | Libera o vínculo sem inferir captura/morte, nem reverter destino já confirmado. |
| BeginRelease | Joe/outro companheiro capaz junto da vítima, acesso ao vínculo e oportunidade física; sem tarefas, esconderijo ou preparação concorrentes. | Reserva participantes, duração e eventual ferramenta. |
| BeginSelfRelease | Própria vítima capaz, oportunidade e acesso; sem tarefa concorrente, ferimento no máximo leve, medo+stress abaixo de 180. | Tenta se soltar naquele lugar. |
| CompleteRelease | Mesmo helper/serial/local, contato contínuo, oportunidade atual, ação concluída, tempo cumprido e sem alteração de saúde desde o início. | Liberta; consome a ferramenta reservada uma única vez, quando usada. |
| CancelRelease | Mesmo helper/serial; permitido após ferimento, dispersão ou término de Joe. | Limpa a tarefa e libera reserva sem consumo. Não solta a vítima por si só. |
| FinalizeCapture | Hunter em contato com vítima vista; serial da contenção, pelo menos 8 s contido e transferência/remoção física concluída. | Captura terminal; cancela resgate concorrente sem consumo do item. Não mata. |

Tempos são hipóteses de playtest: soltura manual **10 s**, ferramenta **4 s**, tentativa própria **12 s**. Neste protótipo `window_tool` é sacrificada ao romper o vínculo, retirada da bolsa de quem ajuda. A integração deve representar a perda antes do ACK; se a produção optar por desgaste/reutilização, revisar regra, identidade e testes juntos. Sem recurso, permanece a opção manual.

Soltar não move ninguém para uma saída. Depois, a pessoa usa opções habituais e suas rotas realmente conhecidas. Outra pessoa oferece ajuda com confiança ≥35, lealdade ≥30, ferimento no máximo leve e medo+stress <180. Harley pode ajudar Barth e Jack pode recusar conforme estado individual. Medo não prova traição; dificuldade não inventa estoque, conhecimento ou teleporte.
Participante já comprometido numa ação especial conjunta não aceita resgate simultâneo durante sua execução/janela. Recrutamento ainda sem execução é uma intenção; pode ajudar, mas precisa cancelar a soltura antes de começar o outro plano.

## Ponte física e interrupções

`physicallyVerified` confirma um marco real, nunca o clique no HUD. `context.actors` contém participantes percebidos pelo executor. `actualHunterNode` serve à validação física, sem virar conhecimento de Joe. `openingVerified` atesta acesso ao vínculo e oportunidade real. Sem interrupção de combate, Hunter no mesmo nó impede soltura nesta versão conservadora; cobertura/alcance dentro de um nó exigem expansão espacial.

`continuousContactVerified` representa **todo o intervalo desde o início da tarefa**, inclusive após load. Perda de contato/oportunidade exige CancelRelease/CancelContainment imediatamente. Não acumular tempo separado e depois afirmar continuidade. Se não for possível restaurar o contato físico salvo, cancelar e recomeçar. Pausa congela; o relógio sozinho não conclui nenhuma dessas operações.

Contidos/helpers não viajam, saqueiam, transferem recursos, operam dispositivos, fazem gestos ou executam outra tarefa. Atividade diferente de descanso no Tick exige cancelar antes; o início não concede deslocamento. Medicina recebida e impactos confirmados interrompem progresso. Ferir e depois tratar não recupera a tarefa antiga: marcos médicos são reconciliados com recibos, mesmo no mesmo instante. Tarefa interrompida conserva serial/reserva até cancelamento.

Impactos e projéteis já comprometidos continuam resolvíveis. Explosão respeita contatos e não desloca preso por callback. Hunter não escolhe outra viagem enquanto inicia contenção; após conter, mantém sua tática perceptiva. Vigiar caminho de reencontro, seguir distração e retornar exigem percepção/deslocamento reais. Contenção não revela posição de Joe nem memória privada de rotas.

Distração e fumaça conservam os sistemas existentes: sons só alcançam sensores com propagação/oclusão reais; fumaça não substitui paredes/raycast. Não há estímulo onisciente de resgate, cena fixa de isca, animação de amarração ou HUD de progresso pronto.

## Conhecimento, persistência e concorrência

NightRecord descreve contenção concluída como alguém imobilizado, sem confirmar destino final. Soltura própria ou por terceiro só entra se Joe testemunhou; sua própria ajuda é conhecimento da ação. Testemunho exige `joeWitnessedAction` e Joe presente. FinalizeCapture fora da sua percepção mantém paradeiro desconhecido. `KnownCompanionFates` confirma apenas captura final testemunhada; libertar não garante fuga futura. Epílogo autoral permanece separado.

Runtime v8 grava fases/local/tempos/seriais/tarefa em `RESCUE/CAPTURE/RELEASE`. Recibos `R` preservam ordem, origem, participantes, item e marcos de saúde. Loader reconstrói o ciclo e reconcilia interrupções antes da troca de sessão; rejeita fase/tempo/tarefa/serial sem prova, corpo deslocado enquanto preso, destino incompatível, item faltante ou reservas excessivas. Checksum detecta corrupção acidental, sem autenticar adulteração intencional.

Início de contenção reserva conclusão/cancelamento e transferência/libertação; tarefa conjunta reserva conclusão/cancelamento. Todas as famílias de comandos respeitam essas vagas. Retentativa antiga não atua sobre serial novo. Captura terminal existente não pode ser revertida como contenção temporária.

Runtime v1–v7 e formatos inferiores exigem nova campanha ou migração explícita, ainda não implementada. Identidade inclui revisão e tempos. CheckpointFile mantém validação integral, sincronização, backup/recovery e troca POSIX/Windows.

## Verificação

Testes nativos cobrem quatro modos/três pessoas, percepção/contato/serial ausentes, contenção cancelada, resgate manual/ferramenta, bolsa individual, pausa/save parcial, concorrência, lesão/medicina/reinício, própria soltura, ajuda/recusa, transferência terminal durante resgate, destino desconhecido, adulteração semântica com checksum recalculado e orçamento de recibos. São asserções de lógica com prova física fornecida pelo teste.

Aceite jogável exige entender risco antes da aproximação, ouvir fonte espacial coerente, poder interromper/sair, preservar alternativa, perceber recusa e restaurar contato no save. Testar distração percebida/não percebida, fumaça, falta de ferramenta, abandono e alternativa fechando. Encenação, calibração e teste humano continuam pendentes.
