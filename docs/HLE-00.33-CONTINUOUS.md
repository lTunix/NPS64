# 00.33: primer modo Vulkan continuo integrado al juego

El renderer ahora separa crear recursos, dibujar un frame y liberar contexto.
La prueba de 64 frames sigue disponible para regresiones nativas; la interfaz
PS4 activa un modo continuo desde la pausa, sin entrar en la secuencia de prueba.

Uso en consola:

1. Abrir Mario 64 y llegar a una escena con geometria.
2. L3+R3 abre la pausa. Triangulo activa Vulkan y reanuda el juego.
3. Jugar normalmente: controles y audio siguen en el bucle habitual.
4. Para volver a CPU, pausar con L3+R3 y pulsar Triangulo nuevamente.
5. Pausa, estados y retorno a biblioteca siguen usando el menu habitual.

La imagen de juego Vulkan se escala a 4:3, con el HUD del frontend. El target
interno sigue siendo 256x256; esta version no aumenta la resolucion ni completa
la emulacion de VI. Los materiales y comprobacion del atlas de 00.32 permanecen.

Buffers, atlas, descriptores y pipelines se crean una vez. Cada actualizacion
espera su fence antes de tocar memoria; un frame vuelve con la GPU terminada.
El contexto se conserva al cambiar a CPU, pausar y cerrar/reabrir una ROM.
En PS4 sigue retenido hasta terminar el proceso: no se da por corregido el
teardown de driver que fallo en las primeras pruebas. En host se libera y
recrea para comprobar el camino completo de destruccion.

Si no hay triangulos ni comandos RDP nuevos, se presenta de nuevo el ultimo
frame Vulkan; no se alterna con CPU en cada VI duplicado. Si hay comandos pero
no geometria capturable, se usa la referencia CPU y se cuenta el fallback.
Antes del primer frame GPU tambien se usa CPU. Un error Vulkan desactiva el
modo y deja motivo en el log; no promete recuperar un cuelgue global de GPU.

Angrylion sigue rasterizando como referencia. Esta version integra juego
continuo, no acredita una mejora de rendimiento ni una migracion sin CPU.
Textured rectangles, sprites/HUD N64, otros materiales, clipping, framebuffer
y retirada de Angrylion siguen en el plan. Falta confirmar la apariencia de
00.32/00.33 en PS4: las texturas visibles en host no demuestran fidelidad Liverpool.

Validacion local realizada con Vulkan validation/Lavapipe:

- Contexto persistente: 1200 frames con cambios de textura, frames vacios,
  rechazo de capacidad invalida, continuidad posterior y recreacion en host.
- Mario 64 PAL suministrado: mas de 500 frames GPU nuevos en primera sesion,
  mas de 500 VI duplicados reutilizados; segunda sesion con el mismo dispositivo.
- Pausa/reanudacion, cambios CPU/Vulkan, audio recibido por plataforma,
  estados guardado/cargado y salida de ambas sesiones.
- Regresiones de pixeles: combiner, atlas, profundidad, secuencia y probe.

La automatizacion usa BENCH_HLE_LIVE=1 BENCH_SM64=1 BENCH_FRAMES=1900 con
scripts/test_hle_game.sh y la ROM local. Son pruebas host sin vsync real;
los FPS de ese backend no son mediciones de PS4. Falta jugar al menos 15 minutos
en consola y comprobar audio, controles, texturas, transiciones y memoria.

Log: [hle-live] mode, GPU frames, unchanged RDP repeated_frames, CPU fallback
y session ended; atlas GPU readback conserva la comprobacion byte a byte.
