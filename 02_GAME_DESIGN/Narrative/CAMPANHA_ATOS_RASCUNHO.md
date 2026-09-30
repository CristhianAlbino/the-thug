# The Thug — campanha de três horas, rascunho para revisão conjunta

**Estado: direção narrativa v0.2.** As decisões do criador registradas em 30/09/2026 na seção seguinte estão fixadas; encenação, ordem fina, falas e gatilhos continuam rascunho para acompanhamento. O [GDD Mestre](../GDD/GDD_MASTER.md) fixa a premissa, cinco personagens, Batman como Hunter, quatro fugas e os cinco atos conceituais. Este documento propõe uma sequência extensa e dinâmica para o Difícil, conectada ao [mapa lógico](../World/MAPA_LOGICO_HOSPITAL.md) e à [bíblia visual](../Visual/BIBLIA_VISUAL_HOSPITAL.md). IDs de cena são estáveis para roteiro e implementação; durações são **hipóteses**, sem playtest.

## Diretrizes confirmadas por Cristhian

| Tema | Regra narrativa aprovada |
| --- | --- |
| Thomas | Batman matou Thomas. **Fácil e Normal:** o jogador não recebe confirmação do destino. **Difícil e Punitivo:** mostrar de forma conclusiva que Batman o matou. A informação revelada varia com a dificuldade; a causa do evento não muda entre partidas. |
| Joe e Harley | Joe se importa com Harley, mas nos modos Fácil, Normal e Difícil demonstra menos cuidado que ela. Algumas omissões/frases dele devem incomodar o jogador. No Punitivo, essa assimetria muda: Joe demonstra cuidado proporcional e pode se expor por ela; ações do jogador ainda importam. |
| Jack | No Punitivo, apresenta roupa distinta, humor deslocado e risadas macabras cada vez mais perceptíveis ao longo da noite. A cadeia de pistas sustenta a revelação secreta do GDD; nos outros modos, não antecipar essa versão. |
| Hospital | Contar pelo ambiente, sem explicação expositiva. **Proposta aceita como possibilidade de direção:** ala clínica desativada associada ao antigo Asilo Arkham, sugerida por placas, arquivos e infraestrutura, sujeita ao blockout. |
| Companheiros | Harley, Bartholomeu e Jack têm histórias e epílogos individuais. Ações de Joe podem criar dilemas reais de ajudar, deixar para trás ou aceitar risco para salvar alguém. O jogador deve ter tempo de conhecê-los antes de enfrentar o custo. |

O Batman desta versão pode matar Thomas, embora suas derrotas usuais contra Joe continuem sendo captura/incapacitação. A morte é uma **exceção estabelecida pelo criador**, não licença para transformar o Hunter num assassino onipresente ou para informar Joe sobre ela no Fácil/Normal. O uso público dos personagens permanece na decisão D-10.

## Promessa dramática

Joe Chill foge do cerco policial com Harley Quinzel, Bartholomeu Kent, Jack Oswald White e Thomas Terrence. O hospital abandonado parece uma vantagem: corredores, muitos pisos, entradas técnicas. No quarto andar, o Bat-Sinal aparece distante; Thomas é arrancado do grupo por uma figura que Joe mal consegue distinguir. Batman não se apresenta como chefe de fase. Ele busca, ouve, observa, erra, aprende com evidências e pode interceptar rotas que Joe revelou. O medo nasce de **não saber o que ele sabe**. Sua vitória comum sobre Joe é captura/incapacitação, de acordo com o GDD.

O eixo emocional é a escolha entre informação, vínculo e sobrevivência. Joe pode ajudar Harley, dividir munição com Bartholomeu, acreditar em Jack, esconder rotas dos demais ou abandonar alguém. O jogo nunca transforma uma única escolha de diálogo em um medidor de bondade: ações, tempo, recursos e testemunhas alteram confiança e conhecimento separadamente. Algumas falas de Joe revelam seu egoísmo mesmo quando o jogador tenta fazer o certo; elas não anulam as ações de ajuda escolhidas.

## Ritmo proposto para a campanha principal (Difícil)

| Janela aproximada | Ato / tensão | Conteúdo variável que sustenta a duração |
| --- | --- | --- |
| 0–25 min | 1. Entrada e ruptura | Orientação no 4º, separação, primeira pista e um retorno diferente ao mesmo corredor |
| 25–70 min | 2. Compreensão | Clínica, câmeras, primeiros reencontros, ruído/evidência e um primeiro confronto evitável |
| 70–125 min | 3. Hospital como ferramenta | Emergência, administração, B1, energia, pistas cruzadas, tubos e rotas conhecidas |
| 125–165 min | 4. Pressão | Uma escolha ou falha pode alterar um circuito, uma relação e um caminho; recursos se tornam significativos |
| 165–180 min | 5. Escolha emergente | Fuga preparada, alternativa, amanhecer ou esconderijo; a corrida do helicóptero é uma opção |

