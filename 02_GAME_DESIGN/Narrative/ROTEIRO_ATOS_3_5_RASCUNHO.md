# The Thug — roteiro de trabalho, atos 3 a 5

**Rascunho v0.1 para revisão com Cristhian.** Desdobra N12–N34 da [campanha](CAMPANHA_ATOS_RASCUNHO.md), seguindo o [roteiro dos atos 1 e 2](ROTEIRO_ATOS_1_2_RASCUNHO.md). A numeração organiza autoria; **não exige executar cenas em ordem numérica**. Energia, portas, relógio, rotas, companheiros e Batman reagem a ações/observações. Gatilhos exatos, valores e falas finais ficam em D-12 ou nas decisões técnicas correspondentes.

## Ato 3 — aprender a usar o hospital

### N12 · Medicamentos — `F1_LOBBY` / `F1_PHARMACY`

**Entrada:** Joe atravessa a emergência e vê recursos atrás do vidro. Ferido, tem motivo imediato; ileso, enxerga uma reserva que pode guardar ou usar por um companheiro. A farmácia é acessível por `F1_TRIAGE` e porta `D_F1_PHARMACY`; o saguão exposto dá contexto de risco, não acesso direto inventado. Abrir a porta pode ser ouvido se houver Hunter próximo e percepção válida.

**Escolhas:** pegar o necessário, guardar espaço no inventário, deixar algo para um reencontro com Harley/Bartholomeu ou evitar a travessia. Nada aqui cura automaticamente. Se a câmera do lobby mostrou vazio antes, a validade daquela imagem termina no instante observado; ela não garante passagem segura agora. Um detalhe de limpeza recente numa prateleira reforça o enigma ambiental sem explicar quem manteve o lugar.

### N13 · A chave existe — `F1_SECURITY`

**Entrada:** Joe encontra o controle de veículos e a pista `key_known`. Um registro liga o veículo de serviço à chave guardada num cofre da Administração; não dá código nem cria chave no inventário. Se Joe ignora o documento, pode voltar depois. O rádio da polícia pode ser ouvido parcialmente, mas a rua não vira saída livre.

> Joe: “Tem carro lá embaixo. Claro que a chave ficou no andar de cima.”

**Saída:** N14 passa a ter motivo narrativo. A garagem pode ser avistada antes disso, mas `locate_safe` no mapa exige a informação de N13. Essa dependência é progressão de **uma** fuga, não bloqueio das demais.

### N14 · O cofre — `F2_ARCHIVE` / `F2_SECURITY` / `F2_SAFE`

**Entrada:** após `key_known`, um inventário em `F2_SECURITY` dá `safe_known`; o cofre fica em `F2_SAFE`, ligado a `F2_ADMIN`. A combinação tem fragmentos `code_a` no posto F4, `code_b` no arquivo F2 e `code_c` no gerador B1. Ler um fragmento nunca preenche sozinho os demais; no Fácil as pistas podem ser mais legíveis sem alterar a origem.

**Escolhas:** reunir evidências, anotar fora da interface se preferir, abandonar temporariamente a garagem e preparar outra fuga. Se a segurança perdeu energia, o inventário físico e o trajeto por `F2_ADMIN` permanecem inspecionáveis. Só concluir `open_safe` e `take_garage_key` transfere a chave. Persistir ambas as ações para que carregar save não replique item.

**Texto de ambiente provisório:** “Controle de frota: chave de serviço recolhida ao cofre da administração. Referência dividida entre turno, arquivo e manutenção.” A redação e os seis dígitos finais continuam D-07.

### N15 · Bartholomeu — `F2_ADMIN` ou `B1_PUMP`

**Entrada:** Bartholomeu realmente alcança um desses pontos e Joe chega ao mesmo nó; se não se encontram, Joe aprende rotas por exploração. Bartholomeu traz algo útil: viu a entrada da linha pluvial **sem saber a saída**, guardou munição ou percebeu a casa de bombas. Não pode comunicar localização do Batman que não viu.

