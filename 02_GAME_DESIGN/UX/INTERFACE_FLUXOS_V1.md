# The Thug — HUD, menus e fluxo de informação v1

**Objetivo:** primeira pessoa com pouca informação e decisões legíveis. Interface é uma proposta verificável para futura UMG/Blueprint; nenhum widget foi construído no editor. Fácil e Normal ajudam mais na orientação, Difícil reduz mapa/escuta, Punitivo remove mapa e escuta conforme GDD. Acessibilidade não deve introduzir posição onisciente do Hunter.

[Ver wireframe vetorial da exploração e da sala de câmeras](WIREFRAME_HUD_CAMERAS.svg). A tela é um exemplo de leitura, não arte final ou widget Unreal.

## Fluxos principais

| Tela/ação | Entrada | Saída e informação permitida |
| --- | --- | --- |
| Menu inicial | Continuar, Novo Jogo, Dificuldade, Opções, Créditos | Continuar mostra dificuldade e checkpoint; Modo Extremo/Joker não aparece como jogável na versão principal. |
| Novo Jogo | Escolher Fácil/Normal/Difícil/Punitivo; resumo claro de mapa, escuta, recursos e amanhecer | Inicia N01. Punitivo avisa que amanhecer não encerra automaticamente a partida, sem revelar Jack. |
| Exploração | Retícula discreta só em objeto interativo ao alcance, pista luminosa física e legenda de som | Nenhuma seta permanente para saída ou marcador vivo do Hunter. Objetivo atual é evidência encontrada, não tarefa invisível. |
| Inventário | Pausa ou risco conforme decisão de editor; slots usados/capacidade, item, ação e origem | Se cheio, mostrar custo e escolha de descarte/retorno antes de tentar pickup. Não descartar chave de rota sem alternativa recuperável. |
| Diário | Documentos já inspecionados, código com três campos ainda vazios, últimas observações datadas | Visto, ouvido e inferido têm marcadores diferentes. Difícil/Punitivo não ganham mapa por abrir diário. |
| Câmeras | Fonte, horário, estado: **ao vivo / repetição / interferência / sem energia** | Imagem antiga nunca recebe rótulo “área livre agora”. Feed atual só mostra nó visível pela câmera e energia correta. |
| Pausa/save | Retomar, Opções, Carregar, Sair | Confirma checkpoint/dificuldade e aviso se um save está em gravação; save não reverte falha de rota sem carregar estado anterior. |
| Desfecho | Resultado de Joe, depois blocos de Harley/Bartholomeu/Jack/Thomas por prova | `unknown` é “sem notícia”, não ícone de morte. Coda N34 Difícil/Punitivo é separada do que Joe sabe. |

## Wireframe funcional — exploração

| Região da tela | Conteúdo e quando aparece |
| --- | --- |
| Centro | Retícula pequena somente perto de objeto alcançável; prompt de segurar/pressionar com alternativa de remapeamento. |
| Canto inferior esquerdo | Ferimento/sangramento e stamina discretos; surgem ao mudar estado ou correr, esmaecem sem ocultar alerta crítico. |
| Canto inferior direito | Item em mãos e quantidade física; animação/sons do objeto também informam. |
| Meio inferior | Legenda contextual com fonte aproximada (“porta metálica”, “voz de Harley”) quando a fonte é audível; sem distância/rumo impossível. |
| Tela inteira | Vinheta de exaustão/ferida opcional e regulável; nunca único sinal de estado. |

## Leitura das quatro dificuldades

| Informação | Fácil | Normal | Difícil | Punitivo |
| --- | --- | --- | --- | --- |
| Mapa permanente | Sim, setores visitados | Sim, setores visitados | Não; placas/diário | Não |
| Escuta | Eficiente | Disponível | Imprecisa | Ausente |
| Pista de rota | Legenda e diário claros | Diário e marcos | Documento físico + diário dos fatos vistos | Mesmos fatos físicos, menos ajuda de orientação |
| Câmeras | Estado visível e útil | Estado visível | Fonte valiosa, sujeita a falha | Fonte física, nunca onisciente |
| Conhecimento de Thomas | Sem confirmação | Sem confirmação | Prova N34/coda | Prova N34/coda |

O diário pode mostrar “ouvi Bartholomeu mencionar bombas” se isso ocorreu; não pode marcar `B1_TUNNEL` como descoberto até Joe saber o acesso. Mostrar a confiança exata do companheiro em número prejudicaria interpretação e revelaria o que ele pensa; usar comportamento/fala. Testar se jogadores conseguem distinguir estado da câmera e da porta sem mini tutorial intrusivo.

## Acessibilidade e controles a especificar no kit

- Legendas configuráveis em tamanho, contraste, fundo e identificação de falante; sons decisivos têm pista visual contextual não onisciente. Não exigir audição perfeita para o túnel.
- Remapeamento de movimento, agachar, interagir, segurar item, inventário/diário, câmera e pausa. Alternativa pressionar versus segurar; sensibilidade, inversão e redução de tremor de câmera.
- Brilho com calibração de três referências: porta obrigatória, silhueta a média distância e canto que deve continuar incerto. Opção de reduzir cintilação, motion blur, grão, vinheta e desconforto de câmera.
- Pistas obrigatórias não dependem só da cor vermelha/verde. Ícones, forma, texto e luz local redundantes. Sem contraste ilegível por “realismo”.
- Texto em português brasileiro inicialmente; IDs/localização preparados para outros idiomas, sem idioma adicional prometido antes de orçamento.

## Critérios de aceite no editor

Jogador identifica porta utilizável, estado de energia, câmera em loop e inventário cheio numa tela comum; entende que perdeu o helicóptero mas pode continuar; chega à janela e sabe que a travessia é mão única; não infere falsamente que Thomas morreu no Fácil/Normal pela interface. HUD legível com baixa visão/legenda ampliada precisa de teste real de resolução, não só wireframe.
