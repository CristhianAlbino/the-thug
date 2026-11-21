# The Thug — pistas e dependências das quatro fugas

**Versão v1 para revisão.** O grafo e as ações de [`MapCore`](../../Tools/MapCore/README.md) definem alcançabilidade e flags; este texto fecha a *primeira hipótese autoral* do quebra-cabeça da garagem e a comunicação das outras fugas. Dígitos, legibilidade e posições finais exigem playtest. Nenhuma pista se torna chave física automaticamente ao ser lida.

## Garagem — código candidato `41-07-23`

| Passo | Área e ID | Texto/efeito | Dependência |
| --- | --- | --- | --- |
| Saber que há uma chave | `F1_SECURITY`, DOC04 | Chave do veículo guardada no cofre da Administração; concede `key_known`. | Pode ser lido antes dos fragmentos. |
| Localizar cofre e ordem | `F2_SECURITY`, DOC05 | Administração via `F2_ADMIN` → `F2_SAFE`; ordem **leitos → arquivo → manutenção**; concede `safe_known` após `key_known`. | Não dá pares numéricos. |
| Primeiro par | `F4_NURSE`, DOC01 | **41**, registro do posto de leitos; `code_a`. | Acessível desde a abertura por rota normal. |
| Segundo par | `F2_ARCHIVE`, DOC02 | **07**, capa do arquivo; `code_b`. | Acessível pelo anel/escada oeste. |
| Terceiro par | `B1_GENERATOR`, DOC03 | **23**, etiqueta de manutenção; `code_c`. | Acessível mesmo sem energia restaurada. |
| Abrir e retirar chave | `F2_SAFE` | `open_safe` requer as três flags e `safe_known`; `take_garage_key` depois. | Código de seis dígitos 410723 é interação de UI, não atalho que burla flags. |
| Abrir portão e sair | `B1_GENERATOR` → `G_CONTROL` → `X_GARAGE` | `power_restored` e `garage_key` permitem `release_gate`; só o evento `GarageExitReached` fecha a fuga. | Falha da garagem não fecha as outras rotas. |

**Justiça da pista:** DOC05 informa ordem em palavras; os três documentos repetem sua posição relativa. A UI deve permitir reler apenas fragmentos de fato inspecionados, sem preencher dígitos automaticamente no Difícil/Punitivo. Não esconder um par atrás do cofre. Quem conhece a combinação de uma partida anterior pode digitá-la no cofre fisicamente encontrado; as pistas são caminho de descoberta, não flags mágicas obrigatórias para os dedos de Joe. O teclado aceita a combinação correta mesmo sem DOC01–05 e não bloqueia permanentemente por erro. Esse comportamento está em [`puzzle_rules.py`](../../Tools/NarrativeCore/puzzle_rules.py), mas a transição de `MapCore` em `open_safe` ainda é uma **abstração de alcançabilidade**, não uma simulação do teclado. Na Unreal, só uma submissão aceita pode emitir `SafeOpened` e liberar a chave real; visitar `F2_SAFE` ou possuir a flag do grafo não basta.

**Complexidade justa:** a dificuldade vem de identificar a chave como objetivo, localizar o cofre, cruzar três registros em pisos distintos, deduzir a ordem, voltar por um hospital que mudou e carregar a chave até o portão com energia. Erro de entrada pode ter clique audível e custo de tempo, mas só gera estímulo para o Hunter se o sensor físico o ouvir; não há tranca irreversível nem pista falsa sem forma de checagem. `DOC01` pode ser visto antes de saber que pertence ao cofre; o diário conserva a fonte e o horário da leitura, sem transformar o dado em objetivo onisciente.

**Teste humano pendente:** observador sem instrução encontra DOC04/05, entende três áreas, descobre os pares e sabe voltar ao cofre sem mapa permanente no Difícil. Rodar um ensaio de leitura cego em papel e depois no blockout; os validadores atuais comprovam coerência/reachability, **não** que um jogador encontrou ou entendeu a pista. Se a amostra ficar presa, ajustar apresentação/atalho, não só adicionar uma seta de objetivo.

## Telhado — informação, energia e prazo

| Passo | Origem | Prova para Joe |
| --- | --- | --- |
| Rádio | `F1_RADIO`, DOC08 | Falta bateria e há interferência; `radio_found`. |
| Pilhas | `F2_SERVER` | `batteries_found`; porta com energia pode ser contornada por `F2_SUPPLY`. |
| Interferência | `F2_CAMERAS` | `interference_clear` após ação concreta, não imagem em loop. |
| Contato | `F1_RADIO` | `batteries_installed` → `radio_contact` → `helicopter_confirmed`; só a confirmação inicia aproximadamente 300 s. |
| Chegada | `R_PAD` → `X_ROOF` | Rotas por duas escadas; se o prazo termina, `roofFailed` persiste e Joe pode tentar outra fuga. |

Não tocar contagem regressiva antes da resposta do helicóptero. Em save/load, prazo absoluto não reinicia. Vento mascara ruído, não apaga percepção visual válida.

## Túneis — descobrir e interpretar

`B1_PUMP` e DOC06 sugerem derivação, mas só `B1_TUNNEL` concede `tunnel_found`. Em `T_LOOP` a corrente de ar, a válvula e o ruído da água distinguem saída de dois ramais mortos. Bartholomeu pode comunicar **entrada** se a viu, não saída que não percorreu. `TunnelIntercepted` requer Hunter/Robin em posição física e sinal observável; ainda deve sobrar rota de fuga ou sobrevivência. Teste com áudio desligado e legendas/indícios visuais para acessibilidade.

## Janela — visão antecipada e ferramenta

De `F3_WINDOW`, o anexo `A_BRIDGE` é visível antes de estar acessível. DOC10 em `F3_EXAMS` aponta a ferramenta em `F2_SUPPLY`. `window_found` + `window_tools` permitem `unblock_window`; cruzar é mão única e exige confirmação visual antes da interação. A porta `D_F3_EXAMS` inicialmente trancada não impede alcançar `F3_WINDOW` pelo `F3_HALL_W`. `WindowDiscovered` só falha a rota se houve observação/interceptação concreta, não pelo jogador olhar a janela.

## Invariantes para testar em cada rota

1. Descoberta de uma fuga não abre outra por acidente; falhas são independentes.
2. Objeto, circuito e porta mostram estado antes de Joe se comprometer com uma travessia irreversível.
3. Uma pista pode ser pulada e reencontrada, mas não entregue por companheiro que nunca a viu.
4. Nenhum documento opcional de Arkham substitui os fragmentos da garagem.
5. Bloqueios por energia e perseguição preservam caminho alternativo ou condição de esconderijo/amanhecer permitida. Verificar geometria e tempos no editor; o grafo só prova conexões abstratas.