> Bartholomeu: “Tem um caminho perto das bombas. Talvez saia do outro lado.”  
> Joe: “Talvez?”  
> Bartholomeu: “Eu vi a entrada, Joe. Não vi a saída. Se quer certeza, vem comigo.”

**Escolhas no espaço:** procurar a bomba junto, dividir munição efetiva, contar uma rota que Joe verificou ou seguir separado. Troca de munição é operação única no inventário e CompanionCore. Se Bartholomeu não ouviu/viu a descoberta da rota, `LearnRoute("tunnel")` não é aplicado. A confiança construída aqui sustenta o dilema N24; não declarar traição antecipada.

**Jack/J2 Punitivo:** se Jack **está presente e ouviu** uma menção a Thomas ou ao ferimento de alguém, lança uma graça cruel e ri tempo demais. Sua roupa traz outra alteração visível. J2 é testemunhada só se Joe ouviu/viu; pode aparecer num reencontro equivalente do ato 3 caso N15 ocorra sem Jack. Fora do Punitivo, não tocar esta variante. Perder J1 não impede J2.

### N16 · Descenso industrial — `B1_CENTRAL` / `B1_GENERATOR`

**Entrada:** o ronco da casa de máquinas orienta a chegada ao gerador. Joe pode ler `code_c` sem prometer abrir a garagem. Restaurar energia concede `power_restored`; o relé produz um sinal físico cuja propagação depende da posição, oclusão e mascaramento. Energizar altera portas e câmeras pelos circuitos reais, não todos os dispositivos do mapa.

**Risco:** `C_SECURITY` e `C_SERVICE` têm papéis separados. A porta `D_F2_SERVER` depende de energia, mas há outro caminho pelo suprimento; a `D_G_CONTROL` libera manualmente sem energia. Essas rotas exigem legibilidade visual no blockout. Se Joe desliga circuito depois, não desfazer pistas ou chave já coletadas.

### N17 · Prumada — `B1_PIPE` → `F1_SERVICE` → `F2_SERVICE` → `F3_PIPE`

**Entrada:** Joe encontra uma boca de tubulação acessível pelo grafo. O metal transmite vibração; ele pode atravessar devagar para economizar risco ou sair por uma abertura anterior. A passagem estreita não comporta o Hunter nessa aresta, mas ele pode perceber a entrada, suspeitar de uma saída ou contornar por escada se tiver evidência. Não adquirir a posição final por telepatia.

**Função:** tornar o hospital grande pela relação entre andares, e não por corredores vazios. Uma volta bem planejada reduz caminho para DOC01, ferramentas ou companheiro. Travessia precisa ser checada contra portas/ocupação no editor.

### N18 · Janela e ferramenta — `F3_WINDOW` / `F2_SUPPLY`

**Entrada:** Joe vê o anexo de `F3_WINDOW` e obtém `window_found`. A trava não cede sem ferramenta localizada em `F2_SUPPLY`, alcançável pelo serviço e, conforme estado de energia, pelo servidor. A porta trancada `D_F3_EXAMS` não elimina o acesso de `F3_HALL_W` à janela.

**Escolhas:** carregar ferramenta ocupando espaço, voltar para a janela ou manter o anexo como plano de reserva. Só `unblock_window` registra o ruído da trava; olhar a janela não alerta Batman. Cruzar é mão única e só ocorre em N31. A visão da ponte no exterior deve comunicar exposição e ausência de retorno.

### N19 · Rádio incompleto — `F1_RADIO` / `F2_SERVER` / `F2_CAMERAS`

**Entrada:** rádio avariado; `radio_found` e `batteries_found` são estados separados. Joe encontra baterias no servidor, remove interferência na segurança, instala baterias na sala do rádio e estabelece contato. O helicóptero **não está confirmado** quando apenas o rádio liga.

> Voz no rádio: “Repete o ponto de retirada. Não vou entrar às cegas.”  
> Joe: “Telhado do hospital. Confirma que vem.”

**Saída:** `helicopter_confirmed` inicia o prazo de aproximadamente 300 segundos; o cronômetro não começa ao achar pilhas, subir escadas ou abrir o rádio. Se o prazo expira, a perda do telhado permanece, enquanto garagem, túnel, janela e outras condições ainda podem ser buscadas. O tempo e a velocidade final são hipóteses até teste físico.

