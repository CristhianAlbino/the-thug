# The Thug — matriz de testes da campanha

**Uso:** casos reproduzíveis para código puro agora e critérios de Play-in-Editor depois. Os testes existentes provam lógica, não perseguição em geometria real. Toda ocorrência importante deve registrar dificuldade, seed, nó, tempo, estímulo observado, estado de porta/circuito, companheiros presentes e resultado de save/load.

| ID | Situação | Resultado exigido | Hoje / editor |
| --- | --- | --- | --- |
| T01 | Joe passa silencioso por N04 | Hunter não ganha posição por cena. | Perception/Hunter; visada no editor. |
| T02 | Carrinho produz som com Hunter fora de alcance | Nenhum estímulo recebido. | Perception; acústica real pendente. |
| T03 | Hunter ouve mas não vê Joe | Investiga área com raio, pode errar. | Hunter/ScenarioLab; nav pendente. |
| T04 | Hunter vê Joe cruzar escada e perde visão | Guarda evidência/última estimativa, não coordenada atual. | Hunter; linha de visão pendente. |
| T05 | Companheiro ausente/capturado em N07/N15/N25 | Sem fala ou teleporte; rota ainda alcançável. | NarrativeCore; ator real pendente. |
| T06 | Bartholomeu recebe munição duas vezes pelo mesmo ID | Só uma transferência física e influência. | Integration/Companion; animação pendente. |
| T07 | Abandono de Bartholomeu no Difícil | Deltas GDD uma vez; traição só após outros requisitos. | Companion; percepção/navegação pendentes. |
| T08 | Câmera online, loop, interferida, sem energia | Quatro feedbacks; loop não vira conhecimento atual. | WorldCore; UI/visada pendentes. |
| T09 | Blackout com porta motorizada aberta e outra fechada | A aberta continua passagem; a fechada não abre pelo motor. Fechamento autorado é ação física explícita, sem parede invisível; desvio persiste. | World/Map/Scenario; colisão pendente. |
| T10 | Garagem sem um dos três fragmentos e sem código digitado corretamente | Cofre não se resolve sozinho; conhecimento prévio pode abrir pelo teclado, outras fugas seguem. | Map/Narrative; UI do código pendente. |
| T11 | Código 410723 com pistas vistas | Ordem correta e chave transferida uma vez. | Validação de dados; interação Unreal pendente. |
| T12 | Rádio achado sem pilhas/interferência resolvida | Sem helicóptero confirmado nem contagem. | RunRules/Map; áudio/UI pendentes. |
| T13 | Helicóptero confirmado, save, tempo 301 s | Telhado falha após reload sem reiniciar prazo; janela viável. | Save/Scenario; relógio editor pendente. |
| T14 | Entrada do túnel descoberta sem saída vista | Joe sabe entrada; companheiro não sabe saída automaticamente. | Map/Companion; geometria pendente. |
| T15 | Janela com `D_F3_EXAMS` trancada | Acesso alternativo por `F3_HALL_W`; travessia de mão única. | Map; colisão pendente. |
| T16 | Falham garagem, telhado e túnel | Janela ou sobrevivência permitida continua verificável. | Map/RunRules; risco real pendente. |
| T17 | Fuga Joe com Harley separada | Harley só “escapou” com prova; caso contrário `unknown`. | EndingRules; ator/epílogo pendentes. |
| T18 | Fácil/Normal investigam Thomas | Nunca N34, coda, DOC16 conclusivo ou fala “morto por Batman”. | NarrativeCore; apresentação pendente. |
| T19 | Difícil/Punitivo ignoram necrotério | Coda prova ao público; Joe não ganha conhecimento. | NarrativeCore; cutscene pendente. |
| T20 | Punitivo perde J1 mas vê J2+J3 | J4 pode ocorrer se Jack presente; fora Punitivo não. | NarrativeCore; figurino/áudio pendentes. |
| T21 | Punitivo amanhece | Sem vitória automática; fuga/abandono ainda valem. | RunRules; iluminação/tempo pendentes. |
| T22 | Save durante perseguição/ferimento/companheiros separados | Estado/restauração atômicos, sem duplicar recursos nem pistas. | CampaignCore v9/CheckpointFile; atores Unreal pendentes. |
| T23 | Hunter abandona busca de esconderijo | Vitória só após estado/rota/percepção compatíveis. | Contrato; nav e limiar D-08 pendentes. |
| T24 | Joins/paths após energia e rota falha | Sem softlock ou teleporte, com indicação de motivo. | Map/Scenario; blockout pendente. |
| T25 | Thomas/Bartholomeu/Harley antes e depois de N03 | Banter pré-ruptura só com Thomas presente; após N03 a ausência não dispara voz, e nenhum modo conclui destino por essa conversa. | Banter/Narrative; atores e VO pendentes. |
| T26 | Jack conta encontro anterior em Normal e Punitivo | Riso habitual existe em ambos e não concede J1–J3; pista macabra Punitivo exige cena própria testemunhada. | Banter/Narrative; atuação pendente. |
| T27 | Salvar com conversa interrompida e recarregar | Retoma na linha correta ou cancela sem marcar ID entregue; revalida presença/perigo, cena principal tem prioridade, cooldown persiste. | StorySave/DirectorRuntime; VO/atores pendentes. |
| T28 | Teclado da garagem: tentativa errada, código correto sabido de outra partida | Sem tranca permanente ou abertura por simples visita; RunRules recebe `SafeOpened` só após entrada aceita no cofre físico. | PuzzleRules; integração Map/World/RunRules pendente. |
| T29 | Hunter sai fisicamente de esconderijo, novo ruído no último segundo, save/load | Janela recomeça após novo estímulo; sem vitória por relógio isolado ou load que perca suspeita/posição. | Contrato; nav/visada/estado persistido pendentes. |
| T30 | Porta comum próxima, caminhada no Difícil e respiração ferida no Normal | Cadeia World/Survival → Perception → Hunter produz reação plausível; oclusão total e grande distância continuam filtrando. | ScenarioLab; calibração acústica pendente. |
| T31 | Restaurar N34 ativo, documento/áudio restrito, pista prematura ou fato de encontro persistido | Rejeição sem mutação quando dificuldade, ato ou duração do fato forem incompatíveis. | StorySave/DirectorRuntime; catálogo Unreal pendente. |
| T32 | Preparar fuga antes do ato nominal e visitar saída sem evento de vitória | Preparação permitida por acesso real; coda só após RunRules. Save rejeita coda de outra rota. | DirectorRuntime/StorySave; travessia Unreal pendente. |
| T33 | Objeto ausente, sangue fatal com vivo=true, morte sem resultado de campanha ou stamina sem exaustão coerente | Sem interação com objeto ausente; snapshot impossível é rejeitado sem alteração parcial; morte por sangramento exige `JoeDied` antes de salvar. | WorldCore/SurvivalCore/CampaignSave; ator físico pendente. |

