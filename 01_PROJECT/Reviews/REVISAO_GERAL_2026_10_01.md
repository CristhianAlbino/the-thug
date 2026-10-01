# The Thug — revisão geral de 01/10/2026

**Leitura segura para Cristhian:** este relatório não revela a combinação da garagem, a sequência de pistas ou os gatilhos internos de saves e esconderijo. Ele comenta somente decisões narrativas que o criador já conhece.

**Parecer:** a identidade do jogo está consistente e existe uma fundação de código verificável. A preparação completa ainda não está fechada: a revisão encontrou falhas que foram corrigidas e sistemas do GDD que continuam apenas parcialmente implementados. O repositório não contém ainda um projeto Unreal executável. Importar pastas não montará automaticamente o hospital nem conectará os personagens.

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

## O que o código realmente entrega hoje

**Atualização após a revisão — coordenação e transações:** o novo [CampaignCore](../../Tools/CampaignCore/README.md) entrega o núcleo P01: relógio compartilhado, pausa, perfis/equipamentos vindos da autoria, ordenação pelo instante confirmado e fim da partida. A primeira cobertura P02 conecta coleta, tratamento de Joe, ferramenta/consumo, portas/energia e munição aos companheiros. Falhas descartam a cópia inteira; retentar um ID confirmado não repete efeito. O envelope runtime salva pausa, ocupação e recibos junto aos estados anteriores. Diretor nativo, disco durável e execução completa dos companheiros continuam abertos.

| Frente | Implementado/testável fora da engine | Limite atual |
| --- | --- | --- |
| Hunter | Patrulha, investigação, perseguição e busca; estimativas, memória observada e escolha probabilística de rota. | Estados especiais, agressividade dinâmica, plano de emboscada, armadilhas e interrogatório ainda não formam o sistema avançado do GDD. |
| Companheiros | Traços individuais, conhecimento de rotas, escolhas ponderadas, consequências de ajuda/abandono e entrega coordenada de munição. | Escolher um objetivo não o executa no mundo. Coleta, tratamento, mentiras verificáveis, manipulação de objetos e ações conjuntas ainda precisam de execução e transações completas. |
| Sobrevivência | Stamina, níveis de ferimento, sangramento, tratamento e inventário limitado; coleta/tratamento de Joe transacionais no coordenador; efeito de ferimento sobre ruído ensaiado. | Disparo, recarga, projéteis, fumaça, explosões, oxigênio e controle voluntário da respiração não estão completos. |
| Mundo e percepção | Estados de porta/circuito/câmera/esconderijo; propagação abstrata de som e memória de alterações vistas. | Não há colisão, raycast, visibilidade por luz/postura, navmesh ou streaming reais. |
| Campanha e narrativa | Coordenador nativo, dados iniciais autorados, pausa, término e quatro rotas convencionais; regras narrativas em Python; registros de leitura, conversas, pistas e epílogos. | Diretor nativo de falas ainda pendente. Prova física depende do adaptador. N33 possui intenção/elegibilidade, mas os finais raros não têm resultados próprios implementados em RunRules. |
| Persistência | Serialização versionada, validação conjunta, restauração atômica em memória e continuação reproduzível nos cenários testados; envelope runtime acrescenta pausa, ocupação e recibos. | Arquivo durável, recuperação de falha de escrita e restauração dos atores/animações não estão concluídos. Sistemas futuros precisam entrar no snapshot. |
| Conteúdo | GDD, roteiros, dados, mapa, direção visual, UX e critérios de integração. | Documentação e transcrições não são níveis, modelos, vozes, música ou uma build jogável. |

## Pendências que podem avançar antes da Unreal

Esta é a lista revisada. Os itens não são requisitos para sequer abrir o editor; são o trabalho restante para chamar **todo o escopo independente da engine** de concluído.

