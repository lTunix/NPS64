# Audio y arranque de Mario Tennis — 00.17

Checkpoint previo: iconos 00.15, commit local `9fad7ac`, sin push. Esta prueba incluye la corrección SRAM de 00.16; sus nueve archivos originales en PS4 se conservaron y se respaldaron localmente.

## Audio

La cola sigue siendo de 8192 frames estéreo, con prellenado de 5760 y salida AudioOut de 1024 frames a 48 kHz. Se conserva el resampler sinc32.

El consumidor añade WSOLA: ventanas solapadas y búsqueda de alineación de ±384 muestras. Su ritmo sigue la ocupación filtrada de la cola, entre 0.88 y 1.08, sin modificar la velocidad de emulación ni los relojes VI/AI. El avance temporal es independiente del desplazamiento que elija la correlación. Así se pueden absorber caídas pequeñas de velocidad y deriva del dispositivo manteniendo el tono. Si el déficit supera ese margen, se cuenta el underrun y se atenúa la transición al silencio; no se oculta la interrupción. No aumenta la capacidad del buffer. Pausar/cambiar juego reinicia su historial.

El registro `[audio]` añade `stretch=wsola tempo=...`, conservando ocupación, underruns, descartes y frecuencia efectiva de AudioOut. La confirmación subjetiva de calidad en PS4 sigue pendiente; no se atribuye a esta corrección una mejora de todos los defectos del HLE ni de las limitaciones de las muestras originales.

En el tramo inicial capturado de Majora, HLE y LLE generaron PCM idéntico (900 VI, SHA-256 873bca798eb8a9276906d3d3be40582cad2f7da440729074856b0b752b544acf tras sinc32). Por ello se conserva HLE y se trabaja sobre la entrega al dispositivo, sin cambiar el motor de audio por defecto.

## Mario Tennis

El volcado de cartucho aportado por el usuario (`Mario Tennis (USA).n64`, MD5 normalizado 759358fad1ed5ae31dcb2001a07f2fe5) mostró el mismo bloqueo gráfico que VC. La traza identificó una lista de inicialización con FULLSYNC enviada mientras `RDP::device_supported` era falso: `CoreDoCommand(EXECUTE)` avanzaba hasta el primer VI durante la creación de contexto, antes de `parallel_init`. La lista se descartaba y el juego esperaba para siempre su interrupción DP. No era una espera de lectura de ROM.

`scripts/patch_vulkan_boot.py` separa ese arranque: inicializa y enciende la máquina, retorna antes de ejecutar su CPU cuando el renderer es paraLLEl, prepara Vulkan y arranca la CPU en el primer `retro_run`. La ruta CPU conserva su comportamiento. No se fabrican interrupciones ni se eliminan comandos gráficos.

La edición VC verificada (MD5 d40fb0e87996af902eff9c194027fc3b) requiere también reemplazar en memoria la instrucción reservada 0x7c000000 en el slot de bootstrap 0x1070 por NOP. La modificación está limitada a tamaño, digest, cabecera y CRC de payload conocidos; el CRC de cabecera se recalcula después. La identidad del contenido se retiene para las partidas y el archivo ROM original no se modifica. Ese ajuste solo no resolvía el bloqueo DP; ahora se valida junto al orden de arranque.

## Validación

- SRAM: pruebas con ASan/UBSan, metadatos de tamaño incorrectos, lectura transaccional, archivos cortos/largos/vacíos/ausentes y nueve copias reales de PS4. Estados corruptos y persistencia mantienen sus comprobaciones.
- Audio: tonos de 100, 997 y 4000 Hz a ritmos 1.00/0.92/1.02/0.97, unos 50 segundos por caso. Sin underruns ni descartes; tono dentro de 3 Hz y transiciones acotadas. Jitter PAL y pausas periódicas de 40 ms; reset/rebuffer; estéreo; prueba del resampler. ASan/UBSan limpios en estas unidades.
- Backend PS4 simulado: buffers retenidos hasta que AudioOut los termina, envíos continuos, rampas y estéreo, cierre, prioridad aceptada/denegada. Esto no mide carga de CPU ni calidad perceptual en hardware.
- Tennis USA: 900 VI, imagen visible desde VI 34, 415293 valores de audio no nulos. Tennis VC: 900 VI, 556100 valores no nulos. Imagen de la intro inspeccionada. Dos cargas de VC en el mismo proceso vuelven a generar imagen y a cargar SRAM; el arranque CPU se registra después de Vulkan activo.
- Majora: sesión Vulkan nativa con validación, shaders asíncronos, más de 180 VI, límite de 30 s y reanudación bloqueada. Sin ASan ni VUID. Persisten 40 reportes UBSan conocidos ajenos a Vulkan; el núcleo completo no se declara limpio de UBSan. Tennis también expone avisos aritméticos existentes en su camino HLE.
- Archivo PS4 comparado por miembros con 00.16: cambian únicamente `libretro.o`, `main.o`, `rom.o`. Objetos de renderer, shaders y perfil RADV/ACO idénticos. Guardas de recursos y límite de sesión sin cambios.
- PKG: ELF, constructores, 28 comprobaciones, payload extraído idéntico, icono, SFO y hashes.

Pendiente: instalar 00.17 en PS4 y confirmar SRAM entre sesiones, arranque/jugabilidad de Tennis y escucha de Majora/otros juegos. No se publica, etiqueta ni sube ningún commit a GitHub. Los ROMs y capturas PCM locales no se empaquetan ni se versionan.

## Confirmación en PS4

El usuario confirmó que Mario Tennis funciona y que el problema de SRAM está corregido. La calidad de audio de Majora's Mask sigue siendo mala; la modificación de entrega de audio no se considera una solución de ese problema. Se guarda este checkpoint y se detienen los cambios a petición del usuario, a la espera de nuevas indicaciones.
