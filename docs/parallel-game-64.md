# 00.64: mezcla de profundidad/color sin salida temprana

PS4 00.63: envío 331, completado 330, traza final 11. La traza 9 se emite
después de la espera de rasterización. La traza 11 se emite después del
dispatch de mezcla; la traza 12, posterior a su sincronización, no llega.
Esto acota el bloqueo a esa etapa o su sincronización, sin demostrar qué
instrucción GPU lo causa. La ejecución del binning y la espera posterior
a la rasterización sí avanzaron.

El experimento 00.64 conserva la ruta separada y modifica solo la entrada
del shader de mezcla: el descarte por máscara gruesa vacía se expresa como
un bloque condicional alrededor del trabajo, en lugar de un return temprano.
Los algoritmos de color, profundidad, texturas y escrituras no cambian.
La hipótesis se basa en la ruta de rechazo del binning que requirió una
reescritura anterior; no implica que ambas fallas tengan la misma causa.

El shader se genera en build, pasa spirv-val y se integra únicamente en las
copias experimentales del núcleo. La rama main y CPU 00.51 permanecen intactas.

Paquete: `out/NPS64-00.64-parallel-rdp-test.pkg`.
Log: `/data/NPS64-Parallel/nps64.log`.
La validación local no emula PS4 ni confirma rendimiento o estabilidad en ella.

Validación completada: Mario ejecutó 650 cuadros RDP locales. Las capturas
150/300/480/600 se compararon con CPU reproduciendo los mismos comandos;
estado VI y tamaño 640x288 coincidentes, error máximo 1/255 por canal en
las cuatro. Cuadro 600 revisado visualmente. Artefactos en
`build/parallel-comparison/mario-64-blend/`. PKG: 28 comprobaciones aprobadas.
La prueba real de PS4 permanece pendiente.

## Resultado PS4: bloqueo reproducido

Los nuevos logs de 00.64 vuelven a mostrar envío 331/completado 330 y
traza final 11. La reescritura del descarte no solucionó el bloqueo. La
compilación del shader nuevo aparece en el log, pero compilar con éxito
no demuestra que sus instrucciones se ejecuten correctamente.

`scripts/analyze_parallel_blend_capture.py` audita las capturas posteriores
al bloqueo. En este lote: 1.200 tiles activos (40x30), máscaras gruesas y
finas coincidentes, offsets únicos 0..1199 y conteo indirecto 1.200.
Las 76.800 lecturas de cobertura calculadas caben en su buffer: 76.160
bytes son FILL (64) y 640 son rechazo (255). Las ocho comprobaciones pasan.
Esto descarta corrupción visible de esas estructuras en esta captura;
no prueba visibilidad/coherencia durante la ejecución GPU ni corrección
del código máquina o de las escrituras al framebuffer.

Reproducción: `python scripts/analyze_parallel_blend_capture.py logs
build/parallel-blend-analysis-64`. Los datos derivados de ROM se conservan
únicamente en logs/build ignorados. El reporte incluye hashes de las
capturas para identificar exactamente la evidencia analizada.

El bloqueo sigue sin resolver. Hace falta una prueba aislada de la etapa
de mezcla con estos buffers para distinguir lecturas, escrituras de
framebuffer, máscaras y sincronización; otra comparación visual de ROM
en Lavapipe no reproduce ese fallo de ejecución PS4.
