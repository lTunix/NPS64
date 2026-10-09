## Prioridad experimental: bloqueo ParaLLEl 00.53 / prueba aislada 00.54

- [x] Analizar logs 00.53: visible=0, 330 envios completos, bloqueo 331 con marcador 5.
- [x] Decodificar shaders y acotar binning/finalizacion; no atribuir WRITE_DATA a shader completado.
- [x] Aislar tile shader upstream y datos triangle/scissor externos de la captura.
- [x] Comparar grupos 32/64 sobre 1200 tiles; 128 fences y 5 MiB completos por envio.
- [x] Validar expectativa rectangular, guardas y cinco asignaciones independientes.
- [x] Compilar/verificar app 00.54 independiente: 28 controles; paquetes CPU51/00.53 intactos.
- [ ] Probar primero caso 2 (64 lanes) en PS4 y contrastar caso 1 original (32 lanes).
- [ ] Investigar dependencia span/setup y controlador segun resultados; solo integrar cambios verificados.

Ver [instrucciones 00.54](docs/PARALLEL-00.54-TILES.md).

## Integracion experimental: ParaLLEl 00.53

- [x] Confirmar en PS4 las siete pruebas compute de 00.52: 448 envios, 896 despachos.
- [x] Integrar juegos completos con Ari64, CXD4 y ParaLLEl-RDP original.
- [x] Separar app, configuracion, partidas y logs; conservar biblioteca y CPU51.
- [x] Capturar comandos RDP y comparar imagenes con Angrylion antes de importar resultados GPU.
- [x] Validar independencia de referencia con framebuffer y cobertura GPU corruptos sinteticos.
- [x] Comparar siete imagenes de cabeza/menu Mario: diferencia maxima 1 por canal.
- [x] Construir PKG 00.53, verificar 28 controles e identidad; CPU51 sin cambios.
- [ ] Confirmar juegos completos, cambios de escena y cierre en PS4 real.
- [ ] Ampliar comparaciones a castillo, particulas, Zelda, GoldenEye y Tennis.
- [ ] Medir FPS y audio en PS4; optimizar lecturas GPU/CPU y sincronizacion segun resultados.

Ver [instrucciones 00.53](docs/PARALLEL-00.53-GAMES.md).

## Historial experimental: ParaLLEl 00.52

- [x] Crear app PS4 independiente sin ROM ni core, conservando CPU51.
- [x] Implementar siete casos: buffers, 1D/2D/3D, LDS, atomicos, barreras e indirectos.
- [x] Validar 448 fences y 896 despachos, memoria completa y guardas en Lavapipe.
- [x] Compilar PKG PS4 independiente y verificar sus 28 controles.
- [x] Confirmar cada caso en PS4 real; recoger parallel-probe.log y boot.log.
- [ ] Ampliar pruebas de texturas compute y reproducir cargas RDP originales.
- [ ] Integrar juegos solo tras validar estabilidad, fidelidad y rendimiento.

Ver [instrucciones 00.52](docs/PARALLEL-00.52-COMPUTE.md).

## Prioridad activa: CPU 00.51, temporizacion y rasterizado

- [x] Analizar por separado las sesiones CPU50 de GoldenEye, Tennis y Mario.
- [x] Evaluar Jaguar sin ampliar SSE2; descartar resultados inconsistentes.
- [x] Comparar candidata Jaguar A-B-B-A y 96 buffers completos.
- [x] Corregir perdida de deuda de temporizacion; validar reloj a 50/60 Hz.
- [x] Comparar 9722 imagenes y 325 estados GoldenEye/Tennis; backend/audio.
- [x] Construir y verificar PKG CPU51: 28 controles y payload igual al stage.
- [x] Empaquetar fuentes CPU51: 210 archivos correspondientes, sin ROMs y SHA256.
- [ ] Medir en PS4 real las mismas escenas y conservar CPU50 como referencia.
- [ ] Reducir el coste de Angrylion en escenas que exceden el periodo completo.

## Futuro: renderizador CPU alternativo seleccionable

- [ ] Evaluar licencia, compatibilidad, fidelidad y coste de integracion de alternativas.
- [ ] Incorporar un segundo backend sin sustituir Angrylion.
- [ ] Permitir elegir backend en ajustes y persistir la seleccion por juego.
- [ ] Aplicar el cambio al reiniciar contenido y explicar diferencias de calidad.
- [ ] Validar texturas, profundidad, audio, estados y rendimiento en PS4.

## Historial: CPU 00.50, GoldenEye y Mario Tennis VC (2026-10-08)

- [x] Reproducir GoldenEye USA en intro, seleccion y Dam con controles fijos.
- [x] Descartar cambios que regresan el juego aunque mejoren algun menu.
- [x] Entrenar PGO completo con GoldenEye, Mario 64 y Zelda PAL GC.
- [x] Medir GoldenEye A-B-B-A: Dam 13.8-23.5 % menos tiempo nativo.
- [x] Completar dos pares de rendimiento de Mario y Zelda sin regresiones.
- [x] Reproducir bloqueo Mario Tennis con Ari64 e interprete; identificar IPL3 CRC.
- [x] Reparar solo la copia en memoria de la ROM verificada y conservar identidad.
- [x] Validar 1800 VIs de Mario Tennis y su imagen de presentacion animada.
- [x] Comparar 17159 cuadros y 574 trazas; 96 buffers de color/profundidad.
- [x] Completar sanitizadores y regresion de backend, prioridad y audio.
- [x] Construir y verificar PKG CPU50: 28 controles y payload igual al stage.
- [x] Empaquetar fuentes correspondientes y SHA256; comprobar 206 archivos sin ROMs.
- [ ] Confirmar VI/s en Dam, Mario Tennis y continuidad del audio en PS4 real.

Ver [CPU 00.50](docs/CPU-00.50.md).

## Historial: CPU 00.49, rasterizador Zelda PAL GC (2026-10-08)

- [x] Confirmar en log CPU48 que la seleccion esta limitada por RDP/core.
- [x] Reproducir seleccion de archivo con Start y ROM local, validar imagen.
- [x] Especializar spans de dos ciclos conservando aritmetica y filtros.
- [x] Entrenar trece funciones nuevas sin cambiar los contadores PGO anteriores.
- [x] Medir dos pares nativos: menu 17.5-26.7 % menos tiempo.
- [x] Comparar 3485 imagenes y 116 estados Zelda; 48 buffers de rasterizador.
- [x] Validar prioridad de render, rechazo del SDK y prioridad de audio con ASan/UBSan.
- [x] Completar regresion de Mario: 3952 imagenes y 133 estados identicos.
- [x] Construir y verificar PKG CPU49: 28 controles y payload igual al stage.
- [x] Empaquetar fuentes correspondientes y SHA256.
- [ ] Confirmar VI/s, RDP y continuidad del audio en PS4 real.

Ver [CPU 00.49](docs/CPU-00.49.md).

## Historial: CPU 00.48, Zelda PAL GC (2026-10-08)

- [x] Analizar la ultima sesion CPU47 del log real de PS4.
- [x] Separar limite del rasterizador y espera de presentacion.
- [x] Usar tres buffers CPU, cola acotada y esperar liberacion antes de reutilizar.
- [x] Validar memoria retenida, orden de flips y cierre con ASan/UBSan.
- [x] Conservar seis trabajadores, heap de 512 MiB y audio CPU46.
- [x] Descartar tabla de reciprocales: dos pares nativos fueron mas lentos.
- [x] Agregar pace_ms, sleep_over_ms y late al log.
- [x] Verificar 3485 imagenes y 116 estados CPU/VI iguales a CPU47.
- [x] Construir y verificar PKG CPU48 con 28 controles.
- [ ] Confirmar flip_ms y VI/s de CPU48 en intro y seleccion en PS4.
- [ ] Reducir el coste RDP de escenas que superan 20 ms por iteracion.

Ver [CPU 00.48](docs/CPU-00.48.md).

## Historial: CPU 00.47 Ocarina of Time PAL GC (2026-10-07)

Audio CPU46 confirmado por el usuario. Optimizar FPS sin modificar audio.

- [x] Reproducir intro con la ROM local PAL GC y obtener referencia de rendimiento.
- [x] Comprobar resto de linea por reciprocales con 64 strides y 8M valores aleatorios.
- [x] Descartar variantes globales que perjudican a Mario en las mediciones locales.
- [x] Seleccionar ruta por CRC de cabecera, incluyendo tres ordenes de bytes.
- [x] Conservar funcion anterior y contadores PGO anteriores para otros juegos.
- [x] Zelda: 3485 imagenes/116 estados; Mario: 3952 imagenes/133 estados identicos.
- [x] 48 comparaciones de framebuffer/profundidad y audio identico a CPU46.
- [x] Construir y verificar PKG 00.47 (28 controles); fuentes correspondientes.
- [ ] Confirmar mejora de VI/s y speed_pct en la intro de Zelda en PS4.

Ver [CPU 00.47](docs/CPU-00.47.md).

