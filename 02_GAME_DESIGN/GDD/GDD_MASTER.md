# THE THUG
## Game Design Document Mestre — Versão Consolidada

**Nome do projeto:** The Thug  
**Cenário principal:** Hospital abandonado  
**Gênero:** Survival Horror / Stealth / Immersive Survival  
**Perspectiva:** Primeira pessoa  
**Engine:** Unreal Engine 5  
**Estrutura:** Semi-open world concentrado em um único complexo hospitalar  
**Experiência principal:** Sobrevivência, perseguição, exploração, furtividade, decisões sistêmicas e terror psicológico  
**Duração-alvo da experiência principal:** aproximadamente 3 horas no Difícil  
**Plataforma inicial:** PC  
**Desenvolvimento:** C++ + Blueprint

---

# 1. VISÃO DO PROJETO

THE THUG é um jogo de terror e stealth em primeira pessoa no qual o jogador assume o papel de um criminoso preso dentro de um hospital abandonado enquanto é caçado pelo Batman.

A fantasia central não é:

> “Eu sou um criminoso enfrentando o Batman.”

É:

> “Eu sou uma das pessoas que normalmente aparecem do outro lado da história do Batman.”

O jogador não controla o predador.

O jogador é a presa.

O objetivo fundamental do projeto é provocar:

- tensão;
- paranoia;
- vulnerabilidade;
- medo de ser localizado;
- dúvida;
- antecipação;
- sensação de perseguição;
- insegurança mesmo quando nada está acontecendo.

O jogador não deve pensar constantemente:

> “Como eu derroto o Batman?”

Ele deve pensar:

> “Onde ele está?”

> “Ele ouviu isso?”

> “Ele sabe que estou aqui?”

> “Esse esconderijo ainda é seguro?”

> “Como eu saio vivo daqui?”

---

# 2. PILARES DE DESIGN

## 2.1 Predador, não boss

Batman — internamente chamado de **Hunter** pelos sistemas — não funciona como um inimigo comum ou chefe tradicional.

Ele é uma presença sistêmica.

Pode:

- observar;
- ouvir;
- investigar;
- procurar;
- perseguir;
- perder o jogador;
- memorizar comportamentos;
- prever rotas;
- preparar emboscadas;
- manipular o ambiente;
- interrogar companheiros;
- mudar de estratégia.

---

## 2.2 Informação é recurso

Conhecer o hospital deve ser tão valioso quanto encontrar munição.

O jogador ganha vantagem descobrindo:

- rotas;
- escadas;
- atalhos;
- câmeras;
- passagens técnicas;
- esconderijos;
- áreas perigosas;
- recursos;
- caminhos de fuga.

No começo:

> “Eu não conheço este lugar.”

No fim:

> “Se ele entrar pelo corredor leste, posso cortar pela manutenção e alcançar a escada sul.”

O próprio hospital se transforma em ferramenta de sobrevivência.

---

## 2.3 Sobrevivência acima de combate

O jogador possui armas.

Mas possuir uma arma não significa possuir controle.

Todo equipamento existe principalmente para:

- ganhar tempo;
- criar oportunidades;
- distrair;
- interromper uma ação;
- quebrar uma linha de visão;
- abrir uma rota;
- sobreviver.

---

## 2.4 Tensão acima de sustos

Jumpscares podem existir, mas não serão a principal ferramenta de horror.

O medo deve surgir principalmente de:

- antecipação;
- silêncio;
- dúvida;
- áudio;
- iluminação;
- vulnerabilidade;
- escassez;
- perseguição;
- isolamento;
- comportamento imprevisível.

---

## 2.5 Sistemas, não apenas scripts

O ideal é que diferentes sistemas consigam interagir e produzir situações inesperadas.

Exemplo:

Joe ajuda Bartholomeu.

Bartholomeu ganha confiança.

Horas depois, Bartholomeu encontra uma ferramenta.

Em vez de fugir sozinho, deixa a ferramenta para Joe.

Essa ferramenta permite abrir uma rota alternativa.

A nova rota possibilita uma fuga que talvez nem estivesse disponível em outra partida.

---

# 3. PREMISSA

Joe Chill e outros quatro criminosos acabaram de participar de um assalto a banco em Gotham.

A polícia inicia a perseguição.

Procurando desaparecer antes que o cerco seja estabelecido, o grupo entra em um hospital aparentemente abandonado.

Eles sobem até o quarto andar.

O prédio parece perfeito:

- grande;
- escuro;
- abandonado;
- cheio de rotas;
- distante de civis.

Então Joe observa pela janela.

Ao longe:

**o Bat-Sinal.**

Inicialmente isso parece apenas mais um problema acontecendo em Gotham.

Pouco depois, enquanto os criminosos estão reunidos, alguma coisa se move acima deles.

Uma forma gigantesca semelhante a um morcego surge das sombras.

Thomas Terrence é agarrado e violentamente puxado para cima.

Ele desaparece.

Não existe explicação.

Não existe apresentação heroica.

Não existe combate.

Existe pânico.

Harley corre.

Bartholomeu corre.

Jack desaparece por outro corredor.

Joe fica sozinho.

**Começa a exploração livre após a ruptura.** A abertura N01–N02 já é jogável, com deslocamento e interações breves para criar vínculo com Thomas e a equipe antes da separação. O roteiro detalhado N01–N03 orienta a encenação; não contar a abertura como uma cutscene inteiramente sem controle.

---

# 4. EQUIPE INICIAL

A versão consolidada utiliza cinco criminosos:

1. Joe Chill — protagonista;
2. Harley Quinzel;
3. Bartholomeu Kent;
4. Jack Oswald White;
5. Thomas Terrence.

Thomas é removido do grupo durante a primeira aparição do Hunter.

Restam inicialmente quatro pessoas espalhadas pelo hospital.

Isso permite que Harley, Bartholomeu e Jack funcionem como sistemas independentes durante a partida.

---

# 5. PERSONAGENS

# 5.1 Joe Chill

**Função:** protagonista / jogador.

Joe é um criminoso experiente, pragmático e extremamente orientado à sobrevivência.

Não é um herói.

**Direção confirmada em 01/10/2026:** anos antes da campanha, ainda no início da vida criminosa, Joe matou os pais Wayne diante do filho. Ele pode mencionar esse crime em conversas opcionais; sobreviver como protagonista não o absolve. Joe e a equipe não ganham automaticamente conhecimento da identidade civil do Batman por lembrar dos Wayne. Bartholomeu é um amigo antigo, e Harley é alguém por quem Joe se importa mais do que admite nos modos comuns; no Punitivo ele demonstra cuidado mais recíproco, sempre sujeito às ações do jogador.

### Personalidade

- frio sob pressão;
- desconfiado;
- oportunista;
- persistente;
- capaz de decisões moralmente questionáveis;
- possui forte instinto de sobrevivência;
- possui vínculo emocional com Harley.

### Conflito

Joe inicia a história acreditando que a polícia representa sua maior ameaça.

O hospital destrói essa percepção.

Seu arco passa a ser:

