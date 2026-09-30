# WorldCore — portas, energia, câmeras e esconderijos

`WorldCore` é uma camada C++17 de interações sobre o `WorldState` já salvo pelo `SaveCore`. O [grafo](../MapCore/hospital_graph.json) autoriza as instâncias por ID; o adaptador Unreal construirá os objetos persistentes com o mesmo catálogo. Os dados iniciais têm 22 portas, 3 circuitos, 5 câmeras e 10 esconderijos lógicos.

```bash
bash Tools/WorldCore/run-tests.sh
```

Portas `manual` abrem independentemente da energia; `needs_power` requer circuito ligado; `releases_when_off` mantém a possibilidade de abertura em blackout. Tranca e emperramento permanecem estados físicos, mesmo com energia. Forçar porta emite sinal alto; abrir normalmente emite sinal menor. Energia pode cortar acesso dependente e imagem ao vivo, sem apagar caminhos manuais alternativos. Câmera `looped` exibe frame antigo com `live=false`, nunca informação atual. Esconderijo ocupado permanece no save; a ocupação **não** informa Batman. Apenas sensor com linha de visão confirmada pode gerar `ConfirmedHideSpot` no HunterCore.

O método `UnlockDoor` pressupõe que o coordenador validou chave/ferramenta antes. `WorldSignal` traz origem real; PerceptionCore só produzirá uma estimativa incerta se o sinal alcançar um ouvinte com oclusão e mascaramento adequados. `WorldState` persiste objetos e relógio no checkpoint conjunto. A geometria, posições métricas, visão, navmesh, efeitos e áudio real ainda dependem da Unreal.
