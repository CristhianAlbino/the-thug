# The Thug — revisão geral de 01/10/2026

**Leitura segura para Cristhian:** este relatório não revela a combinação da garagem, a sequência de pistas ou os gatilhos internos de saves e esconderijo. Ele comenta somente decisões narrativas que o criador já conhece.

**Parecer:** a identidade do jogo está consistente e existe uma fundação de código verificável. A preparação completa ainda não está fechada: a revisão encontrou falhas corrigidas e identificou ampliações nativas agora entregues no runtime v9. Revisão criativa, teste humano independente, produção final e integração continuam abertos. O repositório não contém ainda um projeto Unreal executável. Importar pastas não montará automaticamente o hospital nem conectará os personagens.

## O que foi examinado

GDD, campanha de cinco atos, roteiros N01–N34, bíblias dos personagens, 42 conversas ambientais, documentos e transcrições, variantes de dificuldade, epílogos, mapa lógico, bíblia visual, balanceamento, interface, contratos de integração/persistência, matriz de testes, núcleos C++/Python e workflows de CI.

A revisão combina leitura cruzada e execução de testes de lógica. Não houve sessão jogável na Unreal, teste com jogador externo, gravação de vozes nem medição de desempenho/duração. Os 88 nós do mapa são áreas lógicas; as 34 cenas são unidades de autoria condicionais. Essas contagens não equivalem a 88 salas prontas ou 34 missões implementadas.

## Sete grupos de correções realizados

| Grupo | Problema encontrado | Correção e evidência |
| --- | --- | --- |
| 1. Segredos e persistência | Uma cena ainda ativa e certas entradas de diário podiam atravessar restrições de dificuldade; pistas podiam ser restauradas antes de seus marcos. | Validação de conteúdo ativo/concluído, documentos, gravações, fatos e progressão no diretor e no save. Condições temporárias de encontro não podem virar fatos permanentes. Regressões em `story_tests.cpp` e `director_tests.py`. |
| 2. Conversas durante perigo | Uma conversa opcional bloqueava cena principal; conclusão não revalidava presença/perigo; intervalo entre conversas não era aplicado pela fachada. | Cena principal suspende conversa, retomada revalida o contexto e cancelamento não marca fala entregue. Intervalo persistido entre conversas. Testes de interrupção, ausência, retomada e tempo. |
| 3. Preparação e final de rota | Falas que deveriam acompanhar o preparo só eram liberadas depois da fuga; o número do ato bloqueava cenas espaciais válidas. | Preparação e coda têm etapas distintas. Coda exige confirmação da rota concluída; o checkpoint rejeita um final correspondente a outra rota. Marcos próprios das pistas de Jack continuam obrigatórios. |
| 4. Audição do Hunter | Alguns sons previstos no design nunca alcançavam o limiar de reação; normalizar a respiração incorretamente apagaria o efeito de ferimentos. | Limiares provisórios recalibrados e conversão explícita do efeito de sobrevivência em evento de respiração. Ensaio integrado de porta, passos, ferimento e audição; oclusão e distância continuam importando. |
| 5. Portas e objetos | Blackout podia transformar uma porta aberta em passagem bloqueada sem fechamento físico. Objetos ausentes ainda podiam responder a interações. | Energia impede a abertura motorizada de uma porta fechada; uma passagem aberta precisa de fechamento real para bloquear. Ausência impede interação, câmera, energia e ocupação. Testes no WorldCore. |
| 6. Estado físico e campanha | Snapshots admitiam sangue em nível fatal com Joe vivo e exaustão incoerente; uma morte por sangramento podia ser salva antes de chegar às regras de fim da partida. | Restauração rejeita estados físicos impossíveis. Checkpoint exige coerência entre morte por sangramento e resultado da campanha. Regressões de rejeição sem mutação e retomada conjunta. |
| 7. Falas e epílogos | Textos assumiam conhecimento, retorno, ajuda ou sobrevivência não comprovados. Alguns descreviam instruções de produção como se fossem narração final. | Falas passam a depender da informação realmente obtida. Epílogos distinguem texto exibido de condição de autoria; lembranças específicas dependem de ações registradas. GDD e documentos de apoio foram alinhados. |

