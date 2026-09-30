# The Thug — bíblia visual do hospital (v0.1)

**Material observado:** cinco vídeos enviados pelo criador em 30/09/2026 (`78713.mp4`, `78717.mp4`, `78719.mp4`, `78721.mp4`, `78723.mp4`). Análise visual por amostras de quadros ao longo de cada vídeo. O áudio, a autoria dos jogos e a origem dos assets não foram verificados. As descrições abaixo se referem ao que aparece nas imagens; os planos para The Thug são propostas próprias. Base espacial: [mapa lógico](../World/MAPA_LOGICO_HOSPITAL.md).

## O que os vídeos realmente mostram

| Arquivo / trecho aproximado | Observação visual | Aplicação autorada |
| --- | --- | --- |
| `78713.mp4`, 2–8 s | Clínica em primeira pessoa; divisórias, maca/cadeira e corredor com vermelho intenso ao fundo; figura percebida só depois de avançar | 4º e 3º: abrir visão em camadas, com uma fonte vermelha local que marque direção e também esconda movimento |
| `78717.mp4`, 1–10 s | Personagem à frente sob feixe pequeno, ambiente cinza quase sem cor; corte para vista baixa e vermelho abrupto | Encontros próximos: perigo legível por silhueta e distância, sem depender de brilho total nem de susto a cada sala |
| `78719.mp4`, 1–18 s | Molduras/grades vermelhas muito próximas da câmera, corredor profundo com luz no fim, malha metálica na frente | B1/túneis: usar grades, válvulas e tubos como planos sucessivos; preservar marcos que permitam orientar mesmo na escuridão |
| `78721.mp4`, 1–15 s | Corredor hospitalar quase preto, uma tela acesa num cômodo e aproximação de figura mascarada no escuro | Administração/câmeras: fontes de luz pequenas chamam o jogador; refletir ameaça em vidro/tela somente quando a geometria sustentar a visão |
| `78723.mp4`, 1–19 s | Galeria industrial com tubos paralelos, guarda corpo amarelo, piso molhado esverdeado e iluminação vermelha distante; movimento irrompe junto às janelas | Prumada de serviço: misturar percurso industrial amplo com ameaça súbita atrás de fechamento; vidro, ruído e detritos exigem evento causal, não aparição arbitrária |

Os vídeos variam entre primeira pessoa, equipe e montagem curta. **The Thug continua um jogo solo em primeira pessoa**, conforme o GDD. Não transplantar interface de co-op, armas ou cenas específicas. A sensação procurada é de estar dentro do hospital com pouca informação confiável, ouvindo um predador que também precisa perceber evidências.

## Gramática visual do hospital

1. **Escuro com estrutura:** o contorno do percurso e de interações obrigatórias precisa ser perceptível. Obscurecer o que vem depois da próxima porta, não a própria porta. Confirmar no monitor comum e em configurações de brilho distintas.
2. **Luz com causa:** vermelho de emergência em trechos específicos, frio clínico residual, verde de manutenção, âmbar de lâmpada portátil, claridade azul da chuva. Não banhar todo o hospital no mesmo vermelho. Luz que oscila tem uma fonte e um ritmo consistente.
3. **Profundidade em camadas:** um objeto próximo encobre parte do corredor; o meio oferece cobertura; o fundo promete um destino ou movimento duvidoso. Repetir com vidro, cortinas, vapor e tubulações sem criar ruído visual constante.
4. **Áreas com memória:** inventário do que mudou depois da primeira visita: cadeira deslocada, tela desligada, trilha úmida recente, válvula aberta. Uma alteração só alimenta a IA quando realmente observada pelos sensores definidos.
5. **Grandeza conectada:** vistas para outro andar e silhueta do anexo prometem lugares alcançáveis. A mesma prumada reconhecível reaparece acima e abaixo; a sinalização física confirma o andar.
6. **Ameaça plausível:** a figura pode ser vista de longe, recortada por uma porta, cruzar uma luz ou desaparecer em um ângulo morto. Som e visão não autorizam teleporte, visão através de parede nem ataque sem aproximação.

### Identidade por área