## Historial: CPU 00.46 AudioOut continuo (2026-10-07)

Log CPU45: 3 agotamientos y 35682 muestras descartadas; cola llena pese a ~50 VI/s.

- [x] Sustituir drain por bloque por entrega continua con dos buffers alternados.
- [x] Solicitar prioridad 400 para audio, conservando alternativa si se rechaza.
- [x] Drenar antes de destruir buffers; comprobar punteros retenidos en AudioOut simulado.
- [x] Publicar muestras por lotes y separar indices en lineas de cache.
- [x] Vectorizar FIR con SSE2 y comparar contra referencia escalar (diferencia <=1 S16).
- [x] Agrupar copia de video contiguo; 3952 imagenes/133 estados CPU/VI exactos.
- [x] Mantener seis workers, heap 512 MiB y prebuffer 120 ms.
- [x] Descartar ajuste de compilador sin mejora consistente.
- [x] Usuario confirma audio y funcionamiento mucho mejores en PS4 (sin nuevo log de cadencia).
- [ ] Medir ahorro de core_ms y FPS en escenas pesadas reales.

Ver [CPU 00.46](docs/CPU-00.46.md).

## Historial: CPU 00.45 audio filtrado (2026-10-07)

CPU44 recupera rendimiento en PS4; el usuario informa sonido sucio y microcortes.

- [x] Identificar conversion de frecuencia por repeticion de muestras.
- [x] Sustituirla por filtro FIR sinc de 32 taps y 256 fases.
- [x] Conservar estado entre batches y reiniciar historia al pausar/cambiar frecuencia.
- [x] Validar ocho frecuencias, batches variables, DC/estereo y reset con ASan/UBSan.
- [x] Medir error de seno y supresion de componente espuria contra conversion anterior.
- [x] Mantener seis trabajadores, heap 512 MiB y audio 120 ms.
- [ ] Confirmar nitidez y revisar contadores underruns/dropped en PS4.

Ver [CPU 00.45](docs/CPU-00.45.md).

## Historial: CPU 00.44 (2026-10-07)

00.43 pierde unos 10 FPS segun prueba del usuario. Restaurar configuracion CPU41.

- [x] Volver a seis trabajadores por defecto.
- [x] Volver a heap maximo de 512 MiB; mantener recuperacion ante fallos.
- [x] Conservar audio de 120 ms solicitado y optimizaciones CPU40.
- [x] Confirmar RDRAM de 8 MiB ya habilitada en el core y logs Mario 64.
- [x] Probar allocator y audio con ASan/UBSan.
- [ ] Confirmar recuperacion de FPS y continuidad de audio en PS4.

Ver [CPU 00.44](docs/CPU-00.44.md).

## Historial: CPU 00.43 (2026-10-07)

Por peticion del usuario: ocho trabajadores por defecto y mas RAM para la app.

- [x] Usar ocho trabajadores como predeterminado CPU PGO.
- [x] Ampliar heap directo de 512 MiB a un maximo de 1 GiB en CPU.
- [x] Mantener reserva de 2560 MiB y reducir heap si asignacion/mapeo falla.
- [x] Probar heap completo, limite por disponibilidad y dos rutas de fallo con ASan/UBSan.
- [x] Mantener audio con prebuffer de 120 ms; prueba de jitter aprobada.
- [ ] Confirmar workers efectivos, heap y continuidad del audio en PS4.

Ver [CPU 00.43](docs/CPU-00.43.md).

## Historial: CPU 00.42 audio 120 ms (2026-10-07)

00.41 mejora el audio en PS4; el usuario solicita mas reserva para microcortes.

- [x] Aumentar prebuffer de 3072 a 5760 muestras: 120 ms a 48 kHz.
- [x] Mantener bloques de 1024, filtro VI y perfiles CPU40.
- [x] Validar orden y limites, incluido wrap y rebuffer con reserva no multiplo del bloque.
- [x] Simular 20 segundos PAL con jitter y pausas periodicas de 40 ms; sin cortes ni descartes.
- [x] Comparar seis/siete/ocho trabajadores con entrada fija y orden invertido.
- [ ] Confirmar audio continuo y latencia aceptable en PS4.
- [ ] Confirmar 50 VI/s sostenidos en escenas pesadas de consola.

Ver [CPU 00.42](docs/CPU-00.42.md).

## Historial: CPU 00.41 audio (2026-10-07)

El usuario confirma casi 50 FPS sostenidos en PS4 con 00.40; audio entrecortado.

- [x] Conservar PGO, seis trabajadores y fidelidad del renderer CPU40.
- [x] Cambiar bloques AudioOut de 256 a 1024 muestras.
- [x] Esperar terminacion AudioOut antes de consumir muestras nuevas.
- [x] Reserva inicial de 3072 muestras; rebuffer tras agotamiento.
- [x] Limpiar cola desde consumidor al pausar/cargar/descargar.
- [x] Registrar reserva, agotamientos y muestras descartadas.
- [x] Prueba de 20 segundos con rafagas PAL y jitter, orden exacto y sin agotamiento.
- [x] ASan/UBSan: limites, wrap, reset, overflow y backend PS4.
- [x] PKG OpenOrbis validado: 28 controles y payload exacto.
- [ ] Confirmar continuidad de audio en PS4 y revisar contadores.
- [ ] Medir siete trabajadores frente a seis sin perjudicar audio ni frames.

Ver [CPU 00.41](docs/CPU-00.41.md).

## Historial: CPU 00.40 (2026-10-07)

Vulkan queda aparcado. Se conserva la fidelidad de Angrylion y el filtro VI.

- [x] Analizar el tramo 00.39 del log: RDP domina las escenas lentas.
- [x] Entrenar perfiles LLVM 14 del rasterizador con Mario 64 PAL y casos RDP.
- [x] Aplicar PGO solo al rasterizador, conservando O2 y SSE2.
- [x] Comparar cuatro y seis trabajadores; usar seis por defecto.
- [x] Validar 3952 imagenes y 133 estados CPU/VI exactos, con cuatro y seis trabajadores.
- [x] Validar 48 casos RDP y copia VideoOut con ASan/UBSan.
- [x] Separar tiempos de espera de flip y copia en el log.
- [x] Construir PKG 00.40 con OpenOrbis; conservar 00.13 y 00.39.
- [ ] Confirmar mejora y estabilidad en PS4, revisando workers efectivos y scanout.
- [ ] Optimizar siguientes puntos calientes segun mediciones reales.
- [ ] Verificar 50 VI/s PAL sostenidos en consola.

Ver [CPU 00.40](docs/CPU-00.40.md). Docker no reproduce el hardware PS4.

## Historial: CPU 00.39 (2026-10-07)

Vulkan queda aparcado por las regresiones observadas en PS4. Las listas
Vulkan siguientes conservan el historial; no son trabajo activo.

- [x] Compilar solo Ari64 + Angrylion, ignorando configuraciones GPU antiguas.
- [x] Optimizar escalado/conversion con SSE2 conservando cada pixel.
- [x] Validar limites con ASan/UBSan y comparacion contra escalado escalar.
- [x] Comparar Mario 64: 3952 imagenes y 133 estados CPU/VI identicos.
- [x] Construir PKG CPU 00.39 y preservar PKG estable 00.13.
- [ ] Confirmar ahorro de draw_ms y estabilidad en PS4 con nps64.log.
- [ ] Perfilar escenas pesadas y comparar 2/3/4 workers en PS4.
- [ ] Optimizar el coste dominante medido del RDP/VI sin perder fidelidad.
- [ ] Verificar rendimiento sostenido PAL de 50 VI/s en consola real.

Ver [CPU 00.39](docs/CPU-00.39.md).

## Estado 00.38 (2026-10-07)

- [x] Rasterizacion de pantalla sin huecos negros; perspectiva UV conservada.
- [x] Filtro VI de CPU bajo demanda: solo referencia, fallback o modos no admitidos.
- [x] 1738 hashes coinciden con VI CPU completa, incluido fallback forzado en 2839.
- [x] Validar audio, estados, transiciones, dos sesiones y retorno CPU/Vulkan.
- [x] Medir tres modos: ahorro en VI, sin mejora clara de FPS total en Docker.
- [ ] Retirar spans RDP CPU conservando RDRAM y fallback; aun no implementado.
- [ ] Confirmar fidelidad y 50 FPS en PS4 real.
Ver [alcance y pruebas](docs/HLE-00.38-DEFERRED-VI.md).

## Correccion local posterior a 00.37 (2026-10-07)

- [x] Asociar dumps de geometria y atlas al framebuffer mostrado por la VI.
- [x] Eliminar huecos negros conservando perspectiva de texturas.
- [x] Medir CPU/Vulkan con entradas iguales y sin capturas ni limitador.
- [ ] Retirar rasterizacion CPU redundante conservando coherencia RDRAM y fallback.
- [ ] Completar filtrado VI, cobertura y materiales pendientes.
- [ ] Confirmar 50 FPS en Liverpool real; Docker/Lavapipe no predice PS4.
Ver [resultados](docs/HLE-SCREEN-SPACE-PERFORMANCE.md). PKG 00.37 preservado.

