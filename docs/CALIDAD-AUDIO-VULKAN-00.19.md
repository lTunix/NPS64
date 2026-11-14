# Filtro de voces HLE — 00.19

Comparación con 00.18, que el usuario describió como algo mejor pero aún con mala calidad en Majora. El log físico previo conserva 38 reportes de esa sesión: underruns de 0 a 1, ningún descarte y ocupación final de 197.1 ms. Ese único vaciado no explica necesariamente la distorsión sostenida; no se considera el buffer una solución completa.

## Cambio aislado

La edición Vulkan de prueba activa `parallel-n64-enhanced-hle-audio=enabled` y calidad `32` a través de la interfaz de opciones del núcleo. Este código ya está en el núcleo: sustituye la interpolación de voces de cuatro coeficientes del HLE por un sinc de ventana de 32 coeficientes, con filtrado dependiente del pitch. Los consumidores con ventana corta pueden usar la variante acotada de 16. No se cambia el número de muestras ni el avance de las voces; se alteran sus valores y no se pretende equivalencia bit a bit con el sonido original.

Se mantienen exactamente la cola de 16384 frames, prellenado 11520, objetivo 10240, salida 48 kHz/1024, resampler final sinc32 y WSOLA de 00.18. No se añaden ganancia, ecualización ni filtros sobre la mezcla final. El núcleo, Tennis, SRAM, renderizado y shaders se conservan por SHA-256.

`NPS64_AUDIO_HQ_TEST` limita esta activación al builder de prueba; los demás builds mantienen sus opciones previas. Las tablas finitas de 16 y 32 se preparan durante la carga usando las funciones del propio núcleo. Se evitan cálculos trigonométricos para construirlas al entrar la primera voz. El registro `[audio-quality]` identifica activación, calidad, preparación y tiempo; en caso de fallo de memoria se conserva la caída a la ruta original del núcleo.

## Validación local

- Prueba con ASan/UBSan sobre el código real `resample_hq.c`: desactivación, límites de 16/32, ganancia DC exacta en las 256 fases probadas con pitch 2, reutilización y liberación. Para pitch 2, respuesta de magnitud 0.999881 a 0.1 ciclos/muestra y 0.000071 a 0.4. Es una prueba del filtro, no una medición de calidad de Majora en PS4.
- Capturas de Majora con Vulkan y los mismos inputs, 900 VI con la mejora desactivada/activada. Ambas entregan 555472 frames de entrada y 833325 frames estéreo tras sinc32 a 48 kHz. Cambian 533152 valores: se comprueba que la selección realmente afecta al audio. HLE sigue activo, no se cambia a LLE.
- Preparación nativa de tablas: 10 ms. Mediana de los intervalos de presentación muestreados desde VI 600: 7.410 ms sin mejora y 8.4385 ms con ella. No se usan las medias globales, afectadas por compilaciones iniciales de shaders; no es una medición del coste en PS4 ni un benchmark exclusivamente de audio.
- RMS de captura 3102.91 frente a 3118.98; valores saturados 34 frente a 40 entre 1666650 valores. No se atribuye al filtro la eliminación de todos los defectos de generación de audio.
- Sin errores AddressSanitizer en ambas capturas. El núcleo completo conserva avisos UBSan preexistentes; no se declara limpio de UBSan.
- ELF/PKG: 28 comprobaciones, payload extraído, icono, SFO y hashes. Núcleo idéntico al de 00.18.

Pendiente: repetir la misma escena de Majora en PS4, valorar claridad/ruidos y verificar que la velocidad permanece cerca del objetivo. Conservar 00.18 para comparar con el mismo buffer y filtro de voces desactivado. Confirmar en logs `enhanced_hle=1`, `banks_ready=1`, ocupación, underruns, descartes y tiempos del núcleo. No se hace commit, push, tag ni release automáticamente. Las capturas y ROMs permanecen locales y no se empaquetan.