São **faixas de observação para playtest**, não bloqueios por relógio. Quem souber a rota pode terminar antes; quem explora mais terá outras cenas e riscos. A duração não deve vir de uma porta sem pista, retorno vazio ou Hunter que sempre sabe onde Joe está. Diálogos acontecem enquanto se movimenta ou interage sempre que a situação permitir. O jogador pode iniciar preparos de fuga antes do ato 4.

## Estrutura de cenas

**Legenda:** `M` = marco obrigatório da premissa ou de uma rota escolhida; `C` = condicional ao estado; `O` = opcional. “Obrigatório” não significa cutscene que suspende sistemas. Nós são âncoras de autoria, não spawn garantido de NPC.

### Ato 1 — entrar, perder e reaprender o lugar

| ID | Área / modo | Proposta de cena e consequência |
| --- | --- | --- |
| N01 · Cerco | Entrada → `F4_START` · M | A polícia fecha o quarteirão. A equipe sobe rapidamente; pequenas falas revelam tensão pós-assalto. O exterior não vira uma quinta fuga livre. Joe percebe duas escadas e uma rota de serviço antes de perder o grupo. |
| N02 · Sinal | `F4_OBSERVATION` · M | Joe vê o Bat-Sinal entre chuva e prédios. Thomas ridiculariza o perigo; Harley tenta fazê-lo sair da janela. A vista do telhado cria memória espacial sem revelar a extração. |
| N03 · Ruptura | `F4_START` / `F4_HALL_E` · M | Uma forma vinda de cima arranca Thomas. A confusão separa Harley, Bartholomeu e Jack por direções diferentes. Nenhum botão de combate resolve essa primeira aparição. |
| N04 · Primeira escolha | `F4_HALL_E` ↔ `F4_HALL_W` · M | Joe aprende a andar baixo, escutar, esconder-se e escolher eixo de saída. Um objeto pode cair por ação sua; se Batman estiver perto e ouvir, ganha **estimativa** do som, não coordenada do jogador. |
| N05 · O que ficou | `F4_NURSE`, `F4_STORAGE` · O/C | Primeira parte da combinação e esconderijo entre lençóis. Vestígios de Thomas podem atrair Joe de volta. No Fácil/Normal, nada aqui confirma morte ou autoria. Quem retorna nota uma alteração física justificada por ator/evento, não um susto gratuito. |

**Função do ato:** fazer o jogador distinguir o que viu, ouviu e supôs. N03 não entrega o destino de Thomas imediatamente; no Difícil/Punitivo, a confirmação vem depois pela cena N34. Fácil/Normal mantêm o desaparecimento sem solução exibida.

### Ato 2 — descobrir que o hospital também observa

| ID | Área / modo | Proposta de cena e consequência |
| --- | --- | --- |
| N06 · Clínica | `F3_CENTRAL` → `F3_RECOVERY` · M | Primeiro anel de exploração. Cortinas, leitos e vidro oferecem cobertura com visão parcial; uma passagem para o oeste parece levar de volta a um lugar conhecido. |
| N07 · Harley | `F3_ICU` ou `F1_TREATMENT` · C | Harley aparece conforme sua trajetória e risco, não por teleporte ao objetivo de Joe. Cuida de Joe primeiro; fora do Punitivo ele pode responder com pressa e pouca atenção ao estado dela. Se Joe ajuda ou compartilha, a ação permanece válida apesar do jeito dele; se foge, ela pode sobreviver por conta própria e lembrar. No Punitivo, escrever a reciprocidade de Joe. |
| N08 · Análise de Jack | `F3_EXAMS` ou `F2_CAMERAS` · C | Jack nota que movimentos do Batman respondem a hábitos observáveis. Ele pode estar correto, incompleto ou manipular Joe, conforme o que realmente presenciou. No Punitivo, uma primeira graça inadequada e uma risada curta surgem quando a situação torna isso perturbador. Não entrega posição onisciente. |
| N09 · Monitores | `F2_SECURITY` → `F2_CAMERAS` · O/C | Joe encontra a sala de câmeras. `CAM_LOBBY` mostra o saguão **se** `C_SECURITY` estiver ligado e a câmera online. `CAM_CLINIC` pode estar interferida; `CAM_GARAGE` pode estar sem energia ou em loop. A sala é conhecida pelo Batman e não é segura. |
| N10 · Informação não é certeza | `F2_CAMERAS` / `F2_SERVER` · C | Um monitor mostra um corredor vazio, mas pode ser gravação antiga. O jogador pode conferir presencialmente, religar circuito ou desistir. Só a ação concreta de verificar transforma imagem em conhecimento confiável. |
| N11 · Primeiro confronto aberto | Escada ou anel já visitado · C | Batman surge após pista válida: barulho, visão, objeto inspecionado ou rota repetida. Pode perder Joe, investigar o lugar errado ou bloquear **uma** passagem alcançável por sua própria navegação. Companheiro presente pode alertar, fugir ou errar. |

