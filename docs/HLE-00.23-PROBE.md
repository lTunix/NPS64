# 00.23: primer puente F3DEX2 a Vulkan

00.22 pasó texturas, transparencia y profundidad en la PS4 del usuario.
00.23 sustituye los vértices constantes del shader por un búfer de vértices
alimentado por un decodificador de listas N64. La imagen final es parecida
a 00.22; el cambio es la procedencia de la geometría.

El decodificador acepta este subconjunto F3DEX2: G_MTX (load/multiply,
modelview/projection, push), G_POPMTX, G_VTX, G_TRI1, G_TRI2, G_MOVEWORD
para segmentos, G_DL (call/branch), G_ENDDL y NOOP. Sus layouts se comprobaron
contra el F3DEX2.c incluido en vendor/parallel-n64/gles2n64/src. Lee matrices
N64 de punto fijo 16.16 y vértices de 16 bytes: posición, RGBA y ST. Aplica
matrices con vectores fila, conserva w y convierte la profundidad de clip
N64 a Vulkan. Las coordenadas ST se normalizan con el tamaño de textura
proporcionado por el llamador; el estado G_TEXTURE/tile sigue pendiente.

Acepta RDRAM big endian y el formato con palabras intercambiadas del núcleo.
Valida tamaños, direcciones, caché de vértices, capacidad de salida y pilas;
limita comandos para terminar listas cíclicas. Los opcodes no implementados
devuelven un error explícito. No debe usarse como decoder completo de juegos.

La prueba crea una lista controlada (no extraída de una ROM): define un
segmento, llama otra lista, carga matriz y vértices, emite un triángulo y
retorna. El resultado se carga en un vertex buffer Vulkan con posición,
color y UV. Sigue comprobando 64 frames sólidos y 64 texturados con mezcla,
scissor y profundidad. Los offsets de profundidad por draw son parte de
la prueba controlada, no estados de render N64 implementados.

El log debe mostrar:

    F3DEX2 decode status=0 commands=7 triangles=1 vertices=3
    PASS F3DEX2 display list -> matrix/vertices -> Vulkan triangles
    result screen presented

Conserva recursos Vulkan durante la pantalla como 00.21/00.22. Todavía no
conecta ProcessDList del núcleo ni carga juegos. Falta microcode detection,
iluminación, geometría, viewport/culling, estados de texturas N64, combinador,
modos RDP, framebuffers y la integración con el plugin y la biblioteca.

Validación: tests ASan/UBSan del parser (endian, matrices, UV/color, segmentos,
TRI1/TRI2, listas anidadas y entradas inválidas); 128 frames en Lavapipe con
validación Vulkan y verificación del framebuffer completo. Falta validar
el nuevo vertex buffer y decoder en PS4. El PKG sustituye NPSX00064; conservar
00.13 para jugar y las pruebas 00.21/00.22 que funcionaron en consola.
