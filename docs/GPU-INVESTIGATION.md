# Investigación GPU tras el bloqueo de 00.12

Estado: GPU PS4 sigue suspendida. El usuario confirma que 00.13 funciona,
con bajones ocasionales. No hay una consola conectada a este entorno.

## Lectura de la traza

La submission 37 no termina; 36 es la última completada. En el informe de
00.12, el destino compartido de WRITE_DATA contiene 3. Esto significa que
el procesador de comandos alcanzó el tercer marcador: los valores 1 y 2
fueron sobrescritos. Las etiquetas NOT DONE para esos dos valores no
demuestran que sus escrituras no se ejecutaron.

Entre el marcador 3 y el 4 están CS_PARTIAL_FLUSH, PFP_SYNC_ME y
SURFACE_SYNC. El tercer DISPATCH está después del marcador 4. Por tanto,
la traza apunta a trabajo anterior pendiente al llegar a esa barrera;
no demuestra un fallo del tercer shader, que aún no se despachó.

Los dos trabajos anteriores son `span_setup` (64 hilos por grupo) y
`tile_binning_combined` (32 hilos por grupo, LDS y dos barreras cuando
SUBGROUP=0). El marcador del procesador de comandos no certifica que
ninguno de esos trabajos haya terminado. Todavía no se ha aislado cuál
falla ni si el origen es shader, direcciones, coherencia o estado GNM.

## Nuevas herramientas

`scripts/test_gpu_compute_probe.sh` compila un shader pequeño y ejecuta:

1. Escritura exacta a un storage buffer con 64 hilos.
2. LDS, atomicOr, barreras y recorrido de máscara con 32 hilos.
3. La misma operación con 64 hilos.

Cada caso hace dos despachos separados por dependencia compute, espera
un fence y verifica todos los valores y la zona de guarda. Usa validación
Vulkan y falla si encuentra errores. Son pruebas de Lavapipe en Docker;
no usan RADV PS4, ACO Liverpool ni GNM. Su éxito no valida la consola.
El timeout host tampoco permite recuperar una GPU física bloqueada.

`scripts/patch_core.py` añade `NPS64_RDP_SERIALIZE_SETUP=1`: introduce una
barrera entre spans y tiles, tanto en resolución nativa como escalada.
Esto permite diferenciar el bloqueo previo a tiles del posterior. Está
desactivado por defecto y es diagnóstico, no una corrección demostrada.
Los shaders y resultados de emulación permanecen iguales. La cuarentena
PS4 continúa activa: esta variable no la elude.

Ejecutar desde la raíz del proyecto, con el bind habitual de Docker:

```sh
bash scripts/test_gpu_compute_probe.sh
bash scripts/build_native_gpu.sh
NPS64_RDP_SERIALIZE_SETUP=1 bash scripts/test_gpu.sh
NPS64_RDP_SERIALIZE_SETUP=1 BENCH_FRAMES=90 bash scripts/bench_gpu.sh 'Mario Kart 64 (E) (V1.0) [!].z64'
```

El siguiente paso de hardware requiere una reproducción mínima usando
RADV/GNM y aislamiento de los dos primeros shaders. Hasta entonces no
se genera un PKG que vuelva a habilitar esta ruta ni se considera resuelto
CE-36329-3. Se mantiene el paquete funcional 00.13.

## Resultados locales de esta investigación

- Tres probes de cómputo pasan con validación Vulkan, valores exactos y guarda.
- Núcleo real recompilado con el diagnóstico de separación.
- Dos sesiones Vulkan con el diagnóstico activo: imagen, estados y cierre pasan.
- Fallback por driver ausente y por renderer no disponible pasan.
- Mario Kart PAL: 90 frames solicitados, imagen visible y cierre limpio con el
  diagnóstico activo. No se interpreta la velocidad de Lavapipe como FPS de PS4.
- Petición GPU en configuración y entorno continúa bloqueada en la prueba
  de cuarentena; dos sesiones CPU con imagen y estados pasan.

Evidencia en `build/gpu-compute-probe/results.log`,
`build/gpu-functional-test.log`, `build/gpu-bench-results.log` y
`build/gpu-bench/nps64.log`. Son resultados de host, no de consola.
