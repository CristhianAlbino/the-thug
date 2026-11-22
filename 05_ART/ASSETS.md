# Registro de assets de terceiros (decisão D-10)

Cada asset externo usado pelo jogo, com origem, licença e como reinstalar. **Nenhum deles está no repositório**: as licenças
não permitem redistribuir os arquivos num repositório público, e o volume (GB) não cabe no Git. O código funciona sem eles
(volta à arte procedural) e `UnrealProject/Scripts/setup_art.ps1` reinstala o que estiver disponível.

| Asset | Origem | Licença | Uso com IA (Fab) | Onde fica | Uso no jogo |
| --- | --- | --- | --- | --- | --- |
| UE5 Mannequin (Manny/Quinn, ABP_Unarmed, animações) | Unreal Engine 5.8, *Templates and Feature Packs* (Epic) | EULA da Unreal Engine: uso em produtos feitos com a Unreal | — (conteúdo do motor) | `Content/Characters/Mannequins` | Corpos e locomoção do Hunter, companheiros, Thomas e Robin (`TTShapes::SetupMannequin`) |
| Horror Hospital (740+ modelos, PBR) | Fab — Higher Fusion Studio, grátis, UE 5.5–5.8 | Licença padrão do Fab | Permitido | `Content/Horror_Hospital` | Materiais de piso/parede/teto, camas, móveis, cortinas, portas, objetos de parede e entulho (`TTKit`, `Scripts/build_kit.py`) |
| MetaHuman Creator (dados essenciais) | Unreal Engine 5.8, módulo do Launcher | EULA da Unreal Engine | — | Instalação da engine | Ainda não usado: personagens previstos após o upgrade de RAM |

Arte gerada pelo próprio projeto (material procedural `M_TTSurface`, `M_TTSmoke`, `M_TTEmissive`, sinalização, tubulação,
Bat-Sinal) é código/script deste repositório.

## Ainda pendente

- Personagens licenciados (Batman, Harley, Robin, Coringa, Gordon, Alfred) dependem da decisão de direitos D-10 antes de
  qualquer distribuição pública, independentemente dos assets acima.
- Pacotes pagos avaliados e não adquiridos: Leartes *Abandoned Hospital Modular Fatpack*, Dexsoft *Abandoned Hospital*,
  Meshingun *Apocalyptic Hospital* (todos com "uso com IA: não").
