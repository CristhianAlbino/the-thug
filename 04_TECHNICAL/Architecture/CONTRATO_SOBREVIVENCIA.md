# Contrato — sobrevivência, inventário e outros sistemas

Fonte: [GDD Mestre](../../02_GAME_DESIGN/GDD/GDD_MASTER.md), seções 36–43 e 64. Implementação: [SurvivalCore](../../Tools/SurvivalCore/README.md).

## Entradas autorizadas

| Origem no mundo | Entrada no núcleo | Condição |
| --- | --- | --- |
| Dano validado por colisão/combate/ambiente | `ApplyInjury(nível)` | O mapeamento dano → nível vem de dados de design |
| Uso confirmado de item médico | `Stabilize` ou `Treat` | O inventário deve possuir e consumir o item na mesma ação validada |
| Atualização de jogo | `Advance(deltaSegundos, atividadeTentada)` | Tempo finito, sem avanço enquanto pausado |
| Coleta/uso/descarte de objeto | `Inventory::Add/Remove` | O objeto físico só desaparece após sucesso da operação |
| Transferência de item | `TransferTo` | Ambos os inventários aceitam o item e a quantidade inteira |

## Saídas e efeitos

- `CurrentEffects` comunica capacidade de correr, velocidade relativa e volume relativo da respiração. O adaptador Unreal produz animação, câmera, áudio e ruído espacial; estes efeitos não identificam Joe magicamente para o Hunter.
- Quando `Advance` informa `becameDead`, o coordenador da campanha envia `RunRules::JoeDied`. `becameCollapsed` impede deslocamento até tratamento adequado; o colapso não é Game Over automático.
- Tiro, granada e respiração alta deverão produzir estímulos de áudio com localização aproximada conforme o contrato do [Hunter](CONTRATO_SISTEMAS.md). Nenhuma posição exata é criada aqui.
- `CompanionCore::JoeSharedAmmo` só deve ser aplicado após uma transferência real de recurso. A transferência de Joe e a consequência do companheiro precisam ser tratadas como **uma transação de gameplay** com ID persistente, evitando munição duplicada ou perdida. Este coordenador ainda precisa ser implementado e testado.
- Regras de capacidade, custo e quantidade por item devem vir de dados versionados. Os exemplos do teste servem só para demonstrar a escolha “munição ou ferramenta”.

## Pendências de integração

Catálogo e capacidade finais, relação entre nível de ferimento e dano, balanceamento em tempo real, transações entre módulos, persistência, som/respiração, feedback de câmera, efeitos de medicamento, impactos nos companheiros e teste Play na Unreal. Os valores atuais são hipóteses de balanceamento e podem mudar após medir a experiência real.
