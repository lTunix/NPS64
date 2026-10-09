# Rendimiento y prueba de Mario Kart — 00.06

## Contador permanente y presentación — 00.06

El usuario confirmó una mejora parcial de 00.05 en PS4, pero el juego sigue
lento y el contador desaparece. El contador de arranque sólo se dibujaba
hasta first_visible_video. 00.06 dibuja un HUD permanente, actualiza cada
segundo y separa biblioteca/pausa/juego al medir. FPS cuenta frames de VI
emulados por segundo real; también muestra objetivo, velocidad porcentual,
CPU seleccionada y trabajadores realmente creados. «video» cuenta callbacks
con imagen, no frames únicos de animación. La ROM PAL solicita unos 50 VI/s.

El backend antes esperaba el flip recién enviado dentro de platform_present.
Ahora envía y retorna; emula el siguiente frame mientras el flip está pendiente,
y espera antes de reutilizar el otro buffer de scanout. Sólo hay un flip
pendiente y se drena al apagar. El test de backend verifica no encolar dos,
no liberar buffers pendientes y solapar frames de 10/20 ms con vblank.
En la simulación, 60 frames de 20 ms mantienen 1200 ms, sin redondear cada
frame a dos vblanks de 60 Hz. Eso valida el protocolo, no FPS de PS4 real.

El callback de video copia ahora la imagen nativa (Mario Kart: 640x288 RGBA
en el perfil preciso), y escala directamente al canvas de presentación.
Se elimina el buffer escalado intermedio de 3 686 400 bytes y su copia por
frame; sólo se borran los bordes. El origen nativo se conserva para pausa,
callbacks duplicados y cambio de aspecto. ASan/UBSan prueban formatos, pitch,
cambios de geometría y que el HUD o el menú no corrompan ese origen.

El reloj usa TSC invariante, anclado a sceKernelGetProcessTime y frecuencia
obtenida de sceKernelGetTscFrequency. Sigue el enfoque del runtime estudiado
de DolphinPS4; usa lfence/rdtsc y fallback al reloj de kernel si no hay una
frecuencia válida. Se prueban ambas rutas con ticks simulados: 1000 lecturas
en modo TSC no provocan otras 1000 llamadas al reloj de kernel.
Timers RDP/VI ayudan a localizar el coste de rasterización y filtrado.
Son tiempos de las entradas del renderer en el hilo llamador, incluyen
sincronización y están dentro de core_ms; no sumarlos como trabajos separados.

Se compararon secuencialmente el frontend/core de 00.05 guardados y 00.06,
con la misma carrera y 3600 frames, sin builds concurrentes:

| Perfil | Carrera: ms por VI | Total: ms por VI |
|---|---:|---:|
| 00.05, cuatro trabajadores, preciso | 5,199 | 4,246 |
| 00.06, cuatro trabajadores, preciso | 5,419 | 4,175 |
| 00.06, cuatro trabajadores, rápido | 5,402 | 4,112 |
| 00.06, seis trabajadores, rápido | 5,471 | 3,761 |
| 00.06, dos trabajadores, rápido | 7,087 | 5,392 |
| 00.06, cuatro trabajadores, preciso, -O3 | 6,146 | 4,727 |

El host no muestra una mejora consistente del núcleo por esos ajustes;
se conserva -O2, cuatro trabajadores y modo preciso. El beneficio del flip
solapado y del reloj TSC requiere medir la consola: el backend del benchmark
descarta vsync y no llama al kernel PS4. No extrapolar estos resultados a
Jaguar ni presentarlos como FPS alcanzados en hardware.

La captura precisa del frame 3570 coincide píxel por píxel con 00.05 al
excluir los primeros 56 píxeles de altura del HUD. Imagen rápida es opcional
en pausa: desactiva filtrado VI, cambia nitidez/márgenes y no hace frameskip.
Se verificó carrera también con dos/seis trabajadores y ese modo.
Registros: build/bench-00.06-*.log. Capturas: mario-kart-00.06-filtered.png
y mario-kart-00.06-fast.png dentro de build. Para repetir perfiles actuales,
scripts/compare_render.sh usa el núcleo nativo JIT; la comparación 00.05
requiere además los dos archivos baseline-00.05-* conservados en build.

DolphinPS4 usa Vulkan/RADV y la GPU. NPS64 sigue rasterizando en CPU;
la generación de la consola emulada por sí sola no determina el coste.
La siguiente mejora estructural pendiente es el renderer acelerado por GPU,
con su contexto libretro, shaders y sincronización de memoria. No está
incluido en 00.06. Para diagnosticar la PS4, leer el HUD y [perf] durante
la misma carrera; confirmar Ari64 y workers=4 antes de comparar.

