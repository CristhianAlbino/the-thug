# Interações, variações e registro da noite — v1

**Estado:** primeiro ciclo C++17 de P02/P06, com as cinco extensões aprovadas por Cristhian em 02/10/2026. Integrado a CampaignCore e ao runtime v9. Atores, animações, navegação, propagação sonora e interface continuam dependentes da Unreal. Valores são hipóteses de playtest. Áudio/assets finais e Extremo/Joker seguem posteriores.

## Uma sessão, recursos individuais

`ApplyInteraction(command, context)` trabalha em cópia temporária e só troca a sessão após a conclusão válida. O ID de ocorrência pertence ao registro compartilhado: retentativa retorna `AlreadyApplied`, payload diferente retorna `Conflict`. Pausa, prova ausente, prazo inválido, espaço insuficiente e recusa de pré-requisito não consomem recurso nem alteram RNG. Uma solicitação percebida e recusada é uma resposta válida: `Applied` com `accepted=false`, registrada sem tarefa.

`physicallyVerified` e `actionCompleted` vêm do marco real da interação, nunca do clique no HUD. `context.actors` contém somente pessoas percebidas pelo executor; estar no mesmo nó não prova visada ou audição. Ocupantes de esconderijos permanecem no nó do dispositivo. Nós alcançáveis e rota alternativa vêm de navegação/colisão reais.

| Operação | Vínculo e prova | Consequência confirmada |
| --- | --- | --- |
| `Signal` / Call | Joe sinaliza para uma pessoa percebida; destino é o nó atual de Joe; sinal recebido e destino alcançável. | Intenção com 30 segundos de validade; pode entrar na escolha de `FollowSignal`. |
| `Signal` / Quiet | Sinal recebido pela pessoa; confiança suficiente. | Pedido de silêncio por 15 segundos. Suspende início/retomada/ACK de conversa opcional desse participante. |
| `Signal` / PointArea | Joe aponta uma área reconhecível e alcançável; sinal recebido e `areaPerceived` confirmado. | Intenção com 30 segundos de validade; pode gerar `InspectSignaledArea`. Não identifica recurso, esconderijo, pista ou saída. |
| `Gesture` / HarleyCalmsBarth | Harley, Barth e Joe realmente presentes, fora de perigo; sem tarefa/ocultação concorrente dos participantes. | Medo de Barth −12 e stress −15, limitados a zero. Cooldown de 30 segundos. |
| `Gesture` / BarthHelpsJoe | Barth presente e capaz, confiança ≥40, Joe ferido e tratamento no inventário de Barth. | Consome um tratamento de Barth e usa a regra normal de medicina em Joe. |
| `Gesture` / JackHelpsJoe | Jack presente e capaz, confiança ≥40 e um recurso real próprio. | Passa um magazine, fumaça ou ferramenta de janela para Joe; capacidade e munição são reconciliadas. |
| `PlaceWedge` | Joe junto de uma porta manual autorada, fechada, com ferramenta própria e rota alternativa fisicamente verificada. | Ferramenta sai da bolsa e fica na porta, representada por `wedged`. |
| `RecoverWedge` | Joe junto da mesma instalação, intacta, com espaço na bolsa. | Devolve exatamente uma ferramenta; a porta volta a `closed`. Falha mantém o calço. |
| `Reunion` | Joe percebe a pessoa realmente presente no nó, sem destino terminal. | Registra o encontro uma vez por pessoa/local. Não concede confiança, recurso ou sobrevivência. |
| `WitnessFate` | Joe percebe o acontecimento/corpo; destino e nó precisam corresponder ao estado físico já confirmado. | Conhecimento pessoal de captura, morte, partida ou saída conjunta. Não cria esse destino. |
| `Ambient` | Ocorrência física autorada e percebida no local. | Marca ocorrência, emite fonte sonora real uma vez e permite anotação. |
| `Banter` | Uma fala curta concluída e ouvida, participante presente, sem perigo, silêncio pedido ou apresentação principal/conversa concorrente. | Marca fala opcional entregue uma vez por local. |
| `PerceiveThreat` | O próprio companheiro percebe Hunter; cooldown de 30 segundos. | Medo +10 e stress +12; não revela o Hunter a Joe ou a outras pessoas. |

Call/PointArea exigem confiança ≥35 em segurança ou ≥55 em perigo, medo+stress <175 e pessoa sem tarefa/ocultação concorrente. Quiet não força saída de esconderijo. A confiança modifica elegibilidade e peso; o pedido não substitui o sorteio entre objetivos executáveis. A recusa não precisa de uma rolagem aleatória escondida.

## Viagem, gestos e silêncio

Os dois novos objetivos usam o fluxo existente `BeginTask/ChooseTask → Arrive → Complete/Cancel`, com serial persistente. A chegada exige movimento concluído; a conclusão exige um segundo adicional, ação física e ordem ainda válida. `FollowSignal` exige Joe ainda presente no destino. Investigar uma área não coleta nada por si: novos recursos precisam ser realmente percebidos e observados pelo fluxo habitual. Intenções antigas expiram; tarefas antigas ainda podem ser canceladas, sem teleporte.

`InteractionCue` fornece direção de gesto ou a fala opcional escolhida. Consultá-la não confirma apresentação nem efeito. Os identificadores `IG_*` são pontos de ligação de produção, não assets de animação existentes. O adaptador executa o gesto/tratamento/entrega e envia o ACK no marco final. Nesta versão, gestos e falas curtas não têm um segundo sistema de apresentação parcial: um gesto interrompido antes do ACK deve ser cancelado/reencenado pela ponte; nenhum custo foi aplicado. Cenas e conversas maiores continuam no DirectorCore, com planos e cursores persistidos.

