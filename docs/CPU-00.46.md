# CPU 00.46: AudioOut continuo y menos trabajo de audio

## Evidencia de consola

Se analiza solo la ultima sesion 00.45 de nps64.log, no las sesiones anteriores:
29 informes de audio, ultimo con 165.3 ms en cola, 3 agotamientos y 35682 muestras
rechazadas. Hay 57 ventanas de juego: media 49.71 VI/s, RDP 5.95 ms, core 9.45 ms,
dibujo 1.36 ms y flip 2.57 ms. Cinco ventanas bajan de 48 VI/s. RDP esta incluido
en core_ms. La cola llena y los descartes muestran perdida de continuidad; la
reserva no era la unica causa. El filtro sinc no evita la perdida de muestras.

## Entrega de audio

La ruta anterior hacia Output(NULL) antes de cada bloque: esperaba/vaciaba
AudioOut y luego rellenaba/enviaba el siguiente bloque. Se usa ahora el patron
de PS4AudioStream::OutputLoop en reference/DolphinPS4/patches/dolphin-ps4.patch:
dos buffers alternados y Output(buffer), que marca el ritmo segun el anterior.
El otro buffer se prepara mientras continua la reproduccion. NULL se reserva
para el cierre y se llama desde el worker antes de que sus buffers de stack
dejen de existir. No se permite sobrescribir el buffer pendiente.

Se solicita scePthreadSetprio(scePthreadSelf(),400), como el hilo de audio del
port DolphinPS4. No se fija afinidad; no se cambia la prioridad de renderer
ni frontend. Un rechazo se registra y no impide el sonido. El worker no escribe
logs ordinarios con prioridad alta: evita competir por el lock de log de main.
Los errores AudioOut se publican al hilo principal para registrarlos.

## Rendimiento

La cola publica hasta 256 muestras con un solo store release, frente a un store
por cada muestra; copia por fragmentos contiguos, tanto en entrada como salida.
Los indices de productor y consumidor estan en lineas de cache separadas.
Esto reduce atomicos y competencia entre los hilos sin eliminar muestras.

FIR sinc32 utiliza SSE2, historiales float duplicados para lecturas contiguas y
tablas de coeficientes conservadas. Comparacion local O2: 0.020-0.028 ms SIMD
frente a 0.043-0.053 ms escalar por 1024 muestras de entrada, aproximadamente
la mitad. No son tiempos de PS4 ni equivalen a una mejora de FPS del juego.
La agrupacion de sumas puede diferir hasta una unidad S16 de la referencia.

La copia del framebuffer nativo se hace de una vez si pitch coincide con el
tamano de fila; se conserva la copia por filas para datos con padding.
PGO del rasterizador, seis trabajadores, heap 512 MiB, filtro VI, bloques de
1024 y prebuffer de 5760 muestras (120 ms) permanecen.

Se probo mtune=btver2 solo en el rasterizador con PGO, en dos rondas invertidas.
No hubo mejora consistente en el host: se descarta, manteniendo las opciones
anteriores. Las mediciones de core incluian hashes de imagen; no son FPS PS4.

## Validacion

- ASan/UBSan: AudioOut simulado retiene el puntero previo hasta completar la
  siguiente entrega. Verifica alternancia, datos estereo, ausencia de drain
  entre bloques y drain antes de destruir buffers. Incluye prioridad rechazada.
- Backend: presentacion, variante TSC, modulos preexistentes y fallo UserService.
- Cola: orden exacto, wrap en lotes, reset, limites y 20 segundos con jitter y
  pausas periodicas de 40 ms sin agotamiento ni muestras rechazadas.
- FIR: ocho frecuencias, batches completos/variables identicos, DC y reset.
  SIMD frente a escalar: diferencia maxima 1 S16 en la senal de comparacion;
  pruebas de espectro y seno siguen pasando.
- Mario 64 PAL: 3952 hashes y 133 estados CPU/VI identicos a CPU39. Cuatro
  ejecuciones del experimento de compiler tambien coinciden entre si.
- OpenOrbis genera el paquete; continuidad y rendimiento reales requieren PS4.

## Log esperado

    [audio] ... resampler=sinc32 input_hz=... output_hz=... priority_result=0 protocol=pipelined

output_hz mide bloques enviados por segundo (incluye silencio de arranque),
con ruido de cuantizacion de 1024 muestras en ventanas de unos dos segundos.
Deberia rondar 48000 en juego estable; priority_result=0 indica solicitud aceptada.
Comparar si underruns/dropped dejan de aumentar y si mejoran core_ms y velocidad.
La ausencia de cortes aun no esta confirmada en PS4. CPU45 queda preservada.
