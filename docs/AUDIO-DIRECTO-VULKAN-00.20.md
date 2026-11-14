# Audio directo Vulkan 00.20

Prueba controlada posterior a 00.19: mantiene interpolación HLE de 32 taps, remuestreo sinc32 a 48 kHz, cola de 16384 cuadros estéreo y precarga de 11520. Omite WSOLA y copia el PCM remuestreado directamente a AudioOut. No altera el núcleo ni los tiempos de vídeo. Puede perder tolerancia a una velocidad sostenida inferior al 100 %; no es todavía la solución definitiva de sincronización.

## Diagnóstico

La captura nativa de Majora Collector’s Edition PAL muestra 560 cuadros de audio DMA por VI frente a 639–640 esperados: se agregan 79–80 silenciosos en el relleno del núcleo. Un watchpoint con intérprete encontró que el juego escribe explícitamente 60 en su contexto de audio (PC 0x801900d8) aunque el vídeo funciona a 50 Hz. La edición de cartucho USA entrega aproximadamente 534 cuadros por VI a 60 Hz, sin déficit sistemático en los últimos 100 VI capturados. Esto demuestra una diferencia de esta ruta de emulación; no implica un defecto del juego original.

El usuario confirma mejor sonido y fluidez con cartucho. Los logs físicos 00.19 de esa sesión muestran el contador de underruns pasando de 1 a 2, sin muestras descartadas. La velocidad baja por momentos al 95–97 %. Las métricas son acumulativas: no equivalen a dos cortes nuevos en esta sesión.

## Validación

- Pruebas ASan/UBSan de cola directa: orden, wrap, reset, overflow, precarga, rebuffer y pausas periódicas de 40 ms.
- Backend AudioOut simulado: dos buffers retenidos, envío continuo, prioridad y drenaje al cerrar.
- PKG: 28 comprobaciones, extracción de payload, icono y SFO.
- Comparación auditiva y estabilidad en PS4 pendientes. Mantener 00.19 como referencia.

No se modifican las ROM originales ni se publica un release.
