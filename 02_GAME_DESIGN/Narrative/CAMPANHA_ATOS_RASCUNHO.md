# The Thug — campanha de três horas, rascunho para revisão conjunta

**Estado: proposta narrativa v0.1, não cânone fechado.** O [GDD Mestre](../GDD/GDD_MASTER.md) fixa a premissa, cinco personagens, Batman como Hunter, quatro fugas e os cinco atos conceituais. Este documento propõe uma sequência extensa e dinâmica para o Difícil, conectada ao [mapa lógico](../World/MAPA_LOGICO_HOSPITAL.md) e à [bíblia visual](../Visual/BIBLIA_VISUAL_HOSPITAL.md). A versão de diálogo, a ordem definitiva de alguns eventos, o destino de Thomas e certas revelações serão revisados com Cristhian. IDs de cena são estáveis para roteiro e implementação; durações são **hipóteses**, sem playtest.

## Promessa dramática

Joe Chill foge do cerco policial com Harley Quinzel, Bartholomeu Kent, Jack Oswald White e Thomas Terrence. O hospital abandonado parece uma vantagem: corredores, muitos pisos, entradas técnicas. No quarto andar, o Bat-Sinal aparece distante; Thomas é arrancado do grupo por uma figura que Joe mal consegue distinguir. Batman não se apresenta como chefe de fase. Ele busca, ouve, observa, erra, aprende com evidências e pode interceptar rotas que Joe revelou. O medo nasce de **não saber o que ele sabe**. Sua vitória comum sobre Joe é captura/incapacitação, de acordo com o GDD.

O eixo emocional é a escolha entre informação, vínculo e sobrevivência. Joe pode ajudar Harley, dividir munição com Bartholomeu, acreditar em Jack, esconder rotas dos demais ou abandonar alguém. O jogo nunca transforma uma única escolha de diálogo em um medidor de bondade: ações, tempo, recursos e testemunhas alteram confiança e conhecimento separadamente.

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
| N05 · O que ficou | `F4_NURSE`, `F4_STORAGE` · O/C | Primeira parte da combinação e esconderijo entre lençóis. Um vestígio de Thomas pode atrair Joe de volta. Quem retorna nota uma alteração física justificada por ator/evento, não um susto gratuito. |

**Função do ato:** fazer o jogador distinguir o que viu, ouviu e supôs. A cena de Thomas permanece a abertura estabelecida no GDD; a proposta não confirma sua morte nem inventa o que Batman fez fora da câmera.

### Ato 2 — descobrir que o hospital também observa

| ID | Área / modo | Proposta de cena e consequência |
| --- | --- | --- |
| N06 · Clínica | `F3_CENTRAL` → `F3_RECOVERY` · M | Primeiro anel de exploração. Cortinas, leitos e vidro oferecem cobertura com visão parcial; uma passagem para o oeste parece levar de volta a um lugar conhecido. |
| N07 · Harley | `F3_ICU` ou `F1_TREATMENT` · C | Harley aparece conforme sua trajetória e risco, não por teleporte ao objetivo de Joe. Pode oferecer avaliação de ferimento, recurso ou aviso. Se Joe ajuda ou compartilha, a relação ganha causa; se foge, ela pode sobreviver por conta própria e lembrar. |
| N08 · Análise de Jack | `F3_EXAMS` ou `F2_CAMERAS` · C | Jack nota que movimentos do Batman respondem a hábitos observáveis. Ele pode estar correto, incompleto ou manipular Joe, conforme o que realmente presenciou. Não entrega posição onisciente. |
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

**Função do ato:** várias rotas podem ser preparadas em paralelo. A narrativa principal é o conhecimento adquirido pelo jogador e por cada companheiro, não uma lista de chaves com ordem única. Em uma campanha que escolheu túneis, o cofre pode ficar fechado; em outra, a janela pode nunca ser tocada.

### Ato 4 — pressão sobre pessoas e sobre caminhos