**Função do ato:** tornar câmeras tentadoras e imperfeitas, revelar o padrão de investigação de Batman e oferecer pelo menos duas maneiras de sair de uma zona arriscada. Uma primeira visita silenciosa deve ser possível.

### Ato 3 — o hospital vira ferramenta e risco

| ID | Área / modo | Proposta de cena e consequência |
| --- | --- | --- |
| N12 · Medicamentos | `F1_LOBBY` → `F1_PHARMACY` · O/C | Tratamento visível através do átrio exposto. Um Joe ferido mede custo de atravessar; sem ferimento, o local vira recurso opcional. A câmera do lobby pode ajudar, enganar ou estar apagada. |
| N13 · A chave existe | `F1_SECURITY` · O/M garagem | Registro liga um veículo de serviço a uma chave guardada na Administração. Revela **existência**, não o código. |
| N14 · O cofre | `F2_SECURITY` / `F2_SAFE` · O/M garagem | Joe localiza o cofre e cruza três fragmentos de combinação: `F4_NURSE`, `F2_ARCHIVE`, `B1_GENERATOR`. Os seis dígitos e os textos finais continuam em aberto; cada pista tem referência espacial e segunda camada narrativa. |
| N15 · Bartholomeu | `F2_ADMIN`, `B1_PUMP` ou ligação entre ambos · C | Bartholomeu procura saída e recursos. Se vir túneis ou receber informação verificável, `LearnRoute("tunnel")`. Pode contar a Joe apenas se o encontrar e escolher compartilhar. Entregar munição é ação física: inventário de Joe diminui, Bartholomeu guarda a unidade e o vínculo muda. |
| N16 · Descenso industrial | `B1_CENTRAL` → `B1_GENERATOR` · M para energia | Tubos, água e gerador identificável pelo som. Restaurar `C_SERVICE` abre possibilidades de portão/câmera, mas pode produzir sinal audível ao Batman se estiver em alcance. Apagar `C_SECURITY` corta certas câmeras e uma porta elétrica; existe desvio pelo almoxarifado. |
| N17 · Prumada | `B1_PIPE` ↔ `F1_SERVICE` ↔ `F2_SERVICE` ↔ `F3_PIPE` · O/C | O mesmo duto reconhecível permite retorno inteligente. Joe entra devagar; o Hunter não atravessa a aresta estreita se ela não comporta seu corpo, mas pode investigar uma boca ou contornar por escada. |
| N18 · Janela e ferramenta | `F3_WINDOW` / `F2_SUPPLY` · O/M janela | A sala do anexo é identificada antes de poder ser atravessada. A ferramenta exige retorno e remoção silenciosa da trava. Uma porta trancada opcional em `F3_EXAMS` não elimina o acesso por `F3_HALL_W`. |
| N19 · Rádio incompleto | `F1_RADIO`, `F2_SERVER`, `F2_CAMERAS` · O/M telhado | Encontrar o rádio, buscar pilhas, montar e contornar interferência são ações distintas. A confirmação do helicóptero só acontece depois do contato; o relógio de cinco minutos começa ali. |
| N20 · Entrada pluvial | `B1_PUMP` → `B1_TUNNEL` · O/M túneis | Uma parede/acesso não assinalado leva à bifurcação. Dois ramais mortos ensinam que anel, corrente de ar e válvulas são pistas; no Punitivo, Robin pode investigar por episódio especial, sem segundo Hunter permanente. |

**Função do ato:** várias rotas podem ser preparadas em paralelo. A narrativa principal é o conhecimento adquirido pelo jogador e por cada companheiro, não uma lista de chaves com ordem única. Em uma campanha que escolheu túneis, o cofre pode ficar fechado; em outra, a janela pode nunca ser tocada. No Difícil/Punitivo, a cena transversal N34 pode ser descoberta aqui ou mais tarde; quem não a encontra recebe a coda após o desfecho.

### Ato 4 — pressão sobre pessoas e sobre caminhos