**sobreviver → confiar ou desconfiar → proteger ou abandonar → decidir quanto está disposto a sacrificar para escapar.**

### Exemplos de fala

> “A polícia não vai subir até aqui. Temos alguns minutos.”

Após Thomas desaparecer:

> “Thomas?!”

Posteriormente:

> “Não foi a polícia.”

Sozinho:

> “Pensa, Joe. Pensa.”

---

# 5.2 Thomas Terrence

**Função:** primeira vítima narrativa.

Fisicamente forte e impulsivo. A equipe também conhece seu lado afetuoso: ajuda Bartholomeu a segurar a porta, verifica se o amigo se feriu e promete café para todos ao saírem. A perda precisa doer antes de sua confirmação.

### Personalidade

- agressivo;
- barulhento;
- convencido;
- pouco cauteloso;
- tende a enfrentar problemas.

Thomas serve para estabelecer imediatamente uma regra:

> ninguém está seguro.

Batman o matou após a separação. Fácil/Normal não confirmam o destino ao jogador; Difícil/Punitivo mostram prova da morte/autoria durante a exploração ou em coda. O fato do mundo é o mesmo, mas Joe só sabe o que testemunhou.

Antes do ataque:

> “É só um morcego fantasiado. Relaxa.”

Pouco antes de desaparecer:

> “Merda, que foi isso—”

Silêncio.

---

# 5.3 Harley Quinzel

**Função:** companheira principal e vínculo emocional de Joe.

Não funciona como personagem indefesa.

É inteligente, adaptável e capaz de sobreviver sozinha.

É o coração da equipe: às vezes excêntrica de forma querida, trata Bartholomeu como irmão mais novo e desconfia de Jack. Essa repulsa não a impede de usar uma observação dele se puder verificá-la. Ela conversa com todos, conserva agência e não existe apenas em função do romance com Joe.

### Personalidade

- inteligente;
- provocadora;
- intensa;
- manipuladora;
- imprevisível;
- corajosa;
- emocionalmente ligada a Joe.

### Relação com Joe

Existe intimidade anterior ao jogo.

A relação mistura:

- romance;
- crime;
- confiança;
- dependência;
- sobrevivência.

### Exemplos

> “Joe... eu não gosto desse lugar.”

Após o ataque:

> “Joe, olha pra mim. A gente precisa sair daqui.”

Se Joe estiver ferido:

> “Você tá sangrando.”

Joe:

> “Não é nada.”

Harley:

> “Não mente pra mim.”

Se abandonada:

> “Então é isso? Você vai me deixar?”

Se salva:

> “Eu sabia que você não ia me deixar.”

---

# 5.4 Bartholomeu Kent

**Função:** companheiro sistêmico.

Um criminoso relativamente comum.

Não é extremamente habilidoso nem corajoso.

Quer sobreviver.

### Personalidade

- nervoso;
- pragmático;
- inicialmente cooperativo;
- egoísta quando pressionado;
- extremamente assustado pelo Hunter.

### Relação com Joe

É amigo de Joe há anos, respeita o protagonista inicialmente e tem vínculo de irmã/fraterno com Harley. Seu medo o faz procurar detalhes úteis de manutenção e saída; não deve virar uma caricatura infantil.

Sua lealdade depende das ações da partida.

### Exemplos

> “Joe! Joe, você tá aí?”

> “A gente precisa sair daqui.”

Se Joe quiser procurar Thomas:

> “Pode estar morto, Joe. Eu não vi. Se você voltar lá, podemos ficar presos também.”

No Difícil/Punitivo, uma confirmação de morte e autoria só pode ser dita após Bartholomeu obter prova por encontro ou comunicação. Fácil/Normal preservam a incerteza do destino de Thomas ao jogador.

Após receber munição:

> “Eu vou lembrar disso.”

Com baixa confiança:

> “Não. Dessa vez eu vou sozinho.”

Durante uma traição:

> “Desculpa, Joe. Eu quero viver.”

---

# 5.5 Jack Oswald White

**Função:** sobrevivente experiente e elemento de desconfiança.

### Personalidade

- sarcástico;
- inteligente;
- observador;
- oportunista;
- desconfiado;
- analítico.

Jack percebe padrões antes dos outros.

Já encontrou Batman antes e saiu vivo. Ri ao contar a própria sobrevivência em **todas** as dificuldades. O riso comum pode ser sardônico ou nervoso; no Punitivo, roupa diferente e risos cruéis diante de sofrimento compõem pistas próprias e progressivas. Apenas as pistas testemunhadas em encontros J1–J3 podem sustentar a revelação opcional J4.

Pode ser o primeiro personagem a compreender que o comportamento do Hunter não é aleatório.

### Exemplo

Jack:

> “Ele não tá procurando a gente.”

Joe:

> “Então o que ele tá fazendo?”

Jack:

> “Aprendendo.”

### Linguagem da equipe

Sem palavrões fortes em falas, documentos e gravações do jogo. Expressões leves como “merda”, “bosta” e “ferrou” são permitidas. A [bíblia de personagens](../Narrative/BIBLIA_PERSONAGENS_V1.md), o [pacote de falas](../Narrative/PACOTE_TEXTO_CAMPANHA_V1.md) e as [conversas ambientais](../Narrative/DIALOGOS_AMBIENTAIS_V1.md) aplicam esta direção. Falas condicionais exigem o fato e a presença física de quem fala; não contam um destino ainda desconhecido.

---

# 6. VARIÁVEIS DOS COMPANHEIROS

Cada companheiro possui estado próprio.

Exemplo conceitual:

```text
Trust
Fear
Stress
Loyalty
SelfPreservation
Relationship
Knowledge
Injury
Resources
CurrentGoal
HunterKnowledge
PlayerKnowledge
```

Essas variáveis alteram decisões.

Exemplo:

Alta confiança + baixo medo:

> companheiro pode ajudar Joe.

Baixa confiança + medo extremo:

> companheiro pode abandonar Joe.

Baixa lealdade + alta autopreservação:

> pode usar Joe como distração.

---

# 7. O HUNTER / BATMAN

Batman é o principal antagonista sistêmico.

Sua principal função não é matar.

É dominar a situação.

Joe nunca deve saber completamente:

- onde ele está;
- o que ele sabe;
- o que ouviu;
- o que viu;
- se realmente perdeu Joe;
- se está observando;
- se está preparando alguma coisa.

A sensação desejada é:

> “Ele sabe mais do que eu.”

---

# 8. ESTADOS PRINCIPAIS DO HUNTER

```text
PATROL
↓
AWARE
↓
SUSPICIOUS
↓
INVESTIGATING
↓
SEARCHING
↓
DETECTED
↓
PURSUING
↓
LOST_TARGET
↓
SEARCHING
↓
REPOSITIONING
↓
PATROL
```

Estados especiais:

```text
OBSERVING
AMBUSH
INTERROGATING
TRAP_SETUP
REPOSITIONING
RETREAT
```

---

# 9. PERCEPÇÃO DO HUNTER

## 9.1 Visão

A percepção visual considera:

- distância;
- ângulo;
- campo de visão;
- iluminação;
- movimento;
- postura;
- cobertura;
- obstrução;
- exposição parcial.