## Estado 00.37 (2026-10-07)

- [x] Corregir UV RSP duplicadas (rectangulos conservan su escala).
- [x] Seleccionar framebuffer por VI_ORIGIN, conservando tres imagenes GPU.
- [x] Presentar margenes/pasos VI PAL y corregir desfase del contador de monedas.
- [x] Aproximar cobertura alpha: monedas/particulas sin cuadrados opacos.
- [x] Capturar escena/atlas/VI y comparar contra Angrylion localmente.
- [x] Eliminar huecos negros de ambos bordes: espacio de pantalla y UV perspectivas separadas (validacion local).
- [ ] Completar materiales, cobertura/blender y filtro VI/AA.
- [ ] Confirmar estas correcciones en PS4; referencia CPU aun activa.
Ver [evidencia y limites](docs/HLE-00.37-VI-TEXTURES.md).

## Validacion diferencial local (2026-10-07)

00.36 reportada por el usuario como regresion grafica en PS4. No validada.

- [x] Capturas CPU/Vulkan del mismo retro_run antes del HUD, sin avanzar otra VI.
- [x] Informe por imagen: error RGB, porcentaje sobre umbral y vista CPU/GPU/diferencia.
- [x] Ejecutar ROM real: diferencias visibles de viewport, HUD y texturas.
- [ ] Aislar la regresion 00.35 -> 00.36 con capturas comparables.
- [ ] Corregir VI/viewport antes de usar error por pixel como medida de texturas.
- [x] Binario PS4 en shadPS4 llega al menu; video/audio/mando inicializados.
- [ ] Automatizar ROM/Vulkan en shadPS4; no sustituye Liverpool/GNM real.

## Estado 00.36 (2026-10-07)

00.35 mejora en PS4 segun usuario; texturas extranas y dialogo con fallback.

- [x] Revisar muestreo manual y estado de texturas de Dolphin upstream.
- [x] Adaptar reglas RDP: mask/clamp/mirror, negativos y filtro tres puntos.
- [x] Separar scissor de texto de altura del viewport 3D.
- [x] Empaquetar glifos pequenos; probar 128 glifos distintos en GPU.
- [x] Registrar causas concretas del fallback, conservando la proteccion.
- [ ] Reproducir el dialogo real informado y confirmar causa con log 00.36.
- [ ] Validar mejora de texturas, textos y filtro en PS4.
- [ ] Retirar rasterizado CPU y completar fidelidad framebuffer/VI/materiales.
Ver [alcance y referencias](docs/HLE-00.36-RDP-SAMPLING.md).

## Estado 00.35 (2026-10-07)

00.34 probado por usuario en PS4: texturas/textos aun rotos, 20-40 FPS.
CPU y Vulkan similares; rasterizado CPU de referencia sigue activo.

- [x] Especializar ocho ecuaciones reales adicionales de HUD/escenario.
- [x] Volver a copia completa en PS4 y comprobar atlas durante el juego.
- [x] Fallback CPU del frame completo si falta material/dibujo capturable.
- [ ] Confirmar fidelidad 00.35 en consola; no atribuir mosaicos sin evidencia.
- [ ] Retirar rasterizado CPU: separar spans, rastrear framebuffer/RDRAM,
  resolver lecturas dependientes con writeback/replay y validar fallback.
- [ ] Medir rendimiento sin referencia CPU en PS4; GPU tambien tiene readback.
Ver [alcance y dependencias](docs/HLE-00.35-FIDELITY.md).

## Estado 00.34 (2026-10-07)

- [x] 00.33 confirmada por usuario en PS4: juego continuo funciona, 20-30/50 FPS.
- [x] Implementado y probado localmente: rectangulos RDP/HUD ordenados.
- [x] Texturas hasta 128x128; castillo local sin rechazos 80x20.
- [x] Atlas incremental, asociacion con cursor y escalado reutilizado.
- [ ] Confirmar 00.34 en PS4: HUD, negros/particulas y FPS en castillo.
- [ ] Reducir rasterizado CPU de referencia, readback y espera por frame
  despues de validar dependencias de memoria y fallback.
- [ ] Completar clipping, VI, framebuffer, combiners/blenders y texturas avanzadas.

Ver [entrega 00.34](docs/HLE-00.34-HUD-PERFORMANCE.md).
Las tareas A-F siguientes mantienen el alcance completo del lanzamiento.

# Plan de lanzamiento: juego continuo con HLE Vulkan

Actualizado: 2026-10-07. 00.33 integra el primer modo continuo; no es aun
el lanzamiento final. Ver [alcance y uso](docs/HLE-00.33-CONTINUOUS.md).
Este es el plan activo; las secciones posteriores
conservan el historial tecnico y las pruebas anteriores.

Objetivo: entregar un PKG con Mario 64 PAL jugable de principio a fin en la
ruta HLE Vulkan, con imagen, controles, audio y funciones del frontend.
La compatibilidad con otros juegos se declara por juego y escena comprobados;
no se presenta este lanzamiento como compatibilidad completa de Nintendo 64.
La CPU N64 sigue emulada por Ari64 en CPU PS4. El trabajo a retirar de CPU es
el rasterizado de Angrylion, manteniendo las dependencias de memoria necesarias.

Base confirmada en PS4: triangulos y profundidad, texturas controladas,
geometria real, cabeza de Mario texturizada y secuencias animadas de 64 frames.
La presentacion completa la secuencia en 00.31, pero presenta fondo negro y
cuadros blancos. 00.32 esta empaquetada y validada localmente; falta su resultado
de consola. La estabilidad de estas pruebas permite avanzar al renderer
continuo, pero no demuestra todavia fidelidad ni estabilidad de una sesion larga.

## Forma de trabajar y entregar

- [ ] Implementar los bloques A-F en local, con pruebas de sus resultados.
  Desarrollar la integracion continua mientras se comprueba la apariencia.
- [ ] Entregar PKG por bloque funcional completo, con fuentes y hashes;
  evitar paquetes cuyo unico avance sea otro contador o diagnostico.
- [ ] Mantener para cada bloque tres estados: implementado, probado en Docker,
  confirmado en PS4. No marcarlo completo con solo un PASS de envios GPU.
- [ ] Conservar 00.13 CPU y paquetes anteriores como referencias reproducibles.
- [ ] Registrar las limitaciones reales de cada paquete y actualizar este plan.

## A. Texturas, materiales y particulas visibles

- [ ] Validar 00.32 en PS4: presentacion posterior a Peach, cabeza y exterior.
  Revisar atlas GPU readback, pixels y referencia CPU del mismo frame.
- [ ] Corregir cualquier diferencia entre captura TMEM, copia del atlas,
  direccionamiento UV, shader y salida. Capturar imagenes y datos de una escena
  reproducible para localizarla, sin atribuirla de antemano al compilador.
- [ ] Cubrir los combiners/blenders usados por esas escenas: SHADE, prim/env,
  uno/dos ciclos, alpha de textura, alpha compare, niebla y orden de mezcla.
- [ ] Capturar y dibujar textured rectangles, fill rectangles, sprites y HUD
  en su orden original, incluyendo particulas y fondos que no sean triangulos.
- [ ] Soportar las texturas y paletas usadas por Mario 64, dimensiones mayores,
  filtrado y LOD/multitextura donde el juego los requiera.
- [ ] Registrar cada modo no soportado con comando, material y causa;
  no considerar colores de diagnostico una representacion terminada.

Aceptacion: en PS4 se reconocen las texturas del castillo y Mario, aparecen
HUD/fondos/particulas, y la transparencia no produce cuadros opacos. En Docker,
casos de pixeles conocidos comprueban esos modos; comparar escenas con Angrylion.

## B. Renderer permanente y presentacion del juego

- [x] 00.33: separar crear recursos, dibujar frame y liberar contexto.
  Host libera/recrea; PS4 retiene un contexto por proceso, teardown pendiente.
- [x] 00.33: modo Vulkan continuo integrado al bucle normal del juego.
- [ ] Integrar imagen Vulkan con VideoOut, resolucion VI, aspecto y escalado;
  actualizar frames sin dibujos y transiciones negras correctamente.
- [x] 00.33: controles/audio en bucle normal; VI sin nuevos comandos RDP
  reutiliza ultimo frame. Audio, pausa y cambios CPU/Vulkan probados en host.
- [ ] PS4: confirmar audio, controles y cadencia PAL/NTSC en modo continuo.
- [ ] Reutilizar buffers, atlas, descriptores y pipelines con limites medidos;
  resolver capacidad insuficiente sin perder silenciosamente dibujos.
- [ ] Gestionar fences, recursos en vuelo y cierre/cambio de ROM en PS4.
  Resolver el ciclo de vida que hoy conserva recursos hasta salir de la app.
- [x] 00.33 host: 1200 frames controlados; dos sesiones de ROM reutilizan
  un dispositivo, con pausa, guardar/cargar y audio. PS4 largo pendiente.

