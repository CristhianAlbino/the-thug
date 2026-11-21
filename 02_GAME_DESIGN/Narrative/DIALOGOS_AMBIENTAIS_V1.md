# Conversas ambientais — The Thug, texto v1

**Estado:** primeira passada para revisão criativa. Fonte de dados: [`banter_ptbr.json`](../../Tools/NarrativeCore/banter_ptbr.json). Os 42 diálogos abaixo são opcionais; nenhum NPC surge só para entregar uma fala. Joe conhece o próprio passado, mas nenhuma conversa deduz a identidade civil de Batman. Conversas em todos os modos com risos habituais de Jack **não** registram J1–J3; somente os encontros cruéis específicos do Punitivo o fazem.

**Disparo:** ato + área + cenas vistas + presença ativa de todos os interlocutores no mesmo nó. Perigo/perseguição ou cena principal em andamento suspendem conversas. Após 90 s desde a última conversa completa, escolher uma fala elegível ainda não ouvida; entregar ID apenas ao terminar. Se interromper, guardar ID/linha e tentar retomar com os mesmos atores; se dispersarem, cancelar sem conceder fatos. A ordem entre várias elegíveis é estável por ID até a futura direção espacial. Joe não pode usar conversa com ausente/capturado como fonte de informação.

## Ato 1

### BAN01 — F4_START; antes de N03

**Thomas:** “Barth, sua mão ficou presa?”  
**Bartholomeu:** “Não. Só tremendo.”  
**Thomas:** “Então segura o outro lado. Eu seguro o peso.”  

### BAN02 — F4_HALL_E, F4_HALL_W; antes de N03

**Harley:** “Ei, irmãozinho, respira pelo nariz.”  
**Bartholomeu:** “Você fala assim toda vez que eu fico com medo.”  
**Harley:** “E você ainda fica vivo toda vez. Olha que método bom.”  

### BAN03 — F4_HALL_W, F4_OBSERVATION; antes de N03

**Harley:** “Você trouxe todo mundo para cá sem saber a saída?”  
**Joe:** “Eu sabia como sair da rua.”  
**Harley:** “Com você, metade de um plano sempre vira aventura.”  

### BAN04 — F4_OBSERVATION; antes de N03

**Thomas:** “Quando amanhecer, quero uma mesa grande e café para os cinco.”  
**Joe:** “Você sempre planeja a sobremesa antes da fuga.”  
**Thomas:** “Alguém precisa planejar a parte boa.”  

### BAN05 — F4_OBSERVATION, F4_WARD_E; antes de N03

**Jack:** “Já vi o morcego de perto. Saí vivo. Ha.”  
**Bartholomeu:** “Por que você ri quando diz isso?”  
**Jack:** “Porque funcionou. Não pergunte o quanto corri.”  

### BAN06 — F4_WARD_E, F4_HALL_E; antes de N03

**Jack:** “Se os monitores ainda funcionam, podemos observar o corredor.”  
**Harley:** “Boa ideia. Pena que foi você quem teve.”  
**Jack:** “Guarde o elogio para quando estivermos fora.”  

### BAN07 — F4_HALL_E, F4_START; antes de N03

**Bartholomeu:** “Eu sabia que você escolheria a escada de serviço.”  
**Joe:** “Você lembra melhor dos meus atalhos do que eu.”  
**Bartholomeu:** “É por isso que ainda ando com você.”  

### BAN08 — F4_OBSERVATION; antes de N03

**Thomas:** “Se aquele sinal é pra gente, eu faço o discurso.”  
**Harley:** “Você faz cara de durão. Eu faço você sair da janela.”  
**Thomas:** “Fechado, chefe.”  

## Ato 2

### BAN09 — F3_CENTRAL, F3_RECOVERY; após N03

**Harley:** “Você está contando as portas ou evitando olhar pra mim?”  
**Joe:** “As portas.”  
**Harley:** “Eu sabia. Estava torcendo pela outra resposta.”  

### BAN10 — F3_ICU, F1_TREATMENT; após N03

**Bartholomeu:** “Eu devia ter puxado o Thomas.”  
**Harley:** “Você mal conseguia ficar de pé. Não põe isso nas suas costas.”  
**Bartholomeu:** “Ele não teria deixado ninguém para trás.”  

