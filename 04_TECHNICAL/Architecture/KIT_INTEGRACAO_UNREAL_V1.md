# The Thug — kit do primeiro dia na Unreal

**Estado:** plano para execução no novo computador; não há `.uproject`, `.umap` ou `.uasset` no repositório ainda. O código C++17 puro não é um Actor Unreal arrastável. A versão específica da engine/toolchain será escolhida ao instalar (D-01). Clonar o repositório preserva histórico e atualização; ZIP é apenas cópia estática.

## Ordem de criação e integração

1. Instalar Unreal Engine 5 e toolchain C++ compatível no notebook; clonar `CristhianAlbino/the-thug` com GitHub Desktop ou Git. Criar projeto Games/C++ `TheThug` em pasta versionada do clone. Confirmar que `TheThug.uproject` abre e um projeto vazio compila **antes** de copiar lógica.
2. Adicionar módulos/compilação do projeto e adaptar os núcleos puros em camadas, começando `RunRules`, `SurvivalCore`, `CompanionCore`, `HunterCore`, `PerceptionCore`, `WorldCore` e `SaveCore`. Converter o contrato de autoria do `NarrativeCore` (Python/JSON) em lógica de runtime Unreal com paridade testada. Confirmar conversão de unidades **cm Unreal ↔ m núcleos**, relógio monotônico e enums de dificuldade/eventos em cada ponte; detalhes no [contrato dos adaptadores](CONTRATO_ADAPTADORES_UNREAL_V1.md).
3. Importar o grafo do hospital como dados/IDs e construir blockout de corredor, duas escadas, prumada, B1, garagem, telhado, túnel e anexo. `hospital_graph.json` é topologia e segundos provisórios; não gera paredes automaticamente. Verificar campo real de `CAM_F4_WEST` para N34 antes de gravar cena.
4. Associar portas/circuitos/câmeras/esconderijos a atores com IDs estáveis. Ajustar colisão, navmesh, linha de visão e propagação de som. Cada estímulo ao Hunter guarda origem observada; falha de rota em RunRules só após mudança física.
5. Integrar inventário e companheiros com trajetória física, interação e decisões. Testar munição transferida, ferida, abandono, informação de túnel e epílogo sem desfecho inventado. Converter [conversas ambientais](../../02_GAME_DESIGN/Narrative/DIALOGOS_AMBIENTAIS_V1.md) em dados/IDs runtime com presença, perigo e fatos testemunhados; salvar linhas interrompidas sem repetir falas.
6. Implementar HUD/menus, diário, legendas e quatro dificuldades. O cofre físico precisa validar uma entrada de teclado antes de `SafeOpened`; visitar um nó no MapCore só representa alcançabilidade. Aplicar o [contrato de checkpoint/esconderijo](CONTRATO_CHECKPOINT_ESCONDERIJO_V1.md). Só então importar assets e áudio **após revisão geral**, com registro de origem/licença e placeholders até lá. Usar a matriz de testes antes de produzir iluminação e materiais finais.
7. Save/load pausando simulação, capturando todos os núcleos e atores; restaurar objetos/colisão/nav antes de reativar IA. Testar deadline do helicóptero, Hunter em perseguição e companheiro separado. Criar build PC e medir desempenho/tempo real.

## Contratos de ponte que devem ficar explícitos

| Adaptador | Entradas autorizadas | Saída/efeito e log obrigatório |
| --- | --- | --- |
| Sensor → Hunter | Contato visual com oclusão, som com fonte/atenuação, evidência inspecionada | `Stimulus` com tempo, tipo, origem, estimativa/raio; rejeição registrada. Nunca posição real por puro roteiro. |
| World → regras | Interação concluída, porta/circuito/câmera e inventário físico | `RunRules::Apply` só após validação; log antes/depois para desfazer falha de adaptação. |
| Mundo → companheiro | Joe/rota/recurso/Hunter realmente percebidos, alcance físico | `Situation`, transferência e influência com ID único; destino via navmesh, sem teleporte. |
| Diretor narrativo | Nó, `scene_seen`, atores ativos no nó, prova testemunhada, eventos RunRules | `eligible`/`eligible_late` e ID de fala; não altera estado por estar elegível. Flags de encontro corrente expiradas após cena. |
| Save | `StorySave` (dez estados puros + narrativa) + atores Unreal/UI relevante | Arquivo temporário, versão/checksum, validação antes de aplicar; falha preserva save anterior. A transação em disco e restauração física continuam no editor. |

## Conteúdo e desempenho — hipótese inicial

- Meta de medição: **1080p/60 fps** em preset ajustável no notebook alvo; não prometer resultado até profiler/build. Separar visibilidade de inimigo/pistas de qualidade de pós-processamento.
- Hospital lógico de 88 áreas não precisa de 88 níveis; particionar por andares/zonas após medir memória, iluminação, áudio e nav. Testar streaming sem perder estados persistidos.
- Registrar custo de sombras dinâmicas, vidros, reflexos, chuva, vapor, transparência de cortinas, câmeras ativas e pathfinding. Perfil de pior caso: perseguição + blackout + dois companheiros + feed ativo.
- Preparar Git LFS para `.uasset`, `.umap` e fontes binárias grandes quando entrarem; ignorar `Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`, `.vs`. Conferir política LFS e limites da conta antes de importar grandes volumes. Código, docs e JSON seguem Git normal. Não incluir referências de terceiros sem origem/permissão (D-10).

## Condição de pronto do primeiro dia

Projeto abre/compila; IDs do grafo carregados; Joe atravessa um corredor e uma porta; Hunter recebe um som **com origem válida**; um circuito altera uma câmera; salvar/carregar preserva esse estado; logs mostram tempo e ID. Isso é um marco de integração, não jogo completo. Ajustar a sequência após erros concretos, sem declarar uma mecânica pronta só porque seu núcleo isolado passou em C++.