Exemplo:

Joe parado em um corredor escuro:

**baixa detectabilidade.**

Joe correndo em um corredor iluminado:

**alta detectabilidade.**

Escuridão reduz visibilidade.

Não concede invisibilidade.

---

# 9.2 Audição

Cada ação produz um evento de ruído.

```text
Parado             0
Passos lentos      baixo
Andar              baixo
Corrida            médio
Porta              médio
Objeto quebrado    médio/alto
Queda              alto
Tiro                extremo
Explosão            extremo
```

O Hunter recebe:

```text
local aproximado
+
intensidade
+
tipo
+
tempo do evento
```

Não recebe automaticamente a posição exata do jogador.

---

# 9.3 Evidências ambientais

O Hunter também interpreta:

- portas abertas;
- portas anteriormente fechadas;
- luzes alteradas;
- câmeras modificadas;
- sangue;
- objetos deslocados;
- corpos;
- equipamentos usados;
- companheiros;
- áreas anteriormente investigadas.

---

# 10. MEMÓRIA DO HUNTER

```text
HunterMemory
├── LastPlayerLocation
├── LastNoise
├── KnownPlayerRoutes
├── UsedHidingSpots
├── NoiseHistory
├── EncounterHistory
├── PlayerHabits
├── CompanionLocations
├── SuspiciousAreas
├── UsedItems
└── EscapePatterns
```

---

# 11. APRENDIZADO

Não será utilizado machine learning.

O sistema utiliza:

**memória + pontuação + probabilidade + adaptação.**

Exemplo:

```text
Armário A

1º uso → suspeita 10
2º uso → suspeita 30
3º uso → suspeita 60
4º uso → suspeita 90
```

Outro exemplo:

```text
70% das fugas do jogador
→ Escada Oeste
```

Gradualmente, a probabilidade de o Hunter:

- verificar;
- observar;
- interceptar;
- patrulhar;

essa região aumenta.

Nunca deve chegar a 100%.

---

# 12. REGRA DE OURO DA IA

> O Hunter nunca deve trapacear para parecer inteligente.

Ele não pode:

- saber magicamente a posição do jogador;
- enxergar através de paredes;
- saber instantaneamente qual esconderijo foi usado;
- conhecer decisões que ainda não observou;
- teletransportar silenciosamente apenas para bloquear Joe;
- sempre escolher a resposta correta.

A inteligência deve vir de:

> **Percepção + memória + informação + previsão + comportamento.**

Ao perder, o jogador idealmente pensa:

> “Eu fiz merda.”

E não:

> “O jogo roubou.”

---

# 13. O HUNTER PODE ERRAR

Fundamental.

Ele pode:

- investigar o local errado;
- interpretar um ruído incorretamente;
- verificar um esconderijo vazio;
- perder Joe;
- chegar tarde;
- seguir um companheiro;
- ser enganado por uma distração.

Isso torna o sistema crível.

---

# 14. ARMADILHAS

Batman pode alterar o ambiente.

Exemplos:

- apagar luzes;
- fechar portas;
- bloquear uma rota;
- desligar câmeras;
- deixar portas propositalmente abertas;
- mover objetos;
- observar determinada passagem;
- preparar interceptações.

Exemplo:

```text
Joe utiliza Escada Oeste repetidamente
↓
HunterMemory identifica padrão
↓
Hunter aumenta suspeita da rota
↓
Reposiciona-se
↓
Joe foge novamente
↓
Hunter já está esperando
```

---

# 15. EMBOSCADAS

Emboscada não é igual a armadilha.

**Armadilha:** ambiente manipulado.

**Emboscada:** Hunter posicionado estrategicamente.

Devem ser raras.

Caso aconteçam constantemente, deixam de surpreender.

---

# 16. PATRULHA

Não utilizar patrulha puramente aleatória.

Existem rotas base:

```text
Patrol_A
Patrol_B
Patrol_C
Patrol_D
```

A escolha depende de:

- horário;
- ruídos;
- memória;
- progresso;
- dificuldade;
- atividade dos companheiros;
- comportamento anterior de Joe.

---

# 17. PERSEGUIÇÃO

```text
PLAYER DETECTED
↓
CONFIRM TARGET
↓
PURSUIT
↓
BREAK LINE OF SIGHT
↓
LAST KNOWN LOCATION
↓
SEARCH
↓
PREDICT / INVESTIGATE / ABANDON
```

O Hunter pode:

- perseguir;
- cortar caminho;
- subir ou descer andares;
- utilizar atalhos;
- prever destinos;
- interromper perseguição;
- tentar interceptar.

---

# 18. AGRESSIVIDADE

Variável:

```text
HunterAggression
```

Pode aumentar quando Joe:

- atira repetidamente;
- utiliza explosivos;
- destrói equipamentos;
- interfere nos planos do Hunter;
- escapa repetidas vezes;
- ameaça outros personagens;
- tenta enfrentar Robin;
- provoca confrontos sucessivos.

Em níveis elevados, o Hunter:

- procura por mais tempo;
- patrulha áreas críticas;
- tolera menos distrações;
- aumenta pressão;
- utiliza mais emboscadas.

---

# 19. APARIÇÕES DO HUNTER

Regra:

> Quanto mais perigoso Batman parecer, menos precisamos mostrá-lo.

Pode ser percebido através de:

- sombras;
- silhuetas;
- reflexos;
- passos;
- capa;
- rádio;
- câmeras;
- portas;
- movimentação vertical;
- reação de NPCs;
- objetos.

Quando aparece completamente, o momento precisa significar alguma coisa.

---

# 20. DIÁLOGOS DO HUNTER

Falas diretamente para Joe são extremamente raras.

Exemplos:

> “Eu sei que você está aí.”

> “Pare.”

> “Acabou.”

Ou, depois de o sistema aprender um padrão:

> “Você sempre escolhe essa porta.”

Outro exemplo:

Joe escondido.

Passos do lado de fora.

Silêncio.

Então:

> “Joe.”

Silêncio novamente.

Batman vai embora.

---

# 21. COMUNICAÇÃO EXTERNA

Batman pode conversar com:

- Alfred;
- Gordon;
- Oracle;
- Robin;
- eventualmente Flash.

Essas conversas informam o mundo sem revelar totalmente a IA.

Exemplo:

Alfred:

> “Sir, movimento no terceiro andar.”

Batman:

> “Eu sei.”

---

# 22. HOSPITAL

O hospital será o principal espaço do jogo.

Estrutura:

```text
TELHADO
  │
4º ANDAR
  │
3º ANDAR
  │
2º ANDAR
  │
1º ANDAR
  │
SUBSOLO
  │
GARAGEM
  │
EXTERIOR
```

Não deve ser gigantesco.

O objetivo é densidade e interconexão.

---

# 23. 4º ANDAR — INÍCIO / CAOS

Função:

- introdução;
- primeira aparição;
- separação;
- aprendizado inicial do espaço.

Áreas:

