# The Thug — diretor narrativo nativo

C++17 independente da engine. Implementa seleção de cenas/conversas, prioridade, suspensão, confirmação por fala/gesto, consulta de textos inspecionados e seleção de epílogos. A sessão usa estas funções através de [CampaignCore](../CampaignCore/README.md), preservando recibos e estado numa transação. Python continua como referência independente de regras.

## Conteúdo e autoria

`runtime_ptbr.json` contém textos pt-BR separados das instruções de produção do [roteiro revisado](../../02_GAME_DESIGN/Narrative/PACOTE_TEXTO_CAMPANHA_V1.md). `build_data.py` verifica cada texto contra essa fonte e gera `DirectorData.generated.h` a partir de cenas, conversas, variantes, mapa, pistas e catálogo de fatos. As 301 entradas incluem falas, gestos, textos de documento, transcrições e epílogos; não equivalem a 301 cenas ou assets concluídos.

Gestos e gravações sem texto final têm referência simbólica de asset, sem exibir uma nota de produção como legenda. A encenação física e o conteúdo dos assets continuam pendentes. Somente `pt-BR` está implementado: solicitar outro idioma retorna conteúdo vazio. Tradução não é substituída silenciosamente por texto em português.

`fact_lifetimes.json`, em NarrativeCore, é a fonte compartilhada dos fatos temporários. O gerador também exporta `SaveCore/NarrativeFacts.generated.h`. Observações de um encontro não se tornam conhecimento permanente. Pistas secretas entram exclusivamente no registro de fatos testemunhados e respeitam dificuldade, ato e marcos observados.

```sh
python3 Tools/DirectorCore/build_data.py
python3 Tools/DirectorCore/build_data.py --check
bash Tools/DirectorCore/run-tests.sh
```

## Protocolo de apresentação

1. A ponte observa nó, atores presentes/ativos, perigo, alcance e fatos realmente percebidos. Usa o relógio e o ato atuais da campanha.
2. `ApplyNarrative(BeginScene/BeginBanter, observation)` fixa uma sequência de IDs elegíveis e atribui um serial de ativação. Condições e atores ausentes não recebem falas automaticamente.
3. `CurrentPresentation` consulta a primeira entrada ainda não confirmada. Cena principal suspende conversa opcional; perigo ou ausência podem suspender a apresentação. A sequência fixada não é remontada por frame.
4. A ponte apresenta fala/gesto. Só depois de concluído envia `SceneCue/BanterCue` com **owner, serial, índice e cueId** recebidos. O `eventId` é o ID estável dessa ocorrência. Um callback de ativação cancelada não pode confirmar a ativação nova.
5. Confirmar a última entrada marca a cena/conversa concluída. Cancelar limpa a apresentação ativa sem fingir conclusão; fatos realmente entregues continuam registrados. Uma conversa concluída inicia seu intervalo persistente.

Retomada carrega a sequência selecionada, serial e índice da primeira entrada não confirmada. Uma fala que já terminou antes do corte não é repetida. Uma fala interrompida antes da confirmação pode reiniciar; seek de áudio/animação ainda pertence à ponte. Presença e perigo são reavaliados antes de apresentar ou confirmar. Se uma condição deixa de valer permanentemente, o adaptador cancela/reagenda explicitamente, sem pular linhas silenciosamente.

## Autoridade e limites

`Observation` descreve observações confiadas à ponte; não calcula raycast, distância nem navmesh. Visitar uma sala não prova leitura, gesto, descoberta ou conclusão de saída. CampaignCore deriva fatos que pertencem aos seus sistemas, impede fatos de rota inventados pelo diretor e registra leitura somente em local autorizado. `SubmitKeypad` confirma a entrada nativa e suas tentativas sem conceder documentos não lidos.

`RememberFact` aceita apenas fatos duráveis autorados permitidos à ponte. Ela ainda precisa comprovar o evento físico correspondente. Conhecimento específico de Joe não é criado pela coda destinada apenas ao jogador. Epílogos usam resultados individuais registrados; captura/morte não são deduzidas da ausência de notícia.

As funções de baixo nível operam snapshots para ensaio. Não as chamar paralelamente para alterar o estado da sessão. P08 liga N33 aos resultados raros confirmados pelo CampaignCore e acrescenta cinco apresentações especiais com exposição física/ACK. Ausentes não falam e a destruição não confirma morte do Hunter. Encenação final de Thomas/Jack continua P09. Não há atores, widgets, vozes ou cinematics produzidos por este módulo.

## Verificação

O ensaio nativo cobre prioridades, ausência, perigo, cancelamento, serial antigo, idioma, leitura e variantes. `parity.py` compara 8.384 vetores com funções Python independentes: cenas, conversas, retomada, pistas, materiais restritos e combinações de epílogo. CampaignCore acrescenta cenários integrados de save/load, recibos, diário, entrada de teclado e revelação permitida. Estes ensaios comprovam as regras testadas; não substituem leitura em voz alta ou uma sessão jogável.
