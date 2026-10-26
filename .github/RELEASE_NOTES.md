Versão modificada do [Snes9x TX 1.1.3](https://github.com/saulfabregwiivc/Snes9xTX) (Tanooki) para Wii, com molduras em PNG, suporte a controles USB sem fio, Auto Crop, overclock automático, configurações por jogo e cheats digitados no menu. *English below.*

## Novidades desta versão

**Molduras em PNG** (Game Settings > Video > **Moldura**), para preencher o espaço vazio ao redor do jogo numa TV LCD:
- Aparecem nos modos 480i e 480p (Rendering **Default**). Em 240p (Original) a imagem fica como sempre, sem moldura.
- O emulador acha sozinho a área transparente no meio do PNG e encaixa a tela do jogo nela. A moldura é desenhada por cima, então cantos arredondados ficam certinhos. O Auto Crop continua funcionando dentro da moldura.
- Um PNG sem área transparente no meio vira fundo, com o jogo na frente no tamanho de sempre.
- PNG de qualquer tamanho, por exemplo os de 1920x1080 feitos para o RetroArch. A moldura cobre a imagem inteira da TV; com o Wii em 16:9, use molduras de 16:9.
- Opções: **Desligado**, **Padrão (TV)** (a moldura de TV que já vem no emulador), **Automática** (`<nome do jogo>.png`, senão `default.png`, senão a moldura de TV) e cada PNG da pasta `snes9xtx/bezels`, no SD ou no pendrive.
- Vem ligada na **Padrão (TV)**. A escolha também entra nas configurações por jogo.
- Com uma moldura que tem área transparente, o Zoom e a Posição da tela são ignorados: quem define o lugar do jogo é a moldura.

## O que já vinha nas versões anteriores

- **Overclock automático** (Emulation Hacks > CPU Overclocking > Automático): o overclock só entra quando o jogo fica lento e fica desligado nas transições, onde jogos como Top Gear e Primal Rage travavam em Max. Com Mostrar FPS ligado, aparece o nível em uso.
- **Configurações por jogo** (botão **Por Jogo**), no arquivo `<jogo>.cfg` junto dos saves.
- **Auto Crop** das bordas pretas (Video > Auto Crop: Fit ou Stretch) e **Auto Crop Scaling** (Suave ou Inteira).
- **Cheats digitados no menu** (Game Genie e Action Replay), com nome, renomear e apagar.
- O menu abre com **L + Select + Start** nos controles USB, **Home** no Wiimote e no Classic Controller e o **direcional C para a esquerda** no controle de GameCube.
- **Controles USB do tipo Xbox 360 (XInput)**, com fio ou com receptor 2.4 GHz plugado direto no USB do Wii (testados com o Retroflag Classic Controller-M wireless e com um controle genérico de SNES Classic Mini). Até 4 controles, vistos como controle de GameCube.
- **Região** aplicada em todo jogo, e a opção **NTSC (Lie to PAL)**.

## Instalação

1. Baixe o `Snes9xTX-Wii.zip` e extraia na raiz do cartão SD ou do pendrive. Ele cria `apps/snes9xtx` (o emulador) e a pasta `snes9xtx` com `roms`, `saves`, `cheats` e `bezels`.
2. Se você já tem o Snes9x TX instalado, basta trocar o `boot.dol` da pasta `apps/snes9xtx`. Suas configurações e saves continuam.

O `Snes9xTX-GameCube.zip` traz a versão de GameCube: tem as molduras, o Auto Crop, o overclock automático, as configurações por jogo, os cheats e as melhorias de região, mas não os controles USB.

## Observações

- Plugue o receptor **direto no USB do Wii**. Ligado num adaptador na entrada do Wiimote, ele não funciona e pode travar o Wii.
- Funciona junto com um HD externo plugado na outra porta USB.

---

## English

Snes9x TX 1.1.3 for Wii with PNG bezels, Xbox 360 (XInput) USB controllers, Auto Crop, automatic overclock, per-game settings and cheats typed in the menu.

New in this version:
- **PNG bezels** (Game Settings > Video > Bezel), in 480i and 480p (not in 240p). The transparent window in the middle of the PNG is found by itself and the game fills it, with the bezel drawn over its edges; a PNG without a window is a background behind the game. Any PNG size works (1920x1080 RetroArch bezels included); the bezel covers the whole picture of the TV. Choices: Off, Default (TV) (the TV frame built into the emulator, on by default), Auto (`<game>.png`, else `default.png`, else the TV frame) and each PNG in `snes9xtx/bezels` on the SD card or USB drive. Kept per game too. With a window, screen zoom and position are not used.

Already in the previous versions: automatic CPU overclock, per-game settings, Auto Crop of black borders, cheats typed in the menu, USB controllers read as GameCube controllers (up to 4, hot-plug), the region option and NTSC (Lie to PAL).

Install: extract `Snes9xTX-Wii.zip` to the root of the SD card or USB drive, or replace `apps/snes9xtx/boot.dol` in an existing install.

---

Snes9x TX is a fork of [Snes9x GX](https://github.com/dborth/snes9xgx), based on [Snes9x](https://github.com/snes9xgit/snes9x). Licensed under the GPL (see LICENSE, also in the .zip); the source code of this build is in the "Source code" files of this release.
