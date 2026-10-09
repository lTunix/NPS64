# 00.31: transiciones negras y materiales de la presentacion

El log PS4 00.30 contiene dos pruebas: la presentacion del castillo falla
porque el primer framebuffer Vulkan es negro uniforme; la cabeza de Mario
completa la prueba. La primera captura tiene 414 triangulos y 414 materiales
listos. No muestra un bloqueo GPU ni un rechazo por cantidad de poligonos.
La reproduccion local 00.30 de ausencia de geometria era otro caso distinto.

La secuencia ahora atraviesa frames uniformes dentro del limite de 64 frames.
Solo pasa si obtiene al menos una escena iluminada y no uniforme. Una
secuencia completamente negra falla al terminar; las pruebas estaticas siguen
rechazando imagenes uniformes. El PASS acredita ejecucion, no fidelidad visual.

Se agregan rutas directas para los combiners FC127E24:FFFFF9FC (textura por
SHADE, alpha SHADE) y FCFFFFFF:FFFE793C (SHADE), presentes en la captura del
castillo. Se conservan alpha compare, profundidad y scissor. La ruta directa
de la cabeza de Mario permanece. Casos numericos verifican ambos materiales.

El log registra pixels iluminados de la referencia CPU en los primeros ocho
frames y el ultimo. Esto permite distinguir una transicion negra real de una
diferencia del backend Vulkan; no se supone que la causa sea una transicion
sin comparar las imagenes de consola.

Validacion local: Mario 64 PAL, BENCH_SM64=1 BENCH_FRAMES=1900,
64 frames Vulkan cambiantes. Los 64 hashes coinciden con la ejecucion sin
cache; 1231 decodificaciones frente a 42921. Esta ejecucion no reprodujo el
primer framebuffer negro de PS4. Pruebas sinteticas: dos frames negros seguidos
por seis visibles pasan; ocho negros fallan; se conserva cancelacion y el
frame de 30000 vertices. Pruebas de materiales, profundidad y probe grafico.

Docker usa Lavapipe, no Liverpool. OpenOrbis construye el PKG; la correccion
de la presentacion requiere validacion PS4. Se mantiene la prueba acotada y
la referencia Angrylion, con recursos GPU retenidos hasta salir de la app.

Uso: iniciar Mario 64, pausar con L3+R3 y pulsar Triangulo durante la
presentacion. Conservar nps64.log completo, incluyendo CPU reference,
pixels y scene_frames/uniform_frames. El log alternativo, si falla el
principal, sigue siendo /data/NPS64/hle-error.log.
