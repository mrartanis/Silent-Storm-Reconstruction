# Зависание изображения при перемещении камеры

Диагностика пользовательского запуска 2026-10-02 / исправление 2026-10-03.
Процесс 33248, `geometry-optimized-20261002-01`, Game SHA256
`15B01D28608EC42AC0673D2602E93D3596004F1145ADCF1B939A8C04A28B810A`.

## Подтверждённая причина

Первое сообщение stderr: `bgfx render target creation failed`. В full dump:

- `bgfx::s_ctx->m_frameBufferHandle.m_numHandles = 0x400`,
  `m_maxHandles = 0x400`: исчерпаны все 1024 handles.
- В текущем кадре renderer удерживает 509 framebuffers и 1018 surface refs.
  Остальные handles ещё заняты предыдущими кадрами: bgfx освобождает их отложенно.
- `BgfxDevice::Impl::healthy=false`. После Fail Present прекращает отправку кадров;
  главный поток всё ещё обрабатывает SDL/игру, а renderer thread ждёт api semaphore.
  Это остановка вывода изображения, а не доказательство взаимной блокировки.
- Renderer создавал новый framebuffer для каждого ordered view, даже при тех же
  attachments. Движение камеры меняет состав/число проходов; запас 1024 исчерпывался
  при сумме текущего кадра и ещё не освобождённых предыдущих.

Последняя доступная камера: anchor `(16.662041,14.570512,6.216917)`, yaw `0.372001`,
pitch `-0.741172`, rod `50.037933`, frozen=0. Пользователь подтвердил, что двигал камеру.
Full dump сохранён до последующих действий:
`G:/SS/lab/runs/geometry-optimized-20261002-01/evidence/hang-20261002/hang-20261002-234728.dmp`.
Анализ counters: `G:/SS/lab/perf-hang-counts.log`, состояние: `perf-hang-state2.log`.

Попытка записать отдельный диагностический сейв завершилась access violation в
`CBilinearTexture::Recalc`: отказавший Screenshot давал 1×1 заглушку, а построение
миниатюры читало соседние pixels. Процесс завершился. Каталог
`before_framebuffer_fix_20261002` содержит незавершённый `game.sav` (0 bytes) и копии
вспомогательных файлов; этот слот не является успешным сохранением. Существующие
сейвы не перезаписывались. Текущее состояние интерфейса нельзя считать сохранённым.

## Изменения

`BgfxBackend.cpp`: framebuffer cache по `(color handle, face, mip, depth handle,
face, mip)`, включая одноцветные targets FXAA. Ordered view IDs остаются отдельными:
Clear/тени/прозрачность сохраняют порядок. Cache удерживает surfaces, предотвращая
повторное использование texture handle под старым ключом. Не использовавшиеся
в текущем кадре attachment sets удаляются; resize/shutdown очищают cache целиком.
Лимит 1024 не увеличен. После Fail draw/clear возвращают E_FAIL, первый error
записывается с числом views/cache entries, повторная ошибка не засоряет log.

`GfxBgfx.cpp`: при недоступном Screenshot формируется непрозрачный чёрный кадр
размера экрана (минимум 2×2). Сохранение получает initialized pixels с соседями,
а не недостаточную для bilinear sampling заглушку.

Profiler дополнен `framebuffer_creates`, harness — `perfpan dx dy dz` (фиксирует
камеру и перемещает anchor) и `perfcamera 0|1` (отпустить/зафиксировать камеру).

## Проверки

- `BgfxRendererTests --passes`: 1402 ordered views за кадр × 8 кадров с пятью
  переиспользуемыми attachment sets; правильный цвет финального кадра. Этот объём
  превышает прежний pool limit при framebuffer на каждый view. Отдельно намеренно
  вызывается Fail через неизвестный shader и проверяется безопасное построение
  320×200 save thumbnail. Сообщение Unregistered vertex shader в этом тесте ожидаемо.
- Обычные renderer tests, `--geometry`, `--graphics`: depth/stencil/blend/targets,
  pass order, geometry updates/batching/device epoch, FXAA и точность UI прошли.
- Реальная карта: 900 представленных кадров в пяти положениях, 2560×1440,
  AA=1, anisotropy=16, VSync=0. Ошибок renderer нет. На rod=80: 49,03 FPS;
  после перемещений на rod=50: 50,56 / 49,65 / 53,94 / 61,96 FPS. Это отдельные
  замеры, а не гарантированный FPS во всех положениях камеры или на всех картах.
- В измерениях примерно 296–333 views за кадр, но всего 0,5–0,6 новых framebuffers
  в среднем (обычно 0–2). Создан проверенный сейв `fb_cache_pan_verified_20261002`,
  `game.sav` имеет ненулевой размер; тестовая игра завершилась штатно с кодом 0.

Материалы карты: `G:/SS/lab/runs/perf-investigation-20261002-01/evidence/framebuffer-pan-plan*`,
CSV `_perf_fb_cache_aa*.csv`; тестовые журналы `G:/SS/lab/framebuffer-*-tests.log`.
