# The Thug — roteiro de trabalho, atos 1 e 2

**Rascunho v0.1 para Cristhian acompanhar.** Desdobra N01–N11 da [campanha de cinco atos](CAMPANHA_ATOS_RASCUNHO.md). O Difícil orienta ritmo e exploração; Fácil/Normal/Punitivo usam as mesmas áreas com as variantes indicadas. Falas, posições e duração dependem de direção, atuação e playtest. Nenhum encontro condicional fixa um companheiro num ponto se ele já fugiu, foi capturado ou se moveu por outra ligação.

## Como ler e integrar

- `Visto por Joe` registra informação que o protagonista pode usar. `Mostrado ao público` pode informar apenas o jogador, nunca virar conhecimento de Joe por acidente. O conhecimento de Batman e de cada companheiro segue independente.
- A primeira extração de Thomas (N03) é a premissa do GDD. Depois, Batman só reage a percepção válida, navega por uma ligação possível e pode perder Joe. Uma batida dramática não teleporta o Hunter.
- Escolhas são **ações no espaço**: dar item, esperar, percorrer rota, verificar câmera, deixar recado. Não há botão universal de “ser bom” ou “sacrificar”. `CompanionCore` recebe influência/experiência somente depois de a ação realmente acontecer; números iniciais e deltas autorados continuam D-02.
- `scene_seen:Nxx`, `witnessed:*`, `joe_knows:*` e `player_saw:*` abaixo são nomes de intenção para o diretor narrativo e futura persistência; não afirmam existir como campos C++ hoje. Cada evento tem ID estável para não tocar duas vezes após checkpoint.
- Falas não interrompem stealth por regra. Se o perigo corta uma conversa, a informação indispensável ganha uma fonte física ou uma fala posterior. Nunca se entrega localização atual do Hunter por um áudio gravado.

## Ato 1 — o grupo se desfaz

### N01 · Cerco — entrada até `F4_START`

**Entrada:** abertura, grupo junto. **Objetivo jogável:** acompanhar a fuga da rua, perceber duas escadas e uma prumada de serviço, chegar à sala de observação. A polícia fecha as saídas externas visíveis; o grupo entra no hospital porque a rua já está perdida, não porque surgiu uma quinta rota de fuga.

**Batidas:** Bartholomeu segura a porta e se atrapalha com o trinco; Joe segura o peso e vê que o amigo *tentou* ajudar. Thomas se junta sem alarde, verifica a mão de Bartholomeu e o tranquiliza. Harley recolhe um pequeno suprimento esquecido por Joe e o devolve, mas ainda observa se Bartholomeu respira. Jack olha a circulação e comenta que o lugar tem mais de uma vertical. Thomas mostra o relógio riscado no pulso ao bater numa porta: a marca identificável reaparece somente como peça de identificação da cena N34 nos modos que a permitem. Não há prova de morte aqui.

**Falas de trabalho:**

> Joe: “A polícia não vai subir até aqui. Temos alguns minutos.”  
> Bartholomeu: “Dois caminhos pra descer. Achei que hospital tinha um.”  
> Thomas: “Barth, no três empurra comigo. Está bem a sua mão?”  
> Harley: “Você deixou isso cair.”  
> Joe: “Guarda. A gente conversa lá em cima.”  
> Harley: “Você sempre promete isso.”

**Saída:** N02 ao alcançar `F4_OBSERVATION`; `scene_seen:N01` é único. A fala de Harley estabelece história prévia e a falha de atenção de Joe sem impedir o jogador de demonstrar cuidado por ações. O relógio não vira inventário nem código obrigatório.

### N02 · Sinal — `F4_OBSERVATION`

**Entrada:** grupo presente, N01 concluída. **Objetivo:** olhar a cidade e identificar visualmente o telhado, sem iniciar rádio/helicóptero. Harley vê o Bat-Sinal primeiro; Thomas se aproxima da janela para provocar; o reflexo esconde parcialmente o corredor atrás da equipe. O jogador pode se afastar da janela e ainda ouvir a fala importante.

> Thomas: “É só um morcego fantasiado. Quando sairmos, pago café pra todos.”  
> Bartholomeu: “Você disse isso na última vez.”  
> Thomas: “E paguei.”  
> Harley: “Se ele tá na rua, a gente não devia ficar na janela.”  
> Joe: “Sai daí, Thomas.”  
> Jack: “Já cruzei com ele antes. Saí vivo. Ha. E ele não precisa estar na rua.”

