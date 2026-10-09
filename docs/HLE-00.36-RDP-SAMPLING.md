# 00.36: muestreo RDP y texto en GPU

Resultado PS4 del usuario: 00.35 mejora, pero texturas aun extranas y un
dialogo activa fallback. No hay log nuevo de esa partida en la raiz al
hacer esta entrega: el motivo preciso de ese dialogo no se da por resuelto.

Referencia revisada: Dolphin upstream, VideoCommon/PixelShaderGen.cpp
https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/PixelShaderGen.cpp
Su ruta manual usa texelFetch, wrap/mirror entero y filtrado explicito.
VideoCommon/TextureCacheBase.cpp conserva dimensiones, formato, contenido
y estado de memoria/paletas en sus entradas:
https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/TextureCacheBase.cpp
Se adapta el enfoque; TEV/GameCube/Wii no equivale al RDP de N64.
Las formulas RDP se contrastan con el tex.c de Angrylion fijado en vendor,
no se sustituye su filtro por el filtro de GameCube.

Implementado:

- Estado por material: limites de clamp, periodos de mascara S/T,
  mirror, filtro y midpoint. No se reduce clamp+mask a un unico wrap.
- Muestreo manual de texels RGBA: coordenadas enteras, negativos,
  clamp antes de mask/mirror y atlas sin muestreo de la celda vecina.
- Interpolacion triangular de tres puntos N64, fracciones de cinco bits,
  redondeo y promedio de cuatro muestras en midpoint.
- Atlas empaquetado en unidades de 8x8, hasta 256 texturas distintas;
  un glifo 8x16 deja de reservar 64x64. Sigue siendo atlas 512x512 y
  textura individual hasta 128x128. No se garantiza capacidad ilimitada.
- Rectangulos en framebuffer con viewport 3D conocido usan su altura,
  no el limite inferior del scissor. El scissor se renormaliza con ella.
- Fallback conserva proteccion y registra primera causa: dibujo sin
  captura, textura rechazada/capacidad, mux/ciclo y primera muestra.

Pruebas:

- Oraculo CPU entero vs pixels GPU de repeat/mirror/clamp, UV negativos,
  tres puntos y midpoint.
- 180 glifos pequenos conservan sus texels en atlas; 128 glifos distintos
  por comandos RDP COPY se renderizan en GPU con alpha holes, orden y
  comparacion de todos los pixels. Prueba sintetica, no dialogo de ROM.
- ASan/UBSan: geometria, UV, atlas y scissor de dialogo contra viewport.
- Mario 64 PAL continuo en host: mas de 400 frames GPU, audio, pausa,
  estados, cambios CPU/Vulkan y dos sesiones con el mismo contexto.
- Regresiones: materiales/alpha, contexto persistente y secuencias.

Uso PS4: pausa L3+R3, Triangulo activa/desactiva Vulkan. Conservar el
fallback para modos pendientes. Revisar fallback reason / first rejected
draw y live atlas verify del log si un dialogo vuelve a CPU.

Limites pendientes: fidelidad de VI/clipping, render targets sin viewport
3D, TMEM/LOD/TEXEL1, blenders/fog completos y ese dialogo real reproducido.
Estas pruebas no establecen fidelidad completa de SM64 ni de otros juegos.
Angrylion sigue como referencia; no se retira rasterizado CPU ni se promete
una mejora de FPS. Docker/Lavapipe no reproduce Liverpool/GNM.
