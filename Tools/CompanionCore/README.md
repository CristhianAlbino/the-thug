# The Thug — núcleo dos companheiros

Lógica C++17 independente da Unreal para Harley, Bartholomeu e Jack. Cada instância tem traços, munição, rotas conhecidas e histórico de eventos próprios. Thomas participa da abertura narrativa; o GDD não prevê para ele este mesmo ciclo sistêmico após o desaparecimento.

As **únicas consequências numéricas fechadas pelo GDD** estão implementadas literalmente:

| Ação observada | Consequência |
| --- | --- |
| Joe entrega munição | confiança +20, lealdade +10, munição +1 |
| Joe abandona o companheiro | confiança −40, medo +20, autopreservação +30 |

Outros acontecimentos usam `ApplyInfluence` com deltas fornecidos por dados de design; os perfis iniciais dos três personagens ainda precisam de autoria e aprovação. IDs de eventos impedem dupla aplicação após replay. Traços permanecem no intervalo 0–100.

O sistema oferece objetivos com pesos: esconder-se, procurar recursos, ajudar Joe, compartilhar rota, fugir sozinho e usá-lo como distração. Só oferece informações e fugas que o personagem conhece; opções dependem de uma situação observada que a engine validará. No Fácil, abandono e traição não são opções. Em dificuldades maiores, elas podem surgir sob condições específicas, sem se tornarem inevitáveis. **Pesos e limiares são hipóteses iniciais para balanceamento**, não decisões narrativas finais.

## Testar sem Unreal

Na raiz do repositório, com Bash e compilador C++17:

```sh
bash Tools/CompanionCore/run-tests.sh
```

Ou com CMake e compilador instalado:

```sh
cmake -S Tools/CompanionCore -B Build/CompanionCore
cmake --build Build/CompanionCore --config Release
ctest --test-dir Build/CompanionCore -C Release --output-on-failure
```

Os testes cobrem as duas consequências do GDD, IDs duplicados, conhecimento individual, restrições de dificuldade e situação, limites dos traços e decisões reproduzíveis por semente. O [SaveCore](../SaveCore/README.md) persiste os três companheiros junto ao inventário do Joe, inclusive os IDs aplicados e o estado aleatório. Consulte o [contrato de integração](../../04_TECHNICAL/Architecture/CONTRATO_COMPANHEIROS.md).
