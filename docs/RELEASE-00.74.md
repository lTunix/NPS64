# NPS64 00.74 / v0.74.0

## English

### Improvements since 00.73

- **Vulkan performance:** restrict raster work to active regions, reduce redundant passes and coalesce small submissions. Effects, command order, FullSync and native/2× rendering are retained. Mario Tennis and other demanding scenes received several rounds of optimization.
- **Exact CPU geometry:** avoid redundant projection calculations and vertex copies, improve display-list culling, and normalize RSP reciprocal inputs with a count-leading-zeros instruction. These changes preserve the original arithmetic results.
- **Army Men startup:** initialize Vulkan's HLE callbacks before the first streaming graphics task. Opening Army Men first no longer depends on a previously opened game to select its fast path. PS4 testing confirmed substantial recovery from the slow fallback and a further improvement with the latest arithmetic optimization.
- **Mario Kart 64:** correct legacy microcode perspective/depth handling behind the flicker seen in Choco Mountain. The user confirmed corrected graphics without a noticeable performance regression.
- **Excitebike 64 and 1080 Snowboarding:** correct display-list family detection and CPU startup behavior. Both games were confirmed to start correctly on PS4.
- **Conker stability:** guard a dynarec delay-slot boundary found during startup testing. Its remaining intro performance drops are a separate unresolved issue.
- **Cover library:** a missing cover no longer blocks later downloads or repeatedly stalls the menu. Wave Race 64's catalog subtitle is recognized.
- **Updated application and README icon.** English/Spanish menus, cheats, live resolution/aspect settings and existing audio processing are retained.

### Remaining work

- **Army Men:** intermittent stutters and FPS drops remain, particularly explosions and many effects at 2×. The latest build improves performance; it does not eliminate them.
- **Conker’s Bad Fur Day:** intro and demanding-scene drops remain unresolved.
- **Mario Tennis:** further optimization is needed for particle-heavy replays at 2×.
- Continue longer-session stability and wider game testing. Performance varies by game, region and scene; constant 60 FPS is not guaranteed.
- **Majora’s Mask Collector’s Edition:** reported audio/performance issues remain. Use your own dump of the standard Nintendo 64 cartridge edition for the recommended comparison.

### Install and validation

Install **NPS64-by-lTunix-00.74.pkg**. It updates the original **NPSX00064** app, preserving public saves and preferences under `/data/NPS64`. The tag **v0.74.0** follows the existing update sequence. Downloads go to `/data/pkg`; installation remains manual. Isolated **NPSV00064** test data is preserved and is not migrated automatically. No games are included.

The changes were checked in iterative physical PS4 tests, including the user's confirmation of the latest Army Men improvement and its remaining stutters. Arithmetic regression tests compare 4,263,424 exact outputs; additional checks compare sixteen matching Army Men Vulkan frames and identical Conker command streams. These correctness checks are not console FPS benchmarks. The public PKG retains the tested core, uses the original application identity and updater, and excludes diagnostic frame capture and GPU timestamp queries. ELF/OELF mapping, package validation, extraction, icon and SFO checks pass.

## Español

### Mejoras desde 00.73

- **Rendimiento Vulkan:** limita el trabajo a regiones activas, reduce pasadas redundantes y agrupa envíos pequeños. Conserva efectos, orden de comandos, FullSync y renderizado nativo/2×. Mario Tennis y otras escenas exigentes recibieron varias rondas de optimización.
- **Geometría de CPU exacta:** evita cálculos de proyección y copias redundantes, mejora el descarte de listas gráficas y normaliza los recíprocos del RSP mediante una instrucción de conteo de ceros iniciales. Conserva los resultados aritméticos originales.
- **Arranque de Army Men:** conecta Vulkan con el servicio gráfico HLE antes de la primera tarea streaming. Ya no depende de abrir otro juego antes para utilizar su ruta rápida. Las pruebas en PS4 confirmaron la recuperación respecto al fallback lento y una mejora adicional con la última optimización aritmética.
- **Mario Kart 64:** corrige el manejo de perspectiva/profundidad del microcódigo antiguo que provocaba el parpadeo en Choco Mountain. El usuario confirmó gráficos corregidos sin una pérdida apreciable de rendimiento.
- **Excitebike 64 y 1080 Snowboarding:** corrige la identificación de listas gráficas y el comportamiento de arranque de CPU. Ambos juegos fueron confirmados funcionando al iniciar en PS4.
- **Estabilidad de Conker:** protege un límite del recompilador detectado durante las pruebas de arranque. Los bajones de su intro siguen siendo un problema independiente pendiente.
- **Biblioteca de carátulas:** una imagen ausente ya no bloquea las siguientes descargas ni provoca esperas repetidas en el menú. Se reconoce el subtítulo del catálogo de Wave Race 64.
- **Icono de aplicación y README actualizado.** Se conservan los idiomas inglés/español, trucos, ajustes en vivo de resolución/aspecto y procesamiento de audio.

### Pendientes

- **Army Men:** todavía presenta tirones y bajones, especialmente con explosiones y muchos efectos a 2×. La última versión mejora el rendimiento, pero no los elimina.
- **Conker’s Bad Fur Day:** siguen pendientes los bajones de la intro y escenas exigentes.
- **Mario Tennis:** continuar optimizando repeticiones con muchas partículas a 2×.
- Ampliar las pruebas prolongadas y de más juegos. El rendimiento depende del juego, región y escena; no se garantizan 60 FPS constantes.
- **Majora’s Mask Collector’s Edition:** conserva problemas reportados de audio/rendimiento. Se recomienda comparar con un volcado propio de la edición normal de cartucho de Nintendo 64.

### Instalación y validación

Instala **NPS64-by-lTunix-00.74.pkg**: actualiza **NPSX00064**, conserva partidas y preferencias públicas en `/data/NPS64` y mantiene la secuencia de actualizaciones con **v0.74.0**. Se descarga a `/data/pkg` y se instala manualmente. Los datos de prueba **NPSV00064** se conservan y no se migran automáticamente. No se incluyen juegos.

Las pruebas físicas iterativas de PS4 incluyen la confirmación de mejora de Army Men y de sus tirones restantes. Las regresiones verifican 4.263.424 resultados aritméticos exactos, dieciséis fotogramas Vulkan idénticos de Army Men y comandos gráficos idénticos de Conker; son pruebas de corrección, no mediciones de FPS de consola. El PKG público conserva el núcleo probado, identidad y actualizador originales, y excluye capturas diagnósticas y consultas de tiempo GPU. Pasan las comprobaciones ELF/OELF, paquete, extracción, icono y SFO.
