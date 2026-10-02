# The Thug — primeiro ciclo tático do Hunter

**Estado:** C++17 ligado a CampaignCore, independente da engine. Hunter continua sendo Batman na visão dos bandidos. Este ciclo implementa decisão/percepção/memória e transações de ambiente; não produz navegação, animação, combate ou um inimigo jogável sozinho. Interrogação, armadilhas encadeadas, retirada tática e captura física continuam abertos. Áudio/assets finais e Extremo/Joker permanecem posteriores.

## Autoridade e percepção

`SenseHunter(HunterCommand, HunterContext)` recebe somente `SeeActor`/`LoseActor` no relógio atual. A ponte comprova posição real do Hunter e contato/oclusão; `SeeActor` inclui **posição/nó observados** da pessoa, raio de incerteza e ameaça visível. `LoseActor` não inclui nova posição: conserva o último ponto visto. A atualização frequente usa memória limitada a quatro pessoas, sem consumir os 10.000 recibos. Não enviar a posição escondida ou a intenção privada de Joe para corrigir a perseguição.

`ApplyHunter` usa IDs de ocorrência e o registro compartilhado de transações. Repetir payload/ID confirmado retorna `AlreadyApplied`; outro payload no mesmo ID retorna `Conflict`. Pausa/fim/requisitos ausentes não alteram RNG, memória ou mundo. Atualizações aceitas trabalham em cópia temporária e só então trocam a sessão. `HunterOffers`/`HunterRuntime` são consultas por valor.

`Hear` continua propagando som com distância/atenuação/incerteza e `Subject::Unknown`; não identifica quem produziu o ruído. `InspectEvidence` estabelece baseline de estado visto, depois distingue alterações realmente inspecionadas. A ponte visual deve usar `SenseHunter` para identidade individual; `Observe` inferior continua disponível para estímulos sem identidade individual, incluindo uso de esconderijo confirmado. IDs de esconderijo inexistentes são recusados na sessão.

## Comandos e provas

| Comando | Prova obrigatória / efeito |
| --- | --- |
| `SeeActor` | Pessoa viva/ativa identificada com visada; nó/estimativa observados. Joe, Harley, Bartholomeu e Jack têm memórias separadas. Atualização contínua no mesmo destino conserva tarefa; mudança do destino perseguido ou nova ameaça percebida pode cancelar intenção antiga. |
| `LoseActor` | Perda real do contato informado pela ponte; não localizar novamente a pessoa nem adicionar suspeita de outro esconderijo. |
| `Pressure` | Ocorrência distinta testemunhada. Tiro/explosão/destruição exigem visada; atribuição a uma pessoa exige contato recente. `ConfirmedEscape` exige memória recente e perda visual, além da confirmação física da ponte de evasão concluída, não somente um frame sem visada. |
| `NoticePassage` | Pessoa atualmente vista em uma das extremidades da porta autorada e Hunter observando a passagem nessa região. Não aprender a rota porque Joe a escolheu em menu. |
| `Choose` | Nó real do Hunter e conjunto de nós com caminho físico possível. Escolha ponderada guarda plano/serial/origem e avança somente o RNG próprio. Não move nenhum ator. |
| `Arrive` | Serial/alvo atuais, nó de destino efetivamente alcançado e navegação verificada. Para emboscada, inspeção de esconderijo ou dispositivo, também `reactionCueVerified`: aviso/gesto/sinal realmente apresentado. A janela começa aqui. |
| `Complete` | Mesma tarefa, permanência física no destino e tempo exigido. Manipulação exige visada e estado do dispositivo ainda igual ao que foi visto; fechamento/corte de energia também exigem alternativa de circulação novamente verificada. |
| `Cancel` | Serial/alvo atuais. Libera a intenção sem aplicar consequência de ambiente ou captura. Callback de serial antigo não pode concluir tarefa nova. |

`HunterContext::actualHunterNode` descreve o Hunter, não Joe. `reachableNodes` vem da navegação física. `searchSectors` mapeia a região **estimada** para centros de setores; centros não finitos/fora do raio e IDs não autorados são excluídos. `safeToAlter` contém os dispositivos cuja mudança preserva circulação alternativa já conferida. Nenhum booleano aqui faz raycast/navmesh: eles são declarações confiadas à ponte, a serem ensaiadas no editor.

## Escolha, erro e reação

Alvos individuais atualmente vistos e alcançáveis têm precedência. Joe tem peso base 3, companheiros 2 e ameaça visível soma 6; portanto um companheiro pode ser priorizado, mas nenhum peso garante a escolha. Contato sem atualização expira depois de dois segundos de simulação; perda explícita pode ocorrer antes. Expiração durante delta longo inicia a busca no instante da expiração e consome o tempo restante, em vez de reiniciar uma busca artificial no fim do frame.

