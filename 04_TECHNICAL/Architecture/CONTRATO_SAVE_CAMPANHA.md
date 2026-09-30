# Contrato de save da campanha — integração futura na Unreal

Implementação C++17 em [SaveCore](../../Tools/SaveCore/README.md). O formato `THE_THUG_CAMPAIGN 1` guarda em um mesmo checkpoint os dez estados puros que existem hoje. O formato legado `THE_THUG_PARTY 1` continua válido apenas para a equipe; nunca carregá-lo como uma campanha completa.

## Captura

1. Pausar a simulação no game thread e encerrar interações em andamento de forma definida. Nenhum sistema altera o estado durante `SaveCampaign`.
2. Atualizar `WorldState::AdvanceTo` para o tempo de jogo corrente. Os relógios de Hunter, objetivos, companheiros e evidências não podem estar à frente dele.
3. Capturar cada ator persistente com ID estável por fase e arquétipo authored. Para objetos do mundo, converter posição Unreal de centímetros para **metros** e guardar orientação em **graus**. `stateId` e `present` representam, por exemplo, porta aberta e recurso coletado. O editor deve fornecer o catálogo completo; IDs de testes não são nomes finais.
4. Escrever o checkpoint num arquivo temporário, fechar/confirmar sua gravação e só então substituir o save anterior. Se a escrita falhar, preservar o arquivo anterior. O checksum detecta corrupção acidental, não adulteração deliberada.

## Restauração

1. Criar a fase e os objetos authored correspondentes ao catálogo do save, mantendo simulação pausada. Instanciar módulos com mesma dificuldade, tuning e catálogo de itens.
2. Chamar `LoadCampaign`. Ele rejeita versão futura, dados truncados, checksum inválido, dificuldade/catálogo incompatível e qualquer snapshot que não passe na validação. Em falha, os dez objetos C++ permanecem intactos; oferecer outro save ou uma nova campanha.
3. Aplicar aos atores da Unreal o `WorldState` carregado: estado/presença, transformações, colisão, navmesh e apresentação coerentes. Resolver referências de atores **por ID**, sem depender da ordem de instanciação. Reconciliar estados authored (por exemplo, portão liberado nas regras e animação/colisão do portão no mundo).
4. Reativar percepção, navegação, áudio e relógio só depois da restauração física. O Hunter recebe a última estimativa percebida, sem acesso indevido à localização atual de Joe.

## Limites e migração

- Uma alteração de catálogo de itens, fase, ator persistente ou tuning exige regra de migração explícita e nova versão. O loader atual rejeita incompatibilidade; não altera silenciosamente uma partida antiga.
- O registro genérico do mundo persiste ID, arquétipo, estado, presença e transformação. Ele ainda não conhece velocidades, projéteis, animações, cenas de diálogo e componentes específicos da Unreal. Esses sistemas terão de ser mapeados com estado e testes próprios antes de chamar um save completo de jogável.
- Testar no editor: save durante perseguição, sangramento, prazo do helicóptero, recurso coletado, porta aberta, companheiros separados, pausa, fechamento forçado e retomada. Os testes C++ confirmam apenas a lógica independente da engine.
