# HLE Vulkan con fallback CPU

Rama `experimental/hle-vulkan-fallback`, basada en `main` (`143dfd7`).
El historial importado contiene una sola revisión: se recupera el código HLE
que conserva esa revisión, no una versión histórica identificada por commit.
El paquete de prueba usa 00.65 para distinguirlo de los experimentos anteriores.

## Validación local

- Mario PAL: 2600 cuadros de ejecución local, alternancia CPU/Vulkan, pausa,
  estados, audio y reutilización del contexto entre dos sesiones.
- Recuperación del VI: 1738 hashes coinciden con la variante que ejecuta todo
  el VI en CPU, incluyendo fallback forzado y retorno a Vulkan.
- Diez capturas comparativas de escenas reales en
  `build/hle-restored-comparison`. Persisten diferencias gráficas: los tests de
  continuidad y fallback no certifican equivalencia visual con Angrylion.
- ELF y paquete: alineación de 16 KiB y 28 verificaciones de PKG; contenido
  extraído idéntico al stage.

Estas pruebas usan Vulkan de escritorio en Docker. No estiman FPS de PS4.
La referencia CPU todavía rasteriza: esta recuperación no promete aceleración.

## Reproducción

`scripts/run_hle_branch.ps1` acepta `Compare`, `Fallback` o `Package`.
`-Dependencies` indica el checkout que contiene SDK, vendor y archivos ya
compilados. Esas dependencias se montan de solo lectura; todos los resultados
se escriben en este checkout. `-Rom` permite elegir un archivo de `roms`.

## Prueba física

Abrir Mario por CPU, pausar con L3 + R3 y pulsar Triángulo para activar Vulkan
continuo. Repetir pausa y Triángulo para volver a CPU. Revisar
`/data/NPS64/nps64.log` y comparar escenas iguales, especialmente textos,
transparencias, castillo y cabeza de Mario.

## shadPS4

Se ejecutó el eboot extraído del PKG de consola: Mario funciona por CPU,
pero Vulkan falla al cargar `/sys/common/lib/libSceGnmDriver.sprx` y recupera
CPU correctamente. El emulador necesita imports GNM directos y metadatos de
shader; `EmulatorPackage` genera un paquete separado con los adaptadores
existentes de `build/shadps4/probe`, montados de solo lectura. Esta variante
incluye también el adaptador de `wmemchr` de 32 bits.

La variante de emulador superó 1800 cuadros GPU y 30 fallbacks CPU. También
se verificó el cambio manual Vulkan → CPU → Vulkan; la ejecución se detuvo
manualmente al concluir la prueba, sin crash. El log está en
`build/shad-hle-tested.log`. La captura
`build/shad-runtime/user/screenshots/UNKNOWN_20261012_004555_395_hud_000000.png`
muestra cabeza y fondo texturizados; los ojos y algunos detalles faltan.
El test utiliza intérprete cached para evitar la limitación de memoria
ejecutable del emulador. No demuestra rendimiento ni compatibilidad física.
El paquete físico conserva la carga de firmware y el modo Ari64 original.

## Próximos bloques

- [x] Recuperar HLE en una rama aislada sin modificar main o ParaLLEl.
- [x] Validar continuidad y fallback con la ROM real.
- [x] Generar comparaciones y paquete verificado.
- [x] Probar el paquete extraído en shadPS4 y registrar el resultado.
- [ ] Confirmar gráficos y continuidad en consola física.
- [ ] Corregir diferencias visuales antes de quitar trabajo CPU de referencia.
- [ ] Medir por etapa y evitar rasterizado duplicado donde sea seguro.
