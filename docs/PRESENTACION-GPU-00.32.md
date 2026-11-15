# Presentación Vulkan experimental 00.32

Rama local `perf/gpu-presentation-00.72`, basada en el release público 00.71. No cambia el núcleo, la interpolación de audio, el rasterizado paraLLEl ni FullSync. No publicar antes de verificar imagen, estabilidad y rendimiento en PS4.

## Referencia y alcance

La revisión de Salsa64 `69b4d844df702a9f138bd8fe5689eda4f607e8b0` muestra un enfoque HLE/RSX y trabajo por lotes, diferente del rasterizador compute de NPS64. Se adapta la idea de evitar copias y trabajo repetido; no se traslada código Cell, RSX ni su recompilador PPC a PS4. Sus datos no predicen FPS en esta consola.

El winsys PS4 actualmente devuelve `ENOSYS` en `ac_drm_create_bo_from_user_mem`. Por ello no se importa a Vulkan la memoria de pantalla existente, ni se modifica el driver. El compositor crea tres búferes Vulkan propios, visibles, coherentes y cacheados para CPU, y ofrece esas direcciones a VideoOut.

## Implementación

- Shader compute 8×8: escalado Q8 exacto, interpolación horizontal redondeada antes de la vertical, conversión RGBA→ARGB y bordes. Equivale al filtro CPU actual; no cambia textura, LOD ni antialiasing del RDP.
- Memoria, pipeline, sampler, pool y sets persistentes. Las entradas de descriptor solo se reutilizan mientras sigue viva la misma imagen. Cada `set_image` invalida la caché para evitar handles reciclados.
- Las órdenes pendientes del núcleo y la composición se envían en una sola presentación. Se respetan semáforos, barreras, propiedad de imagen y fence; no se quita FullSync ni se aumenta el trabajo en vuelo del rasterizador.
- Si VideoOut acepta los búferes nuevos, HUD y pausa se dibujan sobre el búfer coherente terminado y se presenta sin copiar el cuadro desde CPU. Antes de escribir se espera la retirada de ese búfer. Después del flip se restaura el canvas de biblioteca.
- Si VideoOut rechaza el registro en PS4, se desactiva el compositor y se conserva la lectura/escalado anterior. Solo la prueba nativa permite componer en GPU y copiar al canvas, para validar la imagen sin disponer de VideoOut. Si no existe memoria adecuada o el formato no es RGBA8 UNORM, se usa la lectura y el escalado anteriores. Los errores de envío/sincronización detienen la ruta; no se reutiliza memoria cuyo estado es incierto.
- Al cerrar un juego se presenta primero un búfer de CPU, se espera a que sustituya la imagen externa y solo entonces se desregistra y libera la memoria. Un fallo de retirada cierra VideoOut antes de destruir los búferes.
- Los cuadros repetidos se vuelven a componer para limpiar HUD/pausa; no se reutiliza una imagen contaminada por la interfaz. No se añade una copia completa extra de caché en cada cuadro nuevo.

## Medición

`[gpu-present-times]` separa adquisición/espera de VideoOut, grabación, envío, fence y copia alternativa. `[scanout]` conserva las medidas de espera y copia. `[frame-times]`, `[cadence]` y `[stutter]` mantienen duración máxima, percentiles y episodios lentos. El fence puede incluir trabajo previo del núcleo: no sumar temporizadores inclusivos.

Comparar la misma ROM, escena, filtro, aspecto y resolución en 00.31/00.71 y 00.32; medir también cuánto duran las caídas y si interrumpen el audio. La compilación inicial debe identificarse por separado. `scripts/analyze_presentation.py` resume los costes y la cadencia sin reducir el resultado al promedio de FPS.

## Validación local

- 36 cuadros completos (33.177.600 píxeles) idénticos a la implementación CPU: nativa/2×, tamaños impares, dimensiones 1×1, ambos aspectos, nearest/bilineal, bordes y sobrescritura de HUD. ASan/UBSan y validación Vulkan activados.
- Backend PS4 real con servicios simulados: 120 presentaciones externas, alternancia con menú CPU, retención de contenido pendiente/visible, retirada antes de liberar y rechazo del registro.
- Pruebas de interfaz y backend anterior conservadas.
- Integración con ROM y validación del PKG se registran en `build/present-72/`. La prueba Vulkan local usa software; no mide rendimiento de PS4 ni valida la aceptación física de memoria Onion por VideoOut.

## Construcción

`python3 scripts/build_gpu_game_test.py --link-only --gpu-present` (requiere el núcleo/librerías ya preparados; omitir `--link-only` para construir el núcleo). La combinación con `--public-release` está bloqueada para esta prueba.

Regenerar el shader con `python3 scripts/compile_present_shader.py --glslang /ruta/a/glslang`. Pruebas: `scripts/test_gpu_presenter.sh`, `scripts/test_gpu_scanout.sh`, `scripts/test_vulkan_game_trace.sh` con `NPS64_NATIVE_GPU_PRESENT=1`.

El perfil expone las carpetas aisladas `vulkan-test-*` y la app NPSV00064. El release público NPSX00064 y sus partidas quedan separados.

Resultado de integración local: Smash USA a 2× completó dos ejecuciones de 120 segundos (1285 VI anteriores y 1193 VI con compositor por software y copia CPU). Ambas conservan FullSync y pasan ASan/validación Vulkan; permanecen 43 avisos UBSan conocidos fuera de Vulkan. El cuadro 300 tiene cero diferencias en 819.200 píxeles fuera del HUD. La variante de software no ganó rendimiento; no extrapolar esos FPS a PS4. La ruta directa y su estabilidad necesitan confirmación física.

## Punto validado en PS4 — 2026-11-15

El usuario confirma una mejora considerable con 00.32, cercana a 60 FPS sostenidos en los juegos probados, y considera satisfactorio el resultado. Es una validación de uso en consola, no una garantía para todos los juegos ni una prueba prolongada de ausencia de fallos. Se guarda este punto antes de continuar el análisis de estabilidad.

PKG probado: `NPS64-Vulkan-Game-Test-00.32.pkg`, SHA256 `ac8ab67f5331a6eaf4dedd81a5cc32e1b562f56f07d8f5274889f7925b013f82`. Núcleo y audio conservados. El análisis posterior debe confirmar la ruta directa en los logs, revisar errores y retirada de búferes; no añadir optimizaciones especulativas. Sin push ni release.
