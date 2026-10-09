# CPU 00.42: audio 120 ms y comparacion de trabajadores

El usuario confirma que CPU41 mejora el sonido, pero quedan microcortes.
Se aumenta la reserva inicial/de recuperacion a 5760 muestras estereo:
120 ms a 48 kHz. Se conservan la capacidad de 8192 muestras y los bloques
AudioOut de 1024 (21.33 ms). No cambia el remuestreo ni la velocidad del juego.
La latencia del sonido aumenta; el prebuffer no corrige falta sostenida de CPU.

La reserva no es multiplo del bloque. La prueba de agotamiento conserva las
muestras residuales al rebuffer, sin eliminarlas ni reordenarlas. ASan/UBSan
aprueba limites, orden, reset, overflow y wrap del contador unsigned.
Simulacion de 20 segundos: batches PAL de 960 muestras cada 20 ms, jitter
alternado de 8 ms, mas retrasos periodicos de 40 ms; sin agotamientos ni
muestras descartadas. No simula el scheduler ni AudioOut de la PS4.

## Trabajadores

Seis ejecuciones Mario 64 PAL de 4000 iteraciones, entrada fija, mismo core
PGO, orden 6/7/8 seguido de 8/7/6. Medidas en Docker Linux, sin limitador;
3251 muestras por ejecucion, intervalos 650..3900. Los 133 estados CPU/VI
coinciden en las seis ejecuciones. Audio de host no reproduce el dispositivo PS4.

| Trabajadores | Total medio ronda 1 (ms) | Total medio ronda 2 (ms) | P95 ronda 1 (ms) | P95 ronda 2 (ms) |
| --- | ---: | ---: | ---: | ---: |
| 6 | 4.526 | 4.399 | 8.925 | 8.922 |
| 7 | 5.048 | 4.532 | 10.192 | 9.258 |
| 8 | 4.774 | 4.735 | 9.229 | 9.344 |

Seis fue mas rapido en ambas rondas. Se conserva el predeterminado de seis;
no se promete mejora por aumentar el numero. Son resultados locales, no una
comparacion de trabajadores en PS4. La opcion diagnostics.cfg existente
permite comparar en consola sin recompilar:

    NPS64DIAG5 6 1 1 1 0
    NPS64DIAG5 7 1 1 1 0
    NPS64DIAG5 8 1 1 1 0

Cada linea es una alternativa para /data/NPS64/diagnostics.cfg, no deben
escribirse las tres. Confirmar workers/requested_workers en el log.
Conservar filtro VI, escena y tiempo de prueba; evaluar audio y speed_pct.

## Paquete y prueba PS4

CPU42 conserva Angrylion PGO, SSE2, rasterizador y presentacion CPU40.
No incluye Vulkan. CPU40 y CPU41 quedan preservadas. OpenOrbis genera el
paquete; las pruebas del backend simulado aprueban cuatro variantes.
En consola revisar:

    [audio] queued_ms=... underruns=... dropped=... block=1024 prefill=5760

underruns/dropped son acumulativos: deberian dejar de aumentar durante juego
estable. queued_ms fluctua segun las rafagas; no tiene que ser exactamente 120.
El inicio acumula hasta 120 ms antes de reproducir; tras un agotamiento vuelve
a acumular la misma reserva. Validacion de continuidad/latencia en PS4 pendiente.

Resultados locales: build/cpu42-workers-performance.json y logs por ronda.