Mudanças de audição são **hipóteses melhores para testar**, não balanceamento aprovado. O estado físico dos atores, a apresentação das falas e a propagação sonora real ainda precisam corresponder a essas regras no editor. Saves experimentais com tuning anterior podem ser rejeitados por incompatibilidade; não há migração automática prometida.

## Parecer sobre a experiência e os personagens

**Joe e Harley.** A assimetria funciona porque cria desconforto sem obrigar o jogador a gostar moralmente de Joe. O risco é o roteiro desmentir quem escolheu ajudar. A revisão preserva frases frias nos modos comuns, mas condiciona cobranças e memórias ao que aconteceu. No Punitivo, reciprocidade muda o tom; não concede resgate ou perdão automático. Harley precisa também ser útil e agir por iniciativa própria, para seu valor não depender só do romance.

**Bartholomeu.** Medo acompanhado de uma contribuição verificável torna o vínculo mais forte. Manter momentos em que ele sustenta uma passagem, observa algo útil ou devolve cuidado. Evitar repetir infantilização nas falas: o tratamento afetuoso não deve apagar que ele é um adulto capaz. Traição não é seu papel obrigatório, e o epílogo não a deduz de uma fuga separada.

**Thomas.** A abertura jogável dá espaço para um gesto concreto de cuidado e para a promessa de café antes da ruptura. A perda ganha peso sem precisar alongar uma introdução parada. A regra por dificuldade permanece intacta. Ainda falta fechar a encenação física da descoberta: identificação, autoria, campo da câmera e eventual deslocamento do corpo precisam contar a mesma história. Um registro automático evita sugerir uma equipe hospitalar invisível operando a recepção.

**Jack.** Sua utilidade precisa ser comprovada antes que a estranheza domine a leitura do jogador. A risada ao recordar Batman existe em todos os modos; isso é diferente da progressão macabra exclusiva do Punitivo. Minha proposta para avaliar na direção final é dar espaço às pausas e reações dos outros: uma graça cruel bem colocada pesa mais que riso constante. O gesto visual que confirma a revelação ainda é decisão de encenação; não tratá-lo como asset pronto.

**Batman e o hospital.** A base mantém percepção limitada e possibilidade de erro. O horror prometido depende de silhueta, som, chegada física e consequências legíveis. O mapa pode parecer amplo com conexões entre andares, tubulações e ambientes reconhecíveis; aumentar caminhada vazia não resolve duração. As referências estão traduzidas em direção visual documentada, ainda sem comprovação em iluminação, câmera ou performance reais.

**Três horas.** É um alvo para a primeira experiência no Difícil. As quatro rotas e cenas condicionais permitem partidas com durações diferentes. Não existe evidência de três horas de conteúdo jogável neste momento. A medição precisa distinguir exploração interessante de tempo perdido por falta de orientação. Uma campanha curta para quem domina o espaço não deve ser esticada por travas artificiais de ato.

**Interface.** Fluxos, HUD e acessibilidade têm especificação e wireframe. Ainda precisam mostrar claramente informação verificada, hipótese e registro antigo, além de responder a teclado/controle e opções de leitura. O teste de regras não prova tamanho de texto, contraste, navegação de menus ou conforto durante uma perseguição.

## O que o código realmente entrega hoje — atualização de 03/10

CampaignCore v9 entrega os itens de implementação nativa identificados nos ciclos P01–P08 desta revisão. O [contrato complementar](../../04_TECHNICAL/Architecture/CONTRATO_RECURSOS_INFORMACAO_PLANOS_V1.md) define recursos, informação, cooperação, armas individuais e planos do Hunter. “Entregue no núcleo” significa transação e persistência testáveis com provas físicas simuladas; ainda não significa ação jogável ou implementação de toda possibilidade descrita no GDD.