| ID | Área / modo | Proposta de cena e consequência |
| --- | --- | --- |
| N21 · Padrão observado | Escada usada repetidamente · C | Batman pode ganhar peso de evidência para uma escada **apenas** se testemunhar a rota ou vestígios específicos. Outra escada, elevador energizado ou prumada ainda oferece alternativa. |
| N22 · Falha de energia | `F2_SERVER` / `B1_GENERATOR` · C | Interação de Joe, acidente ou evento justificado apaga um circuito. Monitores perdem imagem; porta `needs_power` fecha acesso; porta `releases_when_off` pode ser aberta manualmente. O áudio do relé cria risco conforme alcance e mascaramento. |
| N23 · Decisão sobre Harley | `F3_RECOVERY` / `F1_TREATMENT` · C | Se ferida ou sob pressão, Harley pede recurso ou cobertura. Joe pode gastar o que tem, adiar e voltar, ou deixá-la. No Fácil/Normal/Difícil, uma reação fria de Joe pode irritar o jogador sem apagar a opção de agir melhor. No Punitivo, Joe nota o perigo dela e se dispõe a correr risco; o jogador ainda decide como. Harley conserva agência e não tem proteção garantida por vínculo. |
| N24 · Decisão sobre Bartholomeu | `B1_PUMP` / `G_MAINT` · C | Um caminho e munição podem ser divididos. Baixa confiança e Hunter realmente percebido abrem a opção sistêmica de Bartholomeu usar Joe como distração; ele também pode hesitar ou cooperar. A consequência precisa de trajetória física e aviso perceptível. |
| N25 · Jack e a dúvida | `F2_CAMERAS` / `F3_EXAMS` · C | Jack oferece inferência útil sobre padrão do Batman, possivelmente ocultando algo que sabe. No Punitivo, sua roupa diferente, humor deslocado e risadas em situações inadequadas tornam o padrão cada vez mais visível; a revelação secreta exige uma cadeia de pistas presenciadas. Nos outros modos não há pista que confirme essa identidade. Sua ausência ou morte não bloqueia qualquer fuga. |
| N26 · Uma rota quebra | Garagem, telhado, túnel ou janela · C | Portão bloqueado, helicóptero perdido, túnel interceptado ou janela exposta só ocorrem por condição concreta e comunicada. O jogador vê/ouve o estado e conserva outra rota ou alternativas de sobrevivência. Falha não reabre por carregar cena; checkpoint persiste. |
| N27 · Isolamento | `F3_UTILITY` → `F3_ISOLATION` · O/C | Descobrir a entrada pode dar abrigo relativamente seguro. Se Joe entrar sem ser visto, Batman não sabe. Se ele observar a entrada, suspeita aumenta. Vencer escondido requer o Hunter efetivamente abandonar a busca, não esperar um contador. |

**Função do ato:** a pressão altera a leitura de áreas familiares. Estado global não equivale a evento mágico: portas, companheiros, câmeras e Batman só mudam a partir de causas rastreáveis, com evidências diferentes para cada ator.

### Ato 5 — a saída que a partida construiu

| ID | Área / modo | Proposta de cena e consequência |
| --- | --- | --- |
| N28 · Garagem | `F2_SAFE` → `G_CONTROL` → `X_GARAGE` · C | Chave, energia e portão. Percurso previsível, mas com pontos expostos entre veículos. Batman pode tentar interceptar a rota se tiver evidência, não por saber que é o final. |
| N29 · Telhado | `F1_RADIO` → `R_PAD` → `X_ROOF` · C | Só após confirmação há aproximadamente 300 s. Dois eixos de escada; vento mascara parte dos sons. Se Joe chegar tarde, helicóptero vai embora e a campanha segue. |
| N30 · Túneis | `B1_TUNNEL` → `T_LOOP` → `X_TUNNEL` · C | Memória de válvulas/água importa mais que extensão artificial. Desvio errado custa tempo e recursos; Robin é possibilidade Punitivo previamente sinalizada. |
| N31 · Janela | `F3_WINDOW` → `A_BRIDGE` → `X_WINDOW` · C | Ferramenta, trava, ruído e travessia exposta de mão única. Depois de atravessar não há retorno ao hospital pelo mesmo caminho. |
| N32 · Outras vitórias | `F3_ISOLATION` ou local validado; amanhecer · C | Esconderijo exige busca realmente abandonada. Amanhecer só vence nas dificuldades permitidas; no Punitivo Joe continua procurando fuga ou abandono do Hunter. |
| N33 · Finais raros | Condições específicas do GDD · O | Confronto coordenado no Fácil e destruição no Punitivo são exceções custosas. Não usar como atalho central nem transformá-los em combate de chefe. |

**N34 · A verdade sobre Thomas (Difícil/Punitivo):** cena autorada na exploração de `F3_SURGERY`/`B1_MORGUE`, com uma prova inequívoca de que Batman causou a morte e de que o corpo é Thomas. Não tratar uma câmera possivelmente manipulada como única prova: cruzar corpo/vestígios físicos com um registro ou visão direta cuja origem o jogador possa verificar. Preparar entrada por caminhos diferentes, sem exigir a rota da garagem. Se o jogador nunca acessar a descoberta antes de fugir, uma coda final específica desses dois modos deve mostrar a verdade ao **jogador**, distinguindo o que Joe soube do que só o público viu. Fácil/Normal não executam N34 nem a coda. A encenação exata (e quanto Joe consegue fazer ao chegar) continua em revisão.