| ID / prioridade | Trabalho | Evidência exigida para encerrar |
| --- | --- | --- |
| P01 · núcleo entregue | CampaignCore: relógio, pausa, ordem dos eventos, fim da partida e perfis/equipamentos iniciais de autoria. | Testes integrados nas quatro dificuldades; pausa não avança, morte no meio de delta longo interrompe os demais relógios, retomada preserva estados e recibos. Ponte física/editor segue pendente. |
| P02 · parcial | Coleta, tratamento de Joe, ferramentas, portas/energia e munição coordenados. Falta transferência de outros recursos/tratamento entre companheiros e ações longas com reserva/cancelamento. | Falha preserva todos os lados; sucesso consome/transfere uma vez e mantém mundo, inventário e sobrevivência concordantes após retomada. Primeira cobertura testada; as operações restantes ainda precisam de contrato e casos próprios. |
| P03 · primeira | Portar o diretor de referência para C++/dados de runtime; exportar textos condicionais e estruturar confirmação de linhas/gestos. | Mesmo resultado dos vetores Python; prioridades, cancelamento, idioma e conteúdo restrito preservados; Markdown não é o motor de diálogo. |
| P04 · primeira | Persistência durável em disco e cobertura de novos estados de campanha. | Interrupção/erro de escrita preserva um checkpoint válido anterior; carregar é tudo ou nada; versões incompatíveis têm tratamento explícito. Aplicação em atores fica para Unreal. |
| P05 · seguinte | Hunter avançado: agressividade, observação, emboscada, manipulação do ambiente e escolha de alvo. | Cenários demonstram causa observada, opção de erro, caminho possível fornecido pelo mundo e oportunidade de reação do jogador. |
| P06 · seguinte | Execução sistêmica dos companheiros e consequências além dos dois eventos fixos. | Objetivo escolhido gera ação ou falha justificada, sem informação mágica; inventários, ferimentos e resultados individuais persistem. |
| P07 · seguinte | Regras de armas, recarga, consumíveis, fumaça, explosivos, respiração e oxigênio. | Custos/estados/efeitos definidos e ensaiados; colisão, animação e sensação de uso ficam para o editor. |
| P08 · seguinte | Finais raros e participações especiais previstos no GDD. | Condições e consequências autoradas, resultado reconhecido pelo sistema de campanha, save compatível e epílogo correspondente. Robin e demais participações não contam como IA já pronta. |
| P09 · criativa | Encenação final de Thomas, gesto da revelação de Jack e leitura em voz alta de falas/finais. | Versão indexada revisada com Cristhian, ações/reações demonstráveis e nenhum conhecimento inventado. Revisar estes temas não exige expor as soluções das rotas. |
| P10 · validação | Teste humano sem solução prévia e revisão das hipóteses de progressão. | Observações de alguém que não conhece os segredos, com relatório de dificuldade de compreensão sem revelá-los ao criador. O ensaio automático existente não equivale a esse teste. |

O próximo conjunto recomendado é **P03 → P04**, usando o coordenador agora implementado. P02 continua junto de P06 para recursos/execução completos dos companheiros. P05–P08 ampliam o comportamento. A revisão não removeu esses sistemas do projeto nem os marcou como entregues.

## Trabalho que precisa do editor e da produção

Criar o projeto C++ da Unreal e compilar as pontes; montar um trecho jogável; representar portas/objetos e personagens por atores; implementar movimento, percepção visual, navegação e animação; construir HUD/menus; montar o blockout completo; integrar cinematics, áudio e assets; medir duração/performance; testar as dificuldades e empacotar uma build de PC.

Áudio e assets finais permanecem depois desta revisão, conforme a ordem escolhida pelo criador. O Modo Extremo/Joker continua no backlog: não virou um quinto modo parcialmente ligado à campanha principal.

## Verificação e fechamento

Os testes usados nesta revisão estão nos scripts `run-tests.sh` dos núcleos, no ScenarioLab e nos validadores de NarrativeCore/MapCore. Compilação direta usa C++17 com avisos tratados como erro. CMake é verificado no workflow de CI, pois não está instalado no ambiente local desta revisão. A PR mantém o histórico das execuções.

**Resultado local observado:** HunterCore 149, RunRules 37, CompanionCore 533, SurvivalCore 53, PerceptionCore 30, IntegrationCore 23, SaveCore 85, CampaignSave 108, StorySave 62 e WorldCore 24 verificações aprovadas. São contagens de asserções dos executáveis, não campanhas completas jogadas. Também passaram NarrativeCore, DirectorRuntime, auditoria estrutural sem spoilers, MapCore, oito percursos do ScenarioLab e repetição determinística do cenário integrado. `git diff --check` não apontou erro de whitespace.

**Etapa de programação posterior:** CampaignCore acrescenta 257 verificações aprovadas e checagem de geração de dados. Inclui espaço insuficiente, consumo sem pré-requisito, tratamento/estabilização, ação remota enquanto oculto, pausa/carregamento, repetição sem som/custo, quatro resultados de fuga convencionais, fronteira fatal e rejeição de arquivo corrompido/incompatível sem mutação. A regressão dos núcleos, NarrativeCore/DirectorRuntime/auditoria, MapCore e ScenarioLab passou novamente. A cobertura CMake do novo módulo integra o workflow Core game rules. Os cenários usam prova física simulada; não substituem o adaptador real.

O passe de revisão está concluído quando as correções e este diagnóstico estão versionados e os checks passam. **O pacote pré-Unreal completo permanece aberto** até os itens de implementação acima serem concluídos ou terem seu escopo explicitamente revisto com Cristhian. A avaliação criativa do autor e as verificações jogáveis continuam distintas dos testes automáticos.

[PR de preparação e revisão](https://github.com/CristhianAlbino/the-thug/pull/19)
