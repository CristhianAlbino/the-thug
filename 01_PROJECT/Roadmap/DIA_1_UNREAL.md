# Dia 1 — criar o projeto Unreal

**Objetivo:** abrir um projeto C++ de primeira pessoa chamado `TheThug`, jogá-lo no editor e versionar os arquivos de origem no repositório. O GDD está em [GDD_MASTER.md](../../02_GAME_DESIGN/GDD/GDD_MASTER.md).

## Antes de abrir a Unreal

1. Instale Git e Git LFS no computador de desenvolvimento. Confirme `git --version` e `git lfs version`.
2. Instale a Unreal Engine 5 e um compilador C++ compatível com a versão escolhida. No Windows, use o ambiente de desenvolvimento C++ recomendado pela Epic para a versão instalada.
3. Clone o repositório e entre na pasta: `git clone https://github.com/CristhianAlbino/the-thug.git` e `cd the-thug`.
4. Execute `git lfs install`. Confira `.gitattributes` antes de adicionar arquivos binários.

## Criar e incorporar

1. Na Unreal, escolha **Games → First Person → C++**, nome `TheThug`, plataforma Desktop. Comece sem conteúdo adicional se a opção estiver disponível. Crie o projeto em uma pasta temporária fora do repositório.
2. Abra o projeto no editor, use **Play** e confirme que o personagem anda, olha ao redor e que a compilação C++ termina sem erro. Salve e feche o editor.
3. Copie o **conteúdo** da pasta recém-criada `TheThug` para `the-thug/UnrealProject/`. O resultado esperado é `UnrealProject/TheThug.uproject`, `UnrealProject/Config/`, `UnrealProject/Content/` e `UnrealProject/Source/`. Preserve o `UnrealProject/README.md` já existente.
4. Abra `UnrealProject/TheThug.uproject` no novo local, aceite gerar arquivos do projeto quando solicitado, compile e teste **Play** novamente. Corrija referências ao caminho antigo, caso apareçam.

## Primeiro envio

1. Execute `git status --short` na raiz do repositório. Confirme que `Binaries/`, `DerivedDataCache/`, `Intermediate/` e `Saved/` não serão adicionados.
2. Para assets binários, confira `git lfs ls-files` após o `git add`; `.uasset` e `.umap` devem entrar como ponteiros LFS.
3. Faça `git add UnrealProject`, revise com `git status --short`, crie o commit `git commit -m "feat: create TheThug Unreal project"` e envie com `git push origin main` se estiver trabalhando na branch principal atualizada.

O GitHub só recebe alterações depois de **commit + push**. O editor salva localmente; ele não envia cada modificação automaticamente.

## Próxima entrega: M01

Construir um corredor pequeno com duas salas, uma porta interativa, uma lanterna e um esconderijo. Primeiro validar movimentação e interação; depois adicionar o Hunter. Critério inicial: iniciar Play, percorrer o trecho, abrir a porta, ligar a lanterna e entrar/sair do esconderijo sem erros. A fatia completa de 10 minutos está definida no GDD, seção 98.

> **Marco atual:** documentação pronta. O projeto `.uproject` ainda precisa ser criado no computador com a Unreal instalada.