Sem alvo confirmado alcançável, o Hunter pode investigar setores de uma estimativa, retornar à última região individual vista, inspecionar um esconderijo de uso observado, vigiar uma passagem observada, alterar um dispositivo que viu ou patrulhar. Áreas/esconderijos inspecionados aguardam 30 segundos antes de nova inspeção. Agressividade estende a validade da última região individual/passagem até o dobro da duração base do HunterCore; não atualiza suas coordenadas. Exploração permanece possível com peso 0,2 por nó alcançável, mesmo quando existe memória. Isso permite busca errada, esconderijo vazio e uma rota aprendida sem interceptação garantida.

Postura de emboscada e manipulação do ambiente são metas distintas. `WatchPassage` só posiciona/vigia depois de viagem, aviso e reação; a postura expira e possui cooldown. Não bloqueia a passagem por teleporte nem captura quem a cruza. Abrir/fechar porta, desligar circuito e colocar câmera em interferência usam WorldCore, estado físico confirmado e transação. Dispositivos não vistos não entram na decisão por consulta ao estado global. Uma alteração de outra pessoa durante a viagem faz a conclusão falhar, preservando a tarefa para reavaliação/cancelamento. Após a própria alteração, Batman atualiza o baseline sem atribuir seu próprio efeito a Joe.

Chegar ao alvo ou inspecionar um esconderijo não é ataque: não aplica dano/captura, não revela identidade do ocupante e não confirma destino narrativo. A ponte precisa emitir novas percepções quando alguém for realmente identificado e confirmar eventual contato/captura pelo fluxo de campanha/companheiro apropriado. Inventários ou resultados privados dos companheiros não são entradas de percepção; pessoas retiradas da simulação deixam de ser alvos ativos, mantendo a memória de região para uma busca possível.

## Hipóteses iniciais por dificuldade

| Modo | Decadência de agressividade/s | Reação após aviso | Cooldown de emboscada | Duração de vigia | Limiar para ambiente | Peso base de emboscada |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Fácil | 0,030 | 5 s | 180 s | 20 s | 70 | 0,10 |
| Moderado | 0,020 | 4 s | 140 s | 30 s | 50 | 0,25 |
| Difícil | 0,012 | 3 s | 100 s | 40 s | 35 | 0,45 |
| Punitivo | 0,008 | 2 s | 70 s | 50 s | 20 | 0,70 |

A escala de agressividade é 0–100. Tiro soma 12 (intervalo mínimo 1 s), explosão 25 (3 s), destruição 15 (5 s) e evasão confirmada 10 (15 s). Intervalos evitam tratar o mesmo efeito contínuo como inúmeras ocorrências. IDs ainda devem representar ocorrências físicas distintas. Agressividade decai apenas no relógio da campanha. O peso de emboscada cresce por `1 + agressividade/50`; ambiente por `1 + agressividade/100`. Os perfis estão em `HunterProfile` e a revisão de política participa da identidade de conteúdo. Valores exigem playtest real, sem promessa de duração ou equilíbrio final.

## Persistência e integração

Runtime **v4** conserva a camada StorySave v1 e acrescenta pressão/decadência, quatro memórias visuais e suas últimas amostras, perdas explícitas, passagens observadas, setores inspecionados, vigia/cooldown, RNG separado, serial e tarefa em viagem/janela de reação. O loader valida vínculos, relógios, amostras, ocorrência de origem/chegada, resultados de inspeção/vigia e agressividade reconstruída dos eventos. Falha preserva a sessão atual. Runtime v1/v2/v3 exige migração explícita ou nova campanha; não descartar a nova camada ao usar um serializador inferior.

Na Unreal, parar sensores durante load; restaurar mundo/navegação/atores e representar a tarefa salva antes de retomar relógio/percepção. Uma tarefa em viagem retoma a navegação; não transportar Batman até o destino. Uma tarefa com chegada confirmada retoma a preparação e o tempo restante. Reconciliar a apresentação do aviso salvo sem confirmá-lo duas vezes. Aplicar sinais e alterações de dispositivos somente após resultado confirmado; `AlreadyApplied` não devolve sinal novo. Fontes de som do próprio Batman não são encaminhadas automaticamente como evidência de Joe.

Compilar `CampaignHunter.cpp` e `CampaignHunterSave.cpp` com os demais arquivos de CampaignCore. `hunter_tests.cpp` cobre quatro dificuldades, alvo diferente de Joe, expiração em delta longo, renovação contínua, 12.000 amostras sem consumo de recibos, buscas vazias/incertas, pressão, ambiente, aviso/reação, alteração concorrente, callback velho, retomada/RNG e arquivo inconsistente. CMake expõe alvo `runtime_hunter_tests` sem colidir com HunterCore. O custo de cópias/validação e o orçamento de recibos da campanha completa ainda precisam de benchmark; nenhuma medição de 60 fps ou três horas jogáveis foi produzida por estes testes.