| T34 | Dois NPCs disputam pickup/esconderijo; Joe tenta usar o alvo reservado | Um dono/reserva; falha conserva recursos, ocupação e RNG. | CampaignCompanions; disputa física no editor pendente. |
| T35 | Save durante viagem/interação, cancelar e receber callback antigo | Tarefa/fase/serial retomam; callback antigo não conclui nova tarefa. Load não inventa chegada ou conclusão. | CampaignCompanions; navegação/animação pendentes. |
| T36 | NPC sangra, colapsa, recebe medicina ou morre com Joe saudável | Saúde individual; resgate antes da morte; morte não encerra Joe nem remove corpo do esconderijo. | CampaignCompanions; dano/corpo físico pendentes. |
| T37 | Compartilhar rota ou fugir sozinho | Só informação comunicada chega a Joe; fuga de NPC não confirma fuga de Joe nem destino dos demais. | CampaignCompanions; comunicação/travessia pendentes. |
| T38 | Escolher distração, cancelar ou concluí-la com Hunter fora de audição | Intenção não gera captura; conclusão emite fonte real, a audição filtra alcance/oclusão. | CampaignCompanions/Perception; acústica física pendente. |
| T39 | Transferência/medicina/dispositivo com inventário do executor, capacidade insuficiente ou alvo ausente | Um custo/consequência; falha não muda inventários/saúde; repetição após load não duplica. | CampaignCompanions; interação física pendente. |