Silêncio suspende conversas opcionais já iniciadas sem marcar a próxima fala como ouvida. Expiração permite retomar a primeira fala não confirmada. Cenas principais conservam prioridade. O pedido não elimina respiração, ferimento, passos, ruído físico ou reação de perigo.

Depois de uma ameaça percebida, os primeiros 30 segundos conservam a pressão. Durante descanso posterior, apenas o excesso sobre o perfil autorado cai: medo a 0,08/s e stress a 0,12/s. Confiança, lealdade, relações e memória de rotas não são apagadas. Pausa e carregamento não concedem recuperação grátis. O medo não precisa zerar para Barth ajudar.

## Calço e Hunter

Três portas iniciais admitem a ferramenta `window_tool`: `D_F4_NURSE`, `D_F2_SUPPLY` e `D_F1_RADIO_E`. O mesmo item continua necessário para sua função habitual; deixá-lo no calço é uma escolha de recurso. Não existe ferramenta de grupo nem duplicação ao recuperar.

WorldCore bloqueia abertura comum/forçada e emperramento enquanto a porta está `wedged`. Hunter precisa perceber essa alteração por EvidenceMemory. O objetivo `BreakWedge` usa escolha, deslocamento, chegada com aviso, reação normal e **seis segundos adicionais** antes da confirmação física. A conclusão rompe a ferramenta e deixa `forced_open`, com som de objeto rompido. Não concede dano, captura, localização de Joe ou vitória. Recuperar/substituir a instalação durante a espera pode invalidar a conclusão; Hunter precisa reavaliar. Uma reinstalação usa um instante de simulação posterior ao callback concluído do Hunter nessa porta.

O calço não é destruído automaticamente pelo relógio e nenhum efeito ignora uma parede. A ponte mantém colisão e passagem sincronizadas com o estado confirmado e encaminha `WorldSignal` à percepção com alcance/oclusão reais.

Uma instalação percebida pode entrar nas opções de ruptura mesmo com agressividade inicial; o limiar de pressão para alterações discricionárias de energia/câmeras não impede investigar essa obstrução. Uma instalação que Hunter ainda não percebeu não cria alvo.

## Variação opcional e registro pessoal

Um perfil 0–2 é sorteado com RNG separado uma vez na criação da campanha. Cada perfil escolhe uma ocorrência ambiental e uma fala em três locais: triagem, serviço do segundo andar e carga do térreo. `VariationFor` permite à ponte preparar essa apresentação; a consulta não significa que Joe a viu/ouviu. `Ambient/Banter` confirmam os marcos reais. Carregar restaura o perfil; nenhuma pista essencial, combinação, preparação ou rota muda com ele. As falas são rascunhos de encenação sem palavrão pesado e sem revelação antecipada de Jack.

`NightRecord` retorna entradas estruturadas ordenadas por tempo e ID de ocorrência. Contém escolhas próprias, ajuda que Joe recebeu/prestou, leituras realizadas, sinais e respostas, encontros, ocorrências e falas confirmadas. `KnownCompanionFates` distingue mundo e conhecimento: captura/morte/fuga fora da percepção de Joe permanece desconhecida. Partir sozinho não confirma o destino final. Cruzar fisicamente junto de Joe já constitui prova de saída conjunta. O epílogo autoral continua separado do registro pessoal.

Texto completo de documento/gravação permanece na consulta habitual após inspeção. O registro não despeja códigos, pistas privadas de NPC, pensamentos, transforms escondidos nem estado onisciente do Hunter.

## Persistência e aceite

`THE_THUG_RUNTIME_CAMPAIGN 9` conserva o perfil e serial de interações; pedidos, cooldowns, calços, falas/ocorrências e conhecimento pessoal são derivados do registro confirmado, evitando cópias divergentes. O carregamento valida autoria, ordem/tempo, vínculos, provas de tarefas, balanço de instalações/recuperações/rupturas, estado das portas e destinos antes de trocar a sessão. Traits, bolsas, ferimentos, RNG e demais sistemas continuam nos envelopes existentes. Runtime v1–v8 exige migração explícita ou nova campanha.

Testes nativos cobrem quatro modos, sinal não percebido, recusa, viagem retomada, Joe ausente, expiração, suspensão/retomada de fala, prioridade principal, recurso individual, cooldown/pressão temporal, calço e inventário cheio, ruptura retomada, corrida de recuperação, ocorrência opcional, conhecimento limitado e rejeição sem mutação com checksum recalculado. O checksum identifica corrupção acidental; não autentica uma partida.

Aceite no editor: sinais realmente legíveis sem HUD intrusivo; gestos sem travar personagem; rota alternativa e colisão do calço; som e aviso antes da ruptura; diálogo que não cobre um perigo; save/load durante viagem/fala; duração da pressão; registro lido por alguém que desconhece os segredos. Esses ensaios não foram substituídos pelos testes de lógica.

**Atualização v9:** pilhas/informação/interrogação/usos conjuntos, armas dos NPCs e planos observados usam o [contrato complementar](CONTRATO_RECURSOS_INFORMACAO_PLANOS_V1.md). Os limites físicos deste documento permanecem.