**Saída:** retorno ao `F4_START` por ligação existente. `player_saw:bat_signal` não revela onde Batman está. Se Joe não fixou a vista, um clarão atravessa a janela e o grupo comenta; não bloquear N03 numa interação de câmera.

### N03 · Ruptura — `F4_START` / `F4_HALL_E`

**Entrada:** retorno de N02, uma vez. **Encenação da premissa:** luz/interferência, deslocamento curto no teto, Thomas é arrancado do grupo e some do campo de Joe. Uma possível queda de objeto produz ruído; não há alvo nem comando de combate que desfaça a separação. Harley puxa Joe para trás; Bartholomeu foge por uma ligação de escada alcançável; Jack desaparece por corredor acessível. Seus destinos posteriores usam trajetórias próprias, não destinos instantâneos decretados pela cena.

> Thomas: “Merda, que foi isso—”  
> Joe: “Thomas?!”  
> Harley: “Barth, vem comigo. Joe, olha pra mim. A gente precisa sair daqui.”  
> Joe: “Não foi a polícia.”

**Informação:** `joe_knows:thomas_taken` e `story:team_scattered`; **não** `joe_knows:thomas_dead`. Até mesmo no Difícil/Punitivo, a autoria e a morte só se tornam conclusivas em N34 ou na coda ao público. Fácil/Normal nunca exibem tal confirmação. `scene_seen:N03` impede repetir o ataque ao carregar save. Primeiro ponto para salvar a separação como fato; entradas de companheiros seguem condicionadas a presença real depois dela.

### N04 · Primeira escolha — corredor leste/oeste de F4

**Entrada:** Joe recupera controle após N03. **Objetivo:** ler corredor, ruído, cobertura e dois eixos; uma rota ao posto `F4_NURSE`, outra pela enfermaria `F4_WARD_E` e banhos `F4_BATH` também alcança o oeste. O jogador pode voltar à observação ou seguir por uma escada. Tutorial contextual aparece por gesto/legenda acessível, sem bloquear controles até executar uma única solução.

**Variações físicas:** esbarrar no carrinho faz ruído localizado e datado; passar agachado evita esse estímulo; o mesmo ruído pode ser mascarado por relé/chuva. Se Batman estiver realmente em alcance e o perceber, investiga a *origem* com incerteza; se não estiver, não ganha coordenada. Entrar num esconderijo sem ser visto não o informa. Um indício sonoro distante anuncia o eixo de aproximação se ele vier; o retorno pelo outro corredor continua possível segundo portas e geometria.

**Saída:** primeira chegada a `F4_HALL_W` ou uma escada; N05 permanece opcional e pode ser feito mais tarde. `witnessed:cart_noise` só se Joe o produziu/escutou; `hunter_heard:cart_noise` é resultado separado da percepção, nunca efeito automático da cena.

### N05 · O que ficou — `F4_NURSE` / `F4_STORAGE`

**Entrada:** Joe escolhe vasculhar ou retornar. **Objetivo:** encontrar DOC01, primeiro fragmento do cofre, sem tornar garagem a única fuga. Lençóis e armários oferecem recurso/ocultação. Uma pequena alteração do depósito (porta deslocada, vidro no chão) só aparece se algum ator passou ali desde a última observação; persistir origem e momento da mudança.

**Thomas:** um vestígio inespecífico associado à correria pode levar Joe a dizer “Ele passou por aqui?”; nunca mostra corpo, morte nem autoria nos modos Fácil/Normal. No Difícil/Punitivo também não substitui a prova posterior. Se Joe ignorar o posto, DOC01 continua disponível para a rota da garagem; N06 não exige a pista.

**Fecho do ato:** Joe desce ou encontra uma passagem para F3. Não forçar 25 minutos: o ato vive de orientação, decisão e primeira interpretação do espaço. O jogo precisa continuar se o jogador não fez N05 ou não fez ruído.

## Ato 2 — a primeira leitura do hospital

### N06 · Clínica — `F3_CENTRAL` / `F3_RECOVERY`

**Entrada:** Joe chega por qualquer escada/rota permitida. **Objetivo:** encontrar cobertura nos leitos, reconhecer a ligação `F3_ICU` ↔ `F3_RECOVERY` e o caminho `F3_EXAMS` → `F3_HALL_W`; janelas e cortinas oferecem visão parcial sem tornar a área segura. Uma campainha ou porta que bate pode sinalizar presença antiga, sem identificar ator por magia.

**Encontro variável:** a área pode estar vazia, conter vestígio recente de Harley ou acolher N07 se Harley efetivamente estiver ali. Um detalhe de uso clínico recente pode sugerir que o hospital não está completamente morto, sem explicar Arkham. Se N07 não disparar, Joe ainda pode achar materiais básicos por exploração e prosseguir pela clínica.

