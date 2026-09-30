# The Thug — som e evidência percebida

Núcleo C++17 que liga eventos do mundo ao [HunterCore](../HunterCore/README.md), inspirado nas seções 9–13 do [GDD Mestre](../../02_GAME_DESIGN/GDD/GDD_MASTER.md).

## Som

Um passo, porta, queda, tiro, explosão, respiração ou distração contém a posição **real apenas dentro da camada de propagação**. `Hear` recebe distância ao Hunter, transmissão após oclusão e máscara de ruído ambiente. Se o evento for audível, gera um estímulo com:

- origem **desconhecida**, mesmo que internamente o objeto seja Joe ou uma isca;
- classificação de som e intensidade percebida;
- centro estimado deslocado da origem real e raio de incerteza de pelo menos 4 m;
- tempo do evento.

O HunterCore decide se essa intensidade justifica investigar de acordo com a dificuldade. Tiros são mais audíveis que passos lentos, mas nenhum som revela um esconderijo ou rota usada. Parâmetros de alcance, volume e incerteza são **hipóteses de balanceamento**. Geometria, portas e materiais precisam fornecer transmissão real na Unreal.

## Evidência ambiental

`EvidenceMemory::Inspect` registra a primeira observação de uma porta, luz, câmera ou outro objeto como referência. Uma alteração só gera pista quando o Hunter **volta a inspecionar** o objeto. Se a mudança ocorreu e foi revertida sem inspeção, ele não aprende nada. O evento não declara que Joe causou a alteração.

## Testar

```sh
bash Tools/PerceptionCore/run-tests.sh
```

Ou CMake: `cmake -S Tools/PerceptionCore -B Build/PerceptionCore`, depois `cmake --build Build/PerceptionCore --config Release` e `ctest --test-dir Build/PerceptionCore -C Release --output-on-failure`.

O [contrato de integração](../../04_TECHNICAL/Architecture/CONTRATO_PERCEPCAO.md) descreve os dados do editor e limitações. Ainda faltam raycast/linha de visão real, áudio espacial, materiais, comportamento visual, animações e calibração em jogo.
