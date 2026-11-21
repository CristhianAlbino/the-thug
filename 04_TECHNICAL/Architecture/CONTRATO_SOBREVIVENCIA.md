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
- `ApplyCombat` confirma tiro/granada e devolve fontes sonoras; `BreathingSound` inclui controle/oxigênio e ferimentos. Encaminhar estímulos de áudio com localização aproximada conforme o contrato do [Hunter](CONTRATO_SISTEMAS.md). Nenhuma posição exata é criada aqui.
- [`ShareAmmunition`](../../Tools/IntegrationCore/README.md) coordena a entrega de Joe e a consequência no companheiro; a sessão usa `CampaignCore::Apply(ShareAmmo)` com recibo persistente. O mundo confirma proximidade/ação física antes da chamada. SaveCore grava os núcleos e StorySave acrescenta narrativa; runtime v9 conserva também pausa/ocupação/recibos/planos e recursos/saúde/tarefas individuais, com escritor de arquivo e recuperação. Aplicação nos atores Unreal continua pendente. Sangramento fatal de Joe é sincronizado ao fim da campanha; um checkpoint com Joe morto e campanha ainda ativa é rejeitado. Morte de companheiro é individual e não encerra Joe. `ApplyCompanion` acrescenta transferência de itens e medicina entre pessoas.
- Regras de capacidade, custo e quantidade por item devem vir de dados versionados. Os exemplos do teste servem só para demonstrar a escolha “munição ou ferramenta”.

## Pendências de integração

Catálogo e capacidade finais, relação entre nível de ferimento e dano, balanceamento em tempo real, demais transações entre módulos, ligação do save de ferimentos e mundo aos atores Unreal, som/respiração, feedback de câmera, efeitos de medicamento, impactos nos companheiros e teste Play. Os valores atuais são hipóteses de balanceamento e podem mudar após medir a experiência real.

O primeiro ciclo adicional de armas/consumíveis/respiração está no [contrato P07](CONTRATO_COMBATE_RESPIRACAO_V1.md). Runtime v5 conserva munição carregada, tarefas reservadas, projéteis/nuvens temporárias, interrupção/imunidade e oxigênio. Não usar o serializador isolado de SurvivalCore como checkpoint completo.