**Saída:** N07 ou N08 pode surgir por presença, ou Joe segue sozinho. N06 é descoberta espacial, não relógio de perseguição.

### N07 · Harley — `F3_ICU` ou `F1_TREATMENT`

**Entrada:** Harley está presente e alcançável, não capturada/morta, Joe chega ao nó; ocorrência única por encontro. Se ela migrou para F1, a cena toca lá com cenário/objetos coerentes. Se nunca se cruzam, a informação de rota essencial permanece em documento ou exploração, e o epílogo considera o último contato real.

**Ação que dá valor a ela:** Harley percebe um ferimento de Joe **se** ele está ferido e oferece tratamento **se** tem recurso; caso contrário identifica um acesso pelo som de uma porta/relé que acabou de observar, sem fingir que conhece o prédio todo. O jogador pode aceitar ajuda, dar suprimento, cobrir sua passagem, ficar para escutar ou ir embora. Só a troca concluída muda inventário; só cuidado observado muda relação.

**Ramo Joe ferido, recurso disponível:**

> Harley: “Você tá sangrando.”  
> Joe: “Não é nada.”  
> Harley: “Não mente pra mim. Encosta aqui, eu vejo.”

**Ramo Joe ileso ou sem recurso:**

> Harley: “Escutei uma porta de metal bater duas vezes. Alguém usou o corredor do outro lado.”  
> Joe: “Você viu quem?”  
> Harley: “Não. Eu *ouvi* a porta.”

**Fácil/Normal/Difícil, Joe quer encerrar:**

> Harley: “Eu também me cortei naquela sala.”  
> Joe: “Você ainda consegue andar. Vamos.”  
> Harley: “Consigo. Era só isso que você precisava saber?”

**Punitivo, mesmo fato:**

> Harley: “Eu também me cortei naquela sala.”  
> Joe: “Deixa eu ver. Se esse corredor piorar, eu volto por você.”  
> Harley: “Então não me faz procurar você sozinha.”

**Saída:** a resposta verbal é variante de tom, **não** resultado moral automático. Se o jogador voltou, entregou recurso ou cobriu Harley antes, a fala fria reconhece a ação (“Você me ajudou lá atrás”) e o epílogo a preserva. Se ela parte sozinha, marcar último nó/tempo conhecido sem converter “separada” em “morta”. Esta cena prepara o peso de N23.

### N08 · Análise de Jack — `F3_EXAMS` ou `F2_CAMERAS`

**Entrada:** Jack presente, encontra Joe fisicamente. Seu insight deve derivar de algo que ele presenciou: ouviu busca após um ruído, viu um corredor inspecionado ou encontrou vestígio. Se não presenciou dois padrões, formula hipótese, nunca testemunho falso.

**Ramo com duas observações:**

> Jack: “Na primeira vez, ele veio pelo barulho. Na segunda, pela escada.”  
> Joe: “Tá dizendo que ele sabe para onde eu vou?”  
> Jack: “Tô dizendo que você acabou de me contar qual escada costuma usar.”

**Ramo com uma ou nenhuma:**

> Jack: “Se ele ouviu a porta, talvez esteja olhando o corredor errado agora.”  
> Joe: “Você viu ele?”  
> Jack: “Vi a porta aberta. O resto é palpite.”

**Lembrança opcional, qualquer modo:** se Joe perguntar sobre o encontro passado, Jack conta o que presenciou e como escapou, ri da própria sobrevivência e não ganha conhecimento atual de Batman. O riso comum não registra pista J1–J3.

**Punitivo/J1:** já no primeiro reencontro, roupa de corte/forro diferentes da abertura. Uma risada curta e macabra acompanha uma graça deslocada *depois* de Joe relatar um ferimento ou sumiço; Jack precisa estar na conversa e o jogador ter visão/áudio suficientes para a pista ser testemunhada. Se a conversa for interrompida ou Joe não ouvir, não gravar J1; haverá pistas posteriores, mas J4 requer duas pistas incluindo J3. Fora do Punitivo, o figurino e a risada J1 não tocam.

**Escolha:** Joe pode seguir o palpite, conferir ou rejeitar. Jack não altera a posição do Hunter. A utilidade de uma observação verdadeira deve aparecer em uma rota ou atraso real, para que desconfiar dele mais tarde custe algo.

### N09 · Monitores — `F2_SECURITY` → `F2_CAMERAS`

