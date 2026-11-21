# The Thug — kit do primeiro dia na Unreal

**Estado:** plano para execução no novo computador; não há `.uproject`, `.umap` ou `.uasset` no repositório ainda. O código C++17 puro não é um Actor Unreal arrastável. A versão específica da engine/toolchain será escolhida ao instalar (D-01). Clonar o repositório preserva histórico e atualização; ZIP é apenas cópia estática.

## Ordem de criação e integração

1. Instalar Unreal Engine 5 e toolchain C++ compatível no notebook; clonar `CristhianAlbino/the-thug` com GitHub Desktop ou Git. Criar projeto Games/C++ `TheThug` em pasta versionada do clone. Confirmar que `TheThug.uproject` abre e um projeto vazio compila **antes** de copiar lógica.
2. Adicionar módulos/compilação do projeto e adaptar os núcleos puros em camadas, começando `RunRules`, `SurvivalCore`, `CompanionCore`, `HunterCore`, `PerceptionCore`, `WorldCore`, `SaveCore` e `DirectorCore`; expor a sessão por `CampaignCore`, dono único de relógio/estado/transações. Não alterar os núcleos internos por pontes paralelas. O coordenador usa perfis/vínculos exportados de JSON; o diretor nativo já tem paridade contra Python. Confirmar conversão de unidades **cm Unreal ↔ m núcleos**, relógio monotônico e enums de dificuldade/eventos; detalhes nos contratos [dos adaptadores](CONTRATO_ADAPTADORES_UNREAL_V1.md) e [do coordenador](CONTRATO_COORDENADOR_CAMPANHA_V1.md).
3. Importar o grafo do hospital como dados/IDs e construir blockout de corredor, duas escadas, prumada, B1, garagem, telhado, túnel e anexo. `hospital_graph.json` é topologia e segundos provisórios; não gera paredes automaticamente. Verificar campo real de `CAM_F4_WEST` para N34 antes de gravar cena.
4. Associar portas/circuitos/câmeras/esconderijos a atores com IDs estáveis. Ajustar colisão, navmesh, linha de visão e propagação de som. Integrar `SenseHunter`/`ApplyHunter` conforme o [contrato tático](CONTRATO_HUNTER_TATICO_V1.md): prova de visada/navegação, chegada real, aviso apresentado e tempo de reação; cada estímulo ao Hunter guarda origem observada; falha de rota em RunRules só após mudança física.
5. Integrar inventário e companheiros pelo ciclo nativo de escolha/chegada/conclusão com serial e reservas; fornecer trajetória física, interação e contexto percebido conforme o [contrato](CONTRATO_EXECUCAO_COMPANHEIROS_V1.md). Testar munição transferida, ferida, abandono, informação de túnel e epílogo sem desfecho inventado. Conectar [DirectorCore](../../Tools/DirectorCore/README.md) a legendas/gestos: usar `CurrentPresentation` e confirmar serial/índice/ID somente após entrega real; observar presença/perigo e retomar a primeira entrada não confirmada.
6. Implementar HUD/menus, diário, legendas e quatro dificuldades. O cofre físico envia a entrada real por `SubmitKeypad`; o núcleo valida, registra tentativa e confirma abertura. Visitar um nó no MapCore só representa alcançabilidade. Aplicar o [contrato de checkpoint/esconderijo](CONTRATO_CHECKPOINT_ESCONDERIJO_V1.md). Só então importar assets e áudio **após revisão geral**, com registro de origem/licença e placeholders até lá. Usar a matriz de testes antes de produzir iluminação e materiais finais.
7. Save/load por `checkpoint::SaveFile/LoadFile`, pausando simulação e reconciliando núcleos e atores; restaurar objetos/colisão/nav antes de reativar IA. Integrar a política de quando salvar e informar recuperação/erro conforme o [contrato de arquivo](CONTRATO_ARQUIVO_CHECKPOINT_V1.md). Testar deadline do helicóptero, Hunter em perseguição e companheiro separado. Criar build PC e medir desempenho/tempo real.