| ID | Área / modo | Proposta de cena e consequência |
| --- | --- | --- |
| N21 · Padrão observado | Escada usada repetidamente · C | Batman pode ganhar peso de evidência para uma escada **apenas** se testemunhar a rota ou vestígios específicos. Outra escada, elevador energizado ou prumada ainda oferece alternativa. |
| N22 · Falha de energia | `F2_SERVER` / `B1_GENERATOR` · C | Interação de Joe, acidente ou evento justificado apaga um circuito. Monitores perdem imagem; porta `needs_power` fecha acesso; porta `releases_when_off` pode ser aberta manualmente. O áudio do relé cria risco conforme alcance e mascaramento. |
| N23 · Decisão sobre Harley | `F3_RECOVERY` / `F1_TREATMENT` · C | Se ferida ou sob pressão, Harley pede recurso ou cobertura. Joe pode gastar o que tem, adiar e voltar, ou deixá-la. Ela conserva agência: pode achar sua própria saída, guardar ressentimento ou tentar ajudar; não existe proteção garantida por parentesco. |
| N24 · Decisão sobre Bartholomeu | `B1_PUMP` / `G_MAINT` · C | Um caminho e munição podem ser divididos. Baixa confiança e Hunter realmente percebido abrem a opção sistêmica de Bartholomeu usar Joe como distração; ele também pode hesitar ou cooperar. A consequência precisa de trajetória física e aviso perceptível. |
| N25 · Jack e a dúvida | `F2_CAMERAS` / `F3_EXAMS` · C | Jack oferece inferência útil sobre padrão do Batman, possivelmente ocultando algo que sabe. No Punitivo, pistas discretas podem sustentar segredo do GDD, **sem confirmar identidade na campanha comum**. Sua ausência ou morte não bloqueia qualquer fuga. |
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

## Personagens como trajetórias, não pontos fixos

| Personagem | Primeiro encontro proposto | Pressão / decisão relevante | Possíveis resultados sem fixar final |
| --- | --- | --- | --- |
| Harley | Clínica ou tratamento, dependente de percurso após a separação | Ferimento, recursos, confiança anterior e escolhas de Joe | Cooperar, enfrentar desacordo, sair por conta própria, abandonar Joe, ser capturada; vínculo não a torna invulnerável |
| Bartholomeu | Administração, casa de bombas ou garagem | Conhecimento de rota, munição, abandono e medo do Batman | Compartilhar caminho, salvar Joe, fugir sozinho, mentir ou usá-lo como distração se os requisitos sistêmicos surgirem |
| Jack | Exames/câmeras, investigando padrão | Informação incompleta, utilidade e desconfiança | Aliado pragmático, mentiroso, ausente; segredo Punitivo só se pistas e condições sustentarem |
| Thomas | Cena N03 e rastros opcionais | O desaparecimento define ameaça inicial | Destino aberto para revisão; evitar “prova” contraditória sem decisão do criador |

O sistema de companheiros parte de **variáveis separadas** de confiança, medo, estresse, lealdade, autopreservação e relacionamento; valores iniciais continuam na decisão D-02. O roteiro não declara “Harley sempre salva Joe” nem “Bartholomeu trai no minuto 140”. Os mesmos encontros podem mudar falas, prioridades e recursos de acordo com o estado. Nenhum NPC sabe uma rota porque Joe a descobriu: precisa de testemunho, observação ou comunicação.

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

Documentos opcionais ampliam o hospital e permitem suspeitar que houve uso recente, mas não explicam a presença de Batman com uma exposição única. Pistas obrigatórias de **uma rota escolhida** são rastreáveis e continuam acessíveis mesmo quando outra rota falha. A redação exata e a combinação de seis dígitos são pendências de coautoria.

### Gravações e falas de ambiente — ideias de primeira passada

| ID | Fonte | Conteúdo dramático proposto |
| --- | --- | --- |
| AUD01 | Segurança do 2º | Um operador relata que a câmera da clínica perde quadros; Joe aprende a desconfiar da tela sem receber a localização de Batman. |
| AUD02 | Manutenção de B1 | Voz orienta pelo ruído da bomba e por duas válvulas diferentes; ramal morto vira aprendizado, não truque arbitrário. |
| AUD03 | Telefone da emergência | Uma mensagem antiga indica evacuação incompleta, sustentando a sensação de abandono irregular. |
| AUD04 | Rádio policial parcial | Gordon mantém o quarteirão isolado; Joe entende por que a rua é arriscada. Diálogo completo depende de direção de cena. |
| AUD05 | Comunicação interceptada parcial | Uma voz de Alfred pode mencionar tempo e movimentação sem expor plano inteiro de Batman. |
| AUD06 | Mensagem de companheiro | Origem, hora e alcance determinam se Joe a ouve; um recado pode estar desatualizado. |