- corredor principal;
- enfermarias;
- quartos;
- banheiros;
- sala de enfermagem;
- descanso;
- depósito;
- corredor lateral;
- elevador;
- escadas;
- áreas abandonadas.

Esse andar muda com o tempo.

Joe pode perceber:

- portas diferentes;
- objetos deslocados;
- sangue;
- luzes alteradas;
- companheiros desaparecidos.

---

# 24. 3º ANDAR — ÁREA CLÍNICA

Principal área de exploração.

Contém:

- cirurgia;
- recuperação;
- UTI;
- exames;
- enfermarias;
- depósitos;
- banheiros;
- vestiários;
- corredores secundários;
- passagens técnicas.

Possui vários bons esconderijos.

Também pode conter uma antiga área de isolamento potencialmente muito segura.

---

# 25. 2º ANDAR — ADMINISTRAÇÃO

Contém:

- escritórios;
- arquivos;
- administração;
- segurança;
- servidores;
- almoxarifado;
- elevadores;
- escadas;
- sala de reuniões;
- sala de câmeras.

---

# 26. SALA DE CÂMERAS

Ferramenta estratégica de informação.

Permite:

- visualizar corredores;
- acompanhar companheiros;
- detectar movimentação;
- estudar caminhos;
- descobrir mudanças;
- encontrar pistas.

Nem todas as câmeras funcionam.

Algumas podem estar:

- quebradas;
- desligadas;
- sem energia;
- com interferência;
- apontando para locais inúteis;
- atrasadas;
- manipuladas pelo Hunter.

A sala não é segura.

Batman sabe que ela existe.

---

# 27. 1º ANDAR — EMERGÊNCIA

Área relativamente aberta.

Contém:

- recepção;
- emergência;
- ambulatórios;
- pronto atendimento;
- sala de espera;
- farmácia;
- segurança;
- entrada principal;
- depósito médico.

O depósito principal de medicamentos fica aqui.

Chegar ferido até essa região deve gerar tensão por causa da exposição.

---

# 28. SUBSOLO

Área técnica.

Atmosfera:

- apertada;
- escura;
- industrial;
- claustrofóbica.

Contém:

- geradores;
- tubulações;
- manutenção;
- quadros elétricos;
- depósitos;
- lavanderia;
- necrotério;
- túneis;
- acesso à garagem.

Algumas rotas do subsolo não aparecem nos mapas convencionais.

Isso cria atalhos poderosos, porém perigosos.

---

# 29. GARAGEM

Uma das principais rotas de fuga.

Contém:

- veículos abandonados;
- setores escuros;
- portão;
- manutenção;
- acessos técnicos;
- entradas para túneis;
- rotas para o exterior.

---

# 30. TELHADO

Área de alto risco.

Possíveis funções:

- rota do helicóptero;
- eventos narrativos;
- encontros;
- observação de Gotham;
- final específico.

---

# 31. EXPLORAÇÃO

O hospital será semiaberto.

O jogador possui objetivos, mas pode escolher caminhos.

Exemplo:

```text
ADMINISTRAÇÃO
      │
 ┌────┼────┐
Escada Elevador Corredor Técnico
```

A progressão frequentemente ocorre por informação.

Em vez de:

> “Encontre a chave.”

O processo pode ser:

> descubra que uma chave existe;

depois:

> descubra onde ela está;

depois:

> descubra como chegar até ela.

---

# 32. LOOP PRINCIPAL

```text
EXPLORAR
↓
ENCONTRAR INFORMAÇÃO
↓
PLANEJAR
↓
MOVIMENTAR-SE
↓
PERCEBER AMEAÇA
↓
DECIDIR
├── ESCONDER
├── FUGIR
├── DISTRAIR
├── LUTAR
└── ESPERAR
↓
SOBREVIVER
↓
CONTINUAR
```

Loop psicológico:

```text
“Preciso chegar lá.”
↓
“Será que ele está perto?”
↓
“Ouvi alguma coisa.”
↓
“Vou esperar.”
↓
“Está quieto...”
↓
“Será que ele foi embora?”
↓
“Vou sair.”
↓
“MERDA.”
```

Esse é o coração emocional do jogo.

---

# 33. STEALTH

Estados:

```text
UNDETECTED
↓
SUSPICIOUS
↓
INVESTIGATED
↓
DETECTED
↓
PURSUED
```

Joe pode interromper essa cadeia antes da perseguição.

Stealth considera:

- iluminação;
- ruído;
- movimento;
- distância;
- cobertura;
- comportamento;
- rotas;
- postura.

---

# 34. FURTIVIDADE FÍSICA

Ações:

- andar;
- andar lentamente;
- agachar;
- correr;
- fechar portas;
- abrir lentamente;
- evitar objetos;
- apagar luzes;
- atravessar zonas escuras.

Cada ação possui intensidade sonora.

---

# 35. PORTAS

Podem ser:

- abertas;
- fechadas;
- trancadas;
- quebradas;
- bloqueadas;
- eletrônicas;
- restritas.

Interações:

**Abrir lentamente**

baixo ruído / maior tempo.

**Abrir normalmente**

tempo normal / ruído médio.

**Arrombar**

rápido / ruído alto.

---

# 36. EQUIPAMENTO INICIAL

Joe inicia principalmente no Difícil com:

- 1 fuzil;
- 2 carregadores;
- 1 granada comum;
- 2 granadas de fumaça.

A configuração varia por dificuldade.

---

# 37. COMBATE

Combate é último recurso.

O fuzil pode:

- criar distância;
- interromper;
- assustar temporariamente;
- destruir objetos;
- abrir oportunidades;
- criar distração.

Contra o Hunter:

não funciona como duelo tradicional.

Disparar também revela Joe.

```text
TIRO
↓
RUÍDO EXTREMO
↓
EVENTO DE PERCEPÇÃO
↓
HUNTER REAGE
```

---

# 38. GRANADA COMUM

Pode:

- destruir obstáculos;
- interromper perseguições;
- gerar distrações;
- abrir caminhos;
- afetar momentaneamente o Hunter.

---

# 39. GRANADA DE FUMAÇA

Ferramenta extremamente valiosa.

Pode:

- quebrar linha de visão;
- permitir fuga;
- atravessar área exposta;
- confundir;
- criar janela de reposicionamento.

O Hunter também pode aprender o padrão de utilização.

---

# 40. RECURSOS

Principais recursos:

- munição;
- granadas;
- fumaça;
- medicamentos;
- ferramentas;
- baterias;
- pilhas;
- chaves;
- equipamentos de fuga.

Filosofia:

> “Eu realmente preciso usar isso agora?”

---

# 41. INVENTÁRIO

Pequeno e físico.

```text
ARMA
├── Fuzil
└── Munição

EXPLOSIVOS
├── Granada
└── Fumaça

MÉDICO
└── Medicamentos

UTILITÁRIOS
├── Baterias
├── Ferramentas
├── Pilhas
└── Chaves

INFORMAÇÃO
├── Documentos
├── Códigos
└── Pistas
```

Alguns itens competem por espaço.

Isso cria escolhas como:

> munição ou ferramenta?

---

# 42. FERIMENTOS

Três níveis principais.

## Leve

