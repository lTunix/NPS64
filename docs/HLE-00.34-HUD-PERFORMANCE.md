# 00.34: HUD, texturas anchas y reduccion de trabajo por frame

00.33 fue confirmada por el usuario en PS4: juego continuo Vulkan funciona,
pero alcanza aproximadamente 20-30 VI FPS de los 50 PAL, con graficos negros
y sin HUD N64. Esta entrega conserva CPU como referencia y fallback.

Cambios implementados:

- Captura textured rectangles, flipped rectangles y fill rectangles RDP,
  mezclados con triangulos en el orden original. COPY ajusta derivadas y
  alpha; FILL convierte color RGBA5551/RGBA8888. Los fills del Z image no
  se dibujan como overlays blancos. Se conserva scissor.
- Texturas hasta 128x128 ocupan celdas contiguas del atlas 512x512.
  La escena local del castillo usaba 80x20, antes rechazadas: los materiales
  sin textura pasan de 96 a 0 en los frames muestreados.
- Subida GPU solo de celdas modificadas del atlas; atlas sin cambios no
  necesita transferencia. El log informa atlas_dirty_cells y upload_bytes.
- Asociacion habitual de comandos a triangulos con cursor, evitando buscar
  desde cero por cada triangulo. Se conserva busqueda para orden diferente.
- Conversion y escalado CPU calculados una vez por nueva imagen GPU;
  VI duplicadas reutilizan los pixels escalados y filas repetidas se copian.

Uso: abrir Mario 64, pausar con L3+R3 y pulsar Triangulo para activar Vulkan.
Pausar y Triangulo nuevamente vuelve a CPU. Se mantienen audio, controles,
estados y fallback; no hace falta iniciar una prueba separada.

Validacion local con Docker/Lavapipe y validation layers:

- Mario 64 PAL, exterior del castillo: mas de 400 frames GPU nuevos,
  VI duplicadas, dos sesiones con un contexto, audio, pausa, estados y toggles.
  Captura visual muestra Mario texturizado, escenario y contadores del HUD.
- ASan/UBSan: orden de triangulos y rectangulos, COPY, flip, UV y atlas ancho.
- Renderer persistente: 1200 frames, uploads omitidos, todas las celdas,
  textura 80x20 cruzando celdas con lectura del ultimo texel, frames vacios.
- Regresiones de pixels del atlas, combiner/transparencia, profundidad,
  secuencias cambiantes y pruebas sinteticas de geometria.

Limites: no hay una medicion de FPS de esta version en PS4. El renderer aun
rasteriza Angrylion como referencia, espera una fence y lee la imagen GPU
por cada frame nuevo; esos costes siguen pendientes de reducir. Target
interno 256x256, atlas limitado a 64 celdas, hasta 10000 triangulos incluyendo
rectangulos. Filtros, LOD, dos texturas, framebuffer effects, clipping/VI y
blenders completos siguen pendientes. La captura local aun tiene defectos
visuales y no acredita fidelidad completa ni resolver todos los negros.
Docker no reproduce Liverpool/GNM. Confirmacion visual y rendimiento de
00.34 quedan pendientes de consola; se preservan los PKG anteriores.
