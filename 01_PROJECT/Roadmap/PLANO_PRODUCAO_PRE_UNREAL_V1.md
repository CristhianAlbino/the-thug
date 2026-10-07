# Preparação da produção — 03/10/2026

Esta frente pode começar fora da Unreal. O catálogo indexado foi preparado; gravações e assets finais ainda não foram produzidos. Em 07/10/2026 foi gerado um [kit 3D procedural de protótipo](../../05_ART/PrototypeKit_v01/README.md), com malhas, UVs, materiais, rig e clips reais, ainda com aparência simplificada e sem aprovação artística ou importação no editor. A ordem atual do criador é: assets/modelos/texturas/rig/animações primeiro; **vozes por último antes da Unreal**. Não interpretar a autorização para desenvolver como aprovação automática de cada atuação.

## Entregas preparadas

O [manifesto interno](../../Tools/ProductionCore/README.md) liga os 314 cues atuais aos textos e gestos de origem e aos lotes complementares. São 240 cues de fala, 28 de gesto, seis cues de gravação, 14 de documento e 26 de epílogo. Estes são registros de apresentação: não equivalem ao número de arquivos finais, documentos únicos ou gravações AUD da campanha. Os oito roteiros AUD continuam no pacote autoral, incluindo variantes protegidas. O catálogo registra zero arquivos produzidos.

| Ordem | Lote | Entrega concreta | Critério de avaliação |
| --- | --- | --- | --- |
| 1 | Revisão de direção | [Thomas, Jack, vínculos e leitura por ID](../../02_GAME_DESIGN/Narrative/DIRECAO_CRIATIVA_REVISAO_V2.md) | Criador registra manter/ajustar; sem mudar regra de dificuldade ou inventar conhecimento. |
| 2 | Primeiros dez minutos | Personagens provisórios, corredor, portas/circuitos, passos/respiração, abertura e legendas | A ameaça é compreensível e a navegação legível no blockout. Arte final entra após esse teste. |
| 3 | Personagens e gestos | Base procedural v01 produzida; refinar rosto/anatomia/roupas, texturas e movimentos conforme direção visual e gestos do catálogo | Animação mantém mãos/apoios e tempo da tarefa; cancelamento libera pose sem concluir ação. |
| 4 | Hospital e objetos | Porta, quadro e cama de protótipo produzidos; ampliar kit modular e pistas legíveis conforme bíblia visual | Dimensão/colisão/visada aprovadas no blockout; detalhe visual não esconde informação necessária. |
| 5 | Foley, sinais e ambiente | Passos por material, portas, ferramentas, tiros, explosões, respiração, aviso de contenção e soltura | Fonte espacial coincide com ação física; som de produção não cria um segundo evento lógico. Distância/oclusão calibradas na Unreal. |
| 6 | Música e mix | Exploração, tensão, investigação, proximidade, perseguição, esconderijo e recuperação | Mix preserva passos, avisos e fala útil; mudanças seguem percepção real e não revelam posição escondida. |
| 7 | Voz pt-BR e gravações — último lote antes da Unreal | Takes separados por cue/variante, locutor e versão; AUD01–08 segundo seus roteiros | Leitura natural, linguagem permitida, pausa/recusa perceptível e variação coerente com ações. Sem confirmar segredo por áudio fora das condições. |

## Organização para produção

Voz: WAV PCM mono como master; uma versão por take, sem recortar a pausa significativa. Nome provisório `VO_ptBR_<cue>_<speaker>_takeNN.wav`, trocando `:` por `_` apenas no nome do arquivo. O ID original permanece no catálogo. Montagem de rádio/gravação usa cópia processada; preservar master limpo. Música mantém stems separados; loops de ambiente precisam de emenda inaudível. Essas são especificações de trabalho, sujeitas ao orçamento e ao teste de memória/importação.

Modelos: manter fonte editável e exportação de intercâmbio; pivô, escala, sockets, colisão e LOD precisam de verificação no projeto. Documentos/legendas têm master de texto; nunca colocar a única informação útil em uma textura ilegível. Asset de porta usa estados funcionais separados, inclusive ferramenta deixada/calço rompido e falta de energia.

Cada arquivo produzido recebe responsável/origem, versão da fonte, status de revisão e destino de importação. Atualizar `production_file` somente quando o arquivo realmente existir; o índice de requisitos permanece separado do registro de produção final. A origem dos materiais integra D-10 antes da distribuição, conforme o plano existente.

## Pendências reais

- Revisão de direção com o autor e leitura em voz alta.
- Leitura/sessão de mesa opcional conforme [protocolo sem solução prévia](../Reviews/PROTOCOLO_TESTE_HUMANO_PRE_UNREAL_V1.md); não é um playtest de gameplay nem bloqueia o início na Unreal. Gameplay humano requer build jogável na Unreal.
- Gravação/produção e avaliação dos arquivos acima.
- Ensaio espacial, atuação, acústica, interface, desempenho e duração no editor.

Modo Extremo/Joker permanece expansão posterior, conforme D-13. O pacote técnico da campanha principal não promete entregar esse quinto modo nem um jogo completo por importação automática.