### N20 · Entrada pluvial — `B1_PUMP` / `B1_TUNNEL`

**Entrada:** uma marca de nível d'água e uma vibração orientam Joe até a entrada oculta. `tunnel_found` registra o acesso; ramais mortos informam pela corrente de ar, som e diferença de válvulas, em vez de serem puro labirinto arbitrário. Bartholomeu pode saber da entrada antes de Joe, mas o jogador só a conhece ao ouvir informação transferida ou descobri-la.

**Variação Punitivo:** um episódio raro de Robin, previamente sinalizado por vestígio/rádio, pode alterar a investigação do ramal. Não há segundo Hunter permanente nem teleporte; a tentativa de fuga continua testável sem esse episódio. Acesso aos túneis não depende de completar garagem/telhado.

## Ato 4 — o custo das escolhas anteriores

### N21 · Padrão observado — escada realmente usada

**Entrada:** Batman inspecionou evidência de uma passagem ou viu Joe repetir o mesmo eixo. Ele pode preferir a escada correspondente e chegar antes por caminho plausível. Se não viu, não há cena de antecipação: o jogo não pune hábito que só o sistema sabe. Outra escada, elevador no estado correto ou prumada podem oferecer alternativa conforme o mundo.

**Legibilidade:** um som, sombra, marca examinada ou desvio visível deve permitir ao jogador entender *como* sua escolha ficou conhecida. Jack pode suspeitar do padrão, mas seu comentário continua hipótese se não testemunhou as ocorrências.

### N22 · Falha de energia — `F2_SERVER` / `B1_GENERATOR`

**Entrada:** Joe desliga algo, força uma porta ou um evento físico justificado derruba um circuito. `C_SECURITY` apagado invalida câmeras dependentes dele; `C_SERVICE` afeta seus próprios dispositivos. Uma porta `needs_power` deixa de abrir normalmente; `releases_when_off` pode ser operada manualmente. Incluir estado de cada porta em feedback de maçaneta, luz e som.

**Escolhas:** religar no gerador sob risco, usar suprimento/prumada, permanecer na escuridão para reduzir exposição ou mudar a fuga preparada. Blackout não é cortina para surgir Batman atrás do jogador; deslocamento e percepção ainda valem. A conversa com companheiro presente pode ser cortada, com evento pendente e estado persistido.

### N23 · Decisão sobre Harley — `F3_RECOVERY` / `F1_TREATMENT`

**Entrada:** Harley ferida ou sob pressão numa trajetória concreta; Joe recebe aviso se está próximo, por voz alcançável ou recado com origem/tempo. Não forçar ferimento só para esta cena existir. Se ela está ilesa, a decisão equivalente pode ser cobrir sua travessia enquanto outra rota de fuga se fecha.

**Opções executadas:** gastar último tratamento, abrir cobertura e fazer ruído, combinar reencontro e voltar, ou deixá-la seguir sozinha. Salvar Harley pode consumir tempo ou expor Joe; abandonar pode permitir que ela sobreviva separada, seja capturada após encontro real ou tenha paradeiro desconhecido. Nenhuma opção causa morte automática fora da simulação.

**Fácil/Normal/Difícil, Joe é frio mesmo se se importa:**

> Harley: “Você ouviu quando eu pedi ajuda?”  
> Joe: “Ouvi. Estou vendo se dá pra passar.”  
> Harley: “Eu perguntei de mim, Joe.”

**Punitivo, Joe demonstra reciprocidade:**

> Harley: “Eu consigo ir. Você não precisa voltar.”  
> Joe: “Preciso saber que você chegou. Me diz por onde eu te encontro.”

As ações do jogador têm precedência no epílogo: uma fala pobre não apaga o recurso que ele realmente deu; uma fala cuidadosa não salva Harley se Joe a abandonou. Se ela nunca foi encontrada, N23 não inventa reencontro. Registrar última observação separada de destino final.