| T40 | Recarga interrompida, animação não concluída e munição reservada disputada | Nenhum custo antecipado; conclusão/retentativa consome uma vez; transferência não retira reserva. | CampaignCombat; animação/input Unreal pendentes. |
| T41 | Tiro errado, impacto Hunter, repetição e tentativa de travamento contínuo | Um disparo por ocorrência; som passa pela audição; impacto temporário não mata Hunter e não prolonga bloqueio durante imunidade. | CampaignCombat/Perception/Hunter; raycast/atuação pendentes. |
| T42 | Save de granada em voo, delta longo e dois fusíveis iguais | Pose/velocidade/fusível retomam; relógio para no primeiro limite e exige resolver cada consulta física. | CampaignCombat; física/colisão pendentes. |
| T43 | Fumaça entre sensor/alvo, em outro corredor e expirada | Consulta geométrica local; memória não é apagada; sensores reais comprovam perda de visada. | CampaignCombat; paredes/volumetria pendentes. |
| T44 | Explosão com amigo/Joe expostos, alvo obstruído e destrutível já quebrado | Ferimentos e objetos aplicados juntos; erro de prova preserva sessão; vítima individual não encerra Joe. | CampaignCombat/Companion; consulta física/oclusão pendentes. |
| T45 | Segurar/soltar ar, correr, ferimento e pausa | Reserva finita, silêncio temporário, fronteira ofegante audível; sem morte por oxigênio ou avanço durante pausa. | CampaignCombat/Survival; feedback/cadência pendentes. |
| T46 | Registro quase cheio com granada já preparada/lançada | Reserva de recibos permite concluir detonação; demais operações recebem LogFull; nenhum fusível fica impossível de confirmar. | CampaignCombat; orçamento da campanha real pendente. |

**Gate de revisão geral:** nenhuma afirmação de “campanha 3h” até medir playthrough Difícil completo; nenhuma cena N01–N34 pode depender de NPC morto/ausente ou câmera impossível. Cada novo bug ganha seed, passo de reprodução e regressão apropriada ao sistema; não duplicar testes que apenas reproduzem a mesma tabela.

## Extensões de campanha e runtime v9

Os casos abaixo acrescentam critérios aos ciclos anteriores. Provas físicas nos executáveis são fixtures; a coluna de editor continua pendente.

