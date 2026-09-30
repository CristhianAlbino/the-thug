# The Thug — save da equipe e do inventário

`SaveCore` é a primeira fatia de persistência da campanha em C++17, sem Unreal. `SaveParty` grava **juntos** o inventário de Joe e os estados de Harley, Bartholomeu e Jack. `LoadParty` valida o arquivo inteiro, restaura cópias temporárias e só então troca os quatro objetos vivos. Um erro deixa os quatro como estavam.

O arquivo `THE_THUG_PARTY 1` contém:

| Estado | Conteúdo persistido |
| --- | --- |
| Joe | capacidade, catálogo de itens e quantidades |
| Cada companheiro | identidade, dificuldade, traços, ferimento, tempo, rotas conhecidas, IDs de eventos aplicados e estado do gerador aleatório |

O formato tem limite de 2 MiB, limites para IDs/quantidades de registros e checksum FNV-1a de 64 bits para identificar corrupção acidental. O checksum **não é assinatura de segurança**. Versões não suportadas, truncamento, alteração de bytes, catálogo incompatível, identidade trocada e recursos fora dos limites são rejeitados. O catálogo e a dificuldade do jogo devem corresponder aos da instância na qual se carrega; migração de versões e mudanças de catálogo virão com regras explícitas, sem conversão silenciosa.

Depois de carregar, repetir o ID de uma entrega de munição não desconta nem concede outra unidade; uma nova entrega funciona normalmente. As escolhas aleatórias prosseguem da sequência salva **no mesmo ambiente C++**. A reprodução exata entre bibliotecas/compiladores diferentes exige teste específico na Unreal.

```sh
bash Tools/SaveCore/run-tests.sh
```

Com CMake, `cmake -S Tools/SaveCore -B Build/SaveCore`, seguido de build e `ctest --test-dir Build/SaveCore --output-on-failure`, verifica também os núcleos dependentes.

**Ainda não é o save completo da campanha.** Faltam Hunter, objetivos/rotas de fuga, sobrevivência, memória de evidências, posições e estados físicos do mundo. A integração com Unreal deve pausar a simulação durante a captura e gravar o arquivo em um temporário antes de substituir o save anterior; `SaveParty(std::ostream&)` apenas escreve no stream fornecido.