- pequeno impacto;
- respiração alterada;
- resistência levemente reduzida.

## Médio

- corrida limitada;
- respiração pesada;
- recuperação lenta;
- efeitos discretos de câmera.

## Grave

- dificuldade para correr;
- mobilidade reduzida;
- sangramento;
- alta vulnerabilidade.

Medicamentos podem:

- estabilizar;
- controlar sangramento;
- recuperar parcialmente.

Não precisam restaurar Joe magicamente.

---

# 43. STAMINA

Corrida utiliza resistência.

Quando baixa:

- Joe respira mais alto;
- perde velocidade;
- torna-se mais audível;
- demora para recuperar;
- fica vulnerável.

Stamina zerada não mata.

Ela facilita a captura.

---

# 44. MEDO

Não haverá barra explícita de Fear.

O medo é comunicado por:

- respiração;
- batimento;
- tremor;
- animações;
- áudio;
- câmera;
- movimentação;
- pequenas alterações perceptivas.

O efeito deve ser sutil.

---

# 45. ESCUTA

Não é wallhack.

Representa interpretação de som.

Pode comunicar:

- direção;
- distância aproximada;
- intensidade;
- tipo;
- movimento.

Nunca:

> “Batman — 13 metros.”

---

# 46. ESCONDERIJOS

Categorias principais:

## Físicos

- armários;
- lockers;
- compartimentos;
- depósitos.

## Ambientais

- escuridão;
- cortinas;
- equipamentos;
- cantos;
- manutenção.

## Emergenciais

- debaixo de maca;
- banheiro;
- atrás de porta;
- divisórias.

## Técnicos

- dutos;
- túneis;
- manutenção.

Possíveis variáveis:

```text
HideoutRisk
HideoutVisibility
HideoutNoise
HideoutDiscoveryChance
HunterSuspicion
```

Nenhum esconderijo deve ser universalmente seguro.

---

# 47. ESCONDERIJOS ESPECIAIS

## Caixa do almoxarifado

Muito eficiente.

Problema:

**oxigênio limitado.**

## Sala de cirurgia

Boa ocultação, muitos elementos.

Porém pode sofrer busca completa.

## Dutos

Alta eficiência durante perseguições.

Riscos:

- movimento lento;
- barulho;
- poucas saídas;
- possibilidade de encurralamento.

## Manutenção

Pouco visitada normalmente.

Mas se Joe for visto entrando, pode se tornar área prioritária.

---

# 48. LOCAIS SEGUROS

Esconderijo não significa safehouse.

Um esconderijo resolve segundos.

Um safehouse pode resolver uma fase longa da busca.

Devem existir no máximo poucos locais excepcionalmente seguros.

Exemplo:

**antiga sala de isolamento do 3º andar.**

Características:

- porta pesada;
- pequena janela;
- acesso pouco óbvio;
- isolamento;
- acesso restrito.

O jogador precisa descobrir que ela existe.

---

# 49. COMPANHEIROS COMO GAMEPLAY

Eles podem:

- encontrar itens;
- roubar itens;
- fornecer informações;
- mentir;
- criar distrações;
- abrir portas;
- fechar portas;
- salvar Joe;
- trair Joe;
- abandonar Joe;
- morrer;
- entregar informações.

Uma fala como:

> “Tem alguém no corredor.”

Pode ser:

- verdadeira;
- falsa;
- equivocada;
- manipulativa.

---

# 50. SISTEMA DE DECISÕES

Não depender principalmente de escolhas de diálogo.

As decisões importantes devem surgir do gameplay.

Exemplos:

- compartilhar munição;
- esconder munição;
- salvar alguém;
- abandonar alguém;
- revelar uma rota;
- mentir;
- usar determinado recurso;
- confiar em informação;
- utilizar companheiro como distração;
- escolher uma rota;
- tentar fuga antecipada;
- permanecer escondido;
- retornar para ajudar alguém.

---

# 51. CONSEQUÊNCIAS

Exemplo:

Joe entrega munição a Bartholomeu.

Resultado possível:

```text
Trust +20
Loyalty +10
Resource_Bartholomeu +1
```

Mais tarde ele pode:

- salvar Joe;
- abrir uma rota;
- devolver munição;
- recusar abandonar Joe.

Outro cenário:

Joe abandona Bartholomeu.

Resultado:

```text
Trust -40
Fear +20
SelfPreservation +30
```

Posteriormente ele pode:

- fugir sozinho;
- mentir;
- entregar Joe;
- trancar uma porta;
- atrair o Hunter.

---

# 52. CONDIÇÕES DE VITÓRIA

Três categorias principais:

```text
FUGIR
ESCONDER-SE
SOBREVIVER ATÉ O AMANHECER
```

Além de finais secretos/excepcionais.

---

# 53. ROTA DE FUGA A — GARAGEM

A rota mais previsível, mas trabalhosa.

Etapas:

1. descobrir a existência da chave;
2. descobrir que está dentro de um cofre;
3. localizar o cofre na Administração;
4. encontrar a combinação de seis dígitos;
5. reunir pistas;
6. abrir o cofre;
7. obter a chave;
8. alcançar a garagem;
9. reativar/liberar o portão;
10. escapar.

Foco:

**exploração + informação + navegação.**

---

# 54. ROTA B — TELHADO / HELICÓPTERO

Joe encontra um rádio no 1º andar.

Depois precisa:

- obter pilhas;
- montar o equipamento;
- estabelecer comunicação;
- contornar interferência;
- contatar seu chefe.

Quando o helicóptero é confirmado:

**temporizador de aproximadamente cinco minutos.**

Joe precisa chegar ao telhado.

Essa fuga transforma uma rota investigativa em uma sequência intensa.

---

# 55. ROTA C — TÚNEIS

Joe descobre túneis subterrâneos.

Precisa:

- localizar entrada;
- navegar pelo complexo;
- aprender o percurso;
- evitar becos sem saída;
- encontrar saída exterior.

No Punitivo, Robin pode entrar nos túneis.

Isso cria:

```text
JOE
↓
LABIRINTO
↓
SAÍDA

ROBIN
↑
INVESTIGAÇÃO
```

---

# 56. ROTA D — JANELA

Uma sala específica possui acesso a um prédio vizinho.

A janela está bloqueada.

Joe precisa:

- descobrir a rota;
- localizar ferramentas;
- remover travas;
- realizar o processo silenciosamente;
- atravessar.

Foco:

**exploração + stealth.**

---

# 57. VITÓRIA POR ESCONDERIJO

Joe encontra um local suficientemente seguro.

O objetivo passa a ser reduzir progressivamente a confiança do Hunter de que Joe ainda está naquela região.

Não existe botão:

> “Esperar até vencer.”

O jogador precisa realmente:

- evitar ruído;
- controlar respiração;
- sobreviver às buscas;
- evitar deixar pistas.

---

# 58. AMANHECER

Disponibilidade dependente da dificuldade.

## Fácil

Tempo relativamente curto.

## Normal

Exige maior sobrevivência.

## Difícil

É uma vitória longa e difícil.

## Punitivo

Não concede vitória automática.

