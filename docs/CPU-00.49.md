# CPU 00.49: spans especializados de Zelda PAL GC

El log de CPU48 muestra que la intro recupera aproximadamente 50 VI/s,
pero la seleccion de archivo queda en 42-43 VI/s: RDP cuesta unos 18 ms,
core unos 21 ms y dibujo/copia agregan mas tiempo. El limitador ya no
duerme en esa escena. RDP esta incluido en core; no deben sumarse ambos.

## Cambio

Las tres rutas de rasterizado de dos ciclos seleccionan una vez por
primitiva las funciones de framebuffer de 16 bits y division de coordenadas.
Sus copias especializadas integran esas funciones en el bucle de pixeles,
evitando llamadas indirectas repetidas. Mantienen las mismas operaciones
enteras, cobertura, filtros, texturas, color y profundidad de Angrylion.
Hay rutas separadas para perspectiva activada y desactivada, y alternativa
generica cuando el estado no coincide. Se activan solamente para el CRC
de Ocarina of Time PAL GC, como la optimizacion CPU47.

Se entrenan trece funciones nuevas con la ROM local, incluida la seleccion
de archivo. Los contadores PGO anteriores se conservan y se comprueban
registro por registro. El perfil CPU47 queda en cpu-rdp-lanes.profdata;
scripts/train_cpu_spans.sh permite regenerar el nuevo perfil.
Las ROMs no forman parte de los paquetes de fuentes.

En PS4, cada hilo de render consulta su prioridad y solicita 450 solamente
si tiene una prioridad inferior. Conserva una prioridad superior y la
afinidad que asigna el sistema. Si la llamada falla conserva la configuracion
existente; el log cpu-thread registra el resultado y la mascara permitida.
AudioOut sigue en 400, por delante del render. Esto no agrega nucleos ni
permite medir de antemano el efecto de la prioridad en una PS4 real.

Se mantienen seis trabajadores, heap de 512 MiB, buffer de audio de 120 ms,
resampler y AudioOut continuo de CPU46, resolucion y filtro VI. No se
reactiva Vulkan. La presentacion conserva los tres buffers de CPU48.

## Mediciones locales

Dos pares secuenciales de la misma ROM, seis trabajadores y 3500 VIs:

| Seleccion de archivo, VIs 1900-3499 | CPU48 ms | CPU49 ms | Reduccion |
| --- | ---: | ---: | ---: |
| Par 1 | 9.840 | 7.213 | 26.7 % |
| Par 2 | 8.857 | 7.311 | 17.5 % |

Son tiempos nativos de PC en Docker, sin limitador. Docker no emula la
PS4 y estos valores no predicen sus FPS. La dispersion entre pares exige
confirmar la ganancia en la consola, especialmente en la seleccion y juego.

## Validacion

El rasterizador conserva 48 comparaciones completas de color/profundidad,
incluido render-to-texture, con 1/4/6/8 trabajadores. El calculo de propiedad
de lineas pasa 8 millones de valores aleatorios y 64 strides. Zelda conserva
3485 imagenes completas y 116 estados CPU/VI, incluyendo seleccion de archivo.
Mario conserva 3952 imagenes y 133 estados; la muestra local no muestra
regresion, pero no se atribuye una mejora a una ruta que no activa este juego.
El backend PS4 se comprueba con ASan/UBSan, servicios simulados, buffers
retenidos, AudioOut continuo y rechazo de cambios de prioridad.

Pruebas reproducibles: scripts/test_cpu_rdp.sh (dos archivos del core),
scripts/test_cpu_lane.sh (fuente del core parcheada),
scripts/test_cpu_platform.sh y scripts/test_gpu_platform.sh. La comparacion
de ROMs utiliza NPS64_CPU_FRAME_HASHES=1 y tests/compare_cpu_images.py;
BENCH_ZELDA_MENU=1 activa Start para llegar al menu durante el benchmark.

El PKG pasa 28 controles y su payload extraido coincide con el stage.
Artefacto: out/NPS64-00.49-cpu.pkg; CPU48 queda conservado.

Los informes locales estan en build/cpu49-qa.log y
build/cpu49-ps4-build.log. La prueba final de FPS y audio requiere PS4 real.
