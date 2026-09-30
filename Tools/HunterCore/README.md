# The Thug — regras sem Unreal

Código C++17 com duas partes destinadas à campanha completa:

- **HunterCore:** transforma percepções em intenção de investigar, perseguir, buscar ou patrulhar. Guarda apenas evidências observadas, com esquecimento gradual e escolhas falíveis.
- **RunRules:** acompanha quatro rotas de fuga, falhas individuais, prazo do helicóptero, amanhecer por dificuldade e resultados terminais.

Estes arquivos não criam um personagem, mapa, árvore de comportamento, sensor, helicóptero ou interface. O contrato de integração está em [CONTRATO_SISTEMAS.md](../../04_TECHNICAL/Architecture/CONTRATO_SISTEMAS.md).

## Rodar no VS Code / terminal

Com CMake e compilador C++17 instalados, na raiz do repositório:


```sh
cmake -S Tools/HunterCore -B Build/HunterCore
cmake --build Build/HunterCore --config Release
ctest --test-dir Build/HunterCore -C Release --output-on-failure
```

O diretório `Build/` já está ignorado pelo Git. Em um terminal com Bash e `g++`, também é possível executar `bash Tools/HunterCore/run-tests.sh`, que compila em uma pasta temporária e a remove ao terminar.

## Ponto de integração

A Unreal fornecerá somente eventos que seus sensores e objetos observaram. O código retorna intenção e resultado; a engine executará movimento, animações, efeitos e transições. Unidades desta biblioteca: **metros** e **segundos de tempo de jogo**. Ao integrar com coordenadas Unreal em centímetros, converter explicitamente nos dois sentidos.

Os números de balanceamento em `TuningFor` são valores iniciais para testes, não valores finais aprovados. A dificuldade muda limiares e persistência da memória, sem entregar informação privilegiada ao Hunter. Para reproduzir decisões em um mesmo ambiente, forneça uma semente; replays entre bibliotecas padrão C++ diferentes ainda devem gravar as escolhas de rota.

## Estado verificado

Compilado em C++17 com `g++`, avisos tratados como erros; testes cobrem ruído aproximado, isca, perseguição, perda de visão, memória observada, decaimento, decisões falíveis, rotas alternativas, prazo de fuga, dificuldades e terminação. Integração no editor Unreal ainda não foi executada.