Joe precisa fugir ou fazer o Hunter abandonar efetivamente a busca.

---

# 59. FINAIS EXCEPCIONAIS

Rotas contra Batman não fazem parte da solução convencional do jogo.

Devem ser raras, difíceis e narrativamente custosas.

## Fácil — Confronto

Joe reúne sobreviventes e prepara uma tentativa coordenada.

Não deve se transformar em boss fight convencional.

O resultado pode envolver:

- incapacitação temporária do Hunter;
- mortes;
- ferimentos graves;
- fuga.

## Punitivo — Destruição

Rota secreta e extrema.

Joe instala explosivos em diferentes pontos do hospital.

Precisa:

- encontrar explosivos;
- preparar pontos;
- percorrer várias áreas;
- ativar detonador;
- sobreviver ao colapso.

Resultado planejado:

- hospital destruído;
- companheiros mortos ou desaparecidos;
- Hunter aparentemente morto/preso nos destroços;
- Joe gravemente ferido;
- amanhecer;
- polícia encontra Joe;
- Joe é preso.

Vitória amarga.

---

# 60. CONDIÇÕES DE DERROTA

Nem toda falha produz Game Over imediato.

Categorias:

```text
HUNTER
COMPANHEIROS
AMBIENTE
ESTADO FÍSICO
FUGAS
```

---

# 61. HUNTER

Pode causar:

- captura;
- incapacitação;
- derrota durante perseguição;
- interceptação;
- falha em rota de fuga.

Por coerência narrativa, a derrota padrão causada pelo Batman pode ser apresentada como **captura/incapacitação**, não necessariamente execução.

---

# 62. COMPANHEIROS

Podem provocar:

- abandono;
- roubo;
- barulho;
- traição;
- bloqueio de rota;
- entrega de informações;
- armadilhas improvisadas;
- conflito direto.

---

# 63. AMBIENTE

Joe pode morrer por:

- quedas;
- incêndio;
- explosão;
- choque elétrico;
- estruturas instáveis;
- desabamentos;
- fumaça;
- áreas contaminadas;
- acidentes de elevador;
- inundação.

---

# 64. SANGRAMENTO

```text
Ferimento grave
↓
Sangramento
↓
Mobilidade reduzida
↓
Desorientação
↓
Colapso
↓
Morte
```

Isso cria urgência sem simulação médica excessiva.

---

# 65. FALHA DE ROTAS

Exemplos:

- helicóptero vai embora;
- garagem é bloqueada;
- janela é descoberta;
- ferramenta é perdida;
- túnel é interceptado.

Isso geralmente elimina **uma rota**, não a partida inteira.

---

# 66. DIFICULDADES

# FÁCIL

Duração aproximada:

**30 minutos.**

Características:

- tempo acelerado;
- IA mais permissiva;
- companheiros confiáveis;
- mapa permanente;
- recursos abundantes;
- escuta eficiente;
- Hunter menos adaptativo;
- maior margem de erro.

Objetivo:

apresentar os sistemas.

---

# NORMAL

Duração aproximada:

**1h30.**

Características:

- ritmo intermediário;
- companheiros quase confiáveis;
- mapa disponível;
- sala de câmeras fornece vantagens;
- recursos moderados;
- escuta disponível;
- Hunter mais ativo;
- consequências relevantes.

---

# DIFÍCIL

Experiência principal.

Duração aproximada:

**3 horas.**

Características:

- Hunter altamente eficiente;
- memória e adaptação completas;
- companheiros priorizam sobrevivência;
- traições possíveis;
- ausência de mapa permanente;
- maior importância da sala de câmeras;
- códigos e informações necessários;
- recursos limitados;
- escuta imprecisa;
- alto impacto das decisões.

Loadout-base:

- fuzil;
- 2 carregadores;
- 1 granada;
- 2 fumaças.

---

# PUNITIVO

Experiência extrema.

Características:

- sem mapa;
- sem sistema de escuta;
- recursos extremamente escassos;
- companheiros imprevisíveis;
- esconderijos perdem eficiência;
- Hunter altamente adaptativo;
- Robin pode aparecer;
- Coringa pode existir;
- Easter eggs adicionais;
- rotas podem ser alteradas;
- sem vitória automática pelo amanhecer.

Condições convencionais:

> escapar;

ou:

> fazer o Hunter abandonar a busca.

---

# 67. MÚSICA ADAPTATIVA

Estados:

```text
EXPLORATION
TENSION
INVESTIGATION
HUNTER_NEAR
PURSUIT
HIDING
RECOVERY
```

Exemplo:

Exploração:

ambiente + drones discretos.

Ameaça:

camada grave.

Investigação:

pulso.

Hunter próximo:

tensão crescente.

Perseguição:

música intensa.

Esconderijo:

trilha desaparece progressivamente.

O silêncio faz parte da composição.

---

# 68. ÁUDIO

Áudio é uma ferramenta de gameplay.

Fontes:

- passos;
- portas;
- respiração;
- vento;
- chuva;
- máquinas;
- tubulações;
- elevadores;
- objetos;
- vozes;
- rádios;
- equipamento do Hunter;
- ambiente.

O jogador pode sobreviver apenas porque ouviu algo antes de vê-lo.

---

# 69. NARRATIVA SECUNDÁRIA

A narrativa opcional expande o mundo e recompensa exploração.

Não deve ser necessária para compreender a história principal.

---

# 70. DOCUMENTOS

Exemplos:

- prontuários;
- registros médicos;
- documentos administrativos;
- relatórios de segurança;
- mapas;
- ordens de evacuação;
- documentos sobre desaparecimentos;
- fechamento do hospital;
- anotações;
- documentos policiais;
- informações do assalto.

Também podem conter pistas.

---

# 71. GRAVAÇÕES

Tipos:

- segurança;
- emergência;
- telefone;
- médicos;
- pacientes;
- rádio policial;
- mensagens;
- equipamentos.

Exemplo:

> “Se alguém estiver ouvindo isso... não use a escada oeste.”

O motivo pode só ficar claro posteriormente.

---

# 72. CÓDIGOS

Podem proteger:

- cofres;
- segurança;
- arquivos;
- elevadores;
- depósitos;
- câmeras;
- áreas técnicas.

As pistas podem estar:

- em documentos;
- gravações;
- paredes;
- objetos;
- diferentes salas;
- com NPCs.

Nem todo puzzle deve ser obrigatório.

---

# 73. SEGREDOS

Exemplos:

- salas escondidas;
- túneis;
- arquivos;
- passagens;
- cadáveres;
- equipamentos;
- objetos pessoais;
- evidências de investigações anteriores.

Alguns liberam rotas.

---

# 74. NARRATIVA AMBIENTAL

O hospital conta sua história através de:

- marcas;
- equipamentos;
- sangue antigo;
- fotografias;
- luzes;
- salas abandonadas;
- objetos;
- cadáveres;
- sons;
- alterações ambientais.

Progressivamente o jogador percebe:

> o hospital talvez não estivesse tão abandonado quanto parecia.

---

# 75. SEGREDOS DO HUNTER