## Contratos de ponte que devem ficar explícitos

| Adaptador | Entradas autorizadas | Saída/efeito e log obrigatório |
| --- | --- | --- |
| Sensor → Hunter | Contato visual com oclusão, som com fonte/atenuação, evidência inspecionada | `Stimulus` com tempo, tipo, origem, estimativa/raio; rejeição registrada. Nunca posição real por puro roteiro. |
| World → coordenador | Interação concluída, nó/prova física, ID estável e tempo atual | `CampaignCore::Apply` confirma recurso/mundo/regra juntos. A ponte representa o snapshot confirmado; retentativa usa o mesmo ID e não repete efeito. |
| Mundo → companheiro | Joe/rota/recurso/Hunter realmente percebidos, alcance físico | `Situation`, transferência e influência com ID único; destino via navmesh, sem teleporte. |
| Diretor narrativo | Nó, atores ativos/presentes, perigo, prova testemunhada e relógio da campanha | `ApplyNarrative` + `CurrentPresentation`; confirmar owner/serial/índice/cueId entregue. Elegibilidade não marca conclusão. Observações temporárias não entram no diário. |
| Combate/respiração | `ApplyCombat`, provas de animação/colisão e pose/velocidade rastreadas; `BreathingSound` | Consumir retornos uma vez, resolver cada fusível antes de seguir o delta, aplicar janela de interrupção ao ator Hunter. [Contrato P07](CONTRATO_COMBATE_RESPIRACAO_V1.md). |
| Save | Runtime v9 (StorySave + pausa/ocupação/recibos/planos + corpos/inventários/tarefas + memória/tática/RNG Hunter + combate/respiração por pessoa, finais/participações, interações/resgate, pilhas/relatos/usos conjuntos/planos) + reconciliação dos atores/UI | `SaveFile/LoadFile` valida e protege arquivo, com recuperação explícita. Restauração física de atores/animações continua no editor. |

## Extensões v9 e restauração

Conectar `ApplyStrategy`, `KnownDrops/KnownReports`, `HunterPlanOffers` e `CombatCommand.actor` pelo [contrato v9](CONTRATO_RECURSOS_INFORMACAO_PLANOS_V1.md). Restaurar pilhas físicas e armas/munição de cada executor antes de reativar sensores. A UI lê conhecimento pessoal datado, não o runtime global. Interromper/cancelar perguntas, usos, resgates e planos se contato, acesso ou animação contínua não puderem ser restabelecidos. Limpar tarefas interrompidas antes da apresentação terminal. `ArrivePlan` exige viagem real; o relógio não move atores.

## Conteúdo e desempenho — hipótese inicial

- Meta de medição: **1080p/60 fps** em preset ajustável no notebook alvo; não prometer resultado até profiler/build. Separar visibilidade de inimigo/pistas de qualidade de pós-processamento.
- Hospital lógico de 88 áreas não precisa de 88 níveis; particionar por andares/zonas após medir memória, iluminação, áudio e nav. Testar streaming sem perder estados persistidos.
- Registrar custo de sombras dinâmicas, vidros, reflexos, chuva, vapor, transparência de cortinas, câmeras ativas e pathfinding. Perfil de pior caso: perseguição + blackout + dois companheiros + feed ativo.
- Preparar Git LFS para `.uasset`, `.umap` e fontes binárias grandes quando entrarem; ignorar `Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`, `.vs`. Conferir política LFS e limites da conta antes de importar grandes volumes. Código, docs e JSON seguem Git normal. Não incluir referências de terceiros sem origem/permissão (D-10).

## Condição de pronto do primeiro dia

Projeto abre/compila; IDs do grafo carregados; Joe atravessa um corredor e uma porta; Hunter recebe um som **com origem válida**; um circuito altera uma câmera; salvar/carregar preserva esse estado; logs mostram tempo e ID. Isso é um marco de integração, não jogo completo. Ajustar a sequência após erros concretos, sem declarar uma mecânica pronta só porque seu núcleo isolado passou em C++.