**Encenação de trabalho para N34:** em `B1_MORGUE`, Joe reconhece Thomas por peça/ferimento estabelecido na abertura; o corpo é uma constatação física, não uma nova vítima anônima. Um registro bruto da câmera `CAM_F4_WEST`, acessível na segurança, mostra em sequência contínua o Batman conduzindo Thomas depois de N03 e causando sua morte. Marcas e horário correspondem ao que Joe vê no necrotério. A fita não dá a posição **atual** de Batman, nem precisa estar ao vivo para ser prova de um evento passado; se a gravação for corrompida, a alternativa de descoberta física/visão direta precisa ser igualmente conclusiva. A câmera não pode mostrar uma sala fora do seu campo — ajustar o corredor de passagem no blockout. Se Joe pular esses lugares, a coda exibe ao jogador o trecho definitivo do registro após o final, enquanto Joe continua sem essa informação. No Fácil/Normal, o registro e o corpo identificável não aparecem como provas acessíveis nem na coda. Não travar nenhuma fuga para obrigar a visita ao necrotério.

## Personagens como trajetórias, não pontos fixos

| Personagem | Primeiro encontro proposto | Pressão / decisão relevante | Possíveis resultados sem fixar final |
| --- | --- | --- | --- |
| Harley | Clínica ou tratamento, dependente de percurso após a separação | Ela cuida de Joe mais do que recebe em Fácil/Normal/Difícil; no Punitivo ele demonstra cuidado equivalente, sujeito às ações | Cooperar, enfrentar desacordo, sair por conta própria, abandonar Joe, ser capturada; vínculo não a torna invulnerável |
| Bartholomeu | Administração, casa de bombas ou garagem | Conhecimento de rota, munição, abandono e medo do Batman | Compartilhar caminho, salvar Joe, fugir sozinho, mentir ou usá-lo como distração se os requisitos sistêmicos surgirem |
| Jack | Exames/câmeras, investigando padrão | Informação incompleta, utilidade e desconfiança; no Punitivo aparência, risos e humor mudam progressivamente | Aliado pragmático, mentiroso, ausente; revelação secreta Punitivo condicionada a pistas, nunca trava a fuga |
| Thomas | N03 em toda dificuldade; N34 apenas Difícil/Punitivo | Sua morte causada por Batman é fato do mundo, mas não conhecimento automático do jogador | Fácil/Normal terminam sem confirmação; Difícil/Punitivo revelam durante a exploração ou em coda se ele não viu a prova |

O sistema de companheiros parte de **variáveis separadas** de confiança, medo, estresse, lealdade, autopreservação e relacionamento; valores iniciais continuam na decisão D-02. O roteiro não declara “Harley sempre salva Joe” nem “Bartholomeu trai no minuto 140”. Os mesmos encontros podem mudar falas, prioridades e recursos de acordo com o estado. Nenhum NPC sabe uma rota porque Joe a descobriu: precisa de testemunho, observação ou comunicação.

### Fazer o jogador se importar antes do dilema

- **Harley:** N07 dá a ela uma ação competente — ela estabiliza um ferimento, reconhece um som ou identifica um acesso enquanto Joe ainda está confuso. Sua atenção constante a Joe aparece em gestos pequenos. Nos três primeiros modos, ele muitas vezes encerra a conversa assim que obtém a informação. Se o jogador ficar, ajudar e voltar, ela percebe **essa ação**, mesmo que Joe continue ruim de palavras. No Punitivo, Joe pergunta por ela e se oferece para cobrir um deslocamento, sem retirar o controle do jogador.
- **Bartholomeu:** ele não existe para trair. N15 pode trazer um recurso ou descobrir parte da rota por conta própria. Um gesto de munição e uma busca conjunta dão material para confiança real; a possibilidade de abandoná-lo em N24 dói justamente porque ele ajudou antes. Traição só aparece após condições compatíveis de medo, lealdade e informação.
- **Jack:** sua leitura dos padrões de Batman salva tempo ou evita perigo em pelo menos uma cena observável. Isso torna sua estranheza Punitivo uma ameaça emocional, não só uma reviravolta visual. Fora do Punitivo ele continua sarcástico e oportunista, sem pistas enganosas que prometam a identidade secreta.

### Progressão exclusiva de Jack no Punitivo

| Etapa | Momento e sinal visível | Condição de justiça |
| --- | --- | --- |
| J1 · Incômodo | Primeiro reencontro/N08: roupa diferente em corte e detalhes, risada breve após um comentário inadequado | A silhueta ainda permite reconhecê-lo; nenhum pop-up explica a pista |
| J2 · Dissonância | N15 ou reencontro intermediário: ele faz uma graça quando alguém fala de Thomas ou de uma ferida; a risada continua um pouco além do socialmente plausível | Só tocar se Jack estiver presente e ouvir o assunto; não vazar a pista por telepatia |
| J3 · Padrão | N25: diante do perigo de um companheiro, sua reação e um detalhe da roupa reaparecem de modo impossível de descartar como simples nervosismo | Deve haver alternativa de pista para quem não encontrou J2; sequência progride com a exploração |
| J4 · Revelação | Cena secreta opcional perto do desfecho, se Jack ainda estiver disponível e Joe presenciou evidências suficientes | A variante do GDD aponta para Jack/Coringa; não pode bloquear a fuga nem ocorrer automaticamente só por chegar ao minuto final |

