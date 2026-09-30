# The Thug — ações entre sistemas

Coordenador C++17 sem Unreal para atos de gameplay que mudam mais de um núcleo. A primeira ação é `ShareAmmunition`: uma entrega validada pelo mundo consome uma unidade de munição do inventário de Joe, credita uma ao companheiro e aplica confiança +20 e lealdade +10, conforme o GDD (seção 51).

O coordenador rejeita item desconhecido ou de outra categoria, falta de estoque, ID de evento vazio/repetido, tempo inválido/anterior ao último evento e transbordamento da munição do companheiro. Nenhum desses casos altera os participantes. A chamada é síncrona no mesmo thread de jogo; o adaptador da Unreal precisa validar proximidade, disponibilidade física e animação/interação antes de chamá-la. O catálogo deve definir uma unidade do item escolhido como **uma unidade transferível** para o companheiro.

O ID de evento, o inventário e o estado do companheiro são gravados no **mesmo snapshot** pelo [SaveCore](../SaveCore/README.md). Na integração, pausar o mundo para capturar esse estado e gravar um arquivo temporário antes de substituir o save anterior; saves separados ou parciais ainda podem perder a integridade do resto da campanha.

```sh
bash Tools/IntegrationCore/run-tests.sh
```

Com CMake instalado, também é possível compilar e executar `ctest` em `Tools/IntegrationCore`. O projeto inclui os dois núcleos dependentes no build.