Esses áudios precisam ser localizados no mundo, audíveis sob condições próprias e úteis mesmo sem legenda perfeita. A mixagem e as falas finais serão trabalhadas depois da revisão da campanha.

## Diálogo de teste para revisar voz (não texto final)

**N07, reencontro com Harley, Joe ferido:**

> Harley: “Você atravessou esse corredor sozinho?”  
> Joe: “Atravessei.”  
> Harley: “Então por que tá segurando o braço?”  
> Joe: “Achei você. É o que importa agora.”  
> Harley: “Não. Importa se você vai conseguir correr quando ele voltar.”

**N15, Bartholomeu conhece um caminho, mas não confia o bastante:**

> Bartholomeu: “Tem um caminho perto das bombas. Talvez saia do outro lado.”  
> Joe: “Talvez?”  
> Bartholomeu: “Eu vi a entrada, Joe. Não vi a saída. Se quer certeza, vem comigo.”

**N08, Jack observa comportamento do Batman:**

> Jack: “Na primeira vez, ele veio pelo barulho. Na segunda, pela escada.”  
> Joe: “Tá dizendo que ele sabe para onde eu vou?”  
> Jack: “Tô dizendo que você acabou de me contar qual escada costuma usar.”

O subtexto de Jack depende de ele ter visto as duas ocasiões. Se não viu, esta fala não pode tocar; uma versão alternativa deixa claro que é apenas hipótese. Batman fala raramente. Seu silêncio, a posição desconhecida e as consequências físicas contam a maior parte de sua presença.

## Regras de direção para eventos dinâmicos

1. **Causa antes do efeito:** ruído tem fonte/tempo/atenuação; Batman só recebe um `Stimulus` após percepção. Câmera em loop não informa presente. Mudança do mundo só vira evidência quando ele inspeciona o objeto.
2. **Nenhuma perseguição de roteiro inescapável:** a primeira ruptura é premissa cinematográfica; depois Batman usa rotas fisicamente válidas e pode falhar. Se uma porta for emperrada, sinal e alternativa precisam existir.
3. **Conhecimento por pessoa:** Joe, Harley, Bartholomeu, Jack e Batman mantêm fatos diferentes. A cena N15 exige localização/comunicação para transferir a pista de Bartholomeu a Joe.
4. **Consequência reversível quando possível:** circuito pode ser restaurado; porta manual pode ser reaberta; falha definitiva de fuga requer evento claro. Uma traição não é sorteio solto; opções surgem de estado e circunstância.
5. **Cobertura e legibilidade:** vermelho do 4º e dos alarmes, preto profundo da clínica e verde úmido da manutenção vêm dos cinco vídeos, mas cada rota precisa de silhueta, marco e feedback legíveis em primeira pessoa.
6. **Duração conquistada:** usar reinterpretação de espaços, encontros condicionais, preparação paralela e consequências. Medir a proporção exploração/stealth/interação/retorno; cortar corredores vazios após playtest.

## Pontos para decidir com Cristhian antes do texto final

| Questão | Proposta inicial para revisar | O que muda |
| --- | --- | --- |
| Thomas | Manter destino sem confirmação na campanha comum, com rastros contraditórios porém verificáveis | Peso do mistério e possível cena de descoberta |
| Relação Joe–Harley | Intimidade anterior real, mas escolhas podem romper confiança e separá-los | Tom de diálogo e desfechos dela |
| Jack no Punitivo | Guardar a possibilidade do segredo do GDD como variante raríssima; nenhuma fala na campanha comum confirma | Releitura das pistas e produção de finais |
| Hospital | Sugerir uso recente e observação prévia pelo Batman sem explicar tudo numa gravação | Design ambiental, documentos e equipamentos |
| Grau de violência | Perseguição e captura por Batman como padrão; ferimentos graves e risco ambiental sem glamourizar execução | Cenas, classificação e direção de áudio |
| Final emocional | Cada fuga resolve sobrevivência imediata; perdas, confiança e estado dos companheiros dão epílogo variante | Quantidade de epílogos e linhas finais |

Depois de revisar essas decisões, redigir cenas completas, falas condicionais, documentos e gravações por ID; cruzar cada gatilho com flags, estados do mundo e testes. **Não fixar a campanha como cânone nem presumir duração final antes dessa revisão e do playtest.**