O jogador deve poder desconfiar **antes** da revelação. Não exigir colecionar exatamente três pistas em ordem rígida: a regra final de suficiência será calibrada para caminhos diferentes. Fácil/Normal/Difícil não exibem roupa J1, risadas J1–J3 nem J4. A continuidade visual das roupas e a atuação de voz serão especificadas em assets/áudio após o texto de cena.

### Três dilemas sem botão “sacrificar”

| Situação | O que Joe pode fazer | Custo e consequência observável |
| --- | --- | --- |
| N23, Harley ferida perto da clínica | Compartilhar o último recurso, voltar por um caminho mais perigoso, combinar encontro ou sair sem ela | Recurso e tempo contra exposição; Harley pode sobreviver separada, ser encontrada por Batman ou lembrar que Joe voltou. O resultado depende de estado e percurso, não de uma morte automática ao deixá-la. |
| N24, Bartholomeu na casa de bombas | Entregar munição, abrir cobertura para ele, revelar o túnel ou guardar informação e seguir | Jogar junto pode expor Joe a ruído; abandoná-lo reduz confiança e pode levá-lo a uma saída própria ou traição contextual. |
| N25, Jack oferece passagem curta | Confiar nele, verificar a rota por câmera/visita ou recusar ajuda | Verificação custa tempo; confiar em informação não verificada pode custar posição e afetar o companheiro que esperava Joe. No Punitivo, a mesma cena reforça a suspeita sem antecipar J4. |

### Epílogos por pessoa, sobrepostos ao resultado da fuga

O jogo primeiro resolve **como Joe saiu** (ou não); depois monta blocos individuais segundo fatos persistidos. Não existe uma única vinheta “todo mundo morreu” que substitua a história de cada um. `desconhecido` é resultado legítimo quando Joe perdeu contato, não sinônimo automático de morto.

| Pessoa | Estado registrado | Batida emocional proposta |
| --- | --- | --- |
| Harley | Sai com Joe e confiança preservada | Ela lembra o momento em que Joe escolheu voltar; nos três primeiros modos pode cobrar as vezes em que ele a tratou como plano secundário. No Punitivo, a preocupação recíproca aparece sem tornar o final perfeito. |
| Harley | Escapa separada ou rompe vínculo | Ela age por conta própria. Joe sobrevive, mas a ausência dela tem peso específico; a relação não é restaurada só porque ambos vivem. |
| Harley | Capturada, morta ou paradeiro desconhecido | Três variantes distintas e condicionadas a fatos; o jogador vê qual escolha/consequência levou à última informação disponível. Evitar punir uma decisão cuidadosa com morte inevitável roteirizada. |
| Bartholomeu | Coopera e sai com Joe | Seu medo continua humano; a munição/ajuda compartilhada pode receber reconhecimento concreto. |
| Bartholomeu | Escapa sozinho após abandono ou conflito | Joe sabe que sobreviveram separados, mas não sabe se o outro o entregaria. Falas diferentes se houve traição testemunhada. |
| Bartholomeu | Capturado, morto ou desconhecido | Registrar o último encontro e evitar atribuir culpa automática se Joe nunca poderia alcançá-lo. |
| Jack | Ajuda, foge sozinho, é capturado ou desaparece | O epílogo responde ao que Joe de fato viu. No Punitivo, J4 altera a leitura de suas ações; sem J4, só indícios permanecem. |
| Thomas | Qualquer saída | Fácil/Normal: ausência sem resolução. Difícil/Punitivo: morte por Batman mostrada em N34 ou coda; epílogo distingue se Joe viu a prova. |

**Prioridade de implementação:** capturado ≠ morto ≠ paradeiro desconhecido; saída junto ≠ saída separada; testemunho de Joe ≠ informação mostrada só ao público. Flags e última observação são persistidas. O epílogo não precisa absolver Joe nem condená-lo por algo que o jogador não pôde controlar.

## Pacote de exploração opcional (sementes, não texto final)

