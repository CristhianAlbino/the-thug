# MapCore — planta lógica fora da Unreal

O [documento de direção](../../02_GAME_DESIGN/World/MAPA_LOGICO_HOSPITAL.md) explica o hospital. `map_data.py` é a fonte autorada; `hospital_graph.json` é a exportação estável para inspeção e futura importação na Unreal. Ambos são versionados para que divergência seja detectada no CI.

```bash
python3 Tools/MapCore/build_map.py
python3 Tools/MapCore/validate_map.py
```

Requer só Python 3 com biblioteca padrão. O validador confere integridade dos IDs, travas e eventos de `RunRules.h`, um caminho real por estado para cada fuga, a independência de cada falha, três saídas falhas ao mesmo tempo, escadas alternativas, elevador dispensável e a estimativa de travessia rádio → telhado abaixo de 300 segundos. Esse último número cobre **somente deslocamento sem perseguição, interação ou espera**; o tempo jogável precisa de teste na Unreal. O `run_event` informa a transição no núcleo C++ e deve ser emitido quando a ação realmente for concluída.

Contrato JSON (`schema: 1`):

| Campo | Interpretação |
| --- | --- |
| `floors` | Agrupamento lógico e ordem vertical; `A`, `T`, `X` são anexo, túneis e exterior |
| `nodes` | IDs estáveis, nome para autoria, papel e clima; ainda sem coordenadas nem dimensões |
| `edges` | Origem/destino, classe, `seconds` provisórios, flags `requires`/`forbids`, `one_way`, `hunter_access` e exposição |
| `actions` | Interações locais com `requires`, `grants` e evento opcional de `RunRules` |
| `sightlines` | Referência de visada a ser verificada na geometria; não cria percepção ou caminho por si |
| `exits` | Nó final, flag de vitória e flag de falha independente por rota |
| `devices` | Circuitos, portas sobre ligações exatas, câmeras com visão e esconderijos associados aos nós; estados iniciais para `WorldCore` |

`hunter_access: false` marca somente o trecho estreito; perseguição indireta e investigação de entradas continuam possíveis. As flags são estados de campanha a persistir, incluindo falhas; um inventário físico e o tempo real da ação devem ser conciliados com SurvivalCore/SaveCore na integração. Não use os `seconds` como duração de missão, curva de dificuldade ou fato medido.

O validador também verifica IDs e ligações dos dispositivos e prova que cada fuga permanece alcançável mesmo com a porta inicialmente trancada e todas as portas `needs_power` fechadas. Essa análise pressupõe que portas manuais podem ser abertas; perseguição, tempo da interação e bloqueios concretos devem ser reavaliados no blockout.

## Blockout jogável (Unreal)

`blockout_floors.py` traduz este grafo num blockout de 7 níveis (garagem, B1, 1º–4º e telhado) em grades ASCII de 24×15,
com duas escadas em zigue-zague, prumada de serviço (`V`/`v`, só o Joe passa), sala de câmeras, isolamento e as saídas
já implementadas no protótipo (janela do 3º e portão da garagem). `python Tools/MapCore/blockout_floors.py` valida
larguras, escadas, pares de escotilhas e alcance de todas as células a partir do início; `--emit` gera a tabela C++.
Túneis, anexo, telhado/helicóptero, energia/elevador e cofre com código ainda não estão no blockout.
