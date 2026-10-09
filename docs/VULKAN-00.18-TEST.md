# 00.18: completar datos faltantes del reporte

Se analizaron los tres binarios y gpu-hang.txt entregados por el usuario
desde 00.17. CLRX en modo raw/Bonaire decodifica los shaders. Las regiones
solapadas de los tres archivos coinciden byte por byte. El rasterizador
termina su primer bloque de código en 0x2ba0; no contiene instrucciones
LDS, atómicos ni s_barrier. Esto no demuestra que terminara en hardware.

Su código lee descriptores de máscaras en offsets 0xa0 y 0xb0 de la tabla
COMPUTE_USER_DATA_3. El reporte 00.17 guardó solo hasta 0x7c: faltan ambos
descriptores y los datos de máscaras. La captura anterior fue insuficiente.
El análisis local está en build/hang-analysis/REPORT.md y los .s vecinos.

00.18 amplía tablas de 128 a 256 bytes y guarda snapshots de los buffers
referenciados. Cada lectura exige un BO vivo y mapeado, se limita al tamaño
restante, a NUM_RECORDS del candidato y a 16 MiB. Descriptores de textura
pueden tener formato distinto: estos snapshots son candidatos que deben
interpretarse junto con código, dirección y tamaño, no una reproducción
automática garantizada. Los datos se capturan después del bloqueo y pueden
contener escrituras parciales de la GPU.

Paquete: out/NPS64-00.18-vulkan-test.pkg. Sigue siendo DIAGNÓSTICO:
no incorpora una corrección confirmada del rasterizador y puede congelar
la consola. No añade trabajos ni escrituras a la GPU. 00.13 sigue intacto.

Para continuar el análisis se requieren los archivos de esa ejecución:
gpu-hang.txt, gpu-hang-shader-*.bin y los nuevos gpu-hang-data-*.bin,
todos de /data/NPS64. Conservarlos juntos. No borrar ROMs ni SRAM.

Validación: helper real de captura bajo ASan/UBSan incluyendo petición
UINT32_MAX al final de un BO, direcciones no mapeadas y fuera de rango;
parche idempotente, compilación cruzada y verificaciones del paquete.
No hay consola PS4 local para confirmar el comportamiento físico.
