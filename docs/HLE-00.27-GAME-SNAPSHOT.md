# 00.27: bloque de materiales, colores y transparencia básica

El usuario confirmó que 00.26 ejecuta los 64 envíos en PS4, pero Mario queda
oscuro con reflejos y el fondo conserva colores de diagnóstico. Esta versión
amplía la captura de materiales; no afirma que ese aspecto esté corregido en
la consola hasta probar el nuevo PKG.

Cambios del bloque:

- Captura ordenada del combinador, ciclo, prim/env color y umbral alfa del
  estado RDP correspondiente a cada triángulo, incluso si no lleva textura.
- Ecuaciones comunes (A-B)*C+D en uno o dos ciclos con COMBINED, TEXEL0,
  PRIMITIVE, SHADE, ENVIRONMENT, ONE, ZERO y sus canales alfa compatibles.
- Conservación del color y alfa de los vértices calculados por el HLE,
  incluida su iluminación. Los materiales completos usan esos datos;
  los pendientes conservan la vista de diagnóstico o textura sin combinar.
- Descarte por umbral alfa y mezcla estándar SRC_ALPHA/ONE_MINUS_SRC_ALPHA
  para force_blend. La mezcla es una aproximación al blender RDP.
- Selección independiente de comparación y escritura Z; scissor por dibujo.
- Registro de materiales decodificados, listos, no soportados y texturas
  faltantes. Agrupación de dibujos consecutivos iguales, preservando orden.

El combinador usa floats y clamp; no reproduce exactamente el redondeo y
los valores intermedios de nueve bits del RDP. No se admiten TEXEL1,
LOD, ruido, key center/scale, K4/K5, alpha dither ni ciclos copy/fill en este
bloque. No es el blender completo: faltan niebla, cobertura/antialiasing y
ecuaciones de mezcla específicas. Tampoco se resuelven sprites, rectángulos,
framebuffers, tamaños fuera del atlas ni filtrado N64. La captura sigue
pausada, con hasta seis VI de avance para obtener un frame con dibujos.
Angrylion continúa dibujando durante el juego normal.

Pruebas Docker/Lavapipe, con validación Vulkan:

- Colores numéricos conocidos para textura*shade, primitiva y dos ciclos con
  primitiva/entorno. Selección de TEXEL1/ruido rechazada explícitamente.
- Mezcla alfa sobre negro, descarte de un triángulo delante de otro sin
  modificar su fondo, comparación Z sin escritura y comparación con escritura.
- Scissor del material. Cada caso repite 64 frames y comprueba píxeles.
- Regresiones de atlas/UV, profundidad y parser F3DEX2; ASan/UBSan en captura
  de materiales/geometría y parser.
- ROM local de Mario Kart PAL: captura de materiales, atlas, 64 repeticiones,
  presentación, retorno al juego y cierre. No hay ROM local de Mario 64.

Estas pruebas son de PC. La apariencia y estabilidad en Liverpool/GNM siguen
requiriendo la consola. Uso: Mario 64, escena 3D, L3+R3, Triángulo; Círculo
vuelve a pausa y otro Círculo reanuda. Una prueba por inicio de aplicación.
Registro: /data/NPS64/nps64.log, líneas materials supported/ready/unsupported/
missing_textures y snapshot returned. PASS comprueba repetibilidad y salida
no uniforme; no fidelidad completa ni FPS de juego en Vulkan.

Construcción: bash scripts/build_hle_game.sh. Prueba nueva:
bash scripts/test_hle_game_combiner.sh. Se mantienen los scripts de textura,
profundidad, prueba controlada y ROM local. Fuentes, hashes y auditoría de
PKG se generan como antes. Se conservan los paquetes 00.13, 00.25 y 00.26.

Siguiente bloque: separar el ciclo de vida del renderer para reutilizar
dispositivo, atlas y buffers, dibujar una secuencia limitada de frames y medir
reutilización antes de habilitar el juego continuo. Los pendientes completos
y la evidencia de Docker/PS4 se mantienen en TODO.md.
