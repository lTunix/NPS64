# CPU 00.50: GoldenEye y arranque de Mario Tennis VC

## Cambio validado

El rasterizador conserva exactamente el codigo de CPU49. El nuevo perfil
PGO se entrena con GoldenEye USA (8000 VIs, intro, seleccion y Dam), Mario
64 PAL (2000 VIs) y Ocarina of Time PAL GC (2300 VIs, incluido el menu).
El compilador utiliza estos contadores para ordenar y optimizar los caminos
que realmente recorren esos juegos. El perfil se guarda en
`assets/cpu-rdp-games.profdata`; el de CPU49 permanece en
`assets/cpu-rdp-sm64.profdata` para conservar la referencia.

Se descartaron las ampliaciones de spans, la simplificacion de limites por
fila, O3 y el perfil IR: no mejoraban Dam de forma consistente o lo empeoraban.
Ninguno de esos cambios de rasterizado forma parte de CPU50.

Se mantienen seis trabajadores, heap de 512 MiB, ocho MiB de RDRAM emulada,
audio continuo de 120 ms, resampler, prioridades, resolucion, filtros y tres
buffers de presentacion. Vulkan sigue desactivado. Los archivos de cola y
resampler de audio coinciden byte a byte con CPU49.

## Mediciones nativas, sin limitador

Se alternan CPU49 y CPU50 en orden A-B-B-A con seis trabajadores, controles
automaticos iguales y los mismos guardados iniciales. Los dos recorridos de
GoldenEye llegan a Dam; se comprueba visualmente la escena de juego.

| GoldenEye USA: Dam, VIs 6500-7999 | CPU49 ms | CPU50 ms | Reduccion |
| --- | ---: | ---: | ---: |
| Par 1 | 9.160 | 7.009 | 23.5 % |
| Par 2, orden invertido | 7.786 | 6.709 | 13.8 % |

La seleccion de GoldenEye, VIs 1800-3999, baja de 10.725 a 8.111 ms y de
9.445 a 8.011 ms: reducciones de 24.4 % y 15.2 %.

| Regresion, mismo orden A-B-B-A | CPU49 ms | CPU50 ms |
| --- | ---: | ---: |
| Mario 64, VIs 1000-3999, par 1 | 4.525 | 4.253 |
| Mario 64, par 2 | 4.140 | 3.932 |
| Zelda, seleccion VIs 1900-3499, par 1 | 7.352 | 6.066 |
| Zelda, par 2 | 6.618 | 6.493 |

Son mediciones de PC en Docker: no emulan los procesadores ni el planificador
de PS4 y no permiten prometer sus FPS. La variacion entre pares exige probar
el PKG en consola. GoldenEye USA tiene un objetivo de aproximadamente 60 VI/s;
los juegos PAL de estas pruebas tienen 50 VI/s. No se garantiza una mejora
universal por optimizar un solo juego.

## Mario Tennis (E) (VC)

El archivo local, MD5 `D40FB0E87996AF902EFF9C194027FC3B`, conserva el checksum
de encabezado `839F3AD5 406D15FA`, pero su codigo produce `639F3AD5 A394CB4F`.
El cargador IPL3 X102 se queda en su bucle de error en PC `800001C8`, antes
de generar el primer VI. Se reproduce tanto con Ari64 como con interprete
cacheado; no es un bloqueo del rasterizador.

`src/rom_compat.h` verifica tamanio, MD5, encabezado y checksum del contenido,
y corrige solo los ocho bytes correspondientes de la copia en memoria.
La intervencion ocurre despues de identificar la ROM en la base de datos:
conserva identidad, tipo de guardado y nombre. No modifica el archivo del
usuario ni aplica la reparacion a otras ROMs.

Con la reparacion, Mario Tennis completa 1800 VIs y genera imagen visible
desde el cuadro 27. Se verifica visualmente la presentacion animada con Peach
y Daisy. La prueba en PS4 sigue pendiente; no se afirma que toda una partida
haya sido validada en consola.

## Calidad y controles

- GoldenEye: 7937 imagenes completas y 266 trazas CPU/VI identicas a CPU49.
- Mario 64: 3952 imagenes y 133 trazas identicas.
- Zelda: 3485 imagenes y 116 trazas identicas.
- Mario Tennis: 1785 imagenes y 59 trazas identicas al core CPU49 con la
  misma reparacion de arranque, antes del cambio de PGO.
- 96 comparaciones completas de color y profundidad, ambos perfiles de
  propiedad de filas, con 1/4/6/8 trabajadores y render-to-texture.
- Ocho millones de casos y 64 strides para la propiedad de filas.
- ASan/UBSan para la reparacion y backend PS4 simulado: audio, buffers
  retenidos, errores del SDK y rechazos de prioridad.

Los logs de prueba estan en `build/cpu50-qa.log`,
`build/cpu50-golden-fe-perf-*.txt`, `build/cpu50-*-perf-*.txt`,
`build/cpu50-quality-*.log` y `build/cpu50-ps4-build.log`.

## Reproduccion y entrega

`scripts/train_cpu_games.sh` recibe las tres ROMs locales en orden GoldenEye,
Mario y Zelda, y genera un perfil candidato en `build`; no reemplaza el
perfil publicado sin validarlo. Las ROMs no se incluyen en las fuentes.
Para comparar se usan `BENCH_GOLDENEYE=1`, `BENCH_SM64=1` o
`BENCH_ZELDA_MENU=1`, seis trabajadores y `BENCH_CPU_PROFILE=1` con
`scripts/bench.sh`. Se restauran guardados iniciales antes de cada recorrido.
Para calidad se activa `NPS64_CPU_FRAME_HASHES=1` y se comparan los logs con
`tests/compare_cpu_images.py`. Los tiempos con hashes no se usan para medir
rendimiento. `scripts/test_cpu_rdp.sh` recibe los dos archivos del core.

El build normal genera `out/NPS64-00.50-cpu.pkg`: pasa 28 controles y su
payload extraido coincide con el stage. El paquete de fuentes y
su lista SHA256 corresponden a esta misma version. CPU49 queda conservado.
La validacion final de VI/s y continuidad del audio requiere PS4 real.
