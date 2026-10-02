# Checkpoints e vitória por esconderijo — contrato de hipótese v1

**Estado:** regras de primeiro ensaio, sem Unreal. Não antecipar em resumo ao criador que quer descobrir essas mecânicas jogando. Runtime v4 envolve StorySave, pausa, ocupação lógica, recibos e planos de apresentação; o [escritor de arquivo](CONTRATO_ARQUIVO_CHECKPOINT_V1.md) já oferece validação, backup e recuperação. Atores Unreal, ocupação física e retomada de animação exigem integração antes de um save jogável. Política de disparo e hipóteses de vitória continuam distintas da API de arquivo.

## Checkpoint como corte atômico

1. **Candidatos:** após N03 e após marcos irreversíveis de rota/ato. O candidato só é efetivado quando o mundo entrou em estado estável: nenhum item no meio de transferência, nenhuma interação/captura parcialmente aplicada, nenhuma conversa iniciando, conclusão da cena principal ou ponto explícito de retomada. Não salvar durante uma morte/captura já iniciada.
2. **Jogo continua:** perigo por si só não apaga checkpoint anterior nem produz um novo checkpoint no último segundo. A interface mostra “salvo” apenas após `SaveFile` retornar `Ok`. Falha antes da substituição conserva o principal; `CommittedUnconfirmed` exige revalidação, pois a troca já ocorreu. `RecoveredBackup` informa recuperação de progresso anterior, sem ocultar o rollback.
3. **Snapshot coerente:** mesmo `worldTimeSeconds` para RunRules, Hunter, companions, Perception, WorldCore e narrativa. Incluir `scene_seen`, BAN entregue, BAN interrompido com índice de linha e `lastBanterCompletedAt` para não reiniciar cooldown ao carregar, documentos/pistas observados, tentativas de teclado/estado do cofre, provas individuais de epílogo, ocupação do esconderijo, estado da busca, transformações e prazo absoluto do telhado. `scene_seen` é escrito após conclusão; retomada de cena interrompida deve ser idempotente.
4. **Carregar:** pausar, validar arquivo/versão/catálogo inteiro em memória, restaurar atores, portas, colisão, navmesh e visadas, então aplicar estados puros e reativar IA/relógio. Nunca misturar dois checkpoints nem retroceder somente o prazo do telhado. Se migração for necessária, salvar para outro slot; formato `THE_THUG_CAMPAIGN 1` não contém BAN/cenas e não pode ser tratado como campanha narrativa completa.
5. **Política de dificuldade para teste:** Fácil e Normal podem oferecer salvamento manual apenas em estado seguro e sem interação pendente; Difícil e Punitivo usam os mesmos marcos autosalvos inicialmente, com frequência revisável após medir custo de repetição. Evitar um checkpoint imediatamente antes de cada perigo; não apagar progresso longo só para alongar o jogo. A política de slots e perdas ainda é D-04.

## Abandono de busca — sequência observável

| Etapa | Evidência exigida | Por que não basta um cronômetro |
| --- | --- | --- |
| Entrar | Joe ocupa esconderijo válido `WorldCore::EnterHide`, acesso físico e cobertura verificados. | Entrar não informa o Hunter nem concede vitória. |
| Busca | Hunter investiga a última estimativa e pode inspecionar o local se viu pista ou esconderijo. Novos estímulos reabrem a busca. | `HideSpotSuspicion` é memória de evidência, não detector telepático de ocupação. |
| Desistir | Hunter muda de busca para patrulha por lógica própria, e o adaptador observa que ele **saiu fisicamente** da área por navmesh, sem linha de visão/ruído novo nem estimativa válida que ainda inclua Joe. | Estado `Patrol` isolado ou distância criada por teleporte/corte de cena não provam desistência. |
| Janela estável | Joe continua escondido; um novo estímulo revoga a candidatura. Hipótese para testar: 20/30/45/60 segundos de estabilidade **depois** da saída comprovada em Fácil/Normal/Difícil/Punitivo. | Esperar dentro de esconderijo ao lado do Hunter não pode vencer. Valores são alvos de playtest, não GDD aprovado. |
| Fechar | O coordenador emite `HunterAbandonedSearch` uma única vez se RunRules ainda está `Ending::None`; o final usa provas individuais dos companheiros. | Não salvar um final antes da validação nem converter ausência de notícia em morte. |

**Exceções:** se Hunter viu Joe entrar, a ocultação atual não é suficiente até ele perder rastro por meios físicos; suspeita persistente pode levá-lo a revistar. Companheiro que passa/é capturado perto do esconderijo produz estímulo conforme sensor real; não herda posição de Joe. Blackout, porta emperrada, câmera em repetição e loading de área não invalidam silenciosamente a prova de saída física nem mudam o tempo. Um save feito enquanto Joe já se esconde preserva estágio/candidatura ou obriga a revalidar a janela após load, sem conceder tempo grátis.

**Aceite em editor:** ensaiar Hunter no mesmo corredor, Hunter saindo por escada legal, som novo a um segundo da vitória, esconderijo já observado, companheiro separado, blackout e save/load no meio da janela. Medir falsos positivos e sensação de espera. O limiar de suspeita e os segundos finais permanecem D-08 até esses ensaios.