Aceptacion: jugar y controlar Mario durante al menos 15 minutos en PS4,
con audio e imagen continua, atravesando presentacion, menus y cambios de escena.
Prueba larga local sin errores Vulkan ni crecimiento acumulativo de memoria.

## C. Memoria grafica, profundidad y retirada de Angrylion

- [ ] Implementar render targets, cambios de framebuffer y profundidad por target.
- [ ] Resolver clipping, viewport, Z_COMPARE/Z_UPDATE y superficies transparentes
  en escenas reales, ademas de los casos sinteticos.
- [ ] Manejar texturas de framebuffer y lecturas/escrituras RDRAM por CPU/RSP;
  definir sincronizacion y readbacks solo donde hagan falta.
- [ ] Separar captura HLE de rasterizado CPU; conservar Angrylion como referencia
  seleccionable durante desarrollo y medir que trabajo se ejecuta en cada ruta.
- [ ] Retirar el rasterizado Angrylion del modo Vulkan una vez cubiertas sus
  dependencias. Si queda fallback, declarar exactamente cuando se usa y su coste.

Aceptacion: el modo Vulkan no rasteriza tambien cada frame completo por CPU.
Las operaciones de memoria requeridas conservan el comportamiento del juego;
no aparecen regresiones al quitar la referencia CPU.

## D. Funciones del emulador y recuperacion

- [ ] Validar pausa/reanudacion, estados, reinicio y retorno a biblioteca.
- [ ] Probar cambios de ROM y varias sesiones consecutivas sin fugas ni recursos
  liberados mientras la GPU los usa.
- [ ] Mantener selector de renderer y configuracion persistente, con informacion
  clara sobre compatibilidad y fallback.
- [ ] Registrar errores de shader, textura, capacidad y dispositivo en un archivo
  accesible; comprobar ruta alternativa si falla el log principal.

Aceptacion: guardar/cargar y reentrar al juego conserva imagen, audio y controles;
fallos recuperables vuelven al frontend con motivo registrado. Un timeout CPU
no se anuncia como recuperacion de un bloqueo global de GPU.

## E. Rendimiento y compatibilidad comprobados

- [ ] Medir CPU, GPU, upload/readback, texturas, memoria y tiempos por frame;
  distinguir VI, frames nuevos del juego y refresco de pantalla.
- [ ] Comparar las mismas escenas y configuracion con 00.13 CPU.
- [ ] Optimizar lotes, cache, copias y esperas a partir de esas mediciones.
- [ ] Matriz Mario 64 PAL: cabeza, Peach/castillo, exterior, interior, niveles,
  agua, niebla, particulas, HUD, menus y transiciones; ampliar hasta recorrido
  completo de los niveles y final. Anotar lo probado y lo pendiente.
- [ ] Repetir pruebas con Mario Kart 64 local y registrar diferencias de
  microcodigo, materiales y framebuffer; declarar su soporte solo tras PS4.
- [ ] Prueba PS4 de una hora y sesiones repetidas con memoria estable,
  controles/audio correctos y sin cuelgues ni cierres.

Aceptacion: rendimiento real publicado por escena, sin prometer FPS por el
contador de la pantalla de pausa. Los defectos visibles quedan resueltos o
excluidos expresamente del alcance; los esenciales de Mario 64 bloquean salida.

## F. Candidato y version lista

- [ ] Congelar un candidato tras A-E, ejecutar regresiones y revisar pendientes.
- [ ] Compilar con OpenOrbis; verificar ELF/SELF/PKG, version y payload.
- [ ] Auditar correspondencia de fuentes, paquete y hashes; excluir ROMs.
- [ ] Validar el mismo PKG en PS4, incluyendo instalacion y arranque limpio.
- [ ] Publicar instrucciones, compatibilidad, rendimiento y limitaciones,
  junto al paquete y una opcion CPU estable.
- [ ] Marcar lanzamiento listo solo con juego continuo, cobertura del alcance,
  estabilidad y validacion de hardware; no por completar submissions.

Docker/Lavapipe valida logica, pixeles y uso de Vulkan. OpenOrbis construye para
PS4. Ninguno reproduce exactamente Liverpool/GNM/VideoOut/firmware; las puertas
de hardware se validan en la consola del usuario.

## Historial tecnico anterior

> 00.19 experimental: variante de rasterizado sin texturas, con selección conservadora por lote y comparación completa de memoria en PC. Ver [prueba Vulkan](docs/VULKAN-00.19-TEST.md). Pendiente validación en PS4; 00.13 CPU sigue siendo la versión estable.

Prueba actual: **00.18 diagnÃ³stico de descriptores y datos**.
La captura 00.17 omitiÃ³ los descriptores 0xa0/0xb0 usados por el rasterizador.
Ver [datos necesarios y lÃ­mites](docs/VULKAN-00.18-TEST.md).

Prueba actual: **00.17 diagnÃ³stico**, sin correcciÃ³n confirmada.
00.16 tambiÃ©n falla con marcador 8 aunque RSRC1 cambiÃ³ a 002c031f.
Ver [instrucciones y lÃ­mites](docs/VULKAN-00.17-TEST.md).

## Prueba actual 00.16

00.15 avanzÃ³ hasta marcador 8; falla la espera del rasterizador.
00.16 limita la asignaciÃ³n escalar ACO en PS4/GFX7 a 96 antes de compilar.
HipÃ³tesis pendiente de validaciÃ³n fÃ­sica; ver docs/VULKAN-00.16-TEST.md.

## Estado actual: 00.13 estable / 00.15 Vulkan experimental

00.14 sigue fallando en PS4. Marcador 3 tras espera de spans alcanzado;
marcador 5 tras despacho de tiles alcanzado y 6 tras espera no alcanzado.
00.15 reemplaza tiles con un shader sin LDS/atÃ³micos/barreras internas.

- [x] Comparar mÃ¡scaras con shader original en GPU host para 1-256 primitivas.
- [x] Generar y auditar SPIR-V sin Workgroup, atÃ³micos ni barreras.
- [x] Ejecutar 360 frames de Mario Kart y comparar pÃ­xeles de juego en frames 300/330.
- [x] Generar PKG 00.15 y verificar sus 28 comprobaciones y arranque diferido.
- [ ] Confirmar 00.15 Vulkan en PS4 y revisar la traza si vuelve a bloquearse.

Historial 00.14 (fallo confirmado por el usuario): se generÃ³
`out/NPS64-00.14-vulkan-test.pkg` con sincronizaciÃ³n por despacho, una
submission pendiente y separaciÃ³n de spans/tiles. No estÃ¡ validado en PS4.
Instrucciones: `docs/VULKAN-00.14-TEST.md`.

- [x] Compilar RADV PS4 con sincronizaciÃ³n de cÃ³mputo y cola single-flight.
- [x] Mantener habilitaciÃ³n GPU limitada a la compilaciÃ³n experimental.
- [x] Generar PKG 00.14 Vulkan: ELF y arranque diferido verificados, 28 checks del paquete.
- [x] Probar imagen/estados/reentrada y 360 frames de Mario Kart en Lavapipe.
- [x] Registrar fallo 00.14 y localizar la espera posterior al segundo despacho.

La ruta GPU de 00.07-00.12 congela PS4 y queda suspendida. El paquete actual
es `out/NPS64-00.13.pkg`, Ari64 + Angrylion. No ejecutar las pruebas antiguas
GPU que se conservan abajo como historial.

- [x] Excluir Mesa/Vulkan/GNM del PKG principal y bloquear peticiones GPU antiguas.
- [x] Verificar dos sesiones CPU, imagen, estados, HUD y Mario Kart local.
- [x] Usuario confirma que 00.13 funciona y va bien, con bajones ocasionales.
- [ ] Medir FPS de 00.13 en carrera y confirmar todos los mandos en PS4.
- [x] AÃ±adir pruebas host aisladas de storage, LDS/atomic/barrier con 32/64 hilos.
- [x] AÃ±adir diagnÃ³stico optativo que separa spans y tiles con una barrera.
- [ ] Resolver offline el bloqueo previo al tercer despacho y validar una
  reproduccion minima antes de habilitar nuevamente GPU en consola.

# TODO â€” NPS64 / PS4

Orden de trabajo: build reproducible â†’ primer arranque â†’ emulaciÃ³n y datos
en consola â†’ rendimiento â†’ paridad visual/funcional con DolphinPS4.
Una casilla marcada significa implementaciÃ³n o comprobaciÃ³n explÃ­cita;
las pruebas Linux no marcan como hechas las pruebas PS4.

## 1. InvestigaciÃ³n y base