| ID | Área | Informação e função | Obrigação |
| --- | --- | --- | --- |
| DOC01 · Troca de turno | `F4_NURSE` | Rotina do posto e fragmento A do código do cofre | Só para garagem |
| DOC02 · Arquivo de manutenção | `F2_ARCHIVE` | Fragmento B e referência cruzada a B1 | Só para garagem |
| DOC03 · Gerador | `B1_GENERATOR` | Fragmento C sem mostrar a ordem inteira | Só para garagem |
| DOC04 · Controle de veículos | `F1_SECURITY` | Existência de uma chave guardada | Só para garagem |
| DOC05 · Inventário da segurança | `F2_SECURITY` | Localização do cofre | Só para garagem |
| DOC06 · Croqui incompleto | `B1_PUMP` | Linha pluvial sem entrada exata | Opcional/túneis |
| DOC07 · Ordem de isolamento | `F3_UTILITY` | Indício de sala esquecida | Opcional/esconderijo |
| DOC08 · Chamadas falhas | `F1_RADIO` | Por que baterias e interferência importam | Só para telhado |
| DOC09 · Tela antiga | `F2_CAMERAS` | Distinguir câmera online de loop | Opcional/segurança |
| DOC10 · Nota da clínica | `F3_EXAMS` | Possibilidade física do anexo | Só para janela |
| DOC11 · Ocorrência de porta | `F2_SERVER` | Regra de fechamento em queda de energia | Opcional/planejamento |
| DOC12 · Entrada policial | `F1_LOBBY` | Quarteirão cercado; sair não significa segurança imediata | Opcional/ambiente |
| DOC13 · Transferência apagada | `F2_ARCHIVE` | Formulário antigo com origem clínica parcialmente raspada e carimbo de Arkham | Opcional/ambiente; não é explicação completa |
| DOC14 · Placa reaproveitada | `F3_HALL_W` | Duas camadas de sinalização sugerem que a ala mudou de administração antes do fechamento | Opcional/ambiente |
| DOC15 · Inventário antigo | `B1_STORAGE` | Equipamentos e tubulações numerados por um sistema diferente do hospital atual | Opcional/ambiente |
| DOC16 · Thomas (prova) | `B1_MORGUE` ou registro verificável em `F2_SECURITY` | Corrobora N34 e a autoria de Batman sem depender de câmera em loop | **Somente Difícil/Punitivo**; alternativa de coda garante revelação ao público |

Documentos opcionais ampliam o hospital e permitem suspeitar que houve uso recente. DOC13–DOC15 podem sugerir que o prédio foi uma **extensão clínica desativada do antigo Asilo Arkham**, sem um texto que resolva toda sua história. DOC16 não existe ou não revela a conclusão no Fácil/Normal; não usar o mesmo prontuário com texto “censurado” de modo artificial. Pistas obrigatórias de **uma rota escolhida** são rastreáveis e continuam acessíveis mesmo quando outra rota falha. A redação exata e a combinação de seis dígitos são pendências de coautoria.

### Gravações e falas de ambiente — ideias de primeira passada

| ID | Fonte | Conteúdo dramático proposto |
| --- | --- | --- |
| AUD01 | Segurança do 2º | Um operador relata que a câmera da clínica perde quadros; Joe aprende a desconfiar da tela sem receber a localização de Batman. |
| AUD02 | Manutenção de B1 | Voz orienta pelo ruído da bomba e por duas válvulas diferentes; ramal morto vira aprendizado, não truque arbitrário. |
| AUD03 | Telefone da emergência | Uma mensagem antiga indica evacuação incompleta, sustentando a sensação de abandono irregular. |
| AUD04 | Rádio policial parcial | Gordon mantém o quarteirão isolado; Joe entende por que a rua é arriscada. Diálogo completo depende de direção de cena. |
| AUD05 | Comunicação interceptada parcial | Uma voz de Alfred pode mencionar tempo e movimentação sem expor plano inteiro de Batman. |
| AUD06 | Mensagem de companheiro | Origem, hora e alcance determinam se Joe a ouve; um recado pode estar desatualizado. |
| AUD07 | Jack/Punitivo | Riso deslocado muda de breve para longo em encontros sucessivos; só se Jack estiver presente. Não usar como alarme de perigo automático. |
| AUD08 | Thomas/Difícil/Punitivo | Fragmento de registro ligado a N34 corrobora a prova física e a ação de Batman; não aparece nem é legível no Fácil/Normal. |

Esses áudios precisam ser localizados no mundo, audíveis sob condições próprias e úteis mesmo sem legenda perfeita. A mixagem e as falas finais serão trabalhadas depois da revisão da campanha.

## Diálogo de teste para revisar voz (não texto final)

**N07, reencontro com Harley, Joe ferido:**

> Harley: “Você atravessou esse corredor sozinho?”  
> Joe: “Atravessei.”  
> Harley: “Então por que tá segurando o braço?”  
> Joe: “Achei você. É o que importa agora.”  
> Harley: “Não. Importa se você vai conseguir correr quando ele voltar.”

**Resposta adicional fora do Punitivo, se Joe quiser seguir logo:**

> Harley: “Eu também me cortei naquela sala.”  
> Joe: “Você ainda consegue andar. Vamos.”  
> Harley: “Consigo. Era só isso que você precisava saber?”

**Variante Punitivo no mesmo ponto, sem entregar recurso automaticamente:**

