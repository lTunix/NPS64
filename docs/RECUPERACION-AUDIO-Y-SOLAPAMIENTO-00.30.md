# Recuperación de audio y perfil Smash 2x — 00.30

Checkpoint 00.29 validado por el usuario: 3c8e1fd, sin push. El usuario confirma que los silencios aparecen principalmente durante caídas de velocidad. Los registros de 00.29 muestran un intervalo de 50 FPS con FullSync en 6,865 ms de los 7,192 ms RDP y vaciados de audio. La cola existente vuelve a exigir 11520 muestras (240 ms nominales) tras cada vaciado y retiene la cola parcial sin reproducirla.

## Recuperación de audio

El inicio y los reinicios deliberados mantienen 240 ms. Después de una falta de datos, se consume la cola parcial disponible y solo se rellena lo que falta; se reanuda con 4096 muestras (85,33 ms). Rampas de 64 muestras (1,33 ms) suavizan únicamente los límites de una recuperación. La reproducción normal conserva las muestras bit a bit; no hay estiramiento ni cambio de pitch. No se reactiva la interpolación HLE extendida que contaminaba voces.

[audio-recovery] informa cola parcial consumida, umbral y duración acumulada/activa/máxima de salida de recuperación. Estas duraciones cuentan relleno sin nuevas muestras de origen, incluyendo las pequeñas rampas; no son timestamps físicos de silencio. Los contadores son acumulativos desde que se crea la cola, como underruns.

Prueba con un único parón de 360 ms: la racha de bloques totalmente silenciosos baja de 17 a 9 (aproximadamente 363 a 192 ms), con el mismo único underrun. Esto no demuestra que los cortes desaparezcan bajo producción sostenidamente lenta. Un umbral menor podría causar recuperaciones más frecuentes en ese caso: medir también frecuencia y tiempo total, no aceptar solo una menor duración máxima.

## Perfil de ejecución

Solo la ROM exacta rom-cf6bb875013bfa0c, Smash USA, en Vulkan a 2x solicita la opción estándar FullSync asíncrona del núcleo. El comando SyncFull sigue vaciando el renderer; se omite la espera de CPU inmediata, permitiendo continuar emulación mientras el trabajo avanza. El scanout/readback conserva sus dependencias y fence. Nativa y otras ROM mantienen FullSync síncrono; las reglas de compatibilidad del núcleo pueden forzarlo cuando corresponda.

La espera restante puede trasladarse a VI/scanout: un fullsync_ms igual a cero no prueba por sí solo mejor rendimiento. Comparar tiempo total de núcleo, cadencia, frecuencia y duración de caídas, así como underruns y recuperación. El ensayo con GPU por software no demuestra una mejora de FPS en PS4. Se conserva 00.29 como referencia y el resultado físico queda pendiente.

## Validación

- ASan/UBSan de recuperación: 1000 bloques estéreo de datos variables exactos, vuelta de contador/índices, cola parcial, rampas, inicio, recuperación y reset.
- AudioOut real con servicios simulados: buffers retenidos, envíos continuos, drenaje y prioridad aceptada/denegada.
- Frontend: perfil limitado a la clave exacta a 2x; nativa y otra clave vuelven a síncrono. Opciones por juego y persistencia.
- Smash USA 2x asíncrono: 120,010 s y 447 VI, sin errores ASan ni de validación Vulkan. Captura del cuadro 300 idéntica fuera del HUD frente a 00.29. Persisten 43 avisos UBSan conocidos del núcleo.
- Control nativo síncrono: sesión de 90 s y más de 180 VI, sin errores ASan ni de validación; 40 avisos UBSan previos. La prueba inicial de 30 s quedó corta en cobertura y se amplió sin relajar el mínimo.
- PKG con 28 comprobaciones. El archivo del núcleo, shaders y controlador son idénticos a 00.29; cambia la opción seleccionada por el frontend.

Se descargó una copia local de la SRAM de Smash antes de probar el perfil; no se modifica remotamente. Validación física pendiente, incluyendo combate, pausa y menús. Sin push ni release.

Resultado físico posterior: el usuario reportó 45 FPS durante la intro, frente a un mínimo observado de 50 FPS en 00.29. Se rechaza el perfil de FullSync asíncrono para Smash y se retira en 00.31. La recuperación de audio se mantiene separada para su evaluación. No se considera una optimización aceptada por el hecho de que FullSync marque cero.
