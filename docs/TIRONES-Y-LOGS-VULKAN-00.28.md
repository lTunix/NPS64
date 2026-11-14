# Tirones, cadencia y E/S — 00.28

El usuario exige minimizar frecuencia y duración de los bajones, aunque mejoren los promedios. En la sesión PS4 00.27 de Smash hay intervalos cercanos a 60 FPS junto a cuadros de 25–40 ms y vaciados de audio. RDP y FullSync ya tienen mediciones. No se considera corregido un tirón solo por subir el promedio.

## Cambio de ejecución

Antes, nps64_vlog mantenía un bloqueo compartido mientras escribía cada mensaje al sistema de archivos. La 00.28 usa una cola MPSC limitada a 128 mensajes de hasta 2047 bytes. Los productores solo formatean y publican su mensaje; un hilo de baja prioridad escribe lotes de hasta 16 KiB. No hay espera de disco en los productores ni un bloqueo de productores durante la escritura. La cola llena descarta mensajes nuevos y registra cuántos; no bloquea el juego. El cierre normal drena la cola y sincroniza el archivo. Si no se puede crear el hilo se conserva la escritura síncrona y se informa async=0. Un cierre abrupto puede perder mensajes pendientes; el archivo separado de captura fatal sigue su ruta directa.

[log-queue] informa el modo real y el número de descartes. El experimento conserva nativa/2x, filtro, orden FullSync, audio y guardados. No se afirma todavía que la E/S explique todos los picos: los tiempos restantes de emulación/RDP también pueden superar el presupuesto.

## Medición de bajones

[cadence] contiene contadores acumulativos de cada tramo de juego sin pausa: cuadros lentos, porcentaje, tiempo lento acumulado, exceso sobre presupuesto, peor intervalo, rachas y duración máxima/activa. Hay contadores de intervalos de al menos 1,5x y 2x el presupuesto, y de 50 y 100 ms. Una tolerancia de 0,5 ms evita clasificar el pequeño desajuste 59,94/60,02 Hz como una caída continua. Las rachas se conservan entre informes de un segundo; al pausar o terminar se registra el último tramo.

[stutter] detalla rachas terminadas cuya duración suma al menos 1,5 periodos nominales: primer/último cuadro, duración, peor intervalo, exceso y segundo de finalización dentro del tramo. El detalle se complementa con active_ms del resumen cuando la racha no ha terminado. [autosave] mide la escritura periódica existente sin alterar su funcionamiento.

La cadencia se mide entre retornos consecutivos de platform_present: incluye sueño de pacing y actividad entre cuadros, pero no son timestamps físicos de pantalla. Se conservan las distribuciones de trabajo y núcleo con p95/p99. El arranque y la compilación inicial deben analizarse por separado del juego estable, sin ocultarlos de los datos.

Análisis: python3 scripts/analyze_stutter.py --last-run RUTA_DEL_LOG. Devuelve duración y frecuencia normalizada por minuto, porcentaje de tiempo lento, máximo, racha más larga y eventos de mayor duración. No confundir pocos eventos largos con mejor resultado que muchos eventos cortos.

## Validación y criterio de aceptación

- ASan/UBSan: filesystem bloqueado sin bloquear productores, cola limitada y descartes exactos; cuatro productores/20000 mensajes, orden, escrituras parciales, fallo, drenaje, reapertura y fallback síncrono.
- Cadencia: relojes estables, bajón sostenido, rachas entre informes, pausas aisladas/largas, exceso, presupuesto PAL e inválidos.
- Análisis: contadores acumulativos, segmentos de pausa, registros parciales y duraciones.
- Smash USA 2x: sesión nativa de 60 s con Vulkan validation, ASan sin errores y cero descartes de logs; permanecen 43 avisos UBSan conocidos del núcleo. Estas pruebas comprueban funcionamiento, no rendimiento de PS4; el rasterizador por software del PC tiene picos de compilación ajenos a la consola.
- PKG con 28 comprobaciones; ajustes y persistencia pasan sus regresiones.

En PS4, comparar la misma ROM/región, resolución, filtro y combate, incluyendo cambios de cámara. Evaluar frecuencia por minuto, porcentaje y duración total de tiempo lento, racha máxima, p95/p99 y peor intervalo. Un promedio mejor no basta si aparecen episodios más frecuentes o prolongados. La mejora perceptible y de colas largas queda pendiente de prueba física. Mantener 00.27 como referencia; sin push ni release.
