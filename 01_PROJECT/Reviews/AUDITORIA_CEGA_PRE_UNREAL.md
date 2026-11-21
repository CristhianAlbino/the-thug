# Auditoria sem spoilers — preparação pré-Unreal

**Leitura do criador:** este resumo não contém a combinação, a posição das pistas nem os critérios secretos de esconderijo. Os dados autorais permanecem nos contratos técnicos do repositório para integração futura.

## Executado

| Frente | Verificação automática | Resultado | O que ela não comprova |
| --- | --- | --- | --- |
| Descoberta da garagem | Todos os textos necessários existem, têm fonte rastreável e podem ser alcançados sem depender da própria solução ou de energia restaurada; o diário mostra só material inspecionado, erros não travam a rota e a entrada exige interação física. | Passou `blind_audit.py` e o validador de narrativa. | Uma pessoa sem informação prévia entender a dica ou localizar cada documento em cenário real. |
| Outras fugas | Rotas alternativas ainda são percorríveis no grafo depois de falhas de rotas diferentes; fim exige confirmação das regras, não mera visita ao nó. | Passou `blind_audit.py`, `director_tests.py` e ScenarioLab. | Sinalização visual, pressão de tempo, direção de arte e IA em navmesh. |
| Saves | Envelope C++17 guarda narrativa, diário, conversas interrompidas, provas individuais e dez núcleos no mesmo arquivo. Erro de versão, truncamento ou corrupção deixa a memória original intacta; arquivo antigo sem história não é aceito como história completa. | Passaram StorySave, runtime v9 e CheckpointFile (arquivo sincronizado, backup/recuperação, bloqueio e interrupção de processo). | Retomada de atores/animações na Unreal e corte elétrico físico do hardware. |
| Esconderijo | Hipótese mantém observação e saída física do perseguidor como condições a comprovar no editor; temporizador e estado lógico isolados não são evidência espacial. | Contrato e cenários puros revisados; parâmetros continuam provisórios. | Visadas, navegação, falsos positivos e sensação de espera; dependem da geometria real. |

## Ensaio humano às cegas que ainda falta

Entregar a uma pessoa que desconheça a solução **apenas** a abertura, o mapa disponibilizado ao personagem e os textos apresentados na ordem em que ela os encontrar. Observar sem dicas: que objetivo entendeu, quando suspeitou da ligação entre documentos, se conseguiu recuperar informação no diário, onde perdeu a rota e quanto tempo ficou parada. Não incluir neste kit a tabela técnica, código, resposta nem nomes internos dos testes. Registrar só tempo, caminhos tentados, falas espontâneas e intervenção necessária. Ajustar texto/visibilidade após esse ensaio e repetir no blockout da Unreal com o mesmo protocolo.

**Status:** coerência lógica verificada; compreensão humana às cegas e balanceamento final continuam pendentes. O autor não precisa ler pistas nem regras internas para acompanhar este status.

O [protocolo humano v1](PROTOCOLO_TESTE_HUMANO_PRE_UNREAL_V1.md) fornece roteiro do facilitador e formulário. Preparação não equivale a sessão realizada; os campos do participante permanecem em aberto.
