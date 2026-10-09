# Repositorio y ramas

Repositorio: https://github.com/lTunix/NPS64

`main` conserva la base CPU 00.51: Ari64 + Angrylion, seis trabajadores y
audio continuo de 120 ms. Su renderizador GPU experimental no se activa
en el build normal. Los resultados y limites de la version estan en
[CPU 00.51](CPU-00.51.md).

`experimental/parallel-rdp-ps4` parte de esta base para aislar y validar
ParaLLEl-RDP en PS4. Los cambios experimentales se revisan antes de entrar
en `main`; no se sustituye el backend CPU estable durante las pruebas.

## Archivos incluidos

Se versionan fuentes propias, scripts, tests, documentacion, licencias,
Dockerfiles y `sources.lock.json`. Los perfiles PGO pequenos de `assets/`,
la fuente bitmap y los shaders embebidos se incluyen porque son entradas
necesarias para reproducir los builds existentes.

`.gitignore` excluye ROMs, guardados, SDKs, dependencias descargadas,
resultados de compilacion, PKGs, archivos comprimidos, logs y datos locales.
Los archivos excluidos siguen disponibles en el workspace; no se borran.
Los paquetes distribuidos pueden publicarse separadamente como archivos
de una release, sin incorporarlos al historial Git.

## Preparar una copia nueva

Con Docker instalado, desde la raiz del clon:

```sh
docker build -t nps64-builder:local .
docker run --rm -v "${PWD}:/work" nps64-builder:local bash scripts/bootstrap.sh
docker run --rm -v "${PWD}:/work" nps64-builder:local bash scripts/build.sh
```

En PowerShell puede usarse `-v "${PWD}:/work"` del mismo modo. El bootstrap
descarga el SDK publico OpenOrbis y obtiene las revisiones fijadas en
`sources.lock.json`. Las ROMs para pruebas deben proporcionarse localmente.
Para GPU, consultar [GPU](GPU.md), `Dockerfile.gpu` y
`scripts/bootstrap_gpu.sh`.

Docker permite compilar y ejecutar pruebas locales, pero no emula el
hardware ni el controlador grafico de PS4. Las mediciones y la estabilidad
finales requieren pruebas en una consola real.
