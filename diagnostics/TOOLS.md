# Инструменты стенда

Официальные установщики Microsoft сохранены в G:\SS\lab\tools. Подписи обоих: Valid, Microsoft. Build Tools 2022 установлен с компонентами VCTools, VC.Tools.x86.x64, VC.CMake.Project, Windows11SDK.26100 в G:\SS\lab\tools\VS2022. SDK installer установил OptionId.WindowsDesktopDebuggers. Оба exit code 0, перезагрузка не потребовалась.

Версии: Build Tools 17.14.41, MSVC 14.44.35207, CMake 3.31.6-msvc6, SDK headers/libs 10.0.26100.0, debugger installer 10.1.26100.9169. Инвентарь: G:\SS\lab\evidence\visual-studio.json; SDK журнал: sdk-install.log.

Общие компоненты Microsoft и существующий Windows SDK находятся на C:. Сборка, данные, архивы и основные инструменты — на G:. Игры не удалялись.

Бутстраппер Microsoft использует обслуживаемый канал: поздняя установка может дать другую patch-версию. Побитовая воспроизводимость EXE не заявляется. Соответствие EXE/PDB проверено реальным чтением дампа; версии и SHA-256 сохранены в архиве сборки.

Источники: https://aka.ms/vs/17/release/vs_BuildTools.exe и https://go.microsoft.com/fwlink/?linkid=2376216 .
