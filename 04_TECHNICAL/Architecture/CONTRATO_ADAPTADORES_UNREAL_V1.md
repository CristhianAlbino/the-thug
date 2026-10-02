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

`StorySave` já fornece `THE_THUG_STORY_CAMPAIGN 1`, envelope C++17 com os dez estados puros e `NarrativeState`, validação conjunta e troca em memória apenas quando tudo passa. `director_runtime.py` liga elegibilidade das cenas e banter aos fatos observados. DirectorCore C++ já oferece paridade e confirmação de entradas; CampaignCore runtime v3 acrescenta recursos/corpos/tarefas individuais, e CheckpointFile implementa gravação validada em disco com backup/recuperação. Faltam classes Unreal, catálogo físico dos atores, widgets, animações e restauração da representação no editor. Se qualquer parte do adaptador falhar, preservar o checkpoint anterior e não aplicar metade do mundo físico. Migração do formato `THE_THUG_CAMPAIGN 1` deve ser explícita ou começar novas campanhas; não interpretar flags ausentes como cenas nunca vistas de um save antigo.

## Critérios de paridade no primeiro blockout

1. `eligible`/`eligible_late` em Unreal reproduzem os vetores do `Tools/NarrativeCore/validate_variants.py`: ausência de NPC impede fala, N34 só Difícil/Punitivo com prova, J4 só Punitivo com pistas testemunhadas.
2. Salvar após DOC01, perder energia e carregar conserva o fragmento lido, o estado da câmera e a posição física da porta; repetir a interação não duplica item nem fala.
3. Save após confirmação do helicóptero conserva prazo absoluto; coda Thomas aparece só para o público se Joe não testemunhou a prova.
4. Quando o Hunter perde visão, a ponte entrega última estimativa, não coordenada atual; testes com oclusão/navmesh devem mostrar erro possível.

Logs de teste registram `eventId`, tempo, IDs, entrada validada, resultado e motivo de rejeição sem despejar pistas secretas no HUD. O [kit do primeiro dia](KIT_INTEGRACAO_UNREAL_V1.md) dá a ordem; a [matriz](MATRIZ_TESTES_PRE_UNREAL.md) fornece cenários para aceite.

## Ajustes obrigatórios após a revisão de 01/10

1. Separar N28–N31 de suas codas `N28:exit`–`N31:exit`. O ID simples registra preparação, não saída; o sufixo requer confirmação do RunRules e fica salvo. Uma rota fisicamente válida não espera um número de ato; a progressão de Jack mantém marcos próprios.
2. Cena principal suspende a conversa opcional ativa. Guardar linha concluída e revalidar presença/perigo antes de retomar; cancelar sem marcar entregue se perder contexto. Aplicar o cooldown persistido de banter. Tratar `activeBanter` junto com `activeScene` como **suspenso**, nunca duas vozes principais concorrentes.
3. Restringir N34 ativo/concluído, DOC16 e AUD08 aos dois modos superiores; AUD07/J1–J4 ao Punitivo. Ato mínimo das pistas deve sobreviver ao load. Não guardar condições correntes de encontro como fatos históricos nem reconstruí-las só porque existem no save.
4. Uma porta aberta não fica intransponível só por perder energia; sincronizar fechamento físico explícito, estado e colisão. Objetos ausentes não oferecem interação. Reconciliar os estados carregados com o catálogo e as capacidades reais dos atores antes de liberar controles.
5. Gerar respiração por `MakeBreathingEvent` usando efeitos do SurvivalCore; conferir acústica e limiares no corredor de teste. O ensaio em código comprova a cadeia de decisão, sem substituir propagação real.
6. As mudanças de tuning tornam saves experimentais com tuning antigo incompatíveis com a configuração nova; o loader deve rejeitá-los claramente, não misturar regras. O envelope inferior StorySave continua na versão 1; a sessão completa usa runtime v3 com estrutura ampliada; novos IDs de etapa são dados do contrato atualizado.

Para recursos e objetivos de NPCs usar o [contrato de execução](CONTRATO_EXECUCAO_COMPANHEIROS_V1.md): contexto individual percebido, caminhos verificados, escolha/chegada/conclusão com serial e retomada sem callbacks antigos.
