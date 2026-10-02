# Contrato — percepção justa do Hunter

Fonte: [GDD Mestre](../../02_GAME_DESIGN/GDD/GDD_MASTER.md), seções 9–13. Código: [PerceptionCore](../../Tools/PerceptionCore/README.md) e [HunterCore](../../Tools/HunterCore/README.md).

## Emissão e audição

1. A ação de Joe, companheiro ou objeto emite `SoundEvent` no lugar e tempo reais. Essa posição permanece na camada que calcula propagação.
2. O adaptador Unreal calcula transmissão após geometria/oclusão e máscara de som local, em valores de 0 a 1. `PerceptionCore::Hear` só retorna um estímulo se o som for audível.
3. O estímulo enviado ao HunterCore é `Subject::Unknown`, com centro aproximado, raio de incerteza, tipo e força percebida. **Nunca** anexar localização exata ou identidade do ator apenas por causa de um som.
4. O HunterCore pode investigar, ignorar ou continuar perseguindo alguém já visto. A navegação procura na área estimada; chegar ao centro não equivale a descobrir Joe.

| Emissão | Relação qualitativa do GDD | Dados ainda a medir no editor |
| --- | --- | --- |
| Parado | sem evento de passos | respiração/roupa conforme estado físico |
| Passos lentos / caminhada | baixo | piso, distância, propagação |
| Corrida / porta | médio | material e oclusão |
| Objeto quebrado / queda | médio a alto | tamanho e reverberação |
| Tiro / explosão | extremo | alcance e setores do hospital |
| Distração | depende do objeto | duração e credibilidade |

Usar `MakeBreathingEvent(source, time, effects.breathingLoudnessMultiplier)` com o multiplicador do [SurvivalCore](CONTRATO_SOBREVIVENCIA.md). A função aceita 1–3 e normaliza para energia 1/3–1: respiração calma tem intensidade de origem 0,12; ferimento e fadiga podem elevá-la até 0,36. Multiplicar uma energia já igual a 1 e limitar novamente a 1 apagaria o efeito do ferimento. O chamador ainda decide a cadência. Na sessão, usar `CampaignCore::BreathingSound` para incluir silêncio voluntário, oxigênio e recuperação ofegante do [P07](CONTRATO_COMBATE_RESPIRACAO_V1.md); o helper isolado não conhece esses estados.

Após a revisão de 01/10, limiares provisórios de reação do Hunter são Fácil 0,30; Normal 0,14; Difícil 0,08; Punitivo 0,05. O mínimo audível do sensor continua 0,02. O ensaio integrado verifica que uma porta comum próxima, caminhada e respiração agravada podem produzir a reação prevista, e que oclusão total continua impedindo percepção. Esses valores corrigem sons que antes nunca alcançavam o limiar; não constituem balanceamento aprovado. Paredes/andares, frequência de emissão e som audível precisam ser medidos na Unreal.

## Evidência visual do ambiente

`EvidenceMemory::Inspect` só é chamada quando o Hunter vê/inspeciona fisicamente um objeto. A primeira inspeção fornece referência. Ao ver estado diferente, o Hunter recebe pista localizada no **objeto observado**, sem autoria conhecida. Portas, luzes, câmeras, objetos deslocados e corpos precisam de estados legíveis; o design pode escolher quais alterações são relevantes.

Não usar notificações globais de “porta mudou” como percepção. Um objeto pode mudar e voltar ao normal enquanto o Hunter está ausente. O [SaveCore](../../Tools/SaveCore/README.md) já serializa o último estado visto de cada objeto e o RNG dos sons; a engine ainda precisa restaurar os objetos reais e os sensores.

## Limites e telemetria

- Registrar no debug o evento original, cálculo de oclusão, força recebida, área estimada e decisão resultante; ocultar os dados reais da UI do jogador.
- Sementes reproduzem estimativas dentro da mesma implementação C++; replays entre plataformas devem guardar resultados ou estado do RNG.
- A visão de Joe ainda exige linha de visão, luz, postura, cobertura e exposição reais. O PerceptionCore atual implementa **som e evidência ambiental**, não detecção visual completa.
- Calibrar volume, tempos e incerteza com o hospital em blockout antes de marcar balanceamento como final.