> Harley: “Eu também me cortei naquela sala.”  
> Joe: “Deixa eu ver. Se você não conseguir passar, eu volto pelo outro corredor.”  
> Harley: “Você falou ‘volto’ mesmo?”

Essas falas mostram a diferença de Joe, mas não decidem pelo jogador se ele dará recurso, ficará para ajudar ou partirá. Se suas ações anteriores já foram cuidadosas, a variante fria deve reconhecer o esforço concreto sem transformar Joe de repente em alguém indiferente a tudo.

**N15, Bartholomeu conhece um caminho, mas não confia o bastante:**

> Bartholomeu: “Tem um caminho perto das bombas. Talvez saia do outro lado.”  
> Joe: “Talvez?”  
> Bartholomeu: “Eu vi a entrada, Joe. Não vi a saída. Se quer certeza, vem comigo.”

**N08, Jack observa comportamento do Batman:**

> Jack: “Na primeira vez, ele veio pelo barulho. Na segunda, pela escada.”  
> Joe: “Tá dizendo que ele sabe para onde eu vou?”  
> Jack: “Tô dizendo que você acabou de me contar qual escada costuma usar.”

O subtexto de Jack depende de ele ter visto as duas ocasiões. Se não viu, esta fala não pode tocar; uma versão alternativa deixa claro que é apenas hipótese. Batman fala raramente. Seu silêncio, a posição desconhecida e as consequências físicas contam a maior parte de sua presença.

**N34, reação à descoberta de Thomas (só Difícil/Punitivo, fala de trabalho):**

> Joe: “É o Thomas.”  
> [Joe confere a evidência e entende o que Batman fez.]  
> Joe: “Ele não levou o Thomas daqui. Ele o matou.”

O texto só toca quando Joe presenciou prova inequívoca. Se a revelação vier apenas pela coda, Joe não pronuncia uma conclusão que nunca aprendeu. A cena final deve mostrar ao público o ato/resultado de forma conclusiva, sem atribuir a Joe conhecimento retroativo.

## Regras de direção para eventos dinâmicos

1. **Causa antes do efeito:** ruído tem fonte/tempo/atenuação; Batman só recebe um `Stimulus` após percepção. Câmera em loop não informa presente. Mudança do mundo só vira evidência quando ele inspeciona o objeto.
2. **Nenhuma perseguição de roteiro inescapável:** a primeira ruptura é premissa cinematográfica; depois Batman usa rotas fisicamente válidas e pode falhar. Se uma porta for emperrada, sinal e alternativa precisam existir.
3. **Conhecimento por pessoa:** Joe, Harley, Bartholomeu, Jack e Batman mantêm fatos diferentes. A cena N15 exige localização/comunicação para transferir a pista de Bartholomeu a Joe.
4. **Consequência reversível quando possível:** circuito pode ser restaurado; porta manual pode ser reaberta; falha definitiva de fuga requer evento claro. Uma traição não é sorteio solto; opções surgem de estado e circunstância.
5. **Cobertura e legibilidade:** vermelho do 4º e dos alarmes, preto profundo da clínica e verde úmido da manutenção vêm dos cinco vídeos, mas cada rota precisa de silhueta, marco e feedback legíveis em primeira pessoa.
6. **Duração conquistada:** usar reinterpretação de espaços, encontros condicionais, preparação paralela e consequências. Medir a proporção exploração/stealth/interação/retorno; cortar corredores vazios após playtest.

## Decisões confirmadas e execução ainda em revisão

| Questão | Direção confirmada / detalhe pendente | O que muda |
| --- | --- | --- |
| Thomas | Fácil/Normal sem resposta; Difícil/Punitivo confirmação de morte por Batman. **Pendente:** encenação final de N34/coda e opções de Joe ao chegar. | Gatilhos, prova física e falas sem vazamento entre dificuldades |
| Relação Joe–Harley | Amor assimétrico em Fácil/Normal/Difícil, recíproco no Punitivo. **Pendente:** intensidade e quantidade de falas frias para não invalidar ações cuidadosas. | Atuação e epílogos por dificuldade/decisão |
| Jack no Punitivo | Humor cruel, risadas e roupa diferente ficam progressivamente claros. **Pendente:** suficiência das pistas e forma de J4. | Atuação, assets e segredo opcional |
| Hospital | Possível ala clínica desativada associada a Arkham, narrada pelo ambiente. **Pendente:** sinais físicos exatos no blockout. | Arquivos, placas e materiais |
| Companheiros | Epílogos individuais e dilemas de ajudar/abandonar/sacrificar. **Pendente:** condições específicas de captura, morte, separação e último testemunho. | Variantes de cena e estado persistido |

As diretrizes da primeira seção estão aprovadas; o restante continua rascunho para revisão de encenação e ritmo com Cristhian. Redigir cenas completas, falas condicionais, documentos e gravações por ID; cruzar cada gatilho com flags, estados do mundo e testes. A duração final depende de playtest.
