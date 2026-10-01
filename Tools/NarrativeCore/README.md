# NarrativeCore — dificuldade, revelações e epílogos

`variants.json` guarda as diretrizes aprovadas pelo criador. `variant_rules.py` resolve **elegibilidade** de N34/J1–J4 e IDs de epílogo a partir de fatos de campanha que a Unreal deverá persistir. `validate_variants.py` cruza cenas e IDs com o mapa e verifica que segredos não vazam para dificuldades indevidas.

`act12_scenes.json` e `act12_rules.py` são o primeiro contrato de disparo das cenas N01–N11 do [roteiro de trabalho](../../02_GAME_DESIGN/Narrative/ROTEIRO_ATOS_1_2_RASCUNHO.md). A função `eligible(id, Facts)` exige local, cenas já vistas e, nos reencontros, **ator ativo no mesmo nó**. N11 exige evidência percebida pelo Hunter, trajeto físico até a área e alternativa de saída. `can_witness_j1` requer que Joe tenha realmente visto e ouvido a pista no Punitivo. Esses fatos vêm dos sistemas de mundo/percepção e são persistidos pelo futuro diretor; a função não os inventa. O validador ensaia caminhos em que Harley/Jack não aparecem, a sala de câmeras é ignorada e o Hunter não recebeu evidência.

O [roteiro dos atos 3–5](../../02_GAME_DESIGN/Narrative/ROTEIRO_ATOS_3_5_RASCUNHO.md) detalha N12–N34. `act35_scenes.json` e `act35_rules.py` vinculam os 23 IDs ao mapa, à presença física dos companheiros e aos fatos observados. N26 registra `N26:garage`, `N26:roof`, `N26:tunnel` ou `N26:window` separadamente; outra falha pode ganhar cena própria. A visita a `X_GARAGE`, `X_ROOF`, `X_TUNNEL` ou `X_WINDOW` não dá vitória: `can_close_route` exige também a respectiva confirmação do RunRules. N34 exige prova testemunhada e dificuldade correta; sua coda ao público fica no resolvedor de epílogo. J2/J3/J4 só são testemunhadas com Jack presente, contexto e visibilidade/audição suficientes. Flags como `jack_act3_encounter` representam **o encontro corrente** e não devem ficar ativas indefinidamente no save.

`ending_rules.py` deriva os cinco estados individuais de fatos verificados. Abandono ou último contato sem confirmação produz `unknown`; resultado contraditório é rejeitado. O último nó de contato é metadado para texto, não prova de morte. A composição de blocos usa `resolve_epilogue` e distingue o que Joe testemunhou do que a coda mostra ao público.

```bash
python3 Tools/NarrativeCore/validate_variants.py
```

- Fácil/Normal: destino de Thomas permanece não confirmado; nenhuma pista da variante Punitivo de Jack.
- Difícil/Punitivo: morte de Thomas causada por Batman aparece em N34, ou em coda ao jogador caso Joe não a tenha descoberto. A coda não altera o conhecimento de Joe.
- Punitivo: Joe demonstra reciprocidade para Harley; J1/J2/J3 só ficam elegíveis após os atos 2/3/4, respectivamente, com Jack presente e contexto observado pela direção de cena. É possível perder uma pista sem bloquear as seguintes. J4 exige ato 5, J3 e outra pista testemunhada, com Jack disponível. O ato é um marco narrativo, **não** minuto de jogo. A função não dispara cenas sozinha.
- Epílogos preservam `together`, `escaped_alone`, `captured`, `dead` e `unknown` separadamente para cada companheiro. Nunca deduzem morte a partir de abandono ou silêncio.

Esses IDs são contrato para futuras cenas e saves, **não** um gerador de vídeo, diálogos completos, geometria ou duração. O adaptador da Unreal deve fornecer posição/alcance/observações reais e persistir os IDs de cena e evidência; o código Python não move Batman nem companheiros. `world_truth.hospital_history` registra uma proposta ambiental sobre Arkham, não uma explicação obrigatória. A direção está no [rascunho da campanha](../../02_GAME_DESIGN/Narrative/CAMPANHA_ATOS_RASCUNHO.md).
