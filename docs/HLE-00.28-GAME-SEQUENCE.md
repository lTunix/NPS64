# 00.28: secuencia de geometría y materiales reales en Vulkan

La prueba de pausa ahora avanza y presenta 64 frames reales del juego. Se crea
un dispositivo Vulkan y se reutilizan sus pipelines, descriptores, framebuffer,
buffer de vértices y atlas. Después de esperar el fence del frame anterior se
actualizan los vértices y las texturas; las barreras cambian el atlas entre
transferencia y muestreo. El número de triángulos y los materiales pueden
cambiar en cada frame. No se agrega el overlay de la prueba controlada.

Uso: abrir una escena 3D, L3+R3 y Triángulo. El contador muestra 1/64 a 64/64;
el stick y los controles del juego siguen entrando en los frames de la prueba.
Círculo termina la secuencia antes. Al acabar queda el último frame Vulkan:
Círculo vuelve a pausa y otro Círculo reanuda el juego. Una secuencia por inicio
de aplicación; los recursos PS4 se conservan hasta salir, como en las pruebas
anteriores. La prueba silencia el audio y puede avanzar hasta seis VI por frame
para obtener dibujos. No es juego continuo ni una medición de FPS Vulkan.

El usuario confirmó geometría real visible en 00.27, pero Mario sigue oscuro y
hay colores de diagnóstico. **Esta versión no declara corregidos esos materiales.**
Se conservan hasta 16 muestras distintas de mux/ciclo, SHADE, prim/env y tamaño
de textura en el log para distinguir un combinador pendiente de colores oscuros
o texturas ausentes. Los colores de diagnóstico aún indican superficies pendientes.
Siguen los límites de 00.27: combinador común aproximado, blender básico, atlas
de 64 texturas de hasta 64x64, sin TEXEL1/LOD, niebla, sprites/HUD ni tracking de
framebuffer. Angrylion sigue ejecutando el rasterizado de referencia de CPU.

Validación local Docker/Lavapipe con capa Vulkan:

- 120 frames alternan textura, cantidad de vértices y scissor; se comprueban
  píxeles numéricos y orden de callbacks. Detecta también errores después del
  frame 64. Segunda sesión nativa verifica cancelación y destrucción/recreación.
- ROM local de Mario Kart PAL: 64 frames cambiantes, un solo dispositivo,
  actualización de atlas y regreso al juego. No hay ROM local de Mario 64.
- Regresiones de combinador, atlas/UV, profundidad y F3DEX2; ASan/UBSan para
  parser y captura. OpenOrbis construye el ELF/SELF/PKG de PS4.

Lavapipe valida lógica y sincronización API en PC; no reproduce Liverpool/GNM.
La secuencia 00.28 sigue pendiente de prueba en PS4. El PASS de secuencia informa
frames y diferencias respecto al frame previo, sin afirmar fidelidad N64.

Registro: /data/NPS64/nps64.log. Guardar las líneas `[hle-game] material sample`,
`materials`, `TMEM` y `PASS Vulkan sequence frames=64 changed=...`, junto con
una descripción de la escena. `changed=0` puede ser una escena estática.

Comandos: `bash scripts/test_hle_game_sequence.sh`, los scripts de regresión,
`bash scripts/test_hle_game.sh "ROM local"` y `bash scripts/build_hle_game.sh`.
Siguiente bloque: corregir los materiales identificados por estas muestras y
ampliar cobertura antes de habilitar el renderer permanente.