**Entrada:** Joe alcança a sala, com ou sem Jack. **Interação:** conferir energia de `C_SECURITY`, estado da câmera e timestamp. `CAM_LOBBY` online mostra `F1_LOBBY` no presente; `CAM_CLINIC` pode dar interferência; `CAM_GARAGE` em loop/sem energia não mostra o presente. `CAM_F4_WEST` pode ser observado, mas **não** exibe N34 nos modos Fácil/Normal. Arquivo de N34 é item separado da transmissão atual e somente Difícil/Punitivo.

**Feedback:** online tem relógio/ruído de continuidade; loop repete um detalhe verificável; apagado informa perda de energia; interferência informa imagem insuficiente. Se Joe desligar circuito, a tela muda; ele não mantém conhecimento atualizado da área. Se uma porta da sala fizer barulho, o Hunter só reage se perceber de sua posição. Não colocar Batman na sala por tempo de permanência.

**Escolha:** usar imagem como indício, verificar em pessoa, ajustar energia, sair por `F2_SERVICE` ou voltar ao corredor principal. N10 registra o efeito da verificação; DOC09 explica a diferença sem dar certeza gratuita.

### N10 · Informação não é certeza — `F2_CAMERAS` / `F2_SERVER`

**Entrada:** Joe vê uma imagem duvidosa ou investiga servidor. **Situação jogável:** uma gravação repete o mesmo reflexo. O jogador pode comparar a hora com outra câmera, religar `C_SECURITY` ou visitar o corredor mostrado por rota real. Um registro antigo não revela o estado atual do lugar nem posição de Batman; no máximo prova que algo estava ali no passado.

**Ramos:** `verificou` dá conhecimento situado no momento da verificação; `não verificou` mantém hipótese; `não acessou câmeras` permite aprender essa regra pela porta DOC11 ou por observação posterior. Jack, se presente, pode chamar atenção para a repetição **se** ele viu os dois ciclos; sem isso não comenta. A lição é sistêmica, não palestra.

### N11 · Primeiro confronto aberto — escada/anel já visitado

**Entrada:** Batman recebeu evidência real (som, visão, objeto inspecionado ou caminho repetido), existe trajeto físico e pelo menos uma alternativa alcançável conforme portas/energia. É condicional: Joe silencioso pode terminar o ato sem este confronto e vê-lo mais tarde em outra área. Não disparar por “minuto 45”.

**Variações:** Joe escuta deslocamento e pode recuar por segunda escada, esperar, usar cobertura ou escolher risco; se um companheiro *presente* percebe o Hunter, ele pode alertar ou fugir segundo seu estado. Batman chega à última origem estimada, procura, erra ou perde o rastro. A área que Joe ocupava não vira destino onisciente. Uma porta fechada pode aumentar tensão; a alternativa física precisa ser conferida depois de aplicar energia e travas.

> [Ruído de tecido e bota em superfície molhada.]  
> Joe, baixo: “Pensa, Joe. Pensa.”

**Saída:** capturado é derrota segundo regras; escapar preserva evidência que Batman *realmente* recolheu, não uma marca global “agora sabe todas as rotas”. Se não houve evidência válida, N11 fica pendente sem bloquear o ato 3. Harley/Jack ausentes não entram para salvar Joe por exigência dramática.

## Continuidade com os atos seguintes

| Fato que pode ter ocorrido até N11 | Consequência posterior sem exigir caminho único |
| --- | --- |
| Harley tratou Joe, recebeu ajuda ou foi deixada | N23 e epílogo usam ação/último contato; ela pode agir sozinha. |
| Jack ajudou com informação verificável | N25 custa mais emocionalmente; J1, se testemunhada no Punitivo, conta para J4, mas perder J1 ainda deixa J2 possível. |
| Joe coletou DOC01 | N14 preserva o fragmento; sem DOC01, a garagem ainda pode ser preparada após retorno ao posto. |
| Joe consultou câmera ao vivo/loop | Conhecimento tem origem e tempo; blackout N22 invalida só observação corrente. |
| Batman recebeu ou não evidência | N21 reforça apenas hábitos observados. Sem evidência, não “herda” a escada preferida de Joe. |
| Joe ignorou Harley/Jack/N05/N09/N11 | Ato 3 permanece acessível; cenas perdidas não são inventadas como memórias testemunhadas. |

**Revisão criativa próxima:** leitura das falas de Joe/Harley, força da risada J1 e encenação espacial N03. As regras confirmadas pelo criador permanecem D-11; a encenação segue D-12. Nenhum tempo desta versão prova duração de 70 minutos antes da Unreal.
