# Retirada del perfil asíncrono — 00.31

El usuario reporta una regresión en la intro de Smash: 00.30 vuelve a 45 FPS y 00.29 no bajaba de 50 FPS en su comparación. Los registros confirman fullsync=disabled y que la espera se desplazó hacia VI/scanout. La racha lenta máxima registrada en esa sesión alcanzó 10,95 s. No se deduce una mejora por un contador FullSync menor.

Se elimina del código y de la compilación el perfil NPS64_SMASH_OVERLAP_TEST. Todas las ROM, incluidas Smash USA y ambas resoluciones, solicitan FullSync síncrono, como 00.29. No se añaden otras optimizaciones gráficas en esta revisión: se aísla la recuperación de audio ya probada en 00.30.

Se conservan el consumo de la cola parcial y el umbral de recuperación de 4096 muestras (85,33 ms), con las rampas solo al faltar datos. El inicio conserva 240 ms. Esta política debe evaluarse por frecuencia, duración y tiempo total de los cortes, porque una velocidad sostenidamente baja todavía puede agotar la cola. La calidad de reproducción sana mantiene los datos originales.

La librería del núcleo y los archivos GPU/escalado permanecen idénticos a 00.29. Se comprueban las opciones de FullSync en el frontend y su ejecución en Smash 2x. La reproducción de la intro y el audio en PS4 siguen pendientes de repetir para confirmar la recuperación del comportamiento anterior. Mantener 00.29 como referencia y retirar el PKG de prueba 00.30 de la consola cuando se verifique la transferencia de 00.31. Sin push ni release.

Validación local: pruebas de recuperación de audio y backend AudioOut, configuración por juego y empaquetado (28 comprobaciones) correctas. Ejecución Vulkan de 120 segundos con más de 180 VI y FullSync activo, sin errores ASan ni validación Vulkan. Persisten 43 avisos UBSan conocidos fuera de Vulkan; no es una ejecución UBSan limpia. Esta validación usa GPU por software y no demuestra rendimiento en PS4.
