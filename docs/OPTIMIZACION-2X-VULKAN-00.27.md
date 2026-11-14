# Ajuste incremental de 2x y tiempos RDP — 00.27

Base: checkpoint 27e286b (00.26), confirmado en PS4. Los registros históricos muestran el descenso del coste de presentación de Smash 2x desde aproximadamente 6 a 3,7 ms. Hay intervalos en que el núcleo todavía sube a 16–17 ms; no se presupone que ese tiempo sea exclusivamente emulación de CPU.

## Escalado exacto

Se reduce el trabajo de la ruta 4:3 contigua con productos de diferencias SSSE3. Los pesos 42 y 43 se aplican a diferencias de bytes, cuyo rango [-10965,10965] evita saturación en PMADDUBSW. La suma modular de 16 bits y el redondeo conservan exactamente la interpolación Q8. La fase central sigue usando promedio redondeado. No cambia la resolución, la calidad del filtro, los píxeles finales ni la selección por ROM.

La prueba nueva agota los 65536 pares de bytes, incluyendo diferencias extremas, y verifica las tres fases. También pasan referencias matemáticas y ocho hashes de imágenes completos contra el checkpoint anterior. En la mediana de tres ejecuciones de este PC, el caso RGBA 1280x480→960x720 pasó de 0,487792 a 0,443183 ms, una reducción aproximada del 9 %. No es una medición de FPS en PS4.

## Medición del núcleo

Se agregan contadores de pared a process_commands y complete_frame del adaptador RDP, y al wait de FullSync. La orden de esperar antes de emitir la interrupción DP permanece intacta. El único objeto del núcleo que cambia es rdp.o, para medir estos tramos; no se recompilan shaders ni se cambia el planificador GPU.

Los campos rdp_ms y vi_ms dejan de ser ceros en Vulkan. La línea [rdp-times] muestra list_ms, vi_ms, fullsync_ms y el número de FullSync del intervalo. list_ms incluye fullsync_ms y vi_ms incluye el readback: no deben sumarse de nuevo las categorías incluidas. El muestreo se reinicia al inicializar el renderer y al recoger contadores. El consumidor y esas funciones se ejecutan en el hilo de emulación.

## Validación

- Prueba de bytes extremos y referencias del escalador con ASan/UBSan.
- Ocho hashes de presentación idénticos frente a 00.26, con fallback SSE2 conservado.
- Vida útil del frame, configuración por juego, pausa y persistencia.
- Smash USA 2x: sesión nativa de 60 s, cierre y validación Vulkan sin errores ASan ni de validación. Se mantienen 43 avisos UBSan conocidos en el resto del núcleo de este juego. Las mediciones FullSync son no negativas y no exceden el tiempo RDP que las contiene.
- PKG con 28 comprobaciones. FPS y tiempos FullSync de PS4 pendientes de la prueba física.

Mantener 00.26 como referencia. No se cambia el audio ni se publica un release.
