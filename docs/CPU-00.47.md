# CPU 00.47: Ocarina of Time PAL GameCube

El usuario confirma audio correcto con CPU46, pero observa bajones en la intro
de Legend of Zelda, The - Ocarina of Time (E) (GC) [!].z64.
El log disponible en la raiz todavia corresponde a CPU45/Mario; esta version
se evalua con la ROM local, sin atribuirle mediciones de PS4 que no tenemos.

## Cambio

La asignacion de scanlines a los trabajadores calculaba un resto entero por
linea y por triangulo. La ruta nueva prepara un reciproco por triangulo y
obtiene exactamente el mismo resto mediante multiplicaciones y correccion
entera. Conserva la conversion unsigned de las lineas negativas.

Se selecciona solo para el CRC de cabecera 09465AC3/F8CB501B de esta ROM.
Reconoce .z64, .v64 y .n64 independientemente del nombre del archivo.
Otros juegos usan la funcion anterior. La seleccion ocurre antes de arrancar
los hilos del contenido, sin cambiarla durante el rasterizado.

Las variantes globales se descartaron porque perjudicaban a Mario en PC.
La funcion original se conserva y assets/cpu-rdp-base.profdata guarda el perfil
de CPU46. El perfil actual agrega exclusivamente las cuatro funciones nuevas;
los contadores anteriores se mantienen. scripts/train_cpu_lanes.sh y
scripts/merge_cpu_lane_profile.py permiten regenerarlo con ROMs locales.

No cambia el filtro VI, las reglas de color/texturas/profundidad, seis workers,
heap de 512 MiB ni el audio de CPU46 (120 ms, sinc32/SSE2 y AudioOut continuo).

## Rendimiento local

Docker ejecuta el core x86-64 nativo; no emula tiempos ni hardware PS4.
Dos pares con orden invertido, seis trabajadores, sin calculo de hashes:

| Intro Zelda, ms por iteracion | CPU46 | CPU47 |
| --- | ---: | ---: |
| Media, primera pareja | 5.200 | 4.928 |
| Media, repeticion invertida | 5.302 | 5.086 |
| Percentil 95, primera pareja | 15.543 | 14.712 |
| Percentil 95, repeticion | 16.649 | 15.075 |

Media conjunta: 5.251 -> 5.007 ms, 4.65% menos. Percentil 95 medio:
16.096 -> 14.894 ms. No garantiza 50 VI/s en consola.
Mario conserva su ruta estandar: medias 4.436/4.728 -> 4.394/4.277 ms,
sin retroceso observado en esta comparacion; la variacion del PC impide
atribuir esa diferencia a una optimizacion de Mario.

## Validacion

- Seleccion por CRC: tres ordenes de bytes, cabeceras cortas y otras ROMs.
- Resto exacto: 64 strides, lineas -8192..8192 y 8M valores unsigned aleatorios,
  con ASan/UBSan. scripts/test_cpu_lane.sh extrae la funcion realmente compilada.
- scripts/test_cpu_rdp.sh compara buffers completos de color y profundidad con
  1/4/6/8 trabajadores, triangulos fuera de pantalla y renderizado a textura.
- Zelda: 3485 imagenes completas y 116 estados CPU/VI identicos a CPU46.
- Mario: 3952 imagenes completas y 133 estados CPU/VI identicos a CPU46.
- 48 comparaciones de framebuffer/profundidad definitivas aprobadas.
- Los tres archivos de audio coinciden byte a byte con las fuentes de CPU46.
- Build OpenOrbis con rechazo de perfiles desactualizados; PKG de 6619136 bytes,
  28 controles aprobados y payload extraido identico al stage.

## Prueba en PS4

Instalar out/NPS64-00.47-cpu.pkg y abrir esa ROM. Log esperado:

    NPS64 00.47 CPU ONLY core 890323d Ari64 / Angrylion PGO
    [cpu-profile] zelda-pal-gc reciprocal lanes

Otros juegos muestran [cpu-profile] standard lanes. Comparar la misma intro
con CPU46 y revisar vi_fps, speed_pct y core_ms/RDP. RDP esta incluido en core_ms.
Conservar nps64.log de esta prueba para localizar los bajones restantes.
