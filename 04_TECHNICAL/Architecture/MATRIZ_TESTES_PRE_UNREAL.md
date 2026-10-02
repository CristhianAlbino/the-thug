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
| T22 | Save durante perseguição/ferimento/companheiros separados | Estado/restauração atômicos, sem duplicar recursos nem pistas. | SaveCore parcial; atores Unreal pendentes. |
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
