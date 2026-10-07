# Продолжение HD-текстур

Репозиторий `G:\SS\Silent-Storm-Reconstruction`, ветка `develop`. Актуально на
8 октября 2026 года. Это инструкция для продолжения работы, а не только плана.

## Текущее состояние

Прочитай применимые AGENTS.md, корневой HD-TEXTURES.md, assets/terrain-hd/README.md,
sources.json, expanded/queue.json, coverage.json и full-texture-inventory.json.
Последний содержит 5 801 запись БД. Исторические ресурсы
`G:/SS/Silent-Storm/Complete/Textures` и `G:/SS/lab/baseline/res` только читай.
Оригинальную установку Steam не изменяй. На другой машине найди фактические источники.

Принята 2 151 HD-текстура: 178 ландшафта, 30 деревьев, 1 754 окружения,
48 одежды, 37 экипировки, 59 оружия, 2 лиц, 20 UI, 17 эффектов,
4 лагерных атласа и 2 учебные мишени.
Десять RGB565 5076, 5168, 5169, 6997, 7581–7586 уже завершены; НЕ генерируй их снова.
`expanded/rgb565-batch-check.json` подтверждает повторное совпадение RGBA с релизом,
Windows Game-сборку и полную native-валидацию 1 962 текстур/3 924 алиасов/110 архивов.
Все прежние 1 952 PNG и native payload сохранили хеши. Мировой пакет содержит 2 349 записей.
Это не вся игра. Встроенный imagegen использован отдельно для каждого изображения.
Калибровка числовая. У часов сохранена исходная ориентация печати и явная регистрация
островов UV; не переносить её автоматически на другие атласы.
Приняты 189 текстур новых групп; всего 2 556 записей общей очереди, pending нет.
`new-groups-first-batch-check.json` подтверждает первые 20, а
`new-groups-second-batch-check.json` — следующие 29 и тогдашний пакет:
2 011 текстур, 4 022 алиаса, 111 архивов, 6 706 321 654 байта.
Все прежние 1 982 PNG/native payload и источники сохранили хеши.
Игровое меню проверено с пакетом второго этапа в отдельном запуске:
`new-groups-second-live-check.json`; свой процесс штатно закрыт.
Третья порция (jobs-weapons-second и jobs-ui-second) уже принята: 1752–1759,
1801–1804, 6090, 6093–6099. НЕ генерируй повторно.
`new-groups-third-batch-check.json`: 2 031 текстура, 4 062 алиаса, 111 архивов,
6 710 342 282 байта; прежние 2 011 source/PNG/native хешей сохранены.
`new-groups-third-live-check.json`: актуальное реальное меню HD on/off/on,
свой процесс закрыт штатно. Новые independent jobs одежды/экипировки/оружия
могут уже содержать raw; сверяй reviews и импортируй готовые части, не генерируй
их снова. 2151/2235 пока требуют художественного исправления, не являются отказом.

Четвёртая порция уже принята: jobs-clothing-third (9),
jobs-clothing-third-retries (2350), jobs-equipment-third (12),
jobs-weapons-third (1805–1816). НЕ запускай их снова. Flattened accepted jobs:
`jobs-new-groups-fourth.json`, native/check: `new-groups-fourth-batch-check.json`.
После четвёртой порции: 2 065 текстур/4 130 алиасов/112 архивов/6 750 015 264 байта;
прежние 2 031 source/PNG/native хешей сохранены.
`new-groups-fourth-live-check.json`: реальное меню HD on/off/on, штатный quit.
2151/2235 не импортированы; raw/prompts всех попыток и причины сохранены
в review-clothing-third.json. Остальные jobs-clothing/equipment/weapons-fourth,
UI-third и heads-second могут уже содержать готовые raw; проверь reviews,
актуальные sources/queue и импортируй только готовые подмножества.

Пятая порция уже принята: jobs-new-groups-fifth.json (32), из
jobs-clothing-fourth (9), jobs-equipment-fourth-first-ready (6),
jobs-weapons-fourth (12) и jobs-ui-third (5). НЕ генерируй их заново.
После пятой порции: 2 097 текстур/4 194 алиаса/112 архивов/6 779 639 268 байт;
new-groups-fifth-batch-check.json подтверждает неизменность предыдущих 2 065.
new-groups-fifth-live-check.json: реальное меню HD on/off/on, штатный quit.
2443/2542/2546 pending из-за belt marks; UI6100/6101 из-за внутреннего рисунка.
heads-second (1898/1899/2169/2170/2172–2175) сохранён pending зубного рисунка/стиля;
jobs-heads-second.json пустой. Не объявлять эти raw принятыми.
Снаряжение2985/2986/2991/2995/2997/3006 проходит новые direct-original retries:
проверяй review и stable jobs snapshot, часть попыток уже отложена из-за UV drift.
Clothing-fifth и weapons-fifth в работе независимых агентов; сначала ищи raw/jobs.

