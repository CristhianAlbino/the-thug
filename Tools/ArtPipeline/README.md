# ArtPipeline — kit procedural de protótipo v01

Gera geometria 3D real em GLB, UVs, mapas de material, esqueleto e clips básicos. Não converte as imagens conceituais automaticamente em personagens realistas. A aparência deste lote é simplificada, abaixo do acabamento pretendido para o jogo.

O código e a configuração são as fontes editáveis. As saídas são regeneráveis em `05_ART/PrototypeKit_v01`; o pacote entregue contém os GLBs, texturas e prévias. Não há Blender ou Unreal instalado neste ambiente e não foi realizada importação nessas ferramentas.

## Executar

Python 3.12, a partir da raiz do repositório:

Ambiente usado na produção/validação deste lote: NumPy 2.3.5 e Pillow 12.3.0. Geometria e seeds são determinísticos no mesmo ambiente; hashes PNG podem variar entre versões das bibliotecas, sem alterar a direção visual.

```sh
python -m pip install -r Tools/ArtPipeline/requirements.txt
python Tools/ArtPipeline/build_kit.py
python Tools/ArtPipeline/validate_kit.py
python Tools/ArtPipeline/build_viewer.py
python Tools/ArtPipeline/render_preview.py
```

`build_kit.py --only Joe` produz apenas Joe e não produz os props. Executar sem `--only` antes da validação/publicação do pacote completo. O gerador completo produz seis variantes de personagens, três props, 90 mapas PNG e um manifesto. O visualizador é um HTML autossuficiente; não precisa de servidor ou internet. Ele inclui o download de cada GLB, câmera orbital, seletor de animação, inspeção de ossos e quadriculado UV. Requer WebGL 2.

## Dados e contratos

- Medidas em metros, malha no chão em Y=0; glTF Y para cima, frente +Z. Fonte Python usa Z para cima e frente -Y; a conversão é aplicada à malha, juntas e bind matrices.
- 54 juntas por personagem, com dedos, hierarquia, pesos normalizados e inverse bind matrices. Esqueleto próprio, sem compatibilidade automática com Manny/Quinn, IK ou retarget pronto.
- Pose de braços parcialmente abertos. Partes de roupas, membros e detalhes são malhas separadas e sobrepostas; não é uma topologia corporal contínua para produção final.
- UVs locais por parte e materiais repetíveis. Não há atlas único, lightmap UV, textura pintada no rosto ou bake de escultura.
- BaseColor, ORM (`R=1`, `G=roughness`, `B=metallic`) e NormalGL. O canal R não contém oclusão geométrica calculada. O GLB usa roughness/metallic e normal; o render Python é uma prévia difusa com base color, não uma renderização PBR completa.
- Idle_Breath, Walk_InPlace, Crouch_Hold, Look_Over_Shoulder, Help_Reach e Stop_Signal são clips de referência, amostrados a 24 Hz. Não há root motion, IK, contato de pés garantido, controle facial ou sincronização com as tarefas da campanha.
- Porta e quadro elétrico têm pivôs separados e clip de abertura visual. Não produzem eventos do runtime, não têm colisões, sockets ou estados de energia implementados.

## Verificação executada

`validate_kit.py` relê os arquivos exportados: chunks GLB, limites/alinhamento de buffers, imagens PNG, índices, normais, UVs, pesos, bind pose, alturas e deslocamento de vértices sob animação. Confere igualdade geométrica de cabeça/rosto/cabelo entre as variantes de Jack. Não é o validador oficial Khronos e não substitui importação no editor ou avaliação artística.

`render_preview.py` produz vistas a partir dos GLBs reais, com rasterização, profundidade e interpolação de iluminação. Não usa imagens conceituais geradas como se fossem capturas 3D.

A sintaxe JavaScript e a matemática de transformações do visualizador são verificadas por `qa_viewer.js`. A renderização WebGL no navegador local ainda precisa de conferência: o executável de navegador não pôde ser instalado neste ambiente.

## Blender e depois Unreal

O script `export_blender.py` foi preparado e verificado sintaticamente, **não executado**. Com Blender instalado, o comando no início do script importa um GLB, empacota imagens, salva `.blend` editável e tenta exportar FBX com animações. Verificar deformações, escala, nomes/ações e materiais após executar; FBX não garante reconstrução automática dos materiais glTF.

Na Unreal, a verificação ainda inclui escala em centímetros, orientação, importação do esqueleto/FBX, retarget, animações, colisões, sockets, normal maps, materiais, LOD, draw calls, clipping da roupa e desempenho. Não foi feita nenhuma integração com Unreal neste lote.

Referências de formato: [glTF 2.0 — Khronos](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html) e [exportadores Blender](https://docs.blender.org/api/current/bpy.ops.export_scene.html).

## Ordem atual do autor

Assets e personagens primeiro; vozes por último antes de começar na Unreal. Hunter está adiado. A aprovação das fichas 2D não equivale à aprovação artística destas bases 3D; falta escultura e acabamento para chegar às referências.