## Optimización 00.05

El usuario confirmó que 00.04 tiene interfaz rápida y Mario Kart arranca
en PS4, pero estima 15 FPS o menos. 00.05 compila Ari64 x64, regenera los
offsets de ensamblador con el compilador de cada plataforma y adapta mprotect
al kernel PS4 y sus páginas de 16 KiB. Se comprueba el permiso del cache
de 32 MiB antes de elegir JIT; si se rechaza, se usa cached interpreter.
El cache permanece en BSS cerca del ELF para conservar el alcance rel32.
Angrylion usa cuatro trabajadores por defecto; RSP permanece HLE.

Comparación secuencial en el mismo PC, sin builds concurrentes, con el mismo
binario nativo, ROM PAL, memoria comprimida, entradas y 3600 frames de VI.
El promedio de carrera usa las últimas 1000 muestras:

| CPU / renderer | Carrera: ms por frame | VI por segundo sin límite |
|---|---:|---:|
| Intérprete / síncrono | 13,948 | 71,69 |
| Ari64 / síncrono | 8,616 | 116,06 |
| Ari64 / cuatro trabajadores | 4,966 | 201,38 |

El JIT reduce el coste de carrera 38,2 %. Ambos cambios reducen el coste
64,4 %, equivalente a 2,81 veces la capacidad del perfil anterior en este
benchmark. El promedio completo (intro, menús y carrera) pasa de 11,046
a 3,818 ms. Son tiempos del PC y frames de VI, no FPS comprobados de PS4
ni frames únicos de animación; la ROM PAL solicita 50 VI/s. No se extrapolan
estas cifras al procesador Jaguar de la consola.

Las capturas de carrera en el frame 3570 son idénticas píxel por píxel
entre los tres perfiles. Se ve Luigi Raceway, Mario, lap 1 y tiempo 19,88.
Registros preservados: build/bench-00.05-cached.log,
build/bench-00.05-jit.log y build/bench-00.05-jit-workers4.log,
con sus archivos *-core.log. Copias entregadas en out/bench-00.05-*.log.
Captura local: build/mario-kart-jit-workers4-race.png.

Pasaron estados, dos cargas de contenido y retorno a biblioteca con JIT
síncrono y cuatro trabajadores. ASan/UBSan verifican alineación, denegación
y overflow del wrapper de permisos. Eso no verifica el permiso real de PS4.
La cobertura exhaustiva de CPU/TLB/delay slots y otros juegos sigue pendiente.

Para repetir: construir el núcleo con scripts/build_native_jit.sh y ejecutar
scripts/compare_cpu.sh dentro del contenedor. Conserva la ROM en raíz.
Para activar el nuevo perfil en consola, retirar un diagnostics.cfg antiguo
o escribir NPS64DIAG3 4 1 1. Confirmar selected=Ari64 y workers=4 en el log
y comparar la misma carrera con el perfil anterior NPS64DIAG3 0 1 0.

## Historial 00.04

Fecha: 5 de octubre de 2026. Docker Linux en el PC del usuario.
ROM facilitada en raíz: Mario Kart 64 (E) (V1.0) [!].z64, 12 582 912 bytes,
PAL 50 Hz. Las pruebas leen el archivo local; no se añade al PKG ni a las fuentes.

## Problemas de la ruta PS4

00.03 dibujaba rectángulos y glifos directamente en memoria de scanout con
write-combining, mediante muchos stores pequeños y dispersos. 00.04 dibuja en
RAM con caché y hace una copia secuencial por frame, seguida de sfence.
El efecto en PS4 real se debe medir con los contadores nuevos del menú.

freopen sobre stderr no era una ruta fiable para el log: OpenOrbis musl
__stdio_write.c trata fd 1/2 como salida debug, y freopen conserva fd 2.
00.04 usa sceKernelOpen/sceKernelWrite con descriptor propio. Eso también
retira los mensajes de entrada de ese canal. Fuente estudiada:
https://github.com/OpenOrbis/musl/blob/2b0bb8ce2fa64ac255cb656785729d159a5f074c/src/stdio/__stdio_write.c
No se asume que el problema del canal debug explique toda la latencia.
El desensamblado de __stdio_write del ELF final confirma las rutas distintas
para fd 1/2: build/sdk-stdio-write-disassembly.log.