### N24 · Decisão sobre Bartholomeu — `B1_PUMP` / `G_MAINT`

**Entrada:** ele está presente, viu um risco e pode escolher cooperar ou buscar fuga própria. Se Joe dividiu munição em N15, Bartholomeu pode usá-la para cobertura **apenas** com linha de ação possível e sem transformá-la em vitória automática contra Batman. Se nada recebeu, não disparar tiro fictício.

> Bartholomeu: “Eu segurei a porta por você quando entramos.”  
> Joe: “E agora?”  
> Bartholomeu: “Agora é você que decide se deixa ela aberta.”

**Opções:** revelar o túnel, abrir passagem, emprestar munição em transferência única, guardar rota e avançar, ou esperar por ele. Confiança e medo orientam sua decisão: cooperação, hesitação, fuga separada ou uso de Joe como distração **somente** se Hunter percebido e condições sistêmicas compatíveis. O jogador tem aviso de ruído/movimento; consequência não nasce de rolagem invisível sem causa. Sua captura, morte ou desaparecimento requer evento próprio, nunca simples separação.

### N25 · Jack e a dúvida — `F2_CAMERAS` / `F3_EXAMS`

**Entrada:** Jack está ativo, presente e dispõe de uma inferência ou caminho útil que realmente investigou. Ele oferece um atalho; Joe pode confiar, conferir no mundo ou recusar. A verificação custa tempo, mas pode preservar confiança ao provar que Jack acertou. Se ele mente, registrar o que disse, o que sabia e qual testemunho revela a mentira. Sua ausência não bloqueia fuga.

**Punitivo/J3:** diante de perigo real para outro companheiro, Jack reage com graça cruel e risada longa fora de proporção; peça marcante da roupa reaparece. Não mostrar o segredo por texto flutuante. A pista J3 pode ser testemunhada mesmo se J1 ou J2 foi perdida; J4 exige J3 **e** uma das anteriores. Se Joe não o encontra no ato 4, nenhuma pista é concedida por carregar cena.

> Joe: “Ele pode estar preso lá.”  
> Jack, rindo tarde demais: “Então alguém enfim achou um quarto reservado.”  
> Joe: “Isso é engraçado pra você?”

Fora do Punitivo, Jack mantém sarcasmo analítico, sem figurino/risada da cadeia secreta. Seu conhecimento não dispensa evidência espacial.

### N26 · Uma rota quebra — estado de saída específica

**Entrada:** uma fuga preparada falha por evento observável: portão que não liberou, helicóptero que deixou o telhado após confirmação/prazo, ramal interceptado fisicamente ou janela que se tornou arriscada por perseguição. Usar flags independentes; uma rota não quebra porque outra falhou.

**Resposta:** áudio/objeto mostra o que aconteceu, Joe pode procurar um plano restante ou esconder-se. Uma porta inacessível tem caminho/ação alternativa antes do bloqueio definitivo; testar combinações com WorldCore/MapCore. Salvar e carregar preserva a falha e evita relógio “resetado” por cena. Não aplicar fracasso obrigatório para alongar o jogo.

### N27 · Isolamento — `F3_UTILITY` / `F3_ISOLATION`

**Entrada:** Joe encontra a indicação de acesso em `F3_UTILITY`; `isolation_known` habilita a descoberta de `F3_ISOLATION` pelo corredor oeste. Se entrar sem ser visto nem produzir evidência rastreável, Batman não sabe. Se o Hunter presenciou entrada, pode investigar a porta e não deve abandonar busca por um contador cego.

**Escolha:** aguardar silêncio, sair antes, arriscar outra fuga. Vitória por esconderijo exige abandono efetivo da busca e condições do GDD; limiar e duração são D-08. Um companheiro escondido ali só existe se entrou fisicamente, e seus ruídos/decisões importam.

## Ato 5 — cada partida constrói uma saída

### N28 · Garagem — `F2_SAFE` → `G_CONTROL` → `X_GARAGE`