- [x] Estudiar DolphinPS4 v03.10 y fijar su commit de referencia.
- [x] Documentar target, heap, pthread_once, linker, renderer Mesa/GNM y PKG.
- [x] Elegir paraLLEl-N64 y fijar su revisiÃ³n en sources.lock.json.
- [x] Mantener el cÃ³digo de Wii/GameCube como referencia separada.
- [x] Crear frontend libretro nativo de PS4; no intentar emular N64 con Dolphin.
- [x] Crear Dockerfile y entrada PowerShell dentro de este directorio.
- [x] Descargar OpenOrbis v0.5.4 y comprobar SHA-256 del archivo.
- [x] Aplicar cambios del nÃºcleo sÃ³lo a copias de compilaciÃ³n verificando el texto esperado.
- [x] Corregir include residual OpenGL y enlazar libretro-common para frontend independiente.
- [x] Adaptar allocator de Dolphin, lÃ­mite de 512 MiB y addressing comprimido N64.
- [x] Adaptar pthread_once y reglas de secciones del linker.

## 2. CompilaciÃ³n, paquete y entrega

- [x] Compilar el nÃºcleo real para x86_64-pc-freebsd12-elf.
- [x] Enlazar frontend, VideoOut, Pad, UserService, AudioOut y runtime.
- [x] Generar ELF, OELF y eboot.bin; verificar segmentos alineados sin solapamientos.
- [x] Crear SFO NPSX00064, versiÃ³n 00.06, icono propio y GP4.
- [x] Incluir libc.prx, libSceFios2.prx y sce_sys/about/right.sprx pÃºblicos de OpenOrbis.
- [x] Comprobar mÃ³dulos requeridos antes de empaquetar y despuÃ©s de extraer el PKG.
- [x] Confirmar creaciÃ³n final del PKG y registrar tamaÃ±o/hash en VALIDATION.md.
- [x] Verificar manifiesto, cabecera y contenido del PKG generado (28 checks y extracciÃ³n).
- [x] Confirmar repeticiÃ³n de build desde scripts/build.ps1.
- [x] Crear fuente correspondiente comprimida, con licencias y scripts.
- [x] Probar reconstrucciÃ³n desde ese archivo de fuentes en build/rebuild-check usando el SDK local.

## 3. Primer arranque en PS4 â€” obligatorio

- [ ] Registrar modelo (fat/slim/pro), firmware y loader/GoldHEN exactos.
- [x] Instalar 00.01 en consola: el usuario confirmÃ³ instalaciÃ³n correcta.
- [x] Instalar 00.02 y repetir arranque: confirmado por el usuario.
- [x] Detectar ROMs en biblioteca de PS4: confirmado por el usuario.
- [x] AÃ±adir prueba visible del mando, botones ScePad y etapas de carga al log.
- [x] Verificar mapeo fÃ­sico X/CÃ­rculo/Cuadrado/TriÃ¡ngulo, cruceta, gatillo y ejes en host.
- [ ] Confirmar en PS4 los controles de 00.03 con la prueba de mando.
- [x] Registrar informe 00.03: sin cierre, menÃº lento y ROM negra, log vacÃ­o.
- [x] Sustituir stderr/freopen por logger con descriptor de archivo propio.
- [x] Dibujar en framebuffer CPU con cachÃ© y copiar secuencialmente al scanout.
- [x] Usar reloj/sleep nativos y arrancar los flips desde 1.
- [x] Mostrar mediciones UI y escribir perfil de etapas cada dos segundos.
- [x] Verificar backend PS4 con mocks: buffer, flips, reloj, mando y log.
- [x] Optimizar escalado: mapa X precalculado y copia de filas repetidas.
- [x] Comparar RSP HLE/cxd4 con la ROM del usuario, usando memoria comprimida como PS4.
- [x] Verificar tÃ­tulo, menÃºs y carrera Mario Kart en Linux; adoptar HLE por defecto.
- [ ] Medir latencia/FPS y confirmar escritura del log de 00.04 en PS4 real.
- [x] Usuario confirma 00.04: interfaz rÃ¡pida, Mario Kart arranca; estima 15 FPS o menos.
- [ ] Medir carrera de 00.05 y comprobar selected=Ari64 en el log de PS4.
- [ ] Diagnosticar CE-34878-0 al cargar Mario Kart 64 con el log de consola.
- [ ] Comparar Angrylion sÃ­ncrono y cuatro trabajadores en la misma ROM/consola.
- [ ] Arrancar sin ROMs: biblioteca vacÃ­a, texto legible y sin cuelgue.
- [ ] Verificar creaciÃ³n de /data/NPS64 y acceso de lectura/escritura.
- [ ] Recoger nps64.log y confirmar inicializaciÃ³n de VideoOut, Pad y AudioOut.
- [ ] Medir heap flexible/directo disponible y mÃ¡ximo uso al cargar una ROM.
- [ ] Comprobar CRT/TLS y pthreads del SDK en esa consola; ampliar wrappers si hace falta.
- [ ] Comprobar bucle de flip, timeout de tres segundos y limpieza al salir.
- [ ] Si aparece CE-41839-5, revisar loader, PAID y segmentos OELF antes de tocar el nÃºcleo.
- [ ] Salir con CÃ­rculo y volver a abrir; repetir diez veces.

## 4. EmulaciÃ³n y compatibilidad

- [x] Escanear hasta 512 ROMs con cabecera vÃ¡lida; ordenar nombres.
- [x] Reconocer .z64, .v64 y .n64 y normalizar identidad por contenido.
- [x] Conectar CPU cached_interpreter, RSP cxd4 y Angrylion multihilo.
- [x] Ejecutar ROM MIPS propia y verificar imagen con el nÃºcleo real en Linux.
- [x] Corregir y probar unload/reload del nÃºcleo: parar CPU, liberar memoria y reiniciar flags.
- [x] Corregir Ã­ndice fuera de lÃ­mites PLUGIN_BIO_PAK de la tabla de paks del nÃºcleo.
- [ ] Ejecutar la ROM propia en PS4 y comprobar superficie turquesa estable.
- [ ] Cargar dos ROMs consecutivas sin reiniciar la app y comprobar fugas.
- [ ] Probar juegos del usuario NTSC/PAL y tamaÃ±os 8/16/32/64 MiB.
- [ ] Registrar por juego: commit, modelo, FPS/VI, audio, errores grÃ¡ficos y guardado.
- [ ] Comprobar Expansion Pak, EEPROM, SRAM, FlashRAM y Controller Pak por juego.
- [ ] Comparar cxd4 y RSP HLE; mantener fallback LLE para microcÃ³digos no reconocidos.
- [ ] Validar reset, interrupciones, TLB y carga repetida en tÃ­tulos exigentes.
- [ ] Manejar cierre/suspensiÃ³n de la aplicaciÃ³n y logout de usuario con eventos del sistema.
- [ ] AÃ±adir escaneo recursivo y reporte visible de biblioteca truncada o ROM invÃ¡lida.

## 5. Video, audio y mandos

- [x] VideoOut 1280Ã—720, doble buffer y escalado CPU de frames libretro.
- [x] ConversiÃ³n XRGB8888/RGB565/0RGB1555 y control de dimensiones/pitch.
- [x] Aspecto 4:3 o 16:9 estirado; tres temas persistentes.
- [x] AudioOut estÃ©reo 48 kHz con resampling bÃ¡sico y cola SPSC en hilo independiente.
- [ ] Medir latencia, underruns/overruns, velocidad PAL/NTSC y sincronÃ­a A/V en PS4.
- [ ] Sustituir nearest-neighbor de audio por resampler filtrado si la calidad lo requiere.
- [ ] Vaciar audio en pausa/cambio de ROM sin sonido residual de la cola.
- [x] Mapear A/B/Z/L/R/Start, cruceta y botones C al DualShock 4.
- [x] Conservar usuario inicial en puerto 1 y asignar otros usuarios a puertos 2â€“4.
- [ ] Validar hotplug, desconexiÃ³n, reconexiÃ³n y cuatro jugadores reales.
- [ ] Calibrar deadzone, sensibilidad y rango con un programa N64 de test de mando.
- [ ] Implementar Rumble Pak y reasignaciÃ³n de controles global/por juego.

## 6. Persistencia y estados

- [x] Partidas por contenido de ROM, independientes del nombre y del orden de bytes.
- [x] Guardado automÃ¡tico cada 30 s y al volver a la biblioteca/salir normalmente.
- [x] Escribir temporal, fflush/fsync y reemplazar con rename.
- [x] Proteger una SRAM ilegible o con tamaÃ±o incompatible contra sobrescritura.
- [x] Ocho ranuras de estado con ROM, commit, tamaÃ±o y checksum.
- [x] Rechazar estados corruptos y de otra ROM antes de deserializar.
- [x] Verificar escritura/lectura de SRAM y estado del nÃºcleo real en Linux.
- [ ] Confirmar fsync/rename en el filesystem /data de PS4 y en falta de espacio.
- [ ] Verificar que una actualizaciÃ³n de PKG conserve ajustes y partidas.
- [ ] Probar guardar/cargar cada slot en una consola y comparar datos en juego.
- [ ] Mostrar Empty/Other Version/Corrupt por ranura y confirmaciÃ³n antes de reemplazar estado.
- [ ] AÃ±adir backup rotativo y recuperaciÃ³n tras cierre forzado.
- [ ] Invalidar estados al cambiar opciones que alteren estructura interna del nÃºcleo.

