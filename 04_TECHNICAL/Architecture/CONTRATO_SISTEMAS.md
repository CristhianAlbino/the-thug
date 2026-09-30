# Contrato de integração — Hunter e regras da campanha

## Fonte e objetivo

Baseado no [GDD Mestre](../../02_GAME_DESIGN/GDD/GDD_MASTER.md), especialmente percepção e memória do Hunter (seções 8–17), dificuldades (66) e rotas/finais (52–59). Implementação independente em [Tools/HunterCore](../../Tools/HunterCore/README.md). Este código é a lógica de decisão da versão final em desenvolvimento, não um asset temporário de protótipo.

A construção dos estímulos de som e de evidência observada está detalhada no [contrato de percepção](CONTRATO_PERCEPCAO.md).

## Separação de responsabilidades

| Origem na Unreal | Evento para o núcleo | Saída consumida pela Unreal |
| --- | --- | --- |
| Sensor de visão com linha de visão válida | `VisualContact(Player, estimativa)` | `Pursuing` e localização vista |
| Perda real de visão | `VisualLost(Player)` | `Searching` no último local visto |
| Sensor de som após propagação/oclusão | `Sound(Unknown/Decoy/Companion, centro, raio, intensidade)` | `Investigating` em área aproximada |
| Porta, câmera, luz ou objeto observado | `EnvironmentalEvidence(estimativa)` | Investigação, se relevante |
| Jogador visto tomando uma rota | `ObservedRoute(Player, ID)` | Aumenta peso futuro dessa rota |
| Jogador visto entrando no esconderijo, ou prova física conclusiva | `ConfirmedHideSpot(Player, ID)` | Aumenta suspeita desse esconderijo |
| Navegação do nível | Lista de rotas válidas em `ChoosePatrolRoute` | ID escolhido; navegação continua na engine |
| Objetos e objetivos do nível, após validação | `RunRules::Apply(evento, tempo)` | Prontidão/falha da rota ou resultado da partida |

**Regra de justiça:** o adaptador não chama o núcleo com a posição real de Joe quando o Hunter só ouviu um som. Sons precisam de localização estimada e raio de incerteza de pelo menos 4 m (valor inicial configurável). Um evento de rota/esconderijo só existe quando a percepção ou uma pista justificou a informação. A engine deve registrar a origem do evento no debug para explicar as decisões do Hunter.

## Tempo e sequência

- Enviar `AdvanceTo` com tempo de jogo monotônico; pausar o tempo durante pause menu.
- Enviar `VisualLost` quando o sensor perder o alvo; não manter perseguição indefinidamente por falta desse evento.
- O plano de investigação vira busca, e a busca retorna à patrulha após seus prazos. Um novo estímulo pode redirecionar a investigação. Sons durante visão direta de Joe não substituem o alvo visto.
- O núcleo retorna uma intenção. Ele não teletransporta, não calcula caminho e não verifica se um ponto está acessível. A navegação precisa validar/recusar o destino.
- O [SaveCore](../../Tools/SaveCore/README.md) já serializa relógio, memória, estado, rotas e RNG do Hunter. A restauração de sensores, navegação e atores da Unreal segue o [contrato de save](CONTRATO_SAVE_CAMPANHA.md).

## Regras da campanha

- Garagem: abrir cofre → obter chave → liberar portão → alcançar saída. O código não conhece a combinação; o objeto cofre precisa validá-la.
- Telhado: encontrar rádio → instalar pilhas → contato → confirmar helicóptero → chegar ao telhado dentro de cerca de cinco minutos. Um prazo perdido fecha apenas essa rota.
- Túneis: descobrir acesso → alcançar saída. Navegação, labirinto e Robin permanecem na engine.
- Janela: descobrir passagem + ferramentas → destravar → atravessar. Furtividade durante o processo permanece na engine.
- Um evento `DawnReached` deve ser emitido pelo relógio/narrativa do mundo após confirmar sobrevivência. No Punitivo ele não dá vitória automática. A duração aproximada por dificuldade no GDD não é convertida artificialmente em um relógio fixo neste núcleo.
- `HunterAbandonedSearch` só pode vir de uma condição sustentada de busca encerrada, validada por sistemas de mundo e Hunter. Nunca expor isso como botão de espera.
- Ferimentos, companheiros, decisões sistêmicas e finais secretos têm contratos próprios a definir; nenhum está falsamente marcado como implementado aqui.

## Próximas integrações quando a Unreal estiver disponível

1. Criar adaptadores de percepção e tempo, com logs de cada estímulo e da sua origem.
2. Ligar `Intent` a AI Controller/Behavior Tree e navegação, mantendo checagens de caminho e linha de visão na engine.
3. Ligar interações dos objetivos a `RunRules`; reproduzir os testes também em sessões Play.
4. Conectar os snapshots existentes a uma fase real e validar save/load da memória adaptativa em sessões Play.

**Limites verificados:** lógica C++ compilada e testada isoladamente. Nenhuma cena, Blueprint, comportamento em rede ou build Unreal foi testado.
