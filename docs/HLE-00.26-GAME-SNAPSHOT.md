# 00.26: texturas TMEM sobre geometría HLE

El usuario confirmó en PS4 que 00.25 muestra correctamente la geometría 3D
de Super Mario 64 PAL y completa los 64 envíos de la captura. Se conserva
esa vía gráfica y su selección de profundidad por triángulo.

00.26 añade una primera reproducción de texturas reales. El observador HLE
registra sólo triángulos que generan un comando RDP válido. Los relaciona
con sus comandos por un hash de todas sus palabras y consume coincidencias
en orden. Durante la prueba, el trabajador RDP 0 captura la textura antes
de ejecutar cada triángulo, cuando las cargas TMEM y los estados de tile
anteriores ya se aplicaron. No se usa el estado de textura del final del frame
para todos los dibujos. La publicación de entradas HLE usa release/acquire;
la captura se consulta después de que retro_run devuelve el control.

Se reutilizan las funciones de lectura de texels de Angrylion, incluida la
lectura con TLUT cuando está habilitada. Los texels RGBA se deduplican en
un atlas 512x512 con hasta 64 texturas de hasta 64x64. La vista Vulkan
aplica UV, origen del tile, shifts, clamp, repeat y mirror. Se conserva el
W de los vértices para interpolación con perspectiva cuando el estado
RDP la activa. Se mantiene el orden de dibujo agrupando solamente materiales
y estados de profundidad consecutivos iguales.

La captura de texels está desactivada durante la ejecución normal. Al pulsar
Triángulo en pausa se avanza hasta seis intervalos VI para obtener un frame
con geometría; algunos juegos no dibujan en cada VI. Por eso la imagen puede
avanzar ligeramente respecto del instante de pausa. Luego se limpia el audio
acumulado y se vuelve a la vista de resultados. Sigue habiendo una prueba por
inicio de app para acotar los objetos Vulkan retenidos.

Es una prueba de texturas, no un renderer N64 completo. Las superficies con
textura se muestran sin el combinador N64; faltan iluminación/modulación
completa, modos de transparencia/alpha test, niebla, LOD/multitextura,
rectángulos/sprites y render targets. El muestreo es nearest, no el filtro
N64. Se rechazan YUV, LOD no cero, texturas mayores de 64x64 y exceso de
texturas únicas: esos triángulos conservan colores de diagnóstico. La
normalización de Z y el seguimiento de viewport siguen siendo aproximados.
El juego continúa usando Ari64/Angrylion; esta captura no mejora sus FPS.

Pruebas de PC (Lavapipe; no equivalen a una PS4):

- ASan/UBSan: asociación ordenada, comandos repetidos, deduplicación, límite
  del atlas, rechazo de tamaños, desplazamiento UV y W de clip.
- Atlas con origen distinto de cero y textura RGBA 2x2: todos los píxeles
  del framebuffer comprobados en clamp, repeat y mirror, 64 repeticiones
  por caso y sin errores de validación Vulkan.
- Regresión de profundidad y rechazo de imágenes uniformes.
- Prueba F3DEX2 controlada, subida de textura, alfa, scissor y profundidad.
- Mario Kart PAL local: captura de textura real, 64 repeticiones, presentación
  y retorno a pausa/juego. No hay ROM local de Mario 64 para probar su escena.

Uso en PS4: abrir Mario 64, avanzar a una escena 3D, L3+R3 para pausar,
Triángulo para la captura; Círculo vuelve a pausa y otro Círculo reanuda.
Buscar en /data/NPS64/nps64.log la línea TMEM matched=..., unique_textures=...,
replayed_triangles=..., unsupported=..., seguida de snapshot returned=0.
El PASS comprueba repetibilidad y salida no uniforme; no fidelidad de emulación.

Construcción: bash scripts/build_hle_game.sh. Pruebas:
bash scripts/test_hle_game_texture.sh; bash scripts/test_hle_game_depth.sh;
bash scripts/test_hle_probe.sh; bash scripts/test_hle_game.sh "ROM local".
El script sustituye rdp_emit_bridge.o y n64video.o en una copia del núcleo.
Vendor y los paquetes anteriores quedan intactos. El identificador sigue
NPSX00064. No se incluyen ROMs, texturas ni capturas de juegos en las fuentes.
