# 00.32: shaders especializados y comprobacion del atlas GPU

PS4 00.31 completa la secuencia pero el usuario ve fondo negro y cuadros
blancos en movimiento. El log captura 36 texturas, 490 triangulos y todos los
materiales listos, sin rechazos de textura. El primer framebuffer tiene solo
3263 pixeles iluminados, frente a 148125 en la referencia CPU (resoluciones
distintas). Que el material este listo no demuestra que se dibuje correctamente.

En Linux/Lavapipe las texturas del castillo son visibles. No se ha reproducido
la diferencia PS4, ni se atribuye su causa definitivamente al compilador.

Ahora las ecuaciones de cabeza, castillo, SHADE y MODULATERGBA tienen pipelines
especializados al crearlos. El fragment shader recibe la seleccion de textura,
rectangulo del atlas y direccionamiento directamente por push constants, en vez
de transportarlos como interpoladores flat desde el vertex shader. Las variantes
de profundidad y blend siguen separadas, y todas se crean antes de los frames.
La ruta generica conserva los materiales adicionales.

Antes de dibujar se lee de vuelta la imagen GPU del atlas completo y se compara
byte a byte con la captura CPU. El log incluye atlas GPU readback, mismatches y
colored_texels. Una diferencia aborta con motivo explicito. Esta prueba comprueba
upload/tiling/readback; no acredita por si sola muestreo o fidelidad de la escena.

Se prueba MODULATERGBA con alpha de textura cero y opaco sobre una superficie
con profundidad: el texel transparente conserva el fondo y su Z. Es una puerta
para particulas dibujadas con triangulos. No implementa textured rectangles,
sprites/HUD, TEXEL1/LOD, cobertura RDP ni todos los modos de blender; no garantiza
que las particulas reales esten resueltas.

Validacion: pruebas de atlas, direccionamiento, combiner/alpha/profundidad,
secuencia cambiante y probe grafico, con Vulkan validation. ROM PAL suministrada
en presentacion con BENCH_SM64=1 BENCH_FRAMES=1900, 64 hashes comparados entre
cache y referencia sin cache. Docker no reproduce Liverpool; OpenOrbis produce
el PKG y la prueba de consola sigue pendiente.

Probar la misma presentacion y la cabeza. Conservar el log completo, especialmente
atlas GPU readback y pixels. El funcionamiento sigue siendo una secuencia
acotada de 64 frames, con Angrylion como referencia y recursos PS4 retenidos.
