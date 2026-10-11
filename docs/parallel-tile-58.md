# ParaLLEl 00.58: varias primitivas y atómicos concurrentes

Los logs de PS4 00.57 confirman 64 envíos completos del caso 3 (sin memoria
compartida, con atómicos) y 64 del caso 4 (sin memoria compartida, escrituras
directas). En ambos se verificaron los cinco MiB de resultados, entradas
y guardas. No aparece ningún fallo. Estos resultados acotan el bloqueo
del shader original a una ruta que las variantes directas no utilizan;
no identifican una instrucción LDS concreta como causa demostrada.

00.58 conserva los cuatro casos y añade el **caso 5**, seleccionado al
abrir la aplicación. Utiliza exactamente el shader del caso 3 aprobado
en hardware, con una captura sintética más amplia:

- 0, 1, 31, 32, 33, 63, 65, 255 y 256 primitivas.
- Hasta ocho grupos escribiendo distintas palabras de máscara y
  modificando distintos bits del mismo coarse mediante atómicos.
- Scissors de pantalla completa y cuatro cuadrantes, alternados para
  producir máscaras distintas y ejercitar los 32 bits de cada grupo.
- 64 envíos, con comparación completa de resultados y guardas.

Las primitivas se generan copiando el rectángulo de la captura preparada.
La referencia CPU usa la cobertura rectangular y los límites de cuadrante
alineados a tiles; no ejecuta ni copia el algoritmo GLSL de binning.
El rectángulo FILL/COPY capturado incluye el límite horizontal del scissor:
la columna de tiles 20 pertenece a ambas mitades. La referencia conserva
este detalle en vez de tratar los cuadrantes como regiones excluyentes.
El caso 4 sigue limitado a una sola primitiva/grupo: sus escrituras directas
no se utilizan en esta prueba concurrente.

Pasaron 320 despachos locales con validación Vulkan, comprobación completa
de buffers y guardas, y ambos patrones para todos los conteos. También
compilaron los cinco casos con RADV/ACO para BONAIRE/GFX7. Estas son
comprobaciones locales. La prueba de concurrencia requiere ejecutar el PKG en PS4.
Esta versión no cambia el renderizador usado por las ROMs ni el núcleo CPU.

Ejecutar únicamente el caso 5 y conservar `tile-probe.log` y `boot.log`.
Tras superar este límite se podrá integrar el binning sin memoria
compartida en una prueba completa de ParaLLEl-RDP con ROMs, manteniendo
los atómicos necesarios para múltiples grupos.
