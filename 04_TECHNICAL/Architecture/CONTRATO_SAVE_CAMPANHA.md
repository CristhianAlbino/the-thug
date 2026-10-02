# Contrato de save da campanha — integração futura na Unreal

Implementação C++17 em [SaveCore](../../Tools/SaveCore/README.md). O formato `THE_THUG_CAMPAIGN 1` guarda em um mesmo checkpoint os dez estados puros que existem hoje. O formato legado `THE_THUG_PARTY 1` continua válido apenas para a equipe; nunca carregá-lo como uma campanha completa.

O formato `THE_THUG_STORY_CAMPAIGN 1`, acessado por `SaveStoryCampaign`/`LoadStoryCampaign`, envolve os dez estados puros e `NarrativeState`. Ele valida o bloco narrativo e o mundo **antes** de modificar a memória; formatos antigos não ganham flags narrativas inventadas. Para a sessão coordenada, usar **`THE_THUG_RUNTIME_CAMPAIGN 4`** de [CampaignCore](../../Tools/CampaignCore/README.md), acrescentando pausa, ocupação de Joe, recibos, planos/seriais de apresentação, inventários/saúde/conhecimento/tarefas individuais. Para arquivo, usar `checkpoint::SaveFile/LoadFile` e o [contrato de escrita/recuperação](CONTRATO_ARQUIVO_CHECKPOINT_V1.md). A aplicação em atores ainda depende da Unreal.

## Captura

1. Pausar a simulação no game thread e encerrar interações em andamento de forma definida. Nenhum sistema altera o estado durante `SaveCampaign`.
2. Atualizar `WorldState::AdvanceTo` para o tempo de jogo corrente. Os relógios de Hunter, objetivos, companheiros e evidências não podem estar à frente dele.
3. Capturar cada ator persistente com ID estável por fase e arquétipo authored. Para objetos do mundo, converter posição Unreal de centímetros para **metros** e guardar orientação em **graus**. `stateId` e `present` representam, por exemplo, porta aberta e recurso coletado. O editor deve fornecer o catálogo completo; IDs de testes não são nomes finais.
4. Chamar `SaveFile` para temporário validado, sincronização, backup e substituição. `Ok` confirma conclusão do backend; `CommittedUnconfirmed` informa que a troca já ocorreu sem confirmação final e exige revalidação. Falha anterior à troca conserva o principal. O checksum detecta corrupção acidental, não adulteração deliberada.

## Restauração

1. Criar a fase e os objetos authored correspondentes ao catálogo do save, mantendo simulação pausada. Instanciar módulos com mesma dificuldade, tuning e catálogo de itens.
2. Na sessão coordenada, chamar `LoadFile`/`CampaignCore::Load`; `LoadCampaign` permanece para ensaio inferior dos núcleos. Arquivo rejeitado preserva a sessão viva. Recuperação de backup é explícita e informa progresso anterior; versão futura ou conteúdo incompatível não causam rollback silencioso. Não combinar os loaders para trocar só alguns sistemas.
3. Aplicar aos atores da Unreal o `WorldState` carregado: estado/presença, transformações, colisão, navmesh e apresentação coerentes. Resolver referências de atores **por ID**, sem depender da ordem de instanciação. Reconciliar estados authored (por exemplo, portão liberado nas regras e animação/colisão do portão no mundo).
4. Reativar percepção, navegação, áudio e relógio só depois da restauração física. O Hunter recebe a última estimativa percebida, sem acesso indevido à localização atual de Joe.

## Limites e migração

- Uma alteração de catálogo de itens, fase, ator persistente ou tuning exige regra de migração explícita e nova versão. O loader atual rejeita incompatibilidade; não altera silenciosamente uma partida antiga.
- O registro genérico do mundo persiste ID, arquétipo, estado, presença e transformação. Ele ainda não conhece velocidades, projéteis, animações, cenas de diálogo e componentes específicos da Unreal. Esses sistemas terão de ser mapeados com estado e testes próprios antes de chamar um save completo de jogável.
- Testar no editor: save durante perseguição, sangramento, prazo do helicóptero, recurso coletado, porta aberta, companheiros separados, pausa, fechamento forçado e retomada. Os testes C++ confirmam apenas a lógica independente da engine.