**Pré condições:** `garage_key`, `power_restored`, portão liberado por interação em `G_CONTROL`. A chave foi retirada uma vez do cofre, não nasce ao chegar à garagem. Joe precisa atravessar veículos/exposição até o exterior; um companheiro só sai junto se percorreu a rota e chegou à saída. Batman pode interceptar se percebeu ação/trajeto, não porque leu uma flag de final.

**Falha:** portão não libera se faltou estado; informação clara permite voltar, restaurar energia ou migrar para outra rota. Se o portão estava liberado e Joe foi capturado no caminho, não transformar o fato em `garage_escaped`.

### N29 · Telhado — `F1_RADIO` → `R_PAD` → `X_ROOF`

**Pré condições:** contato e confirmação já registrados em N19; aproximadamente cinco minutos desde `helicopter_confirmed`. A sirene/vento mascara sons conforme percepção; duas escadas oferecem escolhas se uma está arriscada. O helicóptero não aguarda Joe indefinidamente por compaixão da narrativa.

**Escolha humana:** cobrir companheiro que chega mais devagar ou embarcar quando a aeronave ainda está ali. Perder a janela de embarque produz consequência própria e deixa outras rotas; não matar o companheiro automaticamente. Só quem alcançou `X_ROOF` embarca. O relógio deve persistir em save/load.

### N30 · Túneis — `B1_TUNNEL` → `T_LOOP` → `X_TUNNEL`

**Pré condição:** `tunnel_found`; leitura de válvula, fluxo e som ajuda a escolher ramal. Entrar num beco sem saída custa deslocamento, não um susto inevitável. Hunter/Robin só alcançam posições por arestas e eventos próprios. Um companheiro que conhece a entrada pode orientar Joe se houver comunicação, mas não sabe a saída sem a ter explorado.

**Epílogo:** separar-se no túnel sem testemunha marca último contato, não morte. Joe pode sair sem todos e carregar o peso de ter escolhido velocidade; estados observados do grupo continuam individuais.

### N31 · Janela — `F3_WINDOW` → `A_BRIDGE` → `X_WINDOW`

**Pré condições:** `window_unblocked` após ferramenta e ruído de interação. Travessia exposta de mão única. Se Harley/Bartholomeu/Jack ficaram para trás, não aparecerão no anexo por montagem; precisam cruzar por sua própria ação antes que Joe se afaste. O jogador deve perceber claramente que não conseguirá voltar pela mesma janela.

### N32 · Outras vitórias — esconderijo ou amanhecer

**Esconderijo:** só conclui se Hunter efetivamente abandonou a busca por aquela área, com suas últimas pistas verificáveis. **Amanhecer:** apenas nas dificuldades que o permitem conforme RunRules; Punitivo mantém a necessidade de fugir ou obter desistência real. Estar numa sala “segura” não substitui o estado do Hunter nem o critério de dificuldade.

**Pessoas:** a vitória de Joe resolve sobrevivência imediata, não todos os destinos. Montar epílogos a partir de fatos observados e estados persistidos. Se Joe foi capturado antes de amanhecer/abandono, não conceder vitória retrospectiva.

### N33 · Finais raros — circunstâncias verificadas

**Fácil:** confronto coordenado é exceção se companheiros confiáveis, recursos e posições permitirem. **Punitivo:** destruição é exceção custosa segundo o GDD. Nenhum vira opção fixa de menu ou chefe com barra de vida. Cada participante mantém o resultado próprio; o fato de ajudar Joe não garante sobrevivência no epílogo, nem a falha mata a equipe por decreto.

### N34 · A verdade sobre Thomas — `B1_MORGUE` / `F2_SECURITY`

**Disponibilidade:** Difícil e Punitivo. Fácil/Normal nunca exibem corpo identificado, arquivo inequívoco, fala conclusiva ou coda. O fato do mundo é o mesmo em todos os modos: Batman matou Thomas; muda a informação entregue ao jogador. Joe só recebe `joe_knows:thomas_killed_by_batman` quando presenciou prova suficiente.