Шестая порция уже принята: jobs-equipment-fourth-retries (2991/2995/2997/3006)
и jobs-effects-third (700/701/702/704). НЕ генерировать их снова.
Flattened jobs-new-groups-sixth.json:8. Актуально 2 105 текстур/4 210 алиасов/
112 архивов/6 781 387 332 байта. new-groups-sixth-batch-check.json: все прежние
2 097 source/PNG/native хеши сохранены; new-groups-sixth-live-check.json:
реальное меню HD on/off/on, свой процесс штатно закрыт.
2985/2986 отложены после v2 из-за UV/detail drift.597 raw сохранён pending:
сдвинут большой фрагмент и пропал tiny source fragment. Его не импортировать.
707/708/709 уже приняты в седьмой порции; не генерировать снова.
Clothing-fifth и следующая одежда/оружие/эффекты в работе: сверяй новые
reviews/jobs прежде imagegen, не повторяй существующие raw/ready jobs.

Седьмая порция принята: jobs-new-groups-seventh.json (20), из weapons-fifth (12),
UI-fourth (5) и effects-third-fragments (707/708/709). НЕ генерировать повторно.
Пакет: 2 125 текстур/4 250 алиасов/114 архивов/6 890 090 830 байт;
new-groups-seventh-batch-check.json подтверждает прежние 2 105 хешей,
new-groups-seventh-live-check.json — реальное меню HD on/off/on и штатный quit.
7199 generation-unavailable после реального отказа, request ID сохранён.
7206 held без вызова; не обходить.7207/4310/4311/5267/5268 pending artwork/UV.
Clothing-fifth coordinator-ready7 и clothing-sixth10 уже приняты в восьмой порции.
3860/3868 initial/v2 удержаны: hardware/buckle и новый ornamentgrain; не импортировать.
1980 уже зарегистрирован как реальный generation-unavailable fallback.
Следующие equipment-fifth/effects-fourth/fifth/UI-fifth/weapons-seventh могут
иметь готовые raw: сверяй reviews/jobs прежде новых вызовов.

Восьмая порция принята: jobs-new-groups-eighth (26): clothing-fifth-coordinator-ready7,
clothing-sixth10, weapons-sixth8 и exactduplicate3002←2998. НЕ генерировать повторно.
Полный пакет2 151/4 302алиаса/115архивов/6 930 287 674байта;
new-groups-eighth-batch-check.json: прежние2 125хеши неизменны, all26nativeRGBA
равны проверенным calibrated PNG. new-groups-eighth-live-check.json:
реальное меню HD on/off/on, свойпроцессзакрыт штатно. Sourceuse3002=donor2998
byte-exact безnewcall, nativepayloadhash равен.1980 отказ записан вqueue.
3399/3860/3861/3868/3869/4555/3873 одежды,4789/5323/5326 оружия pendingart.

Прежде чем продолжать, проверь актуальные статусы, сырые результаты
и jobs, чтобы не повторить уже выполненный вызов imagegen.

Все PNG, включая исходные, сырые, нормализованные, откалиброванные, вспомогательные
и отклонённые, хранятся обычными файлами Git. ZIP, LFS и загрузок из Releases нет.
Готовые .res, manifest, validation и build-state создаются вместе с Game через
S2HDTextures в игнорируемом res-hd рядом с Game и не коммитятся.

63 отсутствующих исходника (48 прежних и 15 новых), несовпадение 5864 и недоступные генерации
5930, 5932, 6732, 7110, 7112, 7113, 7117, 7118, 7199, 1980 остаются fallback. Не обходи отказы.
Замена содержания обложек разрешена только для уже завершённых книжных текстур
3795, 3796, 3797, 3953, 6206, 6207; их повторно не генерируй.

## Следующие группы

Все jobs-clothing-first/second/2007, jobs-equipment-first/rgb565,
jobs-next-groups-b, jobs-effects-second и jobs-camp-tutorial-first уже приняты;
НЕ запускай их заново. Лагерные 6616/6617/6618/6654 и учебные 5957/5958 завершены.
6265 — оригинальный структурный RGB-градиент с альфой 0–251, не генерировать.
Семь RGB565 экипировки 6197/6199/6200/6247–6250 тоже завершены.

Точный список оставшихся проверенных ID находится в
`new-groups-eighth-batch-check.json:remaining_verified_source_queue_ids`.
Всего 1 611: одежда 239, экипировка 222, оружие 67, головы 184, UI 677,
эффекты 189, clues 17, final 3, miscellaneous 13. Снимки source-queues сохраняют
исходные pending-статусы; текущую готовность всегда проверяй по sources/shared queue.
Эта цифра означает ещё не принятый HD; часть raw уже может быть подготовлена
сабагентами. Проверяй ready jobs/reviews до новых вызовов imagegen. Продолжай отдельными небольшими
партиями с независимыми сабагентами и одним координатором метаданных.

Проверь наличие `expanded/groups/README.md`, audit-summary.json и отдельных
source-queues. Эти снимки подготовки источников отделены от общей очереди и
принятия HD; сверяй каждый ID с актуальными sources.json и queue.json.
Не объявляй pending-снимок готовым HD и не импортируй старые целые пакеты.