| ID | Situação | Resultado exigido | Hoje / editor |
| --- | --- | --- | --- |
| T47 | Final raro iniciado, aviso/conclusão física ausentes ou save no marco | Intenção/fala não concede resultado; recursos, consequências individuais e marco retomam sem repetir custo. | CampaignSpecial; destruição/atores/atuação pendentes. |
| T48 | Convidado ausente, encontro repetido ou prova antiga | Não teleportar, duplicar entrega nem inventar destino; ACK e serial validam o encontro. | CampaignSpecial; convidados físicos pendentes. |
| T49 | Pedido não ouvido, recusado ou feito durante silêncio | Sem obrigação automática; tarefa aceita tem viagem/prova e contexto; recusa/silêncio não gera informação. | CampaignInteractions; sinais/voz/nav pendentes. |
| T50 | Calço instalado, bolsa cheia na recuperação, Hunter rompe ou corrida de alteração | Ferramenta real conservada; falha sem consumo; ruptura exige chegada/aviso/tempo/confirmação. | CampaignInteractions; porta/colisão/animação pendentes. |
| T51 | Contenção temporária, transferência terminal e tentativa de soltura | Estados distintos; relógio não captura/mata; contato contínuo, tarefa/seriais e ferramenta conferidos. | CampaignRescue; amarras/contato/movimento pendentes. |
| T52 | Ferimento, medicina, captura de quem ajuda ou término durante soltura | Progresso interrompido, reserva liberada por cancelamento; destino só após prova. Joe não recebe notícia não testemunhada. | CampaignRescue; atuação/feedback pendentes. |
| T53 | Pilha genérica, coleta parcial, capacidade insuficiente e retentativa após load | Balanço de bolsas/pilha conserva quantidade; falha/repetição não duplica. | CampaignStrategy/ResourceCoherent; objeto físico pendente. |
| T54 | NPC pega item sem Joe ver; Joe consulta diário e depois revisita | Última observação pessoal permanece até inspeção; mundo privado não atualiza registro remotamente. | CampaignStrategy; UI/visada pendentes. |
| T55 | Relato verdadeiro antigo, mentira, fonte ausente e conferência atual | Distinguir relato/observação; não revelar intenção privada; informação velha não prova mentira só por movimento. | CampaignStrategy; voz/legibilidade pendentes. |
| T56 | NPC mente para Joe e Joe confere sem falar com ele | Contradição registrada para Joe; não alterar automaticamente confiança do NPC em Joe. | CampaignStrategy; resposta do jogador/atuação pendentes. |
| T57 | Perguntar ao contido: recusa, rota conhecida e engano permitido | Sem tortura/execução automática, sem posição privada de Joe, sem pista da rota concedida a Joe; Hunter pode investigar lugar errado. | CampaignStrategy/Hunter; cena/proximidade/voz pendentes. |
| T58 | Soltura começa e é cancelada durante pergunta; load ou callback antigo | Custódia/serial/saúde invalidam a pergunta antiga, sem restaurar progresso por cancelar soltura. | CampaignStrategy/Rescue; continuidade física pendente. |
| T59 | Uso conjunto recusado, sem recurso, interrompido por lesão ou cancelado | Sem custo antecipado; participantes/bolsa reservados; conclusão aplica mundo/regra/custo juntos e só uma vez. | CampaignStrategy; obstáculo/pose/contato pendentes. |
| T60 | Dois NPCs recarregam/disparam; transferir rifle carregado; interrupção | Estado/estoque por executor, seriais compartilhados válidos; sem duplicar/perder munição; rifle carregado não muda de dono. | CampaignExtensions/Combat; raycast/animação pendentes. |
| T61 | Tiro NPC sem alvo percebido e sem intenção de distração | Não disparar por informação privada; som/impacto precisam de fontes reais e provas. | CampaignExtensions; escolha física/acústica pendentes. |
| T62 | Hunter percebe duas nuvens diferentes ou a mesma nuvem repetidamente | Aprendizado requer duas ocorrências observadas e recentes; vigia usa somente bordas vistas/alcançáveis. | CampaignExtensions/Adaptive; fumaça/visada/nav pendentes. |
| T63 | Armadilha de energia/porta/vigia, alternativa ausente e alteração concorrente | Cada etapa exige deslocamento/reação/prova e revalida estado; não fechar passagem sem desvio nem capturar por chegada. | CampaignExtensions; cenário/nav/aviso pendentes. |
| T64 | Retirada após pressão percebida, novo contato visual ou dano no plano | Só abrigo observado; contato/dano interrompem, cancelamento libera plano; não cura HP nem conhece Joe oculto. | CampaignExtensions; cover físico/atuação pendentes. |
| T65 | Registro quase cheio com plano Q já iniciado; arquivo corrompido/incompatível | Reservas permitem concluir/cancelar; outras famílias recebem LogFull; load inválido não muda sessão. | CampaignExtensions/Strategy/Checkpoint; restauração de atores pendente. |

[Protocolo de teste humano](../../01_PROJECT/Reviews/PROTOCOLO_TESTE_HUMANO_PRE_UNREAL_V1.md): a sessão ainda precisa ser realizada. [Catálogo de produção](../../Tools/ProductionCore/README.md): o índice é verificado por origem/ID/hash, sem afirmar gravações ou assets prontos.
