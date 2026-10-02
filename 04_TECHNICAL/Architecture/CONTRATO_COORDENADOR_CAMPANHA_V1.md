# The Thug — contrato do coordenador de campanha v1

**Estado:** núcleo C++17 implementado e testável fora da Unreal. Este contrato é seguro para o criador ler: não inclui soluções de pistas ou parâmetros secretos de vitória/checkpoint. Os vínculos detalhados ficam nos dados técnicos para quem integrar.

## Dono do estado e sequência

Uma instância de `campaign::CampaignCore` pertence à sessão. O adaptador Unreal não mantém cópias concorrentes mutáveis de inventário, regras ou companheiros. Atores representam o estado confirmado, com movimento/colisão/percepção reais fornecidos pela engine.

1. Reunir os eventos físicos e processá-los em ordem de tempo numa única thread de simulação.
2. Avançar com `Tick` somente o tempo ativo até o evento. O relógio não usa tempo de menus, loading ou pausa do SO.
3. Usar o instante efetivamente avançado; se a partida terminou durante o delta, descartar efeitos posteriores àquele instante.
4. Verificar alcance, presença, contexto e a interação física exigida. Criar `Action` com ID estável da ocorrência, nó real e `atSeconds = Now()`.
5. Chamar `Apply`. Em `Applied`, representar o novo estado confirmado e emitir eventual sinal uma vez. Em rejeição, conservar representação e recursos anteriores. Em `AlreadyApplied`, reconciliar a representação sem emitir o efeito novamente.

Falhas de animação depois da confirmação não podem reaplicar custo. A apresentação deve conseguir reconciliar-se com o snapshot. Uma ação longa só confirma no marco físico definido; tarefas dos companheiros seguem o [contrato de escolha/chegada/conclusão e reserva](CONTRATO_EXECUCAO_COMPANHEIROS_V1.md). Operações diretas de Joe continuam confirmadas no marco físico pelo adaptador. Não interpretar `physicallyVerified` como permissivo automático vindo de widget.

## Responsabilidades verificáveis

| Frente | Coordenador | Adaptador Unreal |
| --- | --- | --- |
| Coleta | Item/quantidade autorados, capacidade, pré-requisitos, presença lógica e retirada conjunta. | Pickup real alcançável, ID correspondente, interação concluída e atualização do ator. |
| Tratamento de Joe | Consume recurso e trata/estabiliza juntos; falha preserva ambos. | Contexto permitido, animação e interrupção antes do marco de confirmação. |
| Ferramenta/porta/energia | Custo reutilizável ou consumido, vínculo de nó, estado anterior e novo, resultado de regra; `SubmitKeypad` valida entrada e registra tentativa real. | Alcance, entrada real no teclado, colisão e mecanismo fisicamente concluído. |
| Auxílio | Recursos individuais, medicina e influência conjuntas; impede nova transferência com o mesmo ID. | Companheiro presente, consciente e alcançável; comunicação/entrega física. Tarefas reservam recursos até a conclusão. |
| Ocupação | Identidade persistente de Joe/companheiros e dispositivo ocupado; impede ações em outro nó e locomoção simultânea. | Espaço físico disponível, entrada/saída e posição real. |
| Fim da partida | Requisitos lógicos e tempo válido; encerra consequências futuras. | Prova física específica do evento. Elegibilidade sozinha não confirma resultado. |
| Percepção | Audição incerta, memória observada e decisão do Hunter. | Visada, distância, oclusão, postura, origem do som e navegação, sem conhecimento concedido por roteiro. |
| Narrativa | Elegibilidade, sequência fixa, serial/índice de entrada, prioridade, diário inspecionado e epílogo. | Contexto realmente observado e conclusão da fala/gesto antes de confirmar; cancelar callback antigo. |

Nós lógicos não rastreiam a posição contínua de Joe. A engine é responsável por movimento entre nós e alcance dentro da sala. Enquanto Joe está registrado num dispositivo de ocultação, confirmar saída antes de qualquer evento localizado fora daquele nó. Uma morte no local pode conservar o registro espacial; ele não concede vitória ou movimento.

## Inicialização e compatibilidade

`CampaignData.generated.h` contém o perfil dos quatro modos e os objetos/vínculos exportados. A versão lógica usa transforms zero; substituir por dados reais ao montar o nível. Objetos e pontos de coleta possuem IDs estáveis. Coordenador valida nós, tipos, recursos e dispositivos antes de aceitar o catálogo.

O hash das fontes de autoria e a configuração completa de mundo/tuning identificam a campanha. Mudanças nos dados podem invalidar saves experimentais. Não relaxar a validação só para abrir um arquivo incompatível; escolher migração testada ou nova campanha. Não existe quinto modo nesta etapa.

## Persistência e falhas

Pausar a aplicação de eventos antes de capturar o checkpoint. O envelope runtime acrescenta pausa, ocupação e recibos ao StorySave. Relógios contínuos devem coincidir, ações históricas devem usar vínculos válidos e o resultado narrativo precisa corresponder à fuga confirmada. O carregamento prepara todos os estados antes de trocar a sessão em memória.

Depois de validar, restaurar atores/colisão/câmeras, reconciliar apresentação e somente então retomar sensores. Um checkpoint pausado restaura pausado; o menu decide quando retomar. Runtime v3 inclui planos/seriais de apresentação, recursos/corpos/tarefas individuais, e `checkpoint::SaveFile/LoadFile` implementa arquivo validado, backup e recuperação explícita. O stream sozinho não garante durabilidade. Seguir o [contrato de arquivo](CONTRATO_ARQUIVO_CHECKPOINT_V1.md), incluindo tratamento de `RecoveredBackup` e `CommittedUnconfirmed`.

## Critério de integração

Reproduzir no editor um circuito, porta, coleta, ferimento e auxílio; salvar, carregar e retentar os IDs anteriores; nenhuma quantidade, influência ou emissão sonora pode duplicar. Conferir erro de espaço/requisito sem mudança visual ou recurso perdido. Verificar pausa real e um fim durante atualização longa. Ações exigem deslocamento físico; os cenários C++ não demonstram geometria, atuação ou três horas de campanha.

O [diretor nativo](../../Tools/DirectorCore/README.md) já coordena texto e confirmação. A primeira execução dos seis objetivos e inventários individuais está ligada à sessão. Cooperação/planos mais ricos, combate/consumíveis adicionais, Hunter avançado e finais raros permanecem etapas posteriores. Apresentação e restauração de atores ainda exigem Unreal. [API e testes](../../Tools/CampaignCore/README.md) · [Revisão e pendências](../../01_PROJECT/Reviews/REVISAO_GERAL_2026_10_01.md).