Joe pode encontrar:

- câmeras alteradas;
- pontos de observação;
- equipamentos;
- marcas;
- acessos utilizados;
- objetos reposicionados.

Isso sugere:

> Batman já conhecia aquele lugar.

Joe não encontrou um esconderijo.

Joe entrou em um território que já estava sendo observado.

---

# 76. ROBIN

Principalmente no Punitivo.

Pode aparecer através de:

- passos distintos;
- sombras;
- voz;
- equipamentos;
- túneis;
- comunicação;
- aparições rápidas.

Não deve funcionar como segundo Hunter permanente.

Sua presença precisa permanecer especial.

---

# 77. ALFRED

Comunicação externa.

Pode fornecer contexto sobre:

- tempo;
- movimentação;
- situação;
- estado do Batman;
- hospital.

Joe normalmente escuta apenas partes.

---

# 78. GORDON

Representa o cerco exterior.

Exemplo:

Gordon:

> “A polícia isolou o quarteirão.”

Batman:

> “Mantenha-os fora.”

Isso explica por que simplesmente alcançar a saída não garante liberdade.

---

# 79. FLASH

Easter egg extremamente raro.

Pode existir apenas como:

- comunicação;
- sombra;
- passagem rápida;
- referência.

Não interfere diretamente na campanha principal.

---

# 80. CORINGA — PUNITIVO

Conteúdo secreto.

No Punitivo, Jack Oswald White pode não ser quem aparenta. Fora desse modo ele ainda relata ter sobrevivido a um encontro com Batman e ri disso, mas a revelação secreta não é antecipada.

Pistas:

- informação que Joe ainda não verificou, mas que Jack poderia ter obtido por percurso ou observação própria;
- ausência incomum de medo;
- desaparecimentos;
- manipulação;
- respostas estranhas;
- informações privilegiadas.

Nunca existe confirmação inicial.

Possível revelação:

Joe:

> “Quem diabos é você?”

Silêncio.

> “Você realmente não me reconheceu?”

Posteriormente:

> “Você passou a noite inteira tentando fugir do monstro.”

Pausa.

> “Nunca percebeu que havia outro monstro andando com você.”

---

# 81. PROGRESSÃO DA CAMPANHA

Estrutura conceitual:

## Ato 1 — Entrada e ruptura

- fuga da polícia;
- entrada no hospital;
- Bat-Sinal;
- Thomas desaparece;
- separação;
- Joe aprende movimentação e stealth.

## Ato 2 — Compreensão

- exploração inicial;
- primeiros recursos;
- contato com companheiros;
- descoberta de câmeras;
- primeiro grande encontro com Hunter;
- percepção de que ele investiga padrões.

## Ato 3 — Hospital como ferramenta

- múltiplos andares acessíveis;
- rotas conhecidas;
- relações se deterioram ou fortalecem;
- Hunter começa a antecipar hábitos;
- Joe descobre rotas de fuga.

## Ato 4 — Pressão

- recursos baixos;
- ferimentos;
- traições;
- rotas bloqueadas;
- agressividade elevada;
- possíveis eventos especiais.

## Ato 5 — Escolha emergente

Joe não seleciona um final em menu.

As circunstâncias da partida empurram o jogador para:

- garagem;
- helicóptero;
- túnel;
- janela;
- esconderijo;
- amanhecer;
- final secreto.

---

# 82. ESTRUTURA DE PROJETO

```text
TheThug/
│
├── 01_PROJECT/
├── 02_GAME_DESIGN/
├── 03_NARRATIVE/
├── 04_TECHNICAL/
├── 05_ART/
├── 06_AUDIO/
├── 07_LEVELS/
├── 08_PROTOTYPES/
├── 09_TESTING/
├── 10_BUILD/
│
├── UnrealProject/
├── Tools/
└── README.md
```

---

# 83. UNREAL PROJECT

```text
UnrealProject/
├── Config/
├── Content/
├── Source/
├── Plugins/
├── Script/
└── TheThug.uproject
```

Não versionar normalmente:

```text
Binaries/
DerivedDataCache/
Intermediate/
Saved/
```

---

# 84. CONTENT

```text
Content/
├── _Project/
├── Characters/
├── Player/
├── Hunter/
├── Companions/
├── AI/
├── Gameplay/
├── Environment/
├── Levels/
├── UI/
├── Audio/
├── VFX/
├── Materials/
├── Animations/
├── Cinematics/
├── Narrative/
├── Data/
├── Development/
└── Test/
```

---

# 85. _PROJECT

```text
_Project/
├── Blueprints/
├── GameModes/
├── GameInstance/
├── GameState/
├── PlayerState/
├── Controllers/
├── Interfaces/
├── Components/
├── Functions/
├── Macros/
├── Structs/
├── Enums/
└── Delegates/
```

Exemplos:

```text
BP_TheThugGameInstance
BP_TheThugGameMode
BPI_Interactable
ST_PlayerStatus
E_GameDifficulty
E_HunterState
```

---

# 86. HUNTER

```text
Hunter/
├── Character/
├── AIController/
├── Perception/
├── Memory/
├── Investigation/
├── Search/
├── Pursuit/
├── Patrol/
├── Navigation/
├── Adaptation/
├── Decision/
├── Components/
├── Data/
└── Animations/
```

Assets conceituais:

```text
BP_HunterCharacter
BP_HunterAIController
BT_Hunter
BB_Hunter
E_HunterState
E_HunterAlertLevel
DA_HunterBehavior
DA_HunterDifficulty
```

---

# 87. COMPANIONS

```text
Companions/
├── Characters/
├── AI/
├── Personality/
├── Trust/
├── Decisions/
├── Relationships/
├── Dialogue/
├── Data/
└── Animations/
```

---

# 88. GAMEPLAY

```text
Gameplay/
├── Health/
├── Damage/
├── Stealth/
├── Noise/
├── Hiding/
├── Interaction/
├── Inventory/
├── Resources/
├── Weapons/
├── Medical/
├── Time/
├── Objectives/
├── Decisions/
├── Save/
└── Difficulty/
```

---

# 89. ENVIRONMENT

```text
Environment/
├── Hospital/
│   ├── Architecture/
│   ├── Floors/
│   ├── Rooms/
│   ├── Corridors/
│   ├── Medical/
│   ├── Administrative/
│   ├── Technical/
│   ├── Basement/
│   └── Garage/
├── Props/
├── Doors/
├── Elevators/
├── Cameras/
├── Lights/
├── HidingSpots/
└── Interactive/
```

---

# 90. LEVELS

```text
Levels/
├── Prototype/
├── Blockout/
├── Production/
├── Test/
└── Cinematic/
```

Produção futura:

```text
Hospital_Floor01
Hospital_Floor02
Hospital_Floor03
Hospital_Floor04
Hospital_Basement
Hospital_Garage
Hospital_Roof
Hospital_Exterior
```

---

# 91. AUDIO

