# The Thug — kit 3D de protótipo v01

Este pacote contém **arquivos 3D reais**, com aparência procedural simplificada. É uma primeira base técnica para testar modelagem, materiais, UVs, rig e clips; não é o elenco final nem uma reprodução do realismo das fichas conceituais.

1. Extraia o ZIP.
2. Abra `TheThug_Kit3D_v01.html` em um navegador com WebGL 2, preferencialmente no computador. Se a prévia não abrir, os arquivos GLB continuam disponíveis nas pastas Characters e Props.
3. Selecione o personagem/objeto. Arraste para girar e use a roda do mouse para aproximar. O menu de movimentos, os ossos e o quadriculado ajudam a revisar a base.

| Arquivos | Conteúdo |
| --- | --- |
| `Characters/SK_Joe_v01.glb` | Joe com boina, sobretudo e gola alta. |
| `Characters/SK_Harley_v01.glb` | Harley loira, pontas rosadas, jaqueta e detalhes vinho. |
| `Characters/SK_Jack_Standard_v01.glb` | Jack padrão com roupa escura. |
| `Characters/SK_Jack_Punitive_v01.glb` | Mesmo Jack com jaleco claro, colete vinho e gravata verde. |
| `Characters/SK_Thomas_v01.glb` | Thomas, altura 1,97 m, pele escura, porte musculoso, dreads/barba e tatuagens de protótipo. |
| `Characters/SK_Bartolomeu_v01.glb` | Bartolomeu jovem, ruivo, óculos e corpo esguio, altura inicial 1,72 m. |
| `Props/` | Porta, quadro elétrico e cama hospitalar. |
| `Textures/` | Mapas PNG procedurais BaseColor, ORM e NormalGL. |
| `Previews/` | Imagens renderizadas a partir dos GLBs reais. |
| `asset_manifest.json` e `validation_report.json` | Inventário, dimensões, hashes e verificações executadas. |

As verificações de estrutura, pesos e transformações estão nos relatórios incluídos. A sintaxe e a matemática do visualizador passaram pela validação; sua renderização WebGL no navegador ainda não foi executada neste ambiente.

Cada personagem tem 54 juntas e seis clips básicos. Eles são referências de movimento; apoios dos pés, mãos, roupa e contato com objetos ainda precisam de ajuste. Os modelos têm partes sobrepostas, muitos objetos de malha e ainda não foram otimizados.

**Ainda falta:** esculpir anatomia e faces, refinar cabelos/roupas, fazer retopologia de produção e texturas finais, atlas/bakes, rig facial/IK, corrigir clipping, produzir animações finais e verificar importação no Blender/Unreal. A semelhança artística com as fichas 2D não está aprovada. Alturas de Joe, Harley e Jack são escolhas provisórias deste lote; 120 kg de Thomas é direção do autor, não um parâmetro de massa simulado.

Hunter permanece adiado. Nenhuma voz foi gerada. Conforme a ordem do autor, vozes ficam por último antes da Unreal.

Fonte de geração e ponte Blender: `Tools/ArtPipeline` no [repositório](https://github.com/CristhianAlbino/the-thug). O script de Blender existe, mas não foi executado aqui; não há `.blend` ou FBX produzido neste pacote.
