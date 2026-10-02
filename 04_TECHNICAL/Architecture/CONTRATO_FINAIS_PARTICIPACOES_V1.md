# The Thug — contrato de finais raros e participações v1

P08 entrega o primeiro ciclo independente da engine. CampaignCore continua sendo o único dono mutável da campanha. `ApplySpecial` confirma transações com recibos `S`, ordem explícita, retentativa idempotente e troca de estado somente após aceitação. `SpecialRuntime()` retorna uma cópia. Autoria lógica está em `campaign_initial.json`; números, setores e custos são hipóteses de blockout/playtest. As condições secretas não devem virar tutorial, menu de seleção de final ou texto de interface.

## Confronto excepcional

Disponível somente no Fácil, no ato final. Recrutar significa intenção; iniciar revalida presença física, confiança, ferimentos, disponibilidade e recursos individuais de pelo menos dois sobreviventes. Joe e cada participante gastam suas próprias provisões no início confirmado. Não há pool de munição do grupo. Uma tentativa não pode reiniciar para renovar recursos ou travar o Hunter.

O início exige aviso apresentado. O relógio para no marco de resolução física; o adaptador confirma se houve interrupção temporária. Falha conserva a campanha aberta e só aplica ferimentos/captura/morte individual realmente confirmados e testemunhados. Nenhum resultado coletivo é inferido. O sucesso cobra ferimento de Joe e abre uma janela curta; não concede vitória nem HP/morte de Batman. A sequência conjunta é uma ação autorada; não implementa o sistema genérico de armas dos NPCs. A ponte emite os sons reais e as pressões percebidas pelos canais existentes, sem entregar identidade/coordenadas ao Hunter por intenção. Durante a janela, ofertas/execuções táticas do Hunter ficam suspensas e a ponte deve suspender os ataques e movimento correspondentes.

`ConfrontationEscape` exige a travessia física dentro da janela, no destino autorado. Produz `Ending::CoordinatedEscape`. Só participantes confirmados junto de Joe recebem resultado `together`; ajudar na tentativa não equivale a fugir. Expirar a janela libera a reserva de conclusão e mantém as rotas convencionais disponíveis. N33 seleciona falas apenas de sobreviventes que chegaram com Joe e continuam presentes. O epílogo usa evidências individuais.

## Destruição excepcional

Disponível somente no Punitivo, no ato final. Cargas estruturais e detonador são itens próprios: granadas comuns não substituem preparação estrutural. Suprimentos são pickups autorados, com capacidade real de inventário, prova física, coleta única e retentativa sem duplicação. Pelo menos três pontos distintos precisam ser preparados em diferentes setores; eles não se confundem com dispositivos essenciais das quatro rotas.

`BeginPlant` reserva uma carga. `CompletePlant` exige serial/local, tempo mínimo e conclusão física da instalação; só então consome o item e altera o ponto. Tempo decorrido sozinho não instala explosivo. Cancelar libera a reserva sem consumo. A ponte deve cancelar uma instalação interrompida: não pode confirmar como trabalho contínuo uma viagem, esconderijo ou animação que foi abandonada. Cancelamento também é permitido depois de Joe entrar num esconderijo, desde que o local real seja confirmado. Transferências/consumos não usam o item reservado.

Detonar exige todos os pontos preparados, detonador real e aviso apresentado. Consome o detonador uma vez e inicia colapso pendente. Ações de campanha/companheiros/Hunter/combate não ultrapassam essa resolução por outro gatilho; o relógio chega ao marco e aguarda física. Ferimentos fatais prévios ainda podem interromper a campanha antes desse marco.

`ResolveCollapse(argument=1)` exige sobrevivência e chegada física ao abrigo autorado. Produz `Ending::BitterDestruction`: Joe gravemente ferido, estabilizado para a recuperação policial, hospital destruído e coda de prisão ao amanhecer. `joeEscaped` permanece falso. O Hunter pode estar preso/aparentemente morto nos destroços; sua morte não é afirmada. Companheiros sem resultado testemunhado ficam desaparecidos/desconhecidos; nenhuma morte é inventada.

`ResolveCollapse(argument=0)` exige morte física confirmada de Joe e produz `Ending::Died`, com estado fatal e colapso preservado. Provas contraditórias de morte/sobrevivência são recusadas. O amanhecer continua sem conceder vitória automática no Punitivo.

## Participações especiais

Robin, Alfred, Gordon, Oracle e Flash têm encontros autorados com local, ato mínimo, dificuldade, apresentação e orçamento de raridade. `WitnessAppearance` exige exposição física/auditiva verificada pelo adaptador e ocorre uma vez por campanha. A observação de narrativa não pode inventar `appearance:*`. O fato é derivado do registro de encontros; `SP_*` passa por apresentação/ACK normal e retoma sem repetir fala confirmada.

Robin tem episódio breve no Punitivo, nos túneis. Este ciclo fornece gatilho e apresentação, não um segundo AI Controller, navegação, combate ou perseguição permanente. Alfred/Oracle são comunicações parciais e Gordon representa o cerco exterior; não concedem rotas nem anulam fugas já confirmadas. Flash é uma referência opcional: um bilhete entre mil sorteado uma vez por campanha permite o encontro, ainda condicionado à exposição física. Consultar/carregar não sorteia novamente e a referência não interfere nos resultados/recursos.

## Persistência e limites

Runtime v6 acrescenta fase/marco, participantes, suprimentos, pontos, instalação/serial e encontros/bilhete aos sistemas anteriores. StorySave v1 mantém sua gramática e amplia os valores de resultado conhecidos. O wrapper v6 exige a identidade integral de conteúdo. Runtime v1–v5 é recusado: migração não foi implementada.

O loader reconstrói a sequência especial por recibos ordenados, verifica estados de objetos, tarefa/reserva, dificuldade, resultado e fatos protegidos antes de substituir a sessão. Checksum identifica corrupção acidental; não autentica um save contra adulteração deliberada. O limite compartilhado de 10.000 recibos reserva a resolução de instalações/colapso e **resolução mais travessia** do confronto já iniciado. Epílogo é consulta sem recibo; N33 usa o orçamento restante de apresentação como as outras cenas.

Após carregar, a ponte precisa reconstruir cargas instaladas, trabalho pendente, colapso/aviso, aparições e suspensão temporária antes de retomar a simulação. Esta entrega não contém física de explosões estruturais, destruição do cenário, animações, atores especiais, cinematics, gravações ou build Unreal. Textos/gestos são rascunhos de produção; geometria, tempo, confiança, custos e probabilidades exigem revisão criativa e playtest.

## Verificação

`special_tests.cpp` cobre quatro modos, autoridade de fatos, recursos individuais, presença/avisos, preparação/cancelamento, resolução física, janela expirada, morte individual, fuga parcial, sobrevivência/morte no colapso, cenas/epílogos, retomada com outra semente, aparições opcionais, corrupção sem mutação e orçamento reservado. `run_rules_tests.cpp` verifica resultados terminais e dificuldade; paridade narrativa inclui os cinco blocos de participação. CMake/CTest e o workflow existente incluem a nova suíte em Linux e Windows/MSVC. São ensaios de regras com provas simuladas, não campanhas jogadas ou validação da destruição física.
