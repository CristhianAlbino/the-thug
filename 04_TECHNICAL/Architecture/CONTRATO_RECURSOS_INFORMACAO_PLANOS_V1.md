# The Thug — recursos, informação e planos nativos v1

**03/10/2026 · CampaignCore C++17/runtime v9.** Completa as lacunas nativas identificadas na revisão P02/P05/P06/P07: pilhas no chão, relatos/mentira, interrogação, usos conjuntos, armas individuais dos NPCs, aprendizado observado de fumaça, armadilha encadeada e retirada. Não produz atores, níveis, física, voz ou animação. Os valores novos são hipóteses para playtest.

## Recursos deixados no chão

`ApplyStrategy(DropItem)` remove quantidade real da bolsa de Joe/NPC depois da ação física; o ID do evento identifica a pilha, com nó e posição de repouso em metros. Só itens marcados `droppable` podem ser soltos. Armas carregadas continuam sob as regras de transferência do combate; ferramentas/chaves essenciais não viram pilhas por esta operação. `NoticeDrop`, `TakeDrop` parcial e `NoticeMissingDrop` confirmam inspeção/coleta/ausência. Capacidade ou disputa rejeitada não altera bolsa nem chão. Reservas de outras ações não podem ser gastas.

O runtime guarda a quantidade física única, inclusive tombstone quando vazia. `KnownDrops(actor)` mostra **a última inspeção dessa pessoa**, com data, quantidade e posição observadas; não mostra o proprietário original de um objeto encontrado. Uma coleta invisível não atualiza o registro alheio. Conferir de novo pode mostrar quantidade alterada/ausência. A ponte trata o ID e o snapshot físico como autoridade para criar/remover o ator de pickup, sem restaurar uma pilha nova a partir de informação antiga.

`ResourceCoherent` reconcilia os estoques iniciais e recibos de todas as famílias: pickups, usos/consumo, transferência, medicina, calços, preparação/finais, resgate, pilhas e ações conjuntas. Estoque conservado não equivale a provar a colisão de um objeto. Uma reserva opcional de fumaça na autoria torna a repetição testável também no Punitivo; não altera seu equipamento inicial nem uma pista essencial. Distribuição continua hipótese.

## Informação e mentira

`ObserveArea` exige observação física da área; presença do Hunter requer local observado correspondente. `ReportThreat` transmite uma observação existente, preservando a data original, ou uma afirmação deliberadamente enganosa sobre uma área conhecida pelo falante. A fala exige destinatário presente e percepção real. Companheiros não mentem no Fácil; nos demais, intenção exige desconfiança/baixa lealdade ou autopreservação alta. Intenção é dado interno, nunca indicador de HUD.

`KnownReports(actor)` devolve somente o que foi comunicado ao destinatário: fonte, área, data, relato, checagem e contradição. Relato de ameaça fica antigo após dez segundos. Não concede rota, pista, posição precisa ou identidade no Hunter. `CheckReport` exige nova inspeção do destinatário na mesma área, até dois segundos do relato e da checagem; informação antiga não prova mentira só porque o Hunter mudou de lugar. Uma mentira de Joe realmente contradita reduz confiança/lealdade do NPC uma vez (-15/-5, hipótese). Joe decide como reagir a um companheiro que mentiu; verificar em privado não altera magicamente a confiança do mentiroso em Joe.

Observação imperfeita/antiga e intenção enganosa permanecem distintas. Se a fala não foi percebida, não há informação entregue. Não conectar a intenção privada ao diário ou a um rótulo “verdade/mentira” exibido antes da verificação.

## Interrogação sem destino automático

`BeginQuestion` exige Hunter e companheiro contido em contato real, sem outra tarefa do Hunter ou soltura ativa. Usar o serial atual do runtime de resgate como época de custódia; não o serial da vítima apenas. A pergunta demora quatro segundos e ocupa o Hunter. `AnswerQuestion` usa o serial próprio da tarefa e exige fala percebida, continuidade e conclusão.

`QuestionOptions` inclui recusa; revelar uma rota exige conhecimento privado prévio e pressão/autopreservação ou lealdade compatíveis; enganar exige modo/autopreservação compatíveis. A resposta vira um **relato de rota ao Hunter**, não posição de Joe nem confirmação de que alguém está na saída. O Hunter pode oferecer investigação dessa hipótese e visitar o lugar errado. Joe não aprende uma conversa fora de sua percepção. Silêncio não causa lesão, morte ou captura terminal.

Ferimento/medicina, soltura iniciada, transferência, incapacidade ou fim interrompem a pergunta; progressos antigos não se tornam válidos depois de curar/carregar. Cancelar libera a tarefa. Contenção/captura e agência continuam no [contrato de resgate](CONTRATO_CAPTURA_RESGATE_V1.md).

## Uso conjunto e armas individuais