```text
Audio/
├── Music/
│   ├── Exploration/
│   ├── Tension/
│   ├── Investigation/
│   ├── Pursuit/
│   ├── Hiding/
│   └── Stingers/
├── SFX/
├── Ambience/
├── Foley/
├── Voice/
├── Dialogue/
├── MetaSounds/
├── SoundClasses/
├── SoundMixes/
├── Reverb/
└── AudioData/
```

---

# 92. DATA

```text
Data/
├── Player/
├── Hunter/
├── Companions/
├── Weapons/
├── Items/
├── Medical/
├── Difficulty/
├── Audio/
├── Levels/
├── Events/
└── Narrative/
```

Grande parte do balanceamento deve ficar orientada a dados.

---

# 93. DEVELOPMENT

```text
Development/
├── Sandbox/
├── Debug/
├── Experiments/
├── AI/
├── Audio/
├── Movement/
├── Combat/
└── Performance/
```

Experimentação fica separada dos assets finais.

---

# 94. C++

```text
Source/
└── TheThug/
    ├── Core/
    ├── Characters/
    │   ├── Player/
    │   ├── Hunter/
    │   └── Companions/
    ├── AI/
    ├── Gameplay/
    ├── Interaction/
    ├── Inventory/
    ├── Weapons/
    ├── Health/
    ├── Stealth/
    ├── Audio/
    ├── Narrative/
    ├── Save/
    └── UI/
```

Regra:

> **C++ = fundação e sistemas.**

> **Blueprint = composição, comportamento de conteúdo, configuração e prototipação.**

Não transformar tudo em C++ sem necessidade.

---

# 95. DOCUMENTAÇÃO EXTERNA

```text
02_GAME_DESIGN/
├── GDD/
├── Gameplay/
├── Systems/
├── Mechanics/
├── Levels/
├── Difficulty/
└── UI/
```

```text
03_NARRATIVE/
├── Story/
├── Characters/
├── Chapters/
├── Dialogues/
├── Endings/
└── Lore/
```

```text
04_TECHNICAL/
├── Architecture/
├── AI/
├── Cpp/
├── Blueprint/
├── Audio/
├── Optimization/
├── Performance/
├── SaveSystem/
└── Development/
```

---

# 96. PRODUÇÃO

```text
01_PROJECT/
├── Roadmap/
├── Milestones/
├── Tasks/
├── Bugs/
├── Decisions/
├── MeetingNotes/
└── VersionHistory/
```

Milestones:

```text
M00_PreProduction
M01_CorePrototype
M02_Hunter
M03_Stealth
M04_Companions
M05_HospitalBlockout
M06_VerticalSlice
M07_Alpha
M08_Beta
M09_Final
```

---

# 97. ORDEM DE IMPLEMENTAÇÃO

## Fase 1 — Core Prototype

Criar:

- personagem;
- primeira pessoa;
- câmera;
- movimentação;
- interação;
- lanterna;
- porta;
- esconderijo.

## Fase 2 — Hunter básico

Criar:

- AI Controller;
- navegação;
- patrulha;
- visão;
- audição;
- investigação;
- perseguição;
- perda do alvo.

## Fase 3 — Stealth

Criar:

- ruído;
- iluminação;
- esconderijos;
- linha de visão;
- distrações.

## Fase 4 — Sistemas de sobrevivência

Criar:

- saúde;
- ferimentos;
- stamina;
- armas;
- munição;
- granadas;
- medicamentos;
- inventário.

## Fase 5 — Companheiros

Criar:

- IA;
- confiança;
- medo;
- decisões;
- ajuda;
- abandono;
- traição.

## Fase 6 — Hospital

Criar:

- blockout completo;
- conexões;
- câmeras;
- medicamentos;
- atalhos;
- esconderijos;
- rotas de fuga.

## Fase 7 — Tempo

Criar:

- relógio;
- velocidades;
- amanhecer;
- comportamento por dificuldade.

## Fase 8 — Hunter avançado

Criar:

- memória;
- suspeita;
- aprendizado;
- previsão;
- emboscada;
- reposicionamento;
- armadilhas.

## Fase 9 — Áudio

Criar:

- ambiência;
- passos;
- respiração;
- áudio do Hunter;
- música adaptativa;
- silêncio dinâmico.

## Fase 10 — Decisões

Criar:

- relações;
- eventos;
- consequências;
- finais;
- rotas emergentes.

## Fase 11 — Polimento

Adicionar:

- modelos;
- animações;
- VFX;
- iluminação;
- cinematics;
- UI final;
- performance;
- otimização.

---

# 98. PRIMEIRO PROTÓTIPO JOGÁVEL

Não construir três horas inicialmente.

Construir aproximadamente **10 minutos excelentes**.

Área pequena contendo:

- Joe;
- câmera;
- movimentação;
- lanterna;
- corredor;
- salas;
- uma porta;
- um esconderijo;
- iluminação;
- ruído;
- Hunter;
- visão;
- audição;
- investigação;
- perseguição;
- possibilidade de despistar;
- pequena saída.

Objetivo:

provar que simplesmente andar por um corredor enquanto o Hunter procura Joe já consegue ser divertido e assustador.

Se esse núcleo funcionar, o restante do hospital multiplica o sistema.

---

# 99. PRINCÍPIOS DE IMPLEMENTAÇÃO

Durante o desenvolvimento de cada sistema, a documentação deve responder:

1. o que estamos criando;
2. por que estamos criando;
3. como funciona;
4. quais classes serão utilizadas;
5. quais assets serão criados;
6. como serão nomeados;
7. como C++ e Blueprint se comunicam;
8. como testar;
9. como depurar;
10. quais são os critérios para considerar a tarefa concluída.

Assumir que o desenvolvedor pode conhecer programação, mas ainda estar aprendendo Unreal Engine.

Nunca pular etapas fundamentais.

---

# 100. REGRAS QUE NÃO DEVEM SER QUEBRADAS

Não transformar o Hunter em inimigo convencional.

Não transformar THE THUG em shooter.

Não entregar posição exata do Hunter gratuitamente.

Não lotar a tela de HUD.

Não depender de jumpscares.

Não fazer Batman aparecer o tempo inteiro.

Não criar mapa enorme artificialmente.

Não utilizar IA trapaceira.

Não tornar todo esconderijo seguro.

Não transformar companheiros em simples seguidores.

Não fazer as decisões existirem apenas em diálogo.

Não fornecer quantidade absurda de recursos.

Não transformar o fuzil em solução universal.

Não permitir que a sala de câmeras funcione como mapa perfeito.

Não fazer todas as partidas seguirem exatamente os mesmos acontecimentos.

---

# 101. IDENTIDADE FINAL

THE THUG não é fundamentalmente:

> “um jogo do Batman”.

É:

> **um jogo de terror no qual você experimenta o Batman da perspectiva de quem está sendo caçado por ele.**

A principal emoção buscada não é medo de morrer.

É:

> **medo de ser encontrado.**

O jogador entrou em um prédio acreditando estar fugindo da polícia.

Gradualmente percebe uma verdade muito pior:

> ele não encontrou um esconderijo.

> ele entrou na área de caça de outra pessoa.

E, durante toda a experiência, uma pergunta acompanha cada corredor, porta, ruído e sombra:

> **“Onde ele está?”**
