# ParaLLEl 00.56: localizar el cierre del compilador PS4

Los nuevos `logs/tile-probe.log` y `logs/boot.log` de 00.55 terminan en
`COMPILE pipeline case=1 local=64 begin`. La inicialización, el hilo con
8 MiB de pila y los recursos completaron correctamente. No aparece ningún
`SUBMIT`: este cierre ocurre dentro de `vkCreateComputePipelines`, antes
de ejecutar el shader. No demuestra que el bloqueo anterior de la GPU
tenga la misma causa.

## Comprobación local

Se construyó Mesa 26.0.8 RADV/ACO nativo con libdrm 2.4.125 y el drm-shim
AMD configurado como BONAIRE/GFX7. Ambas variantes del shader original
de tile binning (32 y 64 hilos, SPIR-V de 19572 bytes) compilan correctamente.
El modo `NPS64_TILE_COMPILE_ONLY=1` omite expresamente los comandos GPU y
las comprobaciones de píxeles. El shim no ejecuta shaders ni emula PS4:
el ABI, el sistema operativo y la GPU reales siguen pendientes de validar.

Reproducir desde Docker:

```sh
docker build -f Dockerfile.radv-compiler -t nps64-radv-compiler:local .
docker run --rm -v "$PWD:/work" nps64-radv-compiler:local bash scripts/build_radv_compiler.sh
docker run --rm -v "$PWD:/work" nps64-radv-compiler:local bash scripts/test_radv_tile_compiler.sh
```

Requiere las dependencias del proyecto y los encabezados de la captura
preparada en `build/parallel-tile-probe`. El resultado se guarda en
`build/parallel-tile-probe/aco-bonaire.log`.

## Prueba PS4 00.56

La nueva aplicación activa trazas únicamente para esta prueba. Los
marcadores escriben directamente y sincronizan `boot.log` sin usar el
asignador de memoria de la aplicación. Delimitan SPIR-V a NIR, optimización,
información del shader, argumentos, postprocesado, ACO y carga del código.
El algoritmo del shader y sus entradas permanecen iguales para aislar
la etapa del fallo. No se declara corregido el cierre.

```sh
bash scripts/build_mesa_ps4.sh
NPS64_TILE_USE_PREPARED=1 bash scripts/build_parallel_tile_probe.sh
```

Instalar `out/NPS64-00.56-parallel-tile-probe.pkg`, ejecutar el primer caso
de 64 hilos y conservar ambos logs. El último marcador `compiler ...`
identifica la siguiente función que debe investigarse. La aplicación CPU
00.51 y los paquetes anteriores conservan su identidad independiente.
