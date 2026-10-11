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
