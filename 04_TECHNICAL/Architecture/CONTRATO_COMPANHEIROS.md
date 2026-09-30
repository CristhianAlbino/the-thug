# Contrato — companheiros sistêmicos

Baseado no [GDD Mestre](../../02_GAME_DESIGN/GDD/GDD_MASTER.md), seções 5–6 e 49–51. Código em [CompanionCore](../../Tools/CompanionCore/README.md).

## Entrada da Unreal

- Instanciar Harley, Bartholomeu e Jack com **perfis authored** de 0–100 para confiança, medo, estresse, lealdade, autopreservação e relacionamento. O núcleo não atribui biografias ou valores iniciais automaticamente.
- Registrar cada ação realmente ocorrida com ID persistente único: usar [`ShareAmmunition`](../../Tools/IntegrationCore/README.md) para a entrega após validar proximidade e interação; `JoeAbandonedCompanion` só depois de uma situação concreta de abandono. Um evento repetido não aplica efeitos outra vez.
- Ensinar rotas individualmente por `LearnRoute` após descoberta ou informação recebida. Conhecer uma rota não significa que outro personagem a conhece.
- Construir `Situation` com fatos percebidos/validados: proximidade do Hunter e de Joe, necessidade de ajuda, acesso físico, recursos observados e saída transitável. Não enviar posição global do jogador a um personagem que não o percebeu.
- Para acontecimentos adicionais, fornecer deltas assinados por dados de design e registrar sua origem. Nunca confundir uma fala, uma suspeita e uma observação confirmada.

## Saída para a Unreal

`AvailableOptions` explica opções e pesos; `Choose` seleciona um objetivo usando semente. A engine deve então verificar navegação, executar ação/animação/diálogo, e informar resultado de volta. Uma opção de traição **não** executa dano nem revela Joe automaticamente. Falhas de caminho exigem nova escolha.

| Objetivo | Pré-condição principal no núcleo | Trabalho que permanece na engine |
| --- | --- | --- |
| Esconder-se | sempre disponível | achar local alcançável e animar |
| Procurar recurso | recurso observado e personagem não ferido | buscar, disputar e transferir objeto |
| Ajudar Joe | Joe percebido, precisa de ajuda, alcançável, companheiro apto | aproximação, cura/interação e risco |
| Compartilhar rota | Joe percebido, companheiro conhece rota | fala e transferência de conhecimento |
| Fugir sozinho | saída conhecida e acessível; exceto Fácil | navegação, porta e consequência |
| Usar Joe como distração | Hard/Punitivo, Hunter e Joe percebidos, baixa confiança/lealdade, alta autopreservação | ação concreta, percepção do Hunter e consequência |

## Limites pendentes

Pesos e limiares são parâmetros iniciais de teste. Faltam perfis numéricos aprovados, objetivos autônomos mais ricos, medo/estresse ao longo do tempo, mentira, ferimentos graduais, recursos além de munição, diálogos, animações e serialização de traços, rotas conhecidas, eventos aplicados e estado do RNG. A integração deve manter a individualidade de Harley, Bartholomeu e Jack sem transformar nenhum deles em seguidor genérico.
