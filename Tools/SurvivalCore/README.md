# The Thug — sobrevivência e inventário

C++17 sem dependência da Unreal. Implementa o núcleo de ferimentos, sangramento, stamina e capacidade física de inventário para a campanha completa, conforme as seções 36–43 e 64 do [GDD](../../02_GAME_DESIGN/GDD/GDD_MASTER.md).

## Sobrevivência

- Ferimentos: nenhum, leve, médio e grave. Dano só piora o estado; tratamento melhora no máximo um nível por aplicação.
- Ferimento grave provoca sangramento. Estabilizar para a perda; sem cuidado, perda de sangue leva a colapso e depois morte. Tratamento também reduz parte da perda, sem ressuscitar Joe.
- Corrida consome stamina. Ao esgotá-la, Joe passa a caminhar até recuperar o limiar; stamina zero não mata. Ferimentos médios e graves pioram gasto e recuperação.
- `Effects` informa velocidade, respiração audível, possibilidade de correr, colapso e morte. A engine aplica movimento, câmera, áudio e feedback visual. Não criar barra explícita de medo.

Os números de `Tuning` (velocidades, limiares e multiplicadores) são **hipóteses iniciais**, centralizadas para ajuste. O GDD estabelece a relação entre estados, mas não fixa esses valores. Uma atualização grande retorna quantos segundos de corrida foram possíveis antes de exaustão/colapso; a integração deve aplicar movimento pelo tempo efetivamente permitido.

## Inventário

- Catálogo de itens fornecido pelo design: ID, categoria, custo de espaço, máximo carregável e regra de descarte.
- Capacidade em unidades de espaço. Munição e ferramentas competem por capacidade; chaves podem ser protegidas contra descarte.
- Adição, consumo, descarte e transferência entre dois inventários são verificados antes de alterar quantidades. Quantidade negativa, item desconhecido, excesso e transferência parcial são rejeitados.
- O loadout do Difícil no GDD é **fuzil + 2 carregadores + 1 granada + 2 fumaças**. Os custos de espaço e capacidade não estão aprovados; exemplos dos testes não são a configuração final.

## Executar no terminal / VS Code

```sh
bash Tools/SurvivalCore/run-tests.sh
```

Ou com CMake e compilador C++17:

```sh
cmake -S Tools/SurvivalCore -B Build/SurvivalCore
cmake --build Build/SurvivalCore --config Release
ctest --test-dir Build/SurvivalCore -C Release --output-on-failure
```

O [contrato de integração](../../04_TECHNICAL/Architecture/CONTRATO_SOBREVIVENCIA.md) lista eventos entre inventário, companheiros, Hunter e regras de vitória. A entrega de munição a um companheiro já tem um [coordenador](../IntegrationCore/README.md) para ambos os estados em memória, e o [SaveCore](../SaveCore/README.md) persiste inventário e companheiros juntos. Save/load de ferimentos e validação no editor ainda faltam.