| Frente | Implementado/testável fora da engine | Limite atual |
| --- | --- | --- |
| Hunter | Percepção/memória limitada, busca, emboscada, manipulação observada, contenção/transferência, interrogação com recusa e hipótese falível, aprendizagem por fumaça observada, armadilha encadeada e retirada. | Escolha entre ofertas depende da ponte; deslocamento, aviso, visada, alteração física e atuação exigem editor. Não há captura por chegada nem HP de Batman. |
| Companheiros | Traços, memória/rotas/bolsas/saúde individuais, oito objetivos, sinais recusáveis, gestos, medicina, recuperação temporal, resgate, usos conjuntos e relatos que podem mentir. | Cooperação usa interações autoradas, sem operação genérica de teclado protegido. Autonomia espacial, voz e animação dependem da ponte. |
| Sobrevivência/combate | Ferimentos, sangue, stamina, capacidade, medicina, pilhas derrubadas/saque parcial, armas individuais, tiro, arremesso, fumaça, interrupção/imunidade e respiração. | Física, oclusão, colisão, atuação e calibração exigem cenário real. Estoque derrubado não cria automaticamente um sistema de saque de cadáveres. |
| Informação e mundo | Dispositivos, som abstrato, calços, ocorrências opcionais e registro pessoal. Joe conserva a última observação de item até conferir de novo; relatos não expõem intenção privada. | Interface precisa comunicar hipótese, relato antigo e observação. Geometria e acústica ainda não existem como nível jogável. |
| Campanha e narrativa | Relógio/pausa/fim, diretor pt-BR, confirmação de entradas, quatro fugas convencionais, dois resultados raros próprios, epílogos e participações pontuais. | Encenação, apresentação, convidados, destruição física e idiomas adicionais permanecem produção/integração. |
| Persistência | Runtime v9, restauração integral validada, conservação dos estoques entre famílias, derivados Q, reservas, recibos, arquivo sincronizado, backup/recuperação e bloqueio POSIX/Windows. | Política de disparo e restauração dos atores/animações dependem de integração e playtest. Encerrar processo não simula corte elétrico real. Saves v1–v8 exigem migração explícita ou nova campanha. |
| Preparação de conteúdo | GDD, roteiros, dados, mapa, direção visual/UX; proposta de direção v2, protocolo humano e catálogo de produção. | Documentação não é nível, modelo, voz, música ou aprovação criativa. |

## Pendências e critério de encerramento

| ID | Estado atual | Evidência / próximo requisito |
| --- | --- | --- |
| P01 | Núcleo entregue | Relógio compartilhado, pausa, fronteira fatal, ações atômicas e retomada testados nas quatro dificuldades. |
| P02 | Ampliações nativas identificadas entregues | Recursos individuais, reservas, gestos, calços, resgate, pilhas genéricas, saque parcial e uso conjunto; conservação/capacidade/falha sem custo testadas. |
| P03 | Núcleo entregue | Diretor C++, 314 cues autorados, confirmação de fala/gesto e 8.484 vetores de paridade. Voz/apresentação ainda pendentes. |
| P04 | Escritor entregue e snapshot ampliado | Arquivo/backup/recuperação, limite, corrupção, incompatibilidade e processos encerrados; runtime v9 valida estados e recursos. Política e atores continuam para integração. |
| P05 | Ampliações nativas identificadas entregues | Tática, contenção, interrogação, adaptação à fumaça, três etapas de armadilha e retirada; cada etapa exige chegada e confirmação física. |
| P06 | Ampliações nativas identificadas entregues | Oito objetivos, comunicação/recusa, pressão temporal, resgate e cooperação; relatos datados/falíveis e conhecimento pessoal persistidos. |
| P07 | Ampliações nativas identificadas entregues | Armas de Joe/NPCs, reservas, impactos, projéteis, fumaça, respiração e fronteiras de física; não fornecem animação/raycast reais. |
| P08 | Primeiro ciclo entregue | Finais raros e participações autoradas com recursos, provas, consequências e retomada; não entregam IA física dos convidados. |
| P09 | Proposta preparada; avaliação aberta | [Direção criativa v2](../../02_GAME_DESIGN/Narrative/DIRECAO_CRIATIVA_REVISAO_V2.md): encenação de Thomas, gesto de Jack e leitura por ID. Requer avaliação do criador/atuação. |
| P10 | Protocolo preparado; sessão aberta | [Teste humano sem solução prévia](PROTOCOLO_TESTE_HUMANO_PRE_UNREAL_V1.md). Asserções automáticas não substituem um participante independente. |
| Produção | Catálogo preparado; arquivos finais abertos | [Plano de produção](../Roadmap/PLANO_PRODUCAO_PRE_UNREAL_V1.md): 314 cues e 35 requisitos rastreáveis; zero arquivos finais produzidos neste catálogo. |