| Andar / IDs principais | Materiais e luz provisórios | Leitura e som de navegação |
| --- | --- | --- |
| 4º `F4_*` | Linóleo gasto, lençóis, vidro fosco; frio baixo e poucos pontos vermelhos após a ruptura | Batida de porta e objetos movidos diferenciam retorno; posto de enfermagem mantém pista legível |
| 3º `F3_*` | Aço cirúrgico, cortina, azulejo e janelas; branco frio intermitente contra sombras profundas | Cortinas dão cobertura mas se mexem; janela do anexo aparece antes de ser utilizável |
| 2º `F2_*` | Papel úmido, concreto pintado, rack e brilho pontual de monitores | Ventoinha, sinal de câmera e interrupção de energia dão informação; sala de câmeras continua perigosa |
| 1º `F1_*` | Vidro, porta dupla, poças, marcas de triagem; reflexos azulados da entrada | Átrio revela distância e exposição; farmácia é legível, mas alcançá-la envolve risco |
| B1 `B1_*` | Tubos de diâmetros distintos, vapor, válvulas, grelhas e concreto saturado; verde frio e lâmpadas âmbar | Gerador tem pulso reconhecível; bomba e água orientam até a entrada dos túneis |
| Garagem `G_*` | Pilares, veículos, óleo e luz exterior entre grelhas | Portão visível de vários ângulos; farol/reflexo pode denunciar movimento sem criar visão da IA por si |
| Túneis `T_*` | Ramais pluviais, ferrugem, linhas de manutenção e água rasa | Dois becos sem saída têm sinais distintos; anel pode ser aprendido por válvulas e corrente de ar |
| Telhado `R_*` | Dutos, máquinas de climatização, água da chuva e cidade sob nuvens | Vento mascara parte dos passos; pouso improvisado e dois acessos verticais são identificáveis |
| Anexo `A_*` | Passarela técnica e fachadas próximas | Horizonte visível da sala da janela, travessia exposta e claramente de mão única |

### Tubulações como sistema espacial

| Família | Aparência | Função de jogo potencial |
| --- | --- | --- |
| Água/combate a incêndio | Tubo mais espesso com sinalização redundante por cor, texto e conexões | Marco de orientação entre pisos; ruído de fluxo ajuda a localizar B1 |
| Vapor/calefação | Juntas, isolamento rompido, condensação e vapor localizado | Máscara sonora temporária e visibilidade menor; área de risco se for interativa |
| Eletrodutos e bandejas | Linha alta repetida e caixas em trechos de serviço | Direcionar o olhar até geradores, servidores e salas técnicas |
| Duto estreito transitável | Geometria consistente de acesso e saída | Atalho lento e claustrofóbico; `hunter_access: false` na aresta não significa imunidade no destino |

Nomes, diâmetros e cores serão definidos no kit modular. Os conceitos acima são pistas de direção, não simulação técnica de instalações hospitalares.

## Momentos espaciais propostos

| Momento | Onde | Como executar sem perder a agência |
| --- | --- | --- |
| Primeira visão da altura | `F4_OBSERVATION` → `R_LEDGE` | Mostrar que o telhado existe; manter silhueta ambígua e sem revelar caminho final |
| Corredor vermelho | `F4_HALL_E` e retorno posterior | Um setor iluminado por emergência; mudar um objeto visível depois do evento inicial, se a campanha justificar |
| Clínica de sombras | `F3_SURGERY` / `F3_RECOVERY` | Cortinas e vidro permitem visão parcial e esconderijo condicionado a ruído/entrada observada |
| Monitor tentador | `F2_CAMERAS` | Algumas câmeras úteis, outras quebradas ou atrasadas; ação de ver requer aproximação e tempo |
| Exposição por remédio | `F1_LOBBY` → `F1_PHARMACY` | Jogador identifica farmácia, mas escolhe rota e momento; esconder-se não apaga evidência vista |
| Entrar na prumada | `B1_PIPE` → `F1_SERVICE` | Vapor e metal amplificam a respiração; Hunter pode chegar à outra boca por rota própria |
| Quebra que tem custo | Galeria entre B1 e garagem | Romper vidro ou grade só se a ação/ameaça produzir som e detritos persistentes; sem susto roteirizado imune a sistemas |
| Cidade distante | `R_PAD` | Iluminação aberta após horas comprimidas; vento, chuva e silhueta da cidade mantêm o risco |

## Kit inicial de assets para o blockout

**Prioridade A — orientação e jogabilidade:** módulos de corredor, esquinas, duas escadas distinguíveis, portas e batentes, elevador, guarda corpo, volumes de cobertura, divisórias, dutos transitáveis, grades, válvulas e placas de andar/saída. Todos em formas simples primeiro.

**Prioridade B — atmosfera com função:** camas, macas, cortinas, arquivo, servidores, monitores, geradores, bombas, veículos, portão, poças e emissores de vapor. Cada peça tem versão de colisão/nav e estado persistente só quando houver interação real.

**Prioridade C — acabamento:** sujeira, ferrugem, reparos, chuva, vegetação pontual, cartazes e objetos menores. A densidade visual não deve esconder pistas, saídas, itens ou a silhueta do Hunter.

Origem e permissão de uso de cada asset ficam em registro próprio antes da distribuição (D-10). Referências não são modelos a copiar. **O que provar na Unreal:** legibilidade real por monitor/brilho, oclusão e alcance de luz, colisão dos dutos, navegação alternativa do Hunter, performance de vidro/vapor/reflexos, som espacial e duração em corrida/perseguição. Nenhum quadro amostrado mede esses resultados.
