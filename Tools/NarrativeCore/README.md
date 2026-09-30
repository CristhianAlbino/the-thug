# NarrativeCore — dificuldade, revelações e epílogos

`variants.json` guarda as diretrizes aprovadas pelo criador. `variant_rules.py` resolve **elegibilidade** de N34/J1–J4 e IDs de epílogo a partir de fatos de campanha que a Unreal deverá persistir. `validate_variants.py` cruza cenas e IDs com o mapa e verifica que segredos não vazam para dificuldades indevidas.

```bash
python3 Tools/NarrativeCore/validate_variants.py
```

- Fácil/Normal: destino de Thomas permanece não confirmado; nenhuma pista da variante Punitivo de Jack.
- Difícil/Punitivo: morte de Thomas causada por Batman aparece em N34, ou em coda ao jogador caso Joe não a tenha descoberto. A coda não altera o conhecimento de Joe.
- Punitivo: Joe demonstra reciprocidade para Harley; J1/J2/J3 só ficam elegíveis após os atos 2/3/4, respectivamente, com Jack presente e contexto observado pela direção de cena. É possível perder uma pista sem bloquear as seguintes. J4 exige ato 5, J3 e outra pista testemunhada, com Jack disponível. O ato é um marco narrativo, **não** minuto de jogo. A função não dispara cenas sozinha.
- Epílogos preservam `together`, `escaped_alone`, `captured`, `dead` e `unknown` separadamente para cada companheiro. Nunca deduzem morte a partir de abandono ou silêncio.

Esses IDs são contrato para futuras cenas e saves, **não** um gerador de vídeo, diálogos completos ou duração. `world_truth.hospital_history` registra uma proposta ambiental sobre Arkham, não uma explicação obrigatória. A direção está no [rascunho da campanha](../../02_GAME_DESIGN/Narrative/CAMPANHA_ATOS_RASCUNHO.md).