**Descoberta de trabalho:** em `B1_MORGUE`, o corpo é identificado por traço que o jogador viu na abertura (relógio riscado, com confirmação adicional). O registro arquivado acessível em `F2_SECURITY` mostra Batman conduzindo Thomas por um corredor que a câmera `CAM_F4_WEST` **de fato pode ver** e causando sua morte; marcas/horário cruzam com o necrotério. A gravação é prova passada, nunca imagem ao vivo do Hunter atual. Essa passagem e visada têm de ser verificadas no blockout; se falharem, trocar a encenação por prova física/visão direta igualmente inequívoca. O jogador pode chegar ao arquivo antes do corpo, sem que uma metade isolada e ambígua gere a conclusão de Joe. As duas partes podem ser revisitadas sem bloquear nenhuma fuga.

> Joe, depois de cruzar a prova: “É o Thomas. Ele não levou o Thomas daqui. Ele o matou.”

**Coda alternativa:** se Joe terminou sem ver a prova, mostrar ao **público** a morte e autoria após o desfecho nos modos Difícil/Punitivo; Joe continua sem esse conhecimento. A coda não muda fuga, inventário ou relações. A encenação exata, a cadeia que moveu o corpo e o que Joe pode fazer ao encontrá-lo seguem em D-12 para revisão com Cristhian.

## Revelação J4 e epílogos — após resolver a saída

**J4 é opcional e exclusiva do Punitivo.** Jack está presente/disponível, Joe testemunhou J3 e pelo menos J1 ou J2, e o encontro acontece em ato 5 com rota física. Uma discussão pode ocorrer antes de sair ou, se as condições permitirem, numa passagem final. O segredo do GDD (Jack/Coringa) se revela por atuação e fala; não surge como texto de menu, nem impede Joe de fugir. Se Jack estiver ausente, capturado ou faltarem pistas, o jogo deixa a suspeita sem confirmação.

> Joe: “Quem diabos é você?”  
> Jack: “Você passou a noite inteira olhando para o homem no teto. Nem uma vez olhou para quem andava ao seu lado?”

**Montagem de epílogo:** resultado da fuga + um bloco para Harley, Bartholomeu, Jack e, conforme dificuldade, Thomas. `saiu_com_Joe`, `escapou_separado`, `capturado`, `morto` e `desconhecido` não se substituem. Um personagem morto requer evidência própria; último contato rompido é desconhecimento. O Punitivo muda tom da relação Joe/Harley sem garantir reconciliação; J4 muda a leitura de Jack só se Joe/jogador presenciaram. A coda de Thomas pode informar exclusivamente o público. Texto final e imagens dependem de checagem dos fatos persistidos.

**Exemplos de fechamento, ainda não texto final:**

| Situação observada | Batida do epílogo |
| --- | --- |
| Harley sai com Joe após ele voltar por ela | Ela recorda o retorno; em Fácil/Normal/Difícil ainda pode cobrar as vezes em que ele não a ouviu. No Punitivo Joe responde com preocupação concreta, sem transformar a relação em prêmio automático. |
| Harley foge sozinha depois de Joe deixá-la | Uma confirmação de sua fuga resolve sua sobrevivência, enquanto a relação pode continuar rompida. Sem confirmação, usar `desconhecido`, não `escapou_separado`. |
| Bartholomeu sai com Joe após dividirem munição | Ele devolve ou reconhece um gesto específico de N15/N24; medo e lealdade não se resumem a uma frase genérica. |
| Bartholomeu é capturado após uma distração vista | O epílogo usa esse fato, não uma narração que imputa a Joe uma morte não observada. |
| Jack desaparece sem J4 | A suspeita permanece aberta no Punitivo; fora dele, não sugerir retroativamente a identidade secreta. |
| Thomas em Fácil/Normal ou Difícil/Punitivo sem descoberta | Nos dois primeiros não mostrar resolução; nos dois últimos a coda confirma ao jogador e não inventa conhecimento de Joe. |

**Verificações para o futuro editor:** tempo e visada N34; legibilidade da janela de cinco minutos; ocupação física de cada companheiro nas saídas; cobertura e acústica dos corredores; possibilidade de uma fuga alternativa após N26. O documento descreve intenção e relações causais, não valida geometria, duração ou atuação final.