Se usa sceKernelGetProcessTime en microsegundos y sceKernelUsleep para el
frontend. El flip comienza en 1 para distinguirlo del flipArg inicial 0.
El escalado precalcula los índices X y reutiliza filas para reducir divisiones
y conversiones repetidas. Se verifican XRGB8888, RGB565, 1555, pitch y aspecto.

## Benchmark del núcleo y frontend

tests/platform_bench.c ejecuta el frontend y núcleo reales con framebuffer
en RAM, reloj del host, audio descartado y entradas programadas. No espera
vsync ni limita los FPS. Usa la memoria comprimida del port PS4, CPU cached
interpreter y Angrylion síncrono. Se excluye el tiempo de capturas/reportes.
La ROM de prueba original sigue verificando estados y dos cargas por separado.

Comparación inicial de 899 frames de intro/título, antes del último ajuste
del escalado, bajo las mismas condiciones:

| Perfil | Tiempo medio por frame | Capacidad sin límite |
|---|---:|---:|
| cxd4 | 12,066 ms | 82,88 FPS |
| HLE | 8,904 ms | 112,31 FPS |

HLE redujo el tiempo medio aproximadamente 26 %. Registros:
build/bench-cxd4-results.log y build/bench-hle-results.log.

Prueba final HLE con navegación programada, 3599 muestras de juego:

| Medida | Resultado |
|---|---:|
| Menú, 197 iteraciones | 0,261 ms de media; 0,439 ms máximo |
| Intro + menús + carrera | 11,347 ms de media; 88,13 FPS sin límite |
| Carrera, últimas 1000 muestras | 14,608 ms de media; 68,45 FPS sin límite |

Llegó al título, seleccionó Mario y entró en una carrera; la captura local
build/mario-kart-race.png muestra el kart, pista, tiempo y clasificación.
La medición de carrera no es sólo la pantalla de título. Los valores varían
con la carga del PC; el benchmark representa una ejecución, no un percentil
estadístico ni una comparación con Dolphin en la misma consola.

## Repetir

Desde PowerShell, después de .\scripts\build.ps1 -Tests:

```powershell
docker run --rm --mount "type=bind,source=$($PWD.Path),target=/work" -w /work -e BENCH_FRAMES=3600 -e BENCH_RSP=hle -e BENCH_RACE=1 -e BENCH_TIMEOUT=240 nps64-builder:local bash scripts/bench.sh 'Mario Kart 64 (E) (V1.0) [!].z64'
```

BENCH_RSP=cxd4 compara el otro plugin; BENCH_WORKERS=4 activa trabajadores.
Los resultados se guardan en build/bench-results.log y build/benchdata/nps64.log.
La prueba utiliza partidas separadas dentro de build/benchdata/saves.
La ejecución final está preservada en build/bench-00.04-results.log y
build/bench-00.04-core.log; out/bench-00.04.log contiene las medidas entregadas.

## Límite de estas medidas

Los FPS anteriores son del PC en Docker. El backend no ejecuta VideoOut,
AudioOut ni memoria write-combined real. Los mocks prueban el contrato de
esas funciones, no su velocidad. La latencia de un botón físico y el FPS de
la PS4 sólo se confirmarán instalando 00.05 y leyendo la telemetría del menú/log.
00.04 no tenía JIT; 00.05 lo incorpora. La ruta GPU se añade en 00.07; falta medirla en PS4.

## 00.07: prueba funcional de Vulkan

Mario Kart llega a una carrera con paraLLEl-RDP y Lavapipe en Docker. No se
usa ese FPS para predecir la GPU de PS4: Lavapipe rasteriza los shaders en CPU.
El coste del primer arranque incluye compilación de pipelines. No hay shader
cache persistente todavía. La presentación conserva el canvas CPU y añade una
transferencia GPU a RAM a resolución nativa; eliminarla es la siguiente mejora.
Se fuerza RDP síncrono para proteger lecturas/escrituras de RDRAM entre CPU y GPU.

Comando local (la ROM no se distribuye):

```powershell
docker run --rm --mount "type=bind,source=$($PWD.Path),target=/work" -w /work -e BENCH_FRAMES=2400 -e BENCH_RACE=1 nps64-gpu-builder:local bash scripts/bench_gpu.sh 'Mario Kart 64 (E) (V1.0) [!].z64'
```

Log y resultados: `build/gpu-bench/nps64.log`, `build/gpu-bench-results.log`.
Confirmar en consola el label GPU, FPS/objetivo, audio y estabilidad antes de
modificar resolución interna o habilitar RDP asíncrono por defecto.