### BAN11 — F3_EXAMS, F2_CAMERAS; após N03

**Jack:** “Na outra vez, o morcego esperou meu ruído passar. Eu saí vivo quando parei de repetir o caminho. Ha.”  
**Joe:** “Você riu daquela vez também?”  
**Jack:** “Depois. Bem depois.”  

### BAN12 — F3_RECOVERY, F3_CENTRAL; após N03

**Bartholomeu:** “Joe, aquela história dos Wayne... você nunca terminou de contar.”  
**Joe:** “Não tem final bom. Quando comecei nessa vida, matei um casal da família Wayne. O filho estava lá.”  
**Bartholomeu:** “Você fala baixo agora. Eu queria que tivesse pensado nele antes.”  

### BAN13 — F3_ICU, F3_RECOVERY; após N03

**Harley:** “Você nunca fala daquele casal Wayne quando tem luz acesa.”  
**Joe:** “Matei os pais de um garoto. Apagar a luz não apaga isso.”  
**Harley:** “Ainda bem que você sabe. Só não usa culpa como desculpa para abandonar quem está aqui.”  

### BAN14 — F3_EXAMS, F2_CAMERAS; após N03; após evidência camera_loop_observed_by_group

**Jack:** “A câmera repete o mesmo reflexo. Alguém quer que acreditemos nela.”  
**Harley:** “Você pode estar certo. Eu detesto quando acontece.”  
**Jack:** “Olhe o relógio. Depois pode me detestar de novo.”  

### BAN15 — F3_CENTRAL, F3_HALL_W; após N03

**Bartholomeu:** “A gente saía dos apertos conversando. Hoje você só aponta portas.”  
**Joe:** “Se eu falar demais, você ouve que estou com medo.”  
**Bartholomeu:** “Eu já ouvi. Ainda estou do seu lado.”  

### BAN16 — F3_RECOVERY, F2_CENTRAL; após N03

**Harley:** “Barth, sua mão parou de tremer?”  
**Bartholomeu:** “Só quando seguro alguma coisa.”  
**Harley:** “Joe, conversa com seu amigo um minuto.”  
**Joe:** “Estou ouvindo, Barth.”  

### BAN17 — F2_SECURITY, F2_CAMERAS; após N03; após evidência garage_camera_loop_observed

**Jack:** “O quadro da garagem parou. Repare no mesmo gotejo.”  
**Joe:** “Então não sabemos quem está lá.”  
**Jack:** “Agora você sabe exatamente o que não sabe.”  

### BAN18 — F3_EXAMS, F3_CENTRAL; após N03

**Jack:** “Bartholomeu, você viu uma passagem?”  
**Bartholomeu:** “Vi uma porta. Não o outro lado.”  
**Harley:** “Ele respondeu. Não começa a completar a história por ele, Jack.”  

## Ato 3

### BAN19 — F2_ARCHIVE, F2_ADMIN; após N12

**Harley:** “Você chama isso de plano ou de pilha de papéis?”  
**Joe:** “Chamo de mais uma chance.”  
**Harley:** “Então guarda uma chance pra quem está vindo atrás.”  

### BAN20 — B1_PUMP, B1_CENTRAL; após N03

**Bartholomeu:** “A bomba maior vibra diferente. Por ali o ar continua.”  
**Joe:** “Você reparou antes de mim.”  
**Bartholomeu:** “Fico olhando saída até quando falo com você.”  

### BAN21 — F2_SERVER, F2_SECURITY; após N03; após evidência doc11_read_by_jack

**Jack:** “O servidor fecha com a energia caída. O suprimento não usa o mesmo motor.”  
**Joe:** “Você conferiu ou está apostando?”  
**Jack:** “Conferi a placa. A porta ainda pode emperrar.”  

### BAN22 — F1_PHARMACY, F1_TREATMENT; após N03

**Bartholomeu:** “Pode usar o remédio. Sua mão está pior.”  
**Harley:** “Você tem pulso acelerado e está mentindo.”  
**Bartholomeu:** “Aprendi com o Joe. Péssimo professor.”  

### BAN23 — B1_PIPE, F1_SERVICE; após N03