Переносимая подготовка находится в prepare_sources.py. Native MMP format 8
(CF_R5G6B5) — обычный цвет; Format=565 нельзя автоматически исключать как normal.
Вход — JSON-list {texture,category,role,optional layout/usage}. Она сверяет
исторические и релизные RGBA/размеры, сохраняет оригинал, пропускает существующие
ID и при --append добавляет новые в общую очередь. Пример:

```powershell
python assets/terrain-hd/prepare_sources.py --candidates candidates.json --historical G:/SS/Silent-Storm/Complete/Textures --release G:/SS/lab/baseline/res --output prepared.json --append
```

Для общего аудита используй audit_texture_groups.py и prepare_group_sources.py,
если они уже опубликованы; команды и ограничения находятся в groups/README.md.
Проверяй реальные typed DB-связи: Models/Heads.MaterialN ведут через
MaterialTemplates -> Materials.TemplateID; Gloss/Bump/Mirror — технические карты.
UIControls.TextureN ведут через UITextures к Texture. Смешанные цвет/control ID
нуждаются в отдельном решении. Отсутствие прямой ссылки не доказывает неиспользование.

Продолжай одежду/персонажей, экипировку, оружие, лица/головы, UI и эффекты отдельными
пакетами. Сохраняй альфу, blend mode, UV, раскладку, число и масштаб деталей,
повторы, отверстия, повреждения, материалы и идентичность персонажей.
Курсоры требуют проверки hardware path; UI с размерами не в степени двух требует
проверки native builder. Векторные шрифты не апскейль. Текст и цифры сохраняй точно.
Для эффектов проверь определения частиц, кадровые bindings, WrapX/Y и реальную
раскладку; число анимационных ссылок не равно размеру atlas grid.

## Цикл генерации и публикации

1. Проверь Git-статус/ветку, свободное место, источники и Python с Pillow/NumPy.
   Не затирай чужие изменения и не повторяй прежнюю публикацию PNG.
2. Прочитай skill imagegen. Осмотри каждый оригинал до отдельного встроенного
   image_gen edit. Сохрани сырой PNG и точный промпт сразу. Не подменяй генерацию
   интерполяцией. Не придумывай содержимое отсутствующих источников.
3. Подготовь отдельный jobs JSON {id,generated,prompt}. Общую очередь, sources,
   импорт, калибровку, build и validation меняет один координатор последовательно.
   Сабагенты разрешены для независимых изображений, review и своих jobs.
4. Импортируй только новые jobs, приватно проверь нормализованный результат,
   затем численно откалибруй только новые ID. Не перегенерируй ради яркости:

```powershell
python assets/terrain-hd/import_generated.py --batch jobs.json
python assets/terrain-hd/match_brightness.py --ids <новые ID>
cmake --build <build> --config <configuration> --target Game
python assets/terrain-hd/validate_pack.py --pack <Game-directory>/res-hd
python assets/terrain-hd/update_coverage.py --pack <Game-directory>/res-hd
```

   Сохраняй существующую конфигурацию CMake. Локальная Windows-сборка:
   `G:/SS/lab/build-x64-stage4`, RelWithDebInfo; Python3_EXECUTABLE в CMakeCache
   указывает на Python с Pillow/NumPy. Lab-скрипты не являются частью клона.
5. Валидируй весь pack: aliases, размеры, полные mips, native alpha, brightness и
   хеши. Только потом обновляй реальные числа документации/coverage.
   --replace используй только на конкретной проверенной повторной попытке.
  register bbox/uv_regions — только для явно осмотренных исходных UV;
   source_rgb_regions — только для явно осмотренной технической области:
   у 1896/2171 это тёмная gum-полоса (0,16,64,34), без художественного лица/зубов.
   не обрезай автоматически все изображения. Точные дубли переиспользуй только
   после совпадения RGBA, размеров, alpha type и layout (reuse_generated.py).
6. Коммить PNG/промпты/метаданные/переносимые инструменты обычными файлами и пушь
   завершённые проверенные пакеты порциями примерно до 500 МиБ, файл ниже 100 МБ.
   .res не добавляй. Проверяй успешность push и опубликованные PNG-ссылки.
7. После существенного расширения проверь HD -> оригиналы -> HD через настоящее
   меню в изолированной игре, сохрани кадры/отчёт, затем штатно выйди (harness quit)
   и убедись в завершении своего процесса. Пользовательские игры не закрывай.

Пользователю не показывай сравнения и не создавай HTML-сравнения. Переключатель HD
остаётся включён по умолчанию, отключение возвращает оригинальные размеры сразу;
приоритет моды -> HD -> база, визуальный pack не влияет на сетевую совместимость.
Прежний live-check.json относится к 1 952 текстурам и прежней сцене, не ко всем моделям.
При завершении сессии оставь короткий отчёт о принятых ID/группах, размерах,
проверках, коммитах/push и точном оставшемся задании.
