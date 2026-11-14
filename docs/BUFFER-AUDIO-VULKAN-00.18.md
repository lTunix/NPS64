# Comparación de buffer de audio — 00.18

Punto de retorno: commit local `dc8af75` y PKG 00.17, con Tennis y SRAM confirmados por el usuario. La calidad de Majora sigue pendiente.

Esta edición experimental prueba una reserva mayor en la salida de audio. Se mantienen sinc32, WSOLA, su búsqueda y límites de tempo, AudioOut a 48 kHz/1024 frames, prioridad, núcleo, renderizado y ajustes por juego. La memoria y los umbrales de la cola son el cambio funcional.

| Perfil | Capacidad | Prellenado | Reserva objetivo |
| --- | --- | --- | --- |
| 00.17 | 8192 frames / 170.7 ms | 5760 / 120 ms | 5120 / 106.7 ms |
| 00.18 | 16384 frames / 341.3 ms | 11520 / 240 ms | 10240 / 213.3 ms |

`NPS64_AUDIO_BUFFER_TEST` habilita el perfil ampliado en el builder del PKG Vulkan de prueba. Afecta al audio de los juegos ejecutados con esta edición, no se guarda como configuración permanente. Los builds sin esa macro conservan los umbrales anteriores. La capacidad adicional ocupa 32 KiB; no es una subida del tamaño de bloque de AudioOut. El coste esperado es más latencia entre acción y sonido, aproximadamente otros 107 ms en el objetivo, no una mejora de calidad de las muestras originales.

Los registros `[audio]` ahora identifican capacidad y objetivo en milisegundos y el prellenado realmente compilado, junto con ocupación, vaciados, descartes y tempo. No se cambia la temporización del núcleo ni el frame pacing.

## Validación local

- Comparación con el mismo consumidor y calendario de producción: 20 segundos virtuales PAL, jitter de 8 ms y retrasos periódicos de 100 ms. Perfil anterior: 10 underruns, 6862 frames descartados. Perfil ampliado: 0 underruns, 0 descartes. Se comprueba también que no reproduce antes de alcanzar el prellenado y que la cola permanece acotada.
- Ambas configuraciones pasan las pruebas con ASan/UBSan de tono/estéreo, desaceleración sostenida, deriva, reinicio, rampas, prellenado, orden, overflow/wrap, resampler y retención de buffers en el backend PS4 simulado. La prueba de audio del backend produce suficiente material para cada umbral; no presupone el antiguo prellenado.
- ELF y PKG: 28 comprobaciones, extracción del payload, icono, SFO y hashes. Núcleo idéntico por SHA-256 a 00.17; no se repite una medición de renderizado para atribuir rendimiento nuevo a esta prueba de cola.

## Prueba física pendiente

Instalar 00.18 y repetir la misma escena de Majora que sonaba mal en 00.17. Comparar chasquidos/cortes, distorsión y retraso del sonido. Conservar 00.17 para volver atrás. Al avisar el usuario, leer los logs y comparar el incremento de underruns/descartes durante esa sesión, ocupación, tempo y velocidad de emulación. Una mejoría sintética no demuestra que esta sea la causa del audio malo de Majora en PS4. Si la calidad sigue mal con la cola abastecida, no atribuirlo a falta de capacidad.

No se modifica ninguna ROM ni partida y no se hace commit, push, tag ni release automáticamente.

Se conservó el log físico previo a instalar 00.18 en `build/audio-buffer-18/ps4-before-18.log`. La última sesión registrada de Majora en 00.17 contiene 27 informes de audio: el contador de underruns pasa de 40 a 43 (incremento de 3), sin nuevos descartes. Sirve como referencia; no identifica por sí solo la causa de toda la distorsión percibida.
