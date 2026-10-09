# Prueba Vulkan 00.22: texturas y transparencia

Parte de la ruta de gráficos que pasó en la PS4 del usuario con 00.21.
Mantiene el dispositivo Vulkan vivo durante la pantalla de resultado para
evitar el cierre que sucedía tras el PASS en 00.20. Todavía no es un
renderer de juegos N64 y no carga ROMs.

Primero sube una textura RGBA de 256 x 256 desde memoria coherente a una
imagen óptima de GPU, con las transiciones y barreras correspondientes.
Usa un descriptor de imagen y sampler nearest/clamp. Ejecuta 64 frames de
triángulos sólidos con profundidad y después 64 con textura de cuadros
rojos y amarillos, coordenadas UV interpoladas y modulación por color.
Un triángulo azul detrás debe ser rechazado por la profundidad. Una capa
verde al 50% se mezcla sobre la mitad derecha usando alpha blending y
scissor. Comprueba todos los píxeles de cada frame texturado, incluidos
fondo, cobertura, patrón, transparencia y alfa. Tolera un nivel UNORM de
redondeo en la mezcla.

La pantalla final muestra el patrón y PASS si todas las comprobaciones
terminan. El registro sigue en /data/NPS64/hle-probe.log. Hay un envío para
subir la textura y 128 envíos de dibujo. No se envía un dispatch compute
explícito desde la aplicación; el driver puede emplear operaciones internas.

Validación en Lavapipe con la capa Vulkan: 128 frames, sin errores de
validación; 20.808 píxeles cubiertos y 44.728 de fondo. Esta validación no
demuestra que las nuevas funciones pasen en PS4: requieren prueba real.
El paquete usa NPSX00064 y reemplaza la instalación anterior. Conservar
00.13 para jugar y 00.21 como prueba gráfica que ya funcionó en consola.

Siguen pendientes el backend HLE de display lists, conversión de texturas
N64, combinador, modos de render y framebuffers de juegos. Esta prueba
valida primitivas necesarias; no afirma compatibilidad con juegos.
