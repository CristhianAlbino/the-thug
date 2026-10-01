# Pós-campanha — Modo Extremo / Joker (ideia do criador)

**Estado: backlog para depois de concluir e revisar a campanha principal, áudio e assets. Não faz parte do escopo pré-Unreal nem das quatro dificuldades atuais.** Registrado em 30/09/2026 a pedido de Cristhian.

## Fantasia central

- Desbloqueio somente após concluir o **Punitivo**; a condição exata de vitória e a persistência entre perfis/saves serão definidas quando a campanha normal estiver jogável.
- Recursos abundantes e companheiros fiéis como no Fácil, contrapostos a um Hunter extremamente mais agressivo.
- Aparição especial do **Batman que Ri** e uso do gás do **Espantalho**. A experiência passa a questionar a percepção: visão, áudio, memória de ambiente e identidade das ameaças podem ser falsos.
- Joe pode tomar uma decisão irreversível contra um companheiro por acreditar numa ameaça ilusória. Isso deve surgir de uma ação concreta do jogador dentro de regras legíveis, com agência e consequências persistentes; nenhum companheiro morre por sorteio invisível.

## Regras de autoria para um protótipo futuro

1. Definir uma camada de **estado físico real** separada da **percepção intoxicada de Joe**. Hunter, companheiros e portas obedecem ao mundo físico; somente pistas sensoriais e interpretação podem mentir segundo regras testáveis.
2. Sinalizar início, intensidade e cessação do gás por fontes plausíveis e pistas verificáveis. O jogador pode ficar em dúvida sobre *qual* imagem é real, mas precisa aprender procedimentos de checagem; não é uma rolagem arbitrária que muda a verdade do mundo.
3. Uma ação potencialmente letal contra companheiro exige contexto, oportunidade e entrada deliberada, com alternativas de hesitar, verificar ou recuar. Legibilidade e acessibilidade do efeito visual/sonoro serão verificadas no editor.
4. O Hunter mais agressivo continua preso a percepção, deslocamento e cobertura. Agressividade não concede posição exata nem teleporte.
5. Saves, epílogos, conteúdo secreto de Jack e progressão de desbloqueio exigirão versão de dados própria. Não alterar `Difficulty` dos núcleos atuais até termos protótipo, playtest e critérios de aceitação da campanha principal.

## Dependências antes de implementar

Campanha Punitivo terminada e testada; sistema de companheiros e dano com alvos/identidades auditáveis; feedback de percepção real versus alucinada; estado do gás e acessibilidade; cenas/VO/visual específicos; nova matriz de testes e migração de save. O custo atinge RunRules, HunterCore, CompanionCore, PerceptionCore, NarrativeCore, SaveCore, interface e Unreal. Por isso não é uma dificuldade adicional rápida. Origem/permissão de uso público desses personagens continua D-10.