## 7. Rendimiento: JIT y GPU

- [x] Registrar informe 00.05 en PS4: mejora parcial, juego todavÃ­a lento y FPS desaparecen.
- [x] Mantener HUD permanente con FPS de VI, objetivo, velocidad, CPU y workers reales.
- [x] Separar mediciones por estado y probar contador sin contaminarlo con tiempo de biblioteca.
- [x] Retener video a resoluciÃ³n nativa y escalar al canvas sin copiar otro buffer completo de 720p.
- [x] Solapar emulaciÃ³n y flip, limitar a un flip pendiente y drenar antes de liberar buffers.
- [x] Probar presentaciÃ³n simulada con frames de 10/20 ms y reloj TSC/fallback.
- [x] AÃ±adir temporizadores RDP/VI y reloj TSC de bajo coste inspirado en DolphinPS4.
- [x] Probar dos/cuatro/seis trabajadores y modo VI rÃ¡pido en carrera de Mario Kart en Docker.
- [x] AÃ±adir imagen precisa/rÃ¡pida en pausa; verificar cambio de renderer y estados en host.
- [ ] Medir 00.06 en la PS4 con FPS, CPU seleccionada y tiempos core/RDP/VI/dibujo/flip.
- [ ] Medir tiempos separados de CPU R4300, RSP, rasterizador, escalado y presentaciÃ³n.
- [ ] Probar 1/2/4/6 workers Angrylion en PS4 base y Pro.
- [x] Compilar Ari64 x86_64 con NASM y regenerar offsets con el compilador PS4.
- [x] Comprobar permisos del cache real antes de seleccionar JIT; fallback si falla.
- [x] Adaptar mprotect a sceKernelMprotect con pÃ¡ginas de 16 KiB; conservar invalidaciÃ³n upstream.
- [x] Comparar intÃ©rprete, JIT y cuatro trabajadores en una carrera de Mario Kart en Docker.
- [x] Verificar guardar/cargar estados y dos sesiones con JIT, sÃ­ncrono y cuatro trabajadores en host.
- [ ] Confirmar permisos, ejecuciÃ³n Ari64 y estabilidad de cuatro trabajadores en PS4 real.
- [ ] Ejecutar tests de CPU/TLB/delay slots y estados con el JIT activado.
- [x] Elegir e integrar paraLLEl-RDP + RADV/GNM de DolphinPS4.
- [x] Reconstruir Mesa 26.0.8 con los parches/winsys de Dolphin.
- [x] Implementar negociaciÃ³n libretro Vulkan, colas, staging y fences; probar en host.
- [x] Ejecutar Mario Kart y comprobar Vulkan con validaciÃ³n en host.
- [ ] Validar coherencia RDRAM/framebuffer y shaders en hardware PS4 real.
- [ ] AÃ±adir cache de shaders, lÃ­mite de resoluciÃ³n y perfiles estables por consola.
- [x] Mantener alternativa Angrylion/intÃ©rprete y perfil NPS64DIAG5 para diagnÃ³stico.

## 8. Experiencia similar a DolphinPS4

- [x] Biblioteca navegable con mando, temas y menÃº de pausa L3 + R3.
- [ ] Crear cover-flow y/o XMB con pestaÃ±as Biblioteca, Partidas, Ajustes y Temas.
- [ ] AÃ±adir carÃ¡tulas locales primero; luego descarga opcional con cache y fuente autorizada.
- [ ] Guardar Ãºltimo juego, favoritos y orden alfabÃ©tico/reciente.
- [ ] Ajustes globales y por juego con herencia explÃ­cita y persistencia.
- [ ] AÃ±adir listado/exportaciÃ³n de Controller Pak y partidas.
- [ ] Cheats GameShark mediante API del nÃºcleo, con archivo y toggles por juego.
- [ ] Pantalla About que muestre versiÃ³n, commit, renderer y logs.
- [ ] Incorporar wallpapers/sonidos propios y animaciÃ³n de inicio.
- [ ] ActualizaciÃ³n automÃ¡tica sÃ³lo despuÃ©s de disponer de releases verificables propias.

## 9. Criterio de versiÃ³n utilizable

- [ ] InstalaciÃ³n y arranque repetidos sin fallo en al menos una PS4 real documentada.
- [ ] ROM de diagnÃ³stico y varios juegos del usuario funcionan con video, audio y mando.
- [ ] Partidas y estados sobreviven a salida normal y actualizaciÃ³n.
- [ ] Informe con lÃ­mites de compatibilidad y FPS medidos, sin afirmar 60 FPS por compilar.
- [ ] Paridad mÃ­nima de biblioteca/ajustes/pausa revisada; features faltantes declaradas.
- [ ] Fuente correspondiente y licencias entregadas junto al PKG antes de distribuciÃ³n.

## 10. ValidaciÃ³n GPU 00.07

- [x] Compilar paraLLEl-RDP con Ari64 y Angrylion en el mismo nÃºcleo.
- [x] Probar Vulkan con Lavapipe, colores RGBA, estados y dos sesiones en host.
- [x] Probar arranque con driver Vulkan inexistente y retorno seguro a Angrylion.
- [x] Completar build Mesa/RADV para PS4 y verificar SELF/PKG GPU.
- [ ] Confirmar `GPU Vulkan` y dispositivo AMD real en nps64.log de PS4.
- [ ] Medir carrera Mario Kart (FPS/velocidad), audio, mando y memoria en consola.
- [ ] Confirmar guardado/carga, cierre/apertura y estabilidad 30 minutos en PS4.
- [ ] Eliminar staging CPU con compositor GPU directo a VideoOut.
- [ ] Cache persistente de pipelines para evitar compilaciÃ³n inicial de shaders.
- [ ] Ajustar async RDP y coherencia sÃ³lo despuÃ©s de medir en PS4.

## RevisiÃ³n 00.08: cierre CE-34878-0 al abrir 00.07

El cÃ³digo CE por sÃ­ solo no identifica la causa. Se comprobÃ³ que 00.07
contenÃ­a tres constructores GPU automÃ¡ticos antes de main y conservaba Sony
mspace. Dolphin sustituye ese allocator por dlmalloc y evita reservar el heap
en el rango utilizado por sus objetos kernel/pthread. 00.08 incorpora esas
correcciones y aplaza los constructores GPU hasta abrir la primera ROM.

`boot.log` se escribe directamente con libkernel, sin malloc ni stdio:
pre-main, entrada en main, inicializaciÃ³n del heap, plataforma y cada constructor
GPU. Si vuelve a cerrar, recoger `/data/NPS64/boot.log` y `nps64.log`.
El test host con ASan/UBSan verifica 16.384 asignaciones/realocaciones, alineaciÃ³n,
overflow, denegaciÃ³n de reserva y direcciÃ³n/flags del mapping simulado.
El ELF se verifica para que sÃ³lo el trace se ejecute antes de main y para que
no enlace Sony mspace. La resoluciÃ³n de CE-34878-0 requiere prueba PS4 real.


## 00.09: cierre dentro de platform_init

El log de la consola en 00.08 termina en `frontend log/config ready; platform
init begin`. Los constructores GPU todavia no se ejecutan en ese punto.
Se agrega trazado directo y sincronizado del reloj, calloc del canvas, UserService,
Pad, VideoOut, cola de flips, reserva/mapeo/limpieza del scanout, fuente y audio.
Los resultados se escriben en hexadecimal sin asignaciones ni stdio.
La memoria de video usa una reserva virtual alta y MAP_FIXED; se abandona la
inicializacion si no se puede reservar. Los atributos de VideoOut se ponen a cero.

Pruebas locales: backend PS4 simulado con ASan/UBSan, reloj TSC y alternativo,
flips, mandos, copia de video y fallo de reserva controlado; heap con calloc de
720p y reutilizacion ademas de las 16384 asignaciones/reallocaciones existentes.
Ejecutar `bash scripts/test_gpu_platform.sh` y `bash scripts/test_gpu_heap.sh`
dentro del contenedor. Estas pruebas no reproducen los servicios de una PS4 real.

Pendiente: instalar `out/NPS64-00.09.pkg`, comprobar que llega a la biblioteca
y despues cargar una ROM. Si sigue cerrandose, recoger el bloque completo que
empieza en `00.09 before main` de `/data/NPS64/boot.log`. No atribuir el fallo
actual a Vulkan ni declarar resuelto CE-34878-0 antes de comprobarlo en consola.


## 00.10: cargar los modulos del sistema antes de usarlos

El log real de 00.09 alcanza `platform: UserService initialize begin` y se
cierra sin devolver resultado. La GPU sigue aplazada. Se encontro una omision:
`LoadSystemModules()` en `reference/DolphinPS4/patches/dolphin-ps4.patch` carga
SystemService, UserService, VideoOut, AudioOut y Pad. El probe GNM de Dolphin
explica expresamente el cierre por PRX_NOT_RESOLVED_FUNCTION al invocar
`sceUserServiceInitialize` sin cargar su modulo.

