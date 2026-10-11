# 00.63: aislar rasterización y mezcla

Los logs de PS4 de 00.62 confirman que se habilitó el binning predicado.
El envío 331 se bloqueó con el contador de trazas en 8, antes de completar
el CS_PARTIAL_FLUSH posterior al tercer dispatch. Los marcadores 6 y 7
implican que terminó la espera posterior al binning. El marcador 8 solo
confirma progreso del procesador de comandos, no finalización del shader.
El tercer dispatch usa el ubershader original, grupos 8x8, 40x30 grupos.
No se detectaron solapamientos físicos entre los 47 buffers.

00.63 prueba la ruta original separada de ParaLLEl: rasterización por
variantes, seguida de mezcla de profundidad/color. Conserva el binning
predicado sin LDS y agrega la generación original de listas de trabajo,
offsets e indirect counts que necesita esta ruta. Mantiene texturas,
combinador, framebuffer y VI de ParaLLEl. No es un fallback CPU.

Esta prueba cambia la ruta de ejecución para aislar el bloqueo; no demuestra
un defecto específico del compilador ni garantiza resolver todas las etapas
en PS4. La sincronización conservadora permanece activa.

Paquete separado: `out/NPS64-00.63-parallel-rdp-test.pkg`.
Log: `/data/NPS64-Parallel/nps64.log`.

Validación: ambos shaders de binning pasan SPIR-V validation. Mario PAL
completó 650 cuadros locales con la ruta separada. Las cuatro capturas
150/300/480/600 tienen el mismo estado VI y dimensiones que la referencia
CPU que reproduce los mismos comandos RDP; error máximo 1/255 por canal.
Comparación del cuadro 600 revisada visualmente. Reporte en
`build/parallel-comparison/mario-63-split/index.html`.
El PKG pasó 28 comprobaciones y CPU 00.51 conserva su hash original.
Estos resultados de Lavapipe no representan FPS ni ejecución confirmada
de la GPU PS4. Falta probar la ROM en consola.