**Harley:** “Eu te sigo porque escolho. Não porque você manda.”  
**Joe:** “Eu sei.”  
**Harley:** “Então pergunta antes de me deixar segurando a porta.”  

### BAN24 — F2_CENTRAL, F2_MEETING; após N03

**Jack:** “Se ele ouviu a escada, pode chegar por cima. Pode.”  
**Bartholomeu:** “Você nunca diz uma coisa boa?”  
**Jack:** “A boa é que ainda temos outra escada. Ha.”  

### BAN25 — F1_LOBBY, F1_EMERGENCY; após N03

**Bartholomeu:** “Você me tirou de uma briga na rua anos atrás.”  
**Joe:** “Você pagou de volta umas dez vezes.”  
**Bartholomeu:** “Amigos não fazem conta. Você que me ensinou.”  

### BAN26 — F2_CAMERAS, F2_SECURITY; após N03; após evidência camera_live_observed_by_group

**Jack:** “Mudei para outra câmera. Essa ainda está viva.”  
**Harley:** “Mostra a hora. Não me vende corredor vazio gravado.”  
**Joe:** “Ela tem razão. A gente confere o caminho.”  

### BAN27 — B1_LAUNDRY, B1_STORAGE; após N03

**Harley:** “Se eu disser corre, você corre primeiro.”  
**Bartholomeu:** “Não vou te deixar sozinha.”  
**Harley:** “Então corre comigo, irmãozinho.”  

### BAN28 — F3_WINDOW, F3_HALL_W; após N18

**Harley:** “Você pensou em pular sozinho, não pensou?”  
**Joe:** “Pensei em ver onde cai.”  
**Harley:** “Olha também quem consegue atravessar. Eu não sou uma mala.”  

## Ato 4

### BAN29 — F3_RECOVERY, F1_TREATMENT; após N23; após evidência joe_delayed_harley_help; modos Easy, Normal, Hard

**Harley:** “Eu cuido de você há anos. Hoje eu precisei pedir duas vezes.”  
**Joe:** “Eu ouvi na primeira.”  
**Harley:** “Ouvir e voltar são coisas diferentes.”  

### BAN30 — F3_RECOVERY, F1_TREATMENT; após N23; após evidência harley_injured; modos Punitive

**Joe:** “Mostra o corte. Eu cubro a porta enquanto você trata.”  
**Harley:** “Você está com medo por mim.”  
**Joe:** “Estou. Não faz eu perder tempo negando.”  

### BAN31 — B1_PUMP, B1_STORAGE; após N15; após evidência bartholomeu_cooperative

**Bartholomeu:** “Se você for pela frente, eu fecho atrás. Mas me espera na válvula.”  
**Joe:** “Você aguenta?”  
**Bartholomeu:** “Com medo e tudo. Me espera.”  

### BAN32 — F2_SERVER, F2_CAMERAS; após N21

**Jack:** “Na primeira vez que cruzei com o morcego, fugi da rota que ele esperava. Ha.”  
**Joe:** “Parece que ele aprendeu seus truques.”  
**Jack:** “E nós aprendemos que ele pode aprender.”  

### BAN33 — F2_SERVER, F2_CAMERAS; após N21; modos Punitive

**Jack:** “Sabe o que eu mais admiro nele? A pausa antes de abrir uma porta.”  
**Joe:** “Você está sorrindo de novo.”  
**Jack:** “Estou pensando numa saída. Ha... ha.”  

### BAN34 — F2_CENTRAL, F3_CENTRAL; após N15

**Jack:** “A escada oeste é mais curta.”  
**Harley:** “E mais exposta. Barth, você viu o corredor?”  
**Bartholomeu:** “Vi a porta aberta. Não vi ele.”  
**Harley:** “Pronto. Isso vale mais que certeza inventada.”  

### BAN35 — B1_CENTRAL, F1_SERVICE; após N15; após evidência bartholomeu_cooperative

**Bartholomeu:** “Se você disser para eu ficar, eu fico. Só não some sem falar.”  
**Joe:** “Eu vou dizer o que decidi.”  
**Bartholomeu:** “É tudo que peço de um amigo.”  

### BAN36 — F3_HALL_W, F3_WINDOW; após N23

