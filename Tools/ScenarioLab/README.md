# ScenarioLab — progressão simulada sem Unreal

Executa caminhos reais do [`MapCore`](../MapCore/README.md), inclusive coleta de pistas, ferramentas e ações locais, e os envia em ordem cronológica para o `RunRules` **C++17 do jogo**. Há quatro fugas diretas, duas falhas com alternativa, atraso do helicóptero seguido da janela e uma escolha por semente. `--seed` controla a escolha no cenário `random`; os casos fixos são reproduzíveis pela própria sequência.

```bash
bash Tools/ScenarioLab/run-tests.sh
```

Para explorar um caso isolado, compile o driver e gere JSONL com o caminho e os eventos:

```bash
c++ -std=c++17 Tools/HunterCore/RunRules.cpp \
  Tools/ScenarioLab/rules_driver.cpp -o /tmp/the-thug-rules-driver
python3 Tools/ScenarioLab/simulate.py --driver /tmp/the-thug-rules-driver \
  --scenario roof_timeout_window --seed 42
```

O programa verifica que cada evento produzido pelo grafo é aceito pelo núcleo C++, que uma tentativa prematura de sair pela garagem é rejeitada, que a fuga termina pelo resultado esperado e que a falha alternativa persiste. No caso do helicóptero, a confirmação vem após visitar rádio/pilhas/câmeras; a espera de 301 s ocorre na sala do rádio; a janela é alcançada desde essa posição física.

`actor_scenarios.cpp` acrescenta um ensaio de apagão, câmera e porta, esconderijo, ruído físico → PerceptionCore → HunterCore, conhecimento/decisões de Bartholomeu, troca de munição com Harley e checkpoint conjunto com replay de escolhas por semente. A posição, oclusão e observação de entrada são fatos **fornecidos pelo roteiro de teste**; na Unreal, somente geometria/sensores podem confirmá-los. O save verifica que o estado dos dispositivos e as memórias dos atores seguem distintos após carregar.

**Limite:** cada aresta usa os segundos hipotéticos do mapa e cada ação custa cinco segundos arbitrários. A fase de rotas não simula posição métrica, colisão, Hunter, companheiros, luz, ruído ou combate. O ensaio de atores passa posições e visadas autoradas; não simula geometria, navegação física, combate, duração total nem timing real. Não prova as três horas de campanha. Esses sistemas serão ligados à geometria e aos atores da Unreal em etapas posteriores.