00.10 enlaza libSceSysmodule, comprueba los modulos ya cargados y carga los
restantes antes de acceder a esas funciones. Registra resultados en boot.log y
nps64.log. Si una carga falla, no invoca los imports no resueltos.
Corrige tambien el identificador del log principal, que aun indicaba 00.08.

Pruebas reproducibles: `scripts/test_gpu_platform.sh` con ASan/UBSan exige que
los cinco modulos esten cargados antes de UserService; cubre modulos ya cargados,
falta de UserService sin invocarlo, reserva de video denegada, TSC y reloj kernel,
mandos, presentacion y cierre. Validacion del PKG y ELF sigue obligatoria.

Pendiente en PS4: instalar `out/NPS64-00.10.pkg`; confirmar biblioteca visible,
mandos y audio; despues cargar Mario Kart y comprobar renderer/FPS. Si falla,
recoger el bloque `00.10 before main` completo de `/data/NPS64/boot.log` junto
con `/data/NPS64/nps64.log`. Las pruebas del PC no certifican los servicios PS4.


## 00.11: pantalla negra despues de activar GPU Vulkan

Confirmado por el usuario: 00.10 inicia correctamente; carga los cinco modulos,
abre mandos/video/audio y negocia un dispositivo Vulkan AMD Liverpool.
El reporte de Mario Kart muestra envios 1-36 completados, scanout inicial 1x1,
y bloqueo del envio 37; la GPU queda en 36 con 39 enviados. El stream 37
contiene clear-indirect, binning, rasterizer con DISPATCH_INDIRECT, resolve y VI.
El reporte no incluye marcadores que permitan saber en cual shader se detiene;
no atribuir el bloqueo exclusivamente al paquete indirecto ni al compilador.

Cambio: usar PARALLEL_RDP_UBERSHADER=1, PARALLEL_RDP_SUBGROUP=0,
PARALLEL_RDP_SMALL_TYPES=0 y PARALLEL_RDP_FORCE_SYNC_SHADER=1 en PS4.
El ubershader rasteriza y mezcla en GPU con dispatch directo, evitando la lista
de rasterizadores especializados y los contadores de despacho indirecto.
Los enteros de 32 bits ya eran el valor por defecto para RADV; se hacen explicitos.
El mismo perfil se activa en los tests y benchmark de PC mediante
NPS64_TEST_PS4_GPU_PROFILE. Ari64 y la ruta de copia de scanout se conservan.
Este es un perfil de compatibilidad, no una optimizacion de FPS demostrada.

Pruebas con VK_LAYER_KHRONOS_validation: ROM sintetica con pixeles esperados,
SRAM/estado, dos sesiones GPU y retorno a biblioteca; driver Vulkan ausente y
dispositivo no apto vuelven a Angrylion. Sin errores de validacion Vulkan.
Mario Kart local: 360 frames, imagen visible desde frame 53, scanout 640x288,
capturas frame 300/330 y cierre limpio. Lavapipe es software: esos tiempos no
predicen rendimiento ni comportamiento del driver RADV/GNM en PS4.

Pendiente: instalar `out/NPS64-00.11.pkg`, comprobar imagen despues de compilar
los shaders iniciales; probar carrera y FPS. Confirmar en el log el perfil de
GPU, `Overriding ubershader = 1`, `Allow subgroups = 0`, geometria mayor a 1x1
y `first_visible_video`. Si persiste: recoger nps64.log y gpu-hang.txt nuevos.
Volver a habilitar rasterizadores especializados/subgrupos solo despues de
validar cada cambio en consola. El fallo al importar VK_EXT_external_memory_host
usa la alternativa de copia y no demuestra la causa del hang. La advertencia
SRAM ilegible se conserva: no sobrescribir ni eliminar las partidas protegidas.


## 00.12: el despacho directo tambien bloquea la GPU

El reporte de consola de 00.11 confirma el perfil nuevo (ubershader=1,
subgroup=0) pero vuelve a detenerse en 37, completado 36/enviado 39. El stream
ahora contiene tres DISPATCH_DIRECT y no DISPATCH_INDIRECT. No hay solapamiento
fisico entre los 47 BO. Esto descarta el despacho indirecto como causa unica;
no hay marcadores que permitan saber cual de los tres shaders se detiene.

Cambios del driver, aplicados reproduciblemente por scripts/patch_gpu_driver.py:
- SFENCE con barrera de compilador justo antes de dev->submit: publica las
  escrituras WC de CPU, incluidas las subidas al BO de codigo de shader.
- Desactivar radv_emit_shader_prefetch solo bajo __PS4__: evita el DMA_DATA de
  copia del codigo sobre la misma direccion; no elimina despachos ni shaders.
- Activar RADV_PS4_TRACE=1 en el perfil: usa los marcadores ligeros ya existentes
  de Dolphin para el flight recorder y gpu-hang.txt. No activa RADV_DEBUG=hang
  ni su sincronizacion costosa de cada shader. El nombre de version es 00.12.

Validacion local: Mesa RADV/GNM recompilado, parches idempotentes comprobados,
barrera antes del handoff verificada, GPU frontend con validacion Vulkan,
pixeles de ROM sintetica, dos sesiones, estados y alternativas Angrylion.
El benchmark de Mario Kart de 00.11 valida el perfil directo en Lavapipe;
NO valida estos cambios de GNM ni elimina la necesidad de probar hardware.
El empaquetado de fuentes sustituye en la cache los dos archivos Mesa editados
para que correspondan a esta compilacion y no incluya codigo antiguo del driver.

Pendiente en PS4: instalar out/NPS64-00.12.pkg y abrir Mario Kart. Si se bloquea,
recoger los archivos nuevos nps64.log y gpu-hang.txt completos. Buscar WRITE_DATA,
valores memory/done/NOT DONE y trace ids en execution check para distinguir
preparacion, binning y rasterizacion. No prometer recuperacion a CPU despues de
un GPU hang: se mantienen las esperas originales, pues liberar BO aun usados
por la GPU sin un reset confirmado puede causar corrupcion. Confirmar imagen,
carrera y FPS antes de declarar esta incidencia resuelta.


## 00.13: restaurar CPU tras congelamiento de consola CE-36329-3

00.12 sigue bloqueando y el usuario informa congelamiento de PS4/CE-36329-3.
El marcador compartido llega a 3; las escrituras 1/2 figuran NOT DONE porque
el mismo destino contiene el ultimo valor, 3. Eso NO prueba que no ocurrieran.
El siguiente marcador, 4, esta DESPUES de CS_PARTIAL_FLUSH/PFP_SYNC_ME y no se
alcanza. La GPU se detiene esperando trabajo anterior al tercer despacho:
span_setup (64 hilos) o tile_binning_combined (32 hilos, LDS/barriers). Los
WRITE_DATA del ME no garantizan finalizacion de compute. No culpar todavia al
ubershader, que queda despues de la barrera sin evidencia de que haya comenzado.

Entrega principal: out/NPS64-00.13.pkg, software sin Mesa/Vulkan/GNM, con
Ari64, RSP HLE, cuatro trabajadores Angrylion, canvas rapido y HUD permanente.
Mismo title/content id; conserva ROMs y partidas. La configuracion grafica vieja
no se borra, pero requested_gpu se fuerza a cero en PS4 despues de leerla.
Ademas nps64_gpu_begin devuelve false antes de constructores/loader/dispositivo
en PS4. Ningun PKG normal de esta revision puede activar la ruta GPU defectuosa.
El script GPU genera un paquete separado `00.13-gpu-disabled`, no una nueva
prueba de hardware. Las fuentes y los parches de investigacion se conservan.

Test scripts/test_gpu_quarantine.sh: configura GPU=1 tanto en diagnostics.cfg
como en BENCH_GPU y sustituye nps64_gpu_begin por abort. El frontend no lo
invoca; dos sesiones Angrylion producen imagen y guardan/cargan estados.
La comprobacion final del ELF principal exige ausencia de RADV/GNM/Vulkan.

Pendientes para retomar GPU sin usar otra ROM como primer experimento:
- Aislar span_setup y tile_binning, diseminar/disensamblar sus shaders ACO gfx7.
- Preparar reproduccion minima de compute con grupos 64 y 32, LDS, atomicas,
  barreras y descriptores; verificar los datos emitidos y registros Liverpool.
- Investigar mapa/coherencia de BO y diferencias del contexto GNM respecto a
  Linux RADV; confirmar antes de culpar al paquete indirecto, precarga o shaders.
- No tratar Lavapipe como sustituto de validacion de hardware. Rehabilitar GPU
  solo cuando exista evidencia del arreglo y un protocolo que no repita el
  bloqueo global. No prometer recuperacion de un GPU hang mediante timeout CPU.
- Validar en PS4 que 00.13 vuelve a mostrar el juego y conserva controles/FPS.
La mejora solicitada de rendimiento GPU permanece sin resolver.

## Plan activo: renderer HLE Vulkan (2026-10-05)