**Harley:** “Eu posso encontrar meu caminho. Só me diz onde você pretende ir.”  
**Joe:** “Pela janela, se a passagem estiver livre. Você quer tentar comigo?”  
**Harley:** “É disso que eu precisava: uma escolha de verdade.”  

## Ato 5

### BAN37 — G_VEHICLES, G_CONTROL; após N24; após evidência bartholomeu_cooperated_n24

**Bartholomeu:** “Eu tremi a noite toda e ainda estou aqui.”  
**Joe:** “Você também me trouxe até aqui.”  
**Bartholomeu:** “Então abre, amigo. Antes que eu comece de novo.”  

### BAN38 — R_HVAC, R_PAD; após N23; após evidência helicopter_confirmed

**Harley:** “Se essa coisa pousar, a gente decide junto quem sobe.”  
**Joe:** “Se não pousar, temos outra saída.”  
**Harley:** “Olha só. Um plano inteiro. Estou orgulhosa.”  

### BAN39 — T_LOOP, T_FORK; após N24

**Bartholomeu:** “O ar vem dali. Não digo que é seguro; digo que não acaba aqui.”  
**Joe:** “Foi por isso que te trouxe.”  
**Bartholomeu:** “Você me trouxe porque somos amigos. Eu encontrei o ar sozinho.”  

### BAN40 — A_BRIDGE, F3_WINDOW; após N23

**Joe:** “Você consegue atravessar?”  
**Harley:** “Consigo decidir. Me diz quanto tempo temos.”  
**Joe:** “Pouco. Mas vou esperar sua resposta.”  

### BAN41 — F2_SECURITY, F2_CAMERAS; após N25; modos Punitive

**Jack:** “Ele quase me pegou uma vez. Ainda rio quando lembro. Ha...”  
**Joe:** “Você ri como se tivesse gostado.”  
**Jack:** “Talvez eu goste de chegar ao fim da história.”  

### BAN42 — F3_ISOLATION, F3_UTILITY; após N27

**Harley:** “Se sairmos daqui, não quero que finja que esta noite não aconteceu.”  
**Joe:** “Não vou esquecer o Thomas.”  
**Harley:** “Nem ele, nem as pessoas que ainda estão aqui.”  

## Origens de fatos condicionais

| Fato | Só registrar após |
| --- | --- |
| `garage_camera_loop_observed` | Jack viu dois quadros repetidos no feed, não apenas entrou na sala. |
| `camera_loop_observed_by_group` | Harley e Jack verificaram os quadros repetidos juntos. |
| `doc11_read_by_jack` | Jack leu a placa/documento ou recebeu informação demonstrável no mesmo encontro. |
| `camera_live_observed_by_group` | Joe, Harley e Jack verificaram timestamp/estado ao vivo juntos. |
| `joe_delayed_harley_help` | Harley pediu ajuda e Joe realmente adiou; não registrar se o jogador ajudou logo. |
| `harley_injured` | Ferimento físico confirmado, com Harley ainda ativa. |
| `bartholomeu_cooperative` | Bartholomeu segue colaborando depois das ações reais de Joe. |
| `bartholomeu_cooperated_n24` | Ele de fato ajudou em N24, sem abandono/traição naquele encontro. |
| `helicopter_confirmed` | Rádio confirmou pouso e RunRules iniciou o prazo. |

## Direção de continuidade

- Thomas só participa antes de N03. Seus gestos com Bartholomeu e sua promessa de café dão algo concreto a lembrar; depois ele não fala como lembrança onisciente.
- Joe menciona ter matado um casal Wayne no início de sua vida criminosa. Isso deve incomodar o jogador e os amigos; não absolver Joe nem revelar que Batman é o filho por dedução automática.
- Harley mantém carinho por Bartholomeu e repulsa lúcida por Jack, mas reconhece quando ele dá informação verificável. Joe comum é frio em alguns momentos; no Punitivo demonstra reciprocidade.
- Jack ri ao lembrar que sobreviveu ao Batman em qualquer modo. Sua reação inadequada a sofrimento e a trilha J1–J4 continuam exclusivas do Punitivo. A fala opcional não substitui prova testemunhada.
- Textos não contêm palavrões fortes; ‘merda’, ‘bosta’ e ‘ferrou’ são o teto coloquial aprovado. Sons e música ainda serão produzidos depois da revisão.
