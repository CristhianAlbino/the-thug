# The Thug — checkpoint dos sistemas da campanha

`SaveCore` persiste os núcleos da campanha em C++17, sem Unreal. `SaveCampaign` grava **juntos** dez estados: inventário do Joe, Harley, Bartholomeu, Jack, Hunter, regras de fuga, sobrevivência, percepção sonora, memória de evidências e o registro de atores do mundo. `LoadCampaign` valida o arquivo inteiro, restaura cópias temporárias e só então troca os dez objetos vivos. Um erro deixa todos como estavam.

O formato anterior `THE_THUG_PARTY 1` continua disponível por `SaveParty`/`LoadParty` para testes e ferramentas; ele **não é convertido** em campanha completa porque não contém os outros estados. O checkpoint completo usa `THE_THUG_CAMPAIGN 1`.

O arquivo `THE_THUG_CAMPAIGN 1` contém:

| Estado | Conteúdo persistido |
| --- | --- |
| Joe | capacidade, catálogo de itens e quantidades |
| Cada companheiro | identidade, dificuldade, traços, ferimento, tempo, rotas conhecidas, IDs de eventos aplicados e estado do gerador aleatório |
| Hunter | tuning, estado/tempo, estimativa percebida, tipo ouvido, evidência de rotas/esconderijos e RNG |
| Rotas | etapas de objetivos, falhas isoladas, final, relógio e prazo do helicóptero |
| Sobrevivência | ferimento, stamina, sangue, sangramento, exaustão, colapso, morte e tuning |
| Percepção | tuning e RNG dos sons; estados vistos e relógio da inspeção |
| Mundo | ID da fase, relógio mestre e catálogo estável de objetos com estado, presença, posição em metros e orientação em graus |

O formato tem limite de 2 MiB, limites para IDs/quantidades de registros e checksum FNV-1a de 64 bits para identificar corrupção acidental. O checksum **não é assinatura de segurança**. Versões não suportadas, truncamento, alteração de bytes, catálogo incompatível, identidade trocada e recursos fora dos limites são rejeitados. O catálogo e a dificuldade do jogo devem corresponder aos da instância na qual se carrega; migração de versões e mudanças de catálogo virão com regras explícitas, sem conversão silenciosa.

Depois de carregar, repetir o ID de uma entrega de munição não desconta nem concede outra unidade; uma nova entrega funciona normalmente. Memória do Hunter, prazos das rotas, ferimentos, evidências vistas e objetos presentes continuam de onde pararam. As escolhas aleatórias prosseguem da sequência salva **no mesmo ambiente C++**. A reprodução exata entre bibliotecas/compiladores diferentes exige teste específico na Unreal.

O relógio do mundo deve estar pelo menos no tempo de cada núcleo antes de capturar um checkpoint. O catálogo do mundo precisa listar objetos persistentes com IDs estáveis; o adaptador Unreal fornece esses IDs e converte centímetros da engine para metros. `stateId` é definido pela lógica authored de cada ator (por exemplo `open` ou `collected`); a engine valida a transição antes de enviar a atualização. O código não inventa a planta do hospital nem movimenta atores.

```sh
bash Tools/SaveCore/run-tests.sh
```

Com CMake, `cmake -S Tools/SaveCore -B Build/SaveCore`, seguido de build e `ctest --test-dir Build/SaveCore --output-on-failure`, verifica também os núcleos dependentes.

**Ainda não é um save jogável na Unreal.** Faltam o catálogo real da fase, restauração de atores/portas/animações, velocidade física, combate em andamento, áudio, UI e qualquer subsistema novo que a engine criar. A integração deve pausar a simulação, gravar o arquivo em um temporário antes de substituir o anterior, carregar a fase e aplicar o registro do mundo antes de retomar o jogo. `SaveCampaign(std::ostream&)` apenas escreve no stream fornecido. Veja o [contrato de integração](../../04_TECHNICAL/Architecture/CONTRATO_SAVE_CAMPANHA.md).

## Envelope narrativo (pré-Unreal)

Para uma sessão coordenada, usar [`CampaignCore::Save/Load`](../CampaignCore/README.md), com envelope **`THE_THUG_RUNTIME_CAMPAIGN 2`** sobre StorySave v1. Ele persiste pausa, ocupação de Joe, recibos, identidade de autoria e planos/seriais de apresentação. Para disco, usar `checkpoint::SaveFile/LoadFile` desse módulo. Os formatos abaixo continuam úteis para testes dos núcleos; não substituir o envelope runtime por eles nem omitir seu registro de ações. Não há migração automática de runtime v1.

`StorySave.h` oferece `SaveStoryCampaign` / `LoadStoryCampaign` com cabeçalho **`THE_THUG_STORY_CAMPAIGN 1`**. Esse formato inclui o checkpoint puro inteiro e `NarrativeState`: IDs de cenas concluídas, conversas entregues, documentos/gravações inspecionados, pistas de Jack testemunhadas, fatos duráveis, ato, linhas de cena/conversa interrompidas, último horário da conversa, tentativas do teclado e provas individuais de epílogo. `LoadStoryCampaign` valida os dois blocos antes de trocar qualquer dos onze estados em memória. Confere dificuldade e relógio do mundo, recusa estados contraditórios e não considera ausência de notícia prova de morte. Não use `THE_THUG_CAMPAIGN 1` como campanha narrativa; ele continua válido apenas para testes dos dez núcleos.

`NarrativeState` guarda **fatos**, não conclui que Joe viu algo só por estar no mesmo nó. A sessão chama DirectorCore através de CampaignCore, confirma entradas realmente apresentadas e guarda a sequência selecionada no envelope externo. Este módulo inferior não implementa transação em disco, widgets, ator Unreal, streaming, navegação nem retomada de animação. O escritor de CampaignCore trata arquivo; pausar/reconciliar mundo e animação continua responsabilidade da ponte. A lista de fatos temporários vem do header compartilhado gerado a partir de `NarrativeCore/fact_lifetimes.json`. `bash Tools/SaveCore/run-tests.sh` também cobre o envelope narrativo inferior.
