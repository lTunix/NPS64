# 00.25: profundidad por triángulo y diagnóstico de geometría

La captura 00.24 de Super Mario 64 PAL completaba los 64 envíos GPU en
la PS4 del usuario, pero mostraba un cuadro blanco. El PASS comprobaba
repetibilidad y salida no negra; no demostraba que la escena fuera correcta.

00.25 guarda el indicador z_buffered del recorrido HLE por triángulo.
Vulkan selecciona el pipeline con o sin prueba/escritura de profundidad,
agrupando únicamente triángulos consecutivos con el mismo indicador para
preservar el orden original. Antes se aplicaba profundidad a toda la captura,
incluidos los triángulos sin profundidad situados en Z=0. Era un error que
podía ocultar geometría posterior; todavía no se ha confirmado que explique
por completo el cuadro blanco de Mario 64.

La vista usa colores de diagnóstico distintos por triángulo. No son los
colores del juego: permiten distinguir superficies mientras faltan las
texturas N64 y el combinador. Se conservan los colores originales en la
captura interna. El validador rechaza una imagen completamente uniforme,
además de comprobar las 64 repeticiones. Ese PASS sigue siendo diagnóstico,
no una prueba de fidelidad de emulación.

Limitaciones: z_buffered no reproduce todos los modos Z_COMPARE/Z_UPDATE
del RDP. La normalización de profundidad y la estimación del viewport siguen
siendo aproximadas. No se reproducen texturas, sprites/rectángulos, combinador,
render targets ni todos los estados RDP. Los juegos siguen usando Ari64 y
Angrylion en CPU; esta versión no ofrece un renderer Vulkan completo ni
promete mejorar sus FPS. Se mantiene desactivado paraLLEl-RDP compute.

Uso: abrir Mario 64, avanzar a una escena 3D, pausar con L3+R3 y pulsar
Triángulo. Debe aparecer una captura de geometría con colores de diagnóstico.
Círculo vuelve a la pausa y otro Círculo reanuda. Se permite una prueba por
inicio de la aplicación para acotar los recursos Vulkan retenidos, necesarios
para evitar el cierre observado en las primeras pruebas PS4.

Validación de PC con Lavapipe y la capa de validación Vulkan:

- Fondo blanco sin profundidad seguido de triángulo rojo con profundidad:
  centro rojo y esquina blanca comprobados por lectura de píxeles.
- Superposición verde sin profundidad posterior: respeta el orden de dibujo.
- Fondo uniforme: rechazado, aunque sea no negro.
- Prueba controlada F3DEX2, texturas, alfa, scissor y profundidad: todos los
  píxeles comprobados; parser con ASan/UBSan.
- Mario Kart 64 PAL local: 1.690 triángulos, 5.070 vértices, 17.578 píxeles
  visibles y no uniformes, 64 repeticiones, regreso a pausa/juego y cierre.

Estas pruebas no sustituyen la validación en PS4. No hay ROM local de Mario
64 para reproducir la captura exacta del usuario. Registro de consola:
/data/NPS64/nps64.log; buscar depth_enabled/depth_disabled,
nonuniform_pixels, snapshot returned y result screen presented.

Construcción: bash scripts/build_hle_game.sh. Pruebas:
bash scripts/test_hle_game_depth.sh; bash scripts/test_hle_probe.sh;
bash scripts/test_hle_game.sh "ruta a una ROM local".
El paquete estable 00.13 se conserva. El identificador sigue NPSX00064.
No se incluyen ROMs ni capturas de juegos en el paquete o las fuentes.