`BeginJointUse` coordena dois participantes reais num uso autorado, com consentimento pelos traços. O parceiro reserva seu próprio recurso; três segundos e `CompleteJointUse` com continuidade, presença e pré-requisitos atuais precedem custo/mudança do mundo. `CancelJointUse` libera tudo sem consumo. Ferimento/medicina/captura/fim invalidam progresso. Ações de outra família não movem ou gastam os participantes ocupados. Este fluxo não permite bypass do teclado protegido. Coletar uma ideia não instala ferramenta nem conclui uma fuga.

`CombatCommand.actor` seleciona Joe/Harley/Bartholomeu/Jack. Cada NPC mantém munição carregada, intervalo e preparação próprios em `CombatRuntime.companions`; serial de tiro/projétil identifica também seu executor. Arma e carregador precisam estar na bolsa de quem usa. Joe continua com oxigênio/respiração próprios. `CompanionCombatOffers` só propõe recarga/disparo executáveis; disparo do NPC exige ameaça percebida ou distração intencional. Os projéteis/impactos usam as mesmas provas e imunidade do Hunter; não há HP de Batman.

Preparação não gasta antes da conclusão, retentativa não duplica e impacto de outro executor não reclama um serial alheio. Ferimento/medicina/captura cancelam preparação do NPC. Transferir arma com balas carregadas é bloqueado; ela não perde/duplica munição por troca de dono. Ruído percorre `Hear` com atenuação real, sem confirmar identidade automaticamente.

## Planos falíveis do Hunter

`NoticeSmoke` registra cada nuvem existente apenas uma vez, depois de visada real. Duas nuvens observadas na mesma área permitem `BeginSmokeWatch` num limite **observado e alcançável** fornecido pela ponte. A memória esquece repetição após intervalo de 120 segundos e limita o contador a oito. A fumaça conserva sua oclusão; observar um padrão não mostra alguém através dela, não reduz sua duração nem escolhe a saída atual de Joe.

`BeginTrap` usa passagem vista em duas ocasiões separadas por pelo menos 15 segundos, memória de porta/circuito e agressividade. O plano tem três etapas: deslocar/desligar circuito; deslocar/fechar porta; deslocar/observar passagem. Cada `ArrivePlan` exige trajeto real e aviso apresentado; cada `CompletePlanStep` exige tempo de reação/espera, continuidade e conclusão física. Alterações bloqueadoras revalidam alternativa e objeto observado no instante do commit. Nenhuma etapa captura alguém.

`BeginRetreat` usa disparo/explosão percebidos recentes (até oito segundos) e cobertura observada/alcançável. Retirada desloca o Hunter fisicamente; não cura HP nem o teleporta. As ofertas têm peso, cooldown por dificuldade e podem deixar de existir. A ponte escolhe entre planos e comportamento habitual respeitando esses pesos; nenhuma consulta inicia uma ação. Novo contato identificado ou interrupção invalida o plano. `CancelPlan` encerra intenção antiga antes de retomar tática/captura. Revalidar contexto e cancelar na perda física de oportunidade.

## Save, retomada e apresentação

Runtime v9 persiste o registro Q e fingerprint de seus derivados, bolsas/munição por pessoa, tiros/projéteis com executor e o restante da sessão. Carregamento reconstrói conhecimento, custódia, observações, reservas e etapas, cruza fontes e conservação e só então troca a campanha. Runtime v1–v8 exige migração explícita ou nova campanha; checksum é detecção de corrupção, não autenticação.

Reservar vagas para etapas já iniciadas impede que outra família ocupe o espaço necessário à conclusão/cancelamento. O limite geral continua 10.000 recibos/4 MiB, com erro explícito. A ponte restaura atores/objetos antes de IA; continuidade de contato/animação/nav não vem do arquivo. Na perda de continuidade ou de acesso, cancelar e reiniciar. Antes da cena final, cancelar perguntas/usos/planos interrompidos junto da soltura pendente. Limpeza não altera destinos.

`StrategyRuntime` e snapshots de combate são views de depuração/integração que incluem estado privado. HUD/diário usam `KnownDrops/KnownReports/NightRecord/KnownCompanionFates` e provas de apresentação, nunca o snapshot global como conhecimento do jogador. Falas, animações e som devem ser realmente apresentados; usar textos indexados do diretor quando existentes e a [direção criativa](../../02_GAME_DESIGN/Narrative/DIRECAO_CRIATIVA_REVISAO_V2.md) como proposta de atuação.

## Evidência

`strategy_tests` ensaia estoque real/disputa, conhecimento antigo, recusa, informação privada, interrogação/medicina, uso conjunto e rejeição atômica. `extension_tests` ensaia quatro modos/três executores, munição própria, preparação concorrente, autoria de impacto, observação única de fumaça, viagem/avisos/alternativas, cadeia de três etapas, retirada/interrupção e retomada. São fixtures com provas físicas declaradas; não substituem Unreal ou [teste humano](../../01_PROJECT/Reviews/PROTOCOLO_TESTE_HUMANO_PRE_UNREAL_V1.md).