[Resgate no escuro](../../02_GAME_DESIGN/Narrative/IDEIA_RESGATE_NO_ESCURO_RASCUNHO.md) tem base de contenção/soltura, distração, pergunta e reavaliação perceptiva; ainda falta o encontro encenado. O pacote completo não está “100%”: P09/P10 e produção têm trabalho concreto fora da Unreal, e integração física/playtest continuam necessários. Extremo/Joker permanece expansão posterior conforme D-13, sem quinto modo ativo.

## Trabalho que precisa do editor e da produção

Criar o projeto C++ da Unreal e compilar as pontes; montar um trecho jogável; representar portas/objetos e personagens por atores; implementar movimento, percepção visual, navegação e animação; construir HUD/menus; montar o blockout completo; integrar cinematics, áudio e assets; medir duração/performance; testar as dificuldades e empacotar uma build de PC.

Áudio e assets finais permanecem depois desta revisão, conforme a ordem escolhida pelo criador. O Modo Extremo/Joker continua no backlog: não virou um quinto modo parcialmente ligado à campanha principal.

## Verificação e fechamento

Os testes usados nesta revisão estão nos scripts `run-tests.sh` dos núcleos, no ScenarioLab e nos validadores de NarrativeCore/MapCore. Compilação direta usa C++17 com avisos tratados como erro. CMake é verificado no workflow de CI, pois não está instalado no ambiente local desta revisão. A PR mantém o histórico das execuções.

**Resultado local observado:** HunterCore 149, RunRules 37, CompanionCore 533, SurvivalCore 53, PerceptionCore 30, IntegrationCore 23, SaveCore 85, CampaignSave 108, StorySave 62 e WorldCore 24 verificações aprovadas. São contagens de asserções dos executáveis, não campanhas completas jogadas. Também passaram NarrativeCore, DirectorRuntime, auditoria estrutural sem spoilers, MapCore, oito percursos do ScenarioLab e repetição determinística do cenário integrado. `git diff --check` não apontou erro de whitespace.

**Etapa de programação posterior:** CampaignCore acrescenta 257 verificações aprovadas e checagem de geração de dados. Inclui espaço insuficiente, consumo sem pré-requisito, tratamento/estabilização, ação remota enquanto oculto, pausa/carregamento, repetição sem som/custo, quatro resultados de fuga convencionais, fronteira fatal e rejeição de arquivo corrompido/incompatível sem mutação. A regressão dos núcleos, NarrativeCore/DirectorRuntime/auditoria, MapCore e ScenarioLab passou novamente. A cobertura CMake do novo módulo integra o workflow Core game rules. Os cenários usam prova física simulada; não substituem o adaptador real.

**Etapa P03/P04 posterior:** passaram localmente DirectorCore 41 verificações, paridade de 8.384 vetores Python/C++, CampaignNarrative 245, CheckpointFile 87 e encerramento de processos 51, além das 257 de CampaignCore. A regressão SaveCore 85/CampaignSave 108/StorySave 62 e os validadores narrativos/mapa passaram novamente. Testes incluem serial antigo, planos incoerentes mesmo com checksum recalculado, retomada sem repetir efeito, leitura restrita, arquivo danificado/incompatível e concorrência de slot. O workflow adiciona CMake/CTest do diretor e campanha Windows/MSVC. A primeira execução Windows revelou que as trocas genéricas dos núcleos antigos não satisfaziam `noexcept` nesse compilador; foram substituídas por trocas explícitas dos membros, mantendo os checks de commit sem exceção e sem alterar formato. O histórico na PR registra a validação final. Nenhum desses ensaios prova perda elétrica física ou restauração dos atores.

