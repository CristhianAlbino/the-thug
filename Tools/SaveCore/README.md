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
