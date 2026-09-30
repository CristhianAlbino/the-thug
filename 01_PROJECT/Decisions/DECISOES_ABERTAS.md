# Decisões abertas — antes da Unreal

O GDD define a visão e as regras centrais. Esta lista impede que um valor de teste vire, por acidente, uma decisão oficial. **Não bloquear trabalho independente:** usar parâmetro/documentar hipótese quando possível.

| ID | Decisão a fechar | Estado em 30/09 | Impacto |
| --- | --- | --- | --- |
| D-01 | Versão exata da Unreal e toolchain C++ no novo computador | Aguardar instalação; código atual usa C++17 puro | Adaptadores, build e plugins |
| D-02 | Valores iniciais de confiança, medo, estresse, lealdade e relação de cada companheiro | Não definidos no GDD; nenhum perfil numérico oficial foi criado | Balanceamento e narrativa |
| D-03 | Planta conectada do hospital: metragem, portas, áreas acessíveis e bloqueios por ato | Apenas setores e funções por andar estão definidos | Blockout e navegação |
| D-04 | Frequência de save, checkpoints e consequências de carregar partida | Não especificadas | Persistência e tensão |
| D-05 | Duração e condições concretas do amanhecer por dificuldade | Metas aproximadas de 30/90/180 min; Punitivo sem vitória automática | Relógio e finais |
| D-06 | Quantidade de inventário, peso/capacidade e valores de ferimentos/stamina | Relações qualitativas no GDD; números pendentes | Balanceamento de sobrevivência |
| D-07 | Sequência exata de pistas da combinação da garagem, localização de ferramentas e rotas alternativas | Pré-requisitos narrativos definidos; posicionamento pendente | Progressão sem softlock |
| D-08 | Como confiança do Hunter em uma área cai até permitir vitória por esconderijo | Princípio definido; limiar e duração pendentes | Final por esconderijo |
| D-09 | UI de informação, controles, acessibilidade e idiomas iniciais | Pilares definidos; fluxos e texto final pendentes | UX e produção de interface |
| D-10 | Origem, licença e uso público de personagens, trilhas e assets de terceiros | Revisão necessária antes de distribuir o jogo | Escopo de publicação |

## Decisões já fixadas

- Nome do projeto: **The Thug**; arquivo Unreal planejado: `TheThug.uproject`.
- Primeira pessoa, PC, Unreal Engine 5, fundação C++ e composição por Blueprint.
- O Hunter não sabe a posição exata do jogador sem percepção, não enxerga através de paredes e pode errar.
- Quatro rotas convencionais: garagem, telhado, túneis e janela; uma falha de rota não encerra necessariamente a partida.
- A dificuldade Fácil tem companheiros confiáveis; Punitivo não termina automaticamente no amanhecer.

Ao fechar uma decisão, registrar data, justificativa, alteração no GDD quando necessário e testes/documentos afetados.
