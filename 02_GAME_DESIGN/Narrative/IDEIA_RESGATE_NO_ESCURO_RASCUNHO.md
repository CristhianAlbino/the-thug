# Resgate no escuro — proposta de momento impactante

**Estado:** proposta nova apresentada a Cristhian em 02/10/2026, distinta das cinco melhorias já aprovadas e implementadas no primeiro ciclo nativo. Não existe ainda um encontro jogável nem um sistema de resgate/captura temporária pronto. A frente de captura de P05/P06 continua aberta.

Joe ouve Bartholomeu chamando baixo do outro lado de uma área escura. Antes de entrar, consegue perceber sinais suficientes para desconfiar: um movimento interrompido, a voz vindo de um lugar coerente, uma silhueta que teve tempo físico de chegar. Hunter capturou um bandido e está vigiando o caminho de quem pode voltar por ele. O medo surge da decisão de se aproximar, sem congelar o jogador num susto inevitável.

Joe pode ajudar, preparar distração/fumaça, pedir silêncio, abrir outro caminho ou sair. Harley tenta proteger Barth se realmente percebe a situação; pode cooperar com Joe, sem surgir à distância. Barth não fica esperando passivamente: tenta se soltar, se esconder ou alcançar uma passagem que conhece. Jack pode ajudar de forma desconfortável ou priorizar sua própria fuga, conforme relação e perigo percebido.

O resgate precisa de uma sequência física interrompível. Uma distração válida pode afastar Hunter; um ruído mal calculado pode revelar quem entrou, somente se alcançar seus sensores. Joe pode perder uma ferramenta ao segurar uma porta ou gastar o tratamento que queria guardar. A marca emocional é concreta: encontrar depois o companheiro que ajudou, ouvir uma frase pequena, perceber a falta de quem ficou para trás. O registro da noite preserva o que Joe testemunhou e deixa destinos incertos quando faltam provas.

Batman não ganha conhecimento por script nem mata o refém como punição automática. A isca vem da observação de um reencontro possível, não de tortura ou informação inventada. Recusar o resgate mantém caminhos de fuga; tentar ajudar deve ter contrajogo legível e possibilidade real de sucesso.

## Implementação necessária

1. Captura temporária distinta de destino terminal: contenção iniciada, contido, tentativa de soltura, libertado ou captura consumada. O comando atual de destino confirmado não deve ser revertido casualmente para simular um resgate.
2. Interações conjuntas com participantes, recursos reservados, duração, serial e cancelamento; ferimento/perda de contato pode interromper sem duplicar custo.
3. Hunter percebe contato, caminho e distração por sensores reais; chega/vigia por navegação e tem janela de reação.
4. Companheiro mantém inventário, medo, agência e memória própria. Sua tentativa não revela Joe nem concede saída não explorada.
5. Save conserva estágio e tempo, sem concluir captura ou resgate pela intenção. Sem morte automática fora da simulação.

## Testes que decidirão se funciona

Testar o encontro sem conhecer sua solução; resgatar com e sem recurso; perder contato durante a soltura; companheiro que tenta escapar sozinho; Hunter que segue a distração e o que não a percebe; Joe que abandona; rota alternativa fechando; carga do save no meio da contenção. Medir se o jogador identifica risco e escolha antes da consequência. Uma encenação fixa que ignora essas decisões não satisfaz a proposta.