O passe de revisão está concluído quando as correções e este diagnóstico estão versionados e os checks passam. **O pacote pré-Unreal completo permanece aberto** até os itens de implementação acima serem concluídos ou terem seu escopo explicitamente revisto com Cristhian. A avaliação criativa do autor e as verificações jogáveis continuam distintas dos testes automáticos.

[PR de preparação e revisão](https://github.com/CristhianAlbino/the-thug/pull/19)

**Etapa P07 posterior (02/10):** recarga/preparo reservam recursos, consumo ocorre na conclusão física e interrupção libera reserva. Tiros não concedem identidade; Hunter pode ser interrompido, com proteção contra travamento contínuo. Granadas persistem com pose/velocidade/fusível; tempo para na primeira detonação para exigir consulta física. Respiração controlada tem reserva finita e retomada audível. 1.372 verificações específicas aprovadas localmente; não são 1.372 partidas nem validação da física Unreal.

**Etapa P08 posterior (02/10):** dois resultados raros próprios, preparação/custos individuais, instalação com reserva, resolução física, janela de fuga, colapso sobrevivido/fatal e consequências testemunhadas. N33/epílogos e cinco participações pontuais integram o runtime v6. Passaram 1.033 verificações de CampaignSpecial, as regressões CampaignCore/Combat/Hunter/Companions/Narrative/Checkpoint, 71 de RunRules, 41 do diretor e 8.484 vetores Python/nativo. Esses números são asserções/vetores; não partidas humanas. Autoria lógica, encenação, destruição física e convidados ainda exigem integração/produção.

**Etapa P02/P06 posterior (02/10):** as cinco extensões aprovadas integram o runtime v7: sinais com recusa, seguir/investigar sem teleporte, silêncio com suspensão/retomada, gestos com recursos reais, calços recuperáveis/ruptura pelo Hunter, variações opcionais sem reroll e registro pessoal com destinos desconhecidos. Pressão temporal após percepção de ameaça também foi ampliada. Passaram 566 verificações específicas, incluindo inventário cheio, corrida de recuperação, prioridade da cena principal, viagem com pressão e save rejeitado sem mutação. Regressões de campanha/combate/Hunter/narrativa/arquivo e paridade de 8.484 vetores passaram. São provas de lógica com contexto físico simulado; encenação e jogo na Unreal permanecem pendentes.

**Etapa P02/P05/P06 posterior (03/10):** contenção temporária distinta de destino terminal; soltura manual/com ferramenta por Joe ou companheiro; tentativa própria e ajuda recusável; participantes/recursos/seriais/tempos persistidos; lesão/medicina, captura de quem ajuda e transferência terminal interrompem progresso. Captura confirmada exige contato visto e remoção física; não se conclui por relógio nem se converte em morte. Joe só registra o que testemunhou. Runtime v8 valida o ciclo contra recibos e reserva vagas para concluir/cancelar ações iniciadas. Passaram 1.605 verificações específicas e regressões nativas de campanha. Provas são de lógica; contato contínuo, restauração espacial, encenação e teste humano continuam pendentes. Interrogação não foi implementada neste ciclo.

**Etapa v9 posterior (03/10):** pilhas genéricas, observação pessoal datada, relatos/mentira, interrogação recusável, usos conjuntos, armas individuais dos NPCs e planos adaptativos persistidos. Resultado local da rodada completa: CampaignCore 257, Extensions 982, Strategy 599, Rescue 1.605, Interactions 566, Special 1.082, Combat 1.372, Hunter 12.762, Companions 579, Narrative 245, CheckpointFile 87 e encerramento de processos 51. DirectorCore 41 e paridade de 8.484 vetores, validadores narrativos/auditoria estrutural e MapCore também passaram. Contagens são asserções/vetores com contexto físico simulado. Catálogo de produção verificado com 314 cues/35 requisitos; avaliação criativa/humana e arquivos finais seguem abertos.
