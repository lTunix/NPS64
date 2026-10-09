# 00.24: captura de geometría de un juego en Vulkan

00.23 pasó el puente F3DEX2 controlado en la PS4 del usuario. Esta versión
recupera la biblioteca y permite abrir ROMs con Ari64/Angrylion. Durante
la pausa, Triángulo dibuja con Vulkan una captura de los triángulos HLE
del último frame. Círculo vuelve a la pausa; Círculo otra vez reanuda.
Se permite una prueba Vulkan por inicio de la aplicación para mantener
acotados los recursos retenidos del driver. Reiniciar la app para otra.

Es una vista diagnóstica de geometría de juego real, no un renderer Vulkan
completo ni una mejora de rendimiento de juegos. Las texturas N64,
combinador, rectángulos/sprites, modos RDP, framebuffer y render targets
no se reproducen. Es normal ver siluetas blancas o colores incompletos.
El juego continúa rasterizado en CPU y conserva sus interrupciones y RDP.
La ejecución gráfica requiere RSP HLE; este perfil lo selecciona.

El núcleo ya trae su propio frontend HLE y detección de microcódigos.
Se añade un observador en bridge_add_triangle, después del culling y antes
de la codificación RDP. Así se reutilizan matrices, iluminación, clipping
y microcódigos ya implementados, sin confundir F3DEX/Fast3D con F3DEX2.
Se guarda posición proyectada y color de hasta 10.000 triángulos por frame.
La captura se reinicia antes de retro_run y se lee cuando éste retorna.
Si se supera el límite, el log muestra cuántos triángulos se omitieron.

La escala se estima desde los viewports observados, porque el output VI
de Angrylion puede ser 640x288 aunque la geometría use 320x240. Esta
estimación no sustituye el seguimiento de render targets necesario para
juegos con varios framebuffers o viewports. La profundidad se normaliza
para la vista; no replica todos los modos Z del RDP.

Vulkan usa el mismo vertex buffer y pipeline de la prueba, dibujando los
triángulos reales sin textura. Verifica salida no negra y que 64 snapshots
repetidos producen los mismos píxeles. Esto comprueba ejecución estable
de esta captura, no fidelidad de emulación. Los recursos del dispositivo
se retienen hasta salir del proceso, como en las pruebas PS4 anteriores.

En host, Mario Kart 64 PAL produjo 1.690 triángulos (5.070 vértices), sin
truncamiento, y 17.578 píxeles visibles. Pasaron 64 repeticiones sin errores
de validación Vulkan, presentación de la captura, vuelta a pausa y al juego,
y cierre del núcleo. También pasó de nuevo la prueba controlada 00.23 y
el parser con ASan/UBSan. Falta verificar este flujo en PS4.

Registro: /data/NPS64/nps64.log. Buscar [hle-game], snapshot returned=0,
result screen presented y PASS real game geometry -> Vulkan. Mantener
00.13 para volver al paquete estable. El identificador sigue NPSX00064.

Construcción: GPU_CORE_ONLY=1 bash scripts/build_gpu.sh crea el núcleo
base si falta; bash scripts/build_hle_game.sh genera el paquete. El script
trabaja en build/hle-game, sustituye sólo rdp_emit_bridge.o en una copia
del archivo estático y mantiene vendor y los paquetes 00.13/00.23.
Prueba de host: bash scripts/test_hle_game.sh "ruta a una ROM local".
No se incluyen ROMs ni capturas de geometría de juegos en el paquete.
