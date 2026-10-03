# The Thug — arquivo de checkpoint v1

**Estado:** escritor/leitor C++17 implementado em `Tools/CampaignCore/CheckpointFile.*`, com backends POSIX e Windows. Este documento não contém soluções de pistas nem parâmetros de vitória. O formato interno da sessão é runtime v8; a versão deste contrato descreve a API de arquivo.

## API e uso

`checkpoint::SaveFile(core, slot)` captura e valida a campanha, grava temporário exclusivo na mesma pasta, sincroniza e fecha os dados, relê/valida os bytes, preserva o principal válido como `.bak`, substitui o principal e conclui a sincronização do backend. `LoadFile(core, slot)` valida antes de trocar o estado da campanha viva. Usar um slot dentro de uma pasta de saves já criada pela aplicação; a API não cria pastas ausentes.

Somente uma operação ocupa o slot. Um arquivo `.lock` persistente coordena processos por bloqueio nativo; não apagá-lo enquanto a aplicação puder usar o slot. Concorrente recebe `Busy` sem aguardar. Temporários usam o namespace reservado `<slot>.thug-tmp.<pid>.<serial>` e sua variante de backup; a próxima gravação limpa órfãos desse namespace depois de adquirir o bloqueio. Não guardar conteúdo externo nesses nomes.

Captura e carregamento pertencem à thread da simulação. Pausar eventos durante o corte; depois de carregar, reconciliar atores, colisão, percepção e apresentação antes de retomar. O envelope contém estado lógico, não o progresso de animação ou a posição física contínua de todos os atores. A política de quando salvar permanece no [contrato de checkpoint/esconderijo](CONTRATO_CHECKPOINT_ESCONDERIJO_V1.md).

## Resultados que a interface deve distinguir

| Resultado | Tratamento |
| --- | --- |
| `Ok` | Gravação concluída pelo backend, ou principal carregado. A UI pode informar “salvo” depois desse retorno. |
| `RecoveredBackup` | A cópia anterior foi carregada. Informar recuperação de progresso anterior; não tratá-la como o último checkpoint. Não reescreve o principal automaticamente. |
| `NotFound` | Nenhum arquivo principal/backup encontrado. |
| `Busy` | Slot ocupado; adiar uma nova tentativa, conservando o ID/contexto da solicitação. |
| `Interrupted` | Observador de teste cancelou antes da substituição. O principal anterior não foi trocado. |
| `CommittedUnconfirmed` | Substituição ocorreu, mas a etapa final não confirmou conclusão. Não anunciar sucesso; não afirmar que o principal ainda é o anterior. Revalidar o slot antes de nova decisão. |
| `InvalidPath`, `InvalidCheckpoint`, `IoError` | Preservar campanha viva quando o load falha, comunicar erro e consultar o detalhe `checkpoint`/`systemError`. Não apagar o progresso para esconder o erro. |

Principal ausente, truncado, corrompido ou acima do limite permite tentativa explícita do backup. Versão futura e conteúdo/dificuldade incompatíveis **não** causam rollback silencioso nem podem ser sobrescritos com uma campanha incompatível. Preservar arquivo e oferecer nova campanha/slot ou migração específica, quando existir. A cópia danificada nunca é promovida sobre um backup válido.

## Backends e alcance da garantia

POSIX usa escrita exclusiva, `fsync` dos dados, `rename` na mesma pasta e `fsync` do diretório. Windows usa caminhos largos, escrita exclusiva, `FlushFileBuffers` e substituição na mesma pasta por `MoveFileExW` com write-through; não usa cópia entre volumes. O ramo Windows não é um `fsync` de diretório POSIX. Usar disco local com semântica compatível; falhas de hardware, controlador que ignora flush ou sistemas de arquivos especiais não são comprovadas pelos testes.

Há ensaios de cancelamento e encerramento abrupto do **processo** em seis etapas, recuperação de corrupção/truncamento, contenção entre processos, incompatibilidade e caminhos Unicode. Isso não simula perda elétrica física. O checksum detecta corrupção acidental; não é assinatura nem proteção contra edição maliciosa.

O observador de etapas existe para injeção de falha/telemetria controlada. Ele roda com o slot bloqueado: não deve lançar exceção nem chamar esta API novamente. Cancelamento depois da etapa final não desfaz uma gravação concluída. Não usar o observador como widget de confirmação.

Referências primárias do backend Windows: [FlushFileBuffers](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers), [MoveFileExW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw) e [LockFileEx](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-lockfileex).
