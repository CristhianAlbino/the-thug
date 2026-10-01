# Contrato dos adaptadores Unreal — campanha v1

**Estado:** especificação para implementar quando existir `TheThug.uproject`; ainda não há classes Unreal compiladas. Os núcleos C++17 são lógica pura. O JSON do NarrativeCore é contrato de autoria validado por Python; sua avaliação em tempo de jogo requer uma implementação C++/dados equivalente e teste de paridade, ou um adaptador explícito. Não executar Python como dependência implícita do build do jogo.

## Unidade e evento comuns

Cada evento vindo do mundo carrega `eventId` único e estável para evitar aplicação dupla, `worldTimeSeconds` monotônico, `phaseId`, `actorId`, `nodeId`, `sourceId`, dificuldade e observadores com evidência válida. A engine mede em centímetros; snapshots do mundo puro guardam posição em **metros** e rotação em **graus**. Só uma ponte converte coordenadas, com teste ida e volta e tolerância definida no editor. Relógio de simulação e prazo do helicóptero são persistidos; tempo de menu/pausa não avança o jogo.

| Origem no editor | Validação antes de chamar lógica pura | Resultado aplicado de volta ao mundo |
| --- | --- | --- |
| Visão/ruído do Hunter | Linha de visão, cobertura, fonte física, distância/atenuação, momento e ID de evidência | Estímulo com posição estimada e incerteza; nav escolhe deslocamento legal, nunca posição atual de Joe sem observação. |
| Interação com porta/circuito/câmera | Joe no alcance, estado do ator, energia e item gasto; `eventId` ainda não aplicado | Transição WorldCore/RunRules, estado físico, colisão e sinal visível coerentes; falha mantém recurso e porta anteriores. |
| Companheiro | Ator vivo/presente, rota percorrida ou informação comunicada, objeto transferido realmente disponível | Traços, conhecimento e objetivo; movimento em navmesh e reação só após condição percebida. |
| Diretor N01–N34/J1–J4 | Nó e fase; `scene_seen`; NPC ativo **no mesmo local** quando requerido; prova vista por Joe, câmera válida e evento de rota | Seleciona variante e dispara uma vez, registra `scene_seen` após conclusão ou política de interrupção; elegibilidade isolada não move NPC nem grava prova. |
| Inventário/diário/HUD | Inspeção e aquisição física concluídas; idioma, fonte e estado da câmera realmente conhecido | Itens físicos têm custo; DOC/AUD inspecionados viram entradas consultáveis. HUD não antecipa segredo, rota ou posição do Hunter. |

## Estado narrativo para checkpoint

Persistir, com versão própria: dificuldade; `scene_seen` incluindo `N26:route`; `BANxx` entregues e conversa interrompida com índice de linha; documentos e áudios efetivamente inspecionados; pistas `code_a/b/c`, `key_known`, `safe_known`; tentativas/abertura física do teclado; `joe_identified_thomas_body`, `joe_verified_batman_record`, revelação J1–J4 testemunhada; provas de saída/captura/morte e último contato de cada companheiro; instância/etapa de cena interrompida. Separar **verdade do mundo**, **o que Joe presenciou** e **o que a coda mostra ao público**. A coda nunca aumenta conhecimento do personagem. Flags de encontro corrente (por exemplo `jack_act3_encounter`) expiram ao sair ou encerrar a cena e não substituem evidência histórica. O [contrato de checkpoint/esconderijo](CONTRATO_CHECKPOINT_ESCONDERIJO_V1.md) detalha a consistência temporal e a prova de abandono do Hunter.

`SaveCore` atual grava dez estados puros, **sem** essa estrutura narrativa, os widgets, o diretor e os atores Unreal. Antes de permitir carregar uma campanha jogável, criar envelope de save versionado que contenha o snapshot puro, o estado narrativo e o catálogo completo de atores, gravados num arquivo temporário e restaurados com simulação pausada. Se qualquer parte for inválida, preservar o checkpoint anterior e não aplicar metade do estado. Migração do formato `THE_THUG_CAMPAIGN 1` deve ser explícita ou começar novas campanhas; não interpretar flags ausentes como cenas nunca vistas de um save antigo.

## Critérios de paridade no primeiro blockout

1. `eligible`/`eligible_late` em Unreal reproduzem os vetores do `Tools/NarrativeCore/validate_variants.py`: ausência de NPC impede fala, N34 só Difícil/Punitivo com prova, J4 só Punitivo com pistas testemunhadas.
2. Salvar após DOC01, perder energia e carregar conserva o fragmento lido, o estado da câmera e a posição física da porta; repetir a interação não duplica item nem fala.
3. Save após confirmação do helicóptero conserva prazo absoluto; coda Thomas aparece só para o público se Joe não testemunhou a prova.
4. Quando o Hunter perde visão, a ponte entrega última estimativa, não coordenada atual; testes com oclusão/navmesh devem mostrar erro possível.

Logs de teste registram `eventId`, tempo, IDs, entrada validada, resultado e motivo de rejeição sem despejar pistas secretas no HUD. O [kit do primeiro dia](KIT_INTEGRACAO_UNREAL_V1.md) dá a ordem; a [matriz](MATRIZ_TESTES_PRE_UNREAL.md) fornece cenários para aceite.