Este plan sustituye la ruta compute como siguiente trabajo de renderizado.
Conservar 00.13 estable y las capturas 00.25/00.26 como referencias. Marcar
por separado implementacion, validacion Docker y validacion PS4: un PASS
en Lavapipe no implica que Liverpool ejecute correctamente el mismo trabajo.

### Base y evidencia disponible

- [x] Construccion PS4 con OpenOrbis, Clang/LLD y RADV adaptado a GNM.
- [x] Verificacion ELF/SELF/PKG, payload y version; fuentes sin ROMs.
- [x] Pruebas Docker con Lavapipe, capa de validacion Vulkan y ASan/UBSan.
- [x] PS4: triangulos, texturas controladas, alfa, scissor y profundidad.
- [x] PS4 00.25: geometria 3D real de Mario 64 visible, 64 envios completados.
- [x] 00.26: asociacion comando/triangulo, captura TMEM y atlas con UV.
- [x] Docker 00.26: textura de Mario Kart, atlas y UV comprobados por pixeles.
- [x] PS4 00.26: completa 64 envios y presenta resultado sin bloqueo informado.
- [ ] Fidelidad de 00.26: usuario ve Mario oscuro con reflejos y superficies
  de diagnostico. Esa apariencia sigue pendiente; PASS mide repetibilidad.

### Materiales y apariencia (siguiente bloque de implementacion)

- [x] 00.29: probar el mux exacto FC30B261:44664924 del log de Mario en
  Vulkan con colores conocidos y una ecuacion directa equivalente.
- [x] PS4 00.29: usuario confirma textura de Mario y movimiento; ya no negro.
- [x] 00.30: reproducir rechazo de captura en Mario 64 PAL y eliminar
  comprobacion del ultimo VI vacio; permitir reintento antes de crear Vulkan.
- [x] Docker: exterior del castillo, 64 frames y referencia sin cache;
  prueba sintetica con 30 000 vertices. Validacion PS4 00.30 pendiente.
- [x] PS4 00.30: log identifica primer framebuffer negro con 414 triangulos;
  cabeza de Mario pasa. Es distinto del rechazo local previo por cero vertices.
- [x] 00.31: atravesar frames uniformes en secuencia sin validarlos como escena;
  fallar si todos son negros. Casos de fade y negro persistente verificados.
- [x] 00.31: rutas directas de SHADE y textura por SHADE del castillo;
  registrar referencia CPU para comparar el negro con Vulkan.
- [ ] PS4 00.31: validar presentacion tras Peach y revisar CPU reference/pixels.
- [x] PS4 00.31: secuencia completa, pero cuadros blancos/fondo negro;
  captura de texturas y materiales lista no acredita fidelidad del framebuffer.
- [x] 00.32: especializar ecuaciones comunes al crear pipelines y recibir
  estado de textura en fragment por push constants. Comparar atlas GPU con CPU.
- [x] 00.32: caso MODULATERGBA con texel transparente conserva fondo/Z.
- [ ] PS4 00.32: comprobar texturas de castillo/cabeza y alpha de particulas;
  sprites/rectangulos y otros materiales siguen pendientes.
- [x] Capturar por dibujo combine mux, ciclo, prim/env color y alpha (00.27).
- [x] Reproducir modos comunes de uno y dos ciclos del combinador (00.27;
  floats, sin equivalencia exacta con los intermedios de nueve bits del RDP).
- [x] Recuperar SHADE e iluminacion calculada por HLE; quitar colores de
  diagnostico solo cuando el material tenga una representacion soportada.
- [x] Aplicar alpha compare/discard y mezcla basica sin escribir profundidad
  indebidamente para superficies transparentes.
- [x] Capturar scissor y separar Z_COMPARE de Z_UPDATE por dibujo (00.27).
- [ ] Contar y registrar materiales/modos/texturas no soportados y su causa.
- [x] Docker: casos con resultado numerico conocido para prim/env/shade,
  textura, ciclos, alpha, orden y Z; comprobar pixeles, no solo imagen no negra.
- [ ] PS4: comprobar Mario, fondo y transparencia en una escena concreta.
  00.27 completa la prueba y muestra geometria, pero el usuario sigue viendo
  Mario oscuro y colores de diagnostico. 00.28 agrega muestras de mux y colores.
- [ ] Ampliar combinador y blender: TEXEL1, LOD, ruido, keying, cobertura,
  niebla y redondeo N64. Alpha dither y copy/fill siguen pendientes.

### Renderizado continuo

- [x] 00.28: secuencia acotada de 64 frames reales con recursos reutilizados;
  Docker valida cambios en 120 frames controlados y 64 frames de Mario Kart.
- [x] 00.28: cancelacion entre fences y muestras acotadas de materiales para
  diagnosticar la silueta oscura; ejecucion de esta version en PS4 pendiente.
- [ ] Separar crear dispositivo/recursos, actualizar captura y dibujar frame.
- [ ] Reutilizar buffers, atlas, pipelines y descriptores con limites definidos.
  Implementado en secuencia acotada 00.28 (30 000 vertices, atlas 512x512),
  pendiente convertirlo en ciclo de vida del renderer permanente.
- [ ] Gestionar fences y vida de recursos sin liberar trabajo en vuelo.
  00.28 espera cada fence antes de actualizar buffers/atlas; conserva recursos
  en PS4 hasta salir. Destruccion y recreacion comprobadas solo en host.
- [ ] Primero dibujar una secuencia limitada de frames; luego modo continuo.
  Secuencia de 64 frames reales implementada en 00.28; modo continuo pendiente.
- [ ] Docker: variacion entre frames, actualizacion de textura y reutilizacion
  sin acumulacion de memoria ni errores Vulkan.
  00.28: 120 frames controlados con cambios de atlas/vertices/scissor y pixeles
  comprobados; 64 frames distintos de Mario Kart, un dispositivo, sin errores
  Vulkan. Falta medir memoria/rendimiento en pruebas largas.
- [x] PS4 00.28: 64 frames distintos completados; usuario ve movimiento,
  lento, con Mario negro. Log confirma changed=63 y retorno a pantalla.
- [ ] PS4: varios minutos de juego continuo con controles.

### Cobertura de juegos y presentacion

- [ ] Textured rectangles, fill rectangles y sprites/HUD.
- [ ] Filtrado N64, LOD/multitextura y formatos/tamanos fuera del atlas inicial.
- [ ] Render targets, cambios de framebuffer y profundidad por target.
- [ ] Texturas procedentes del framebuffer y sincronizacion CPU/GPU.
- [ ] Presentacion VideoOut del resultado del juego y escalado/aspecto correctos.
- [ ] Determinar fallback de operaciones no soportadas y evitar duplicarlas
  entre CPU y GPU; no desactivar Angrylion hasta cubrir las dependencias.
- [ ] Verificar guardar/cargar, pausa, biblioteca y cambio de juego.

### Rendimiento y entrega

- [x] 00.29: cache acotada de texeles entre comandos consecutivos de
  triangulos; invalidar ante cada comando de estado/carga/sincronizacion.
- [x] Docker 00.29: comparar 64 frames de ROM con y sin cache; hashes de
  framebuffer y capturas inicial/final. Menos decodificaciones sin cambiar imagen.
- [ ] PS4 00.29: medir tiempo de secuencia y revisar rechazos de textura.
- [ ] Medir CPU, GPU, copias, cache de texturas, memoria y frame pacing.
- [ ] Comparar la misma escena y ROM con 00.13; distinguir VI, frames del juego
  y FPS de la pantalla de pausa.
- [ ] Optimizar batches y cache solo despues de comprobar fidelidad.
- [ ] Publicar PKG con version, fuentes, hashes, pruebas y limitaciones.
- [ ] Mantener una opcion estable y documentar resultados reales de PS4.

### Entorno de validacion y sus limites

Docker ejecuta Linux. OpenOrbis proporciona headers, bibliotecas y herramientas
para construir binarios PS4; no ejecuta el sistema operativo de PS4 ni reproduce
su GPU Liverpool, GNM, coherencia de memoria, VideoOut o firmware. El SDK no
convierte las pruebas Docker en un entorno identico a una consola.

Usar tres puertas: pruebas de logica/pixeles en Docker; compilacion y auditoria
del artefacto PS4; ejecucion del PKG en hardware real. Para la ultima hace falta
la PS4 del usuario (o acceso autorizado a hardware compatible). Actualmente
no hay una consola conectada al entorno de desarrollo.

Comandos locales existentes, desde Docker nps64-gpu-builder:local con /work:

    bash scripts/test_hle_game_texture.sh
    bash scripts/test_hle_game_combiner.sh
    bash scripts/test_hle_game_sequence.sh
    bash scripts/test_hle_game_depth.sh
    bash scripts/test_hle_probe.sh
    bash scripts/test_hle_game.sh "ruta a ROM local"
    bash scripts/build_hle_game.sh
    python3 scripts/check_hle_game_artifacts.py

Actualizar este TODO al implementar cada bloque y registrar que evidencia
proviene de Docker y cual proviene de la consola.
