# ParaLLEl-RDP 00.53: ROMs y comparacion de imagen

Resultado PS4 recibido: vuelve a bloquearse antes del primer cuadro visible,
en el envio 331, marcador 5. La comparacion local no demuestra estabilidad
de Liverpool. Continuar con la [prueba aislada 00.54](PARALLEL-00.54-TILES.md).

App experimental independiente, TITLE_ID `NPSP00053`. El paquete es
`out/NPS64-00.53-parallel-rdp-test.pkg`; CPU51 permanece instalada.

## Probar en PS4

- Abrir la biblioteca y elegir primero Mario 64 con X.
- Reutiliza las ROMs de `/data/NPS64/games`; no contiene juegos.
- Guarda configuracion y partidas separadas en `/data/NPS64-Parallel`.
- El HUD debe mostrar **GPU Vulkan**, y el log `renderer=paraLLEl-RDP active`.
  Si muestra CPU Angrylion, la inicializacion GPU no tuvo exito.
- L3 + R3 abre pausa, estados y regreso a biblioteca.
- Probar cabeza, seleccion de archivo, carta de Peach y exterior del castillo.
- Recoger `/data/NPS64-Parallel/nps64.log`. Ante bloqueo GPU, recoger tambien
  `/data/NPS64/boot.log`, `gpu-hang.txt` y los archivos de captura del controlador.
- Reiniciar la app para probar otra ROM con Vulkan. Se retiene el dispositivo
  hasta salir para evitar el cierre que fallo en las primeras pruebas.

El core usa Ari64, RSP CXD4 y ParaLLEl-RDP a resolucion nativa. Angrylion
solo entra si la inicializacion GPU falla. No ejecuta el rasterizador CPU
como referencia durante el juego Vulkan. La lectura de scanout GPU->CPU
sigue presente y sera una optimizacion posterior.

Los shaders originales de ParaLLEl estan activos; los reemplazos anteriores
de binning directo y rasterizado sin texturas quedan desactivados. Se
mantiene el perfil conservador: ubershader, sin subgrupos/tipos pequenos,
compilacion sincrona, barrera de setup y envios serializados. Eso permite
validar el RDP completo antes de quitar sincronizaciones y medir velocidad.
Las siete pruebas 00.52 pasaron en PS4, pero no demuestran por si solas que
el bloqueo del RDP original este corregido. Este paquete sigue siendo de prueba.

## Comparaciones locales reproducibles

```bash
NATIVE_GPU_DIR=build/parallel-game/native bash scripts/build_native_gpu.sh
bash scripts/compare_parallel_stream.sh 'roms/Super Mario 64 (Europe) (En,Fr,De).n64' mario-stream
```

Se ejecutan dentro del contenedor `nps64-gpu-builder:local` con `/work`
montado. Las capturas y el dump permanecen en `build/parallel-comparison/`,
excluido de Git. No se incorporan ROMs, estados ni imagenes al repositorio.

`index.html` muestra CPU, GPU y diferencia amplificada x8; `report.json`
registra dimensiones, coincidencia de VI y errores de color. La herramienta
usa solo la biblioteca estandar de Python.

Las ejecuciones independientes del juego avanzan con diferente fase; por
eso no se usan para afirmar equivalencia del rasterizador. La comparacion
principal captura el stream RDP original de una sola ejecucion Vulkan y
lo reproduce en Angrylion con la misma VI. La referencia retiene sus propios
buffers de color, profundidad y cobertura: excluye sus regiones de las
actualizaciones posteriores de memoria GPU y captura antes de importarlas.
Una prueba sintetica inyecta framebuffer verde y cobertura incorrecta en
ese punto; la referencia CPU debe seguir mostrando su relleno rojo.

Los filtros dedither y divot de la VI se igualan a los de ParaLLEl. Las
comparaciones de Mario VI 150, 480 y 600 muestran diferencia maxima de
1 nivel por canal (0..255), sin errores mayores de 8. Cuatro capturas adicionales
del menu de sonido/idioma, tras restaurar un estado CPU en el runner GPU,
obtienen el mismo limite. No son bit-exactas. Los informes locales estan en
`build/parallel-comparison/mario-stream/index.html` y
`build/parallel-comparison/mario-menu-state/index.html`.
Una VI invalida no se transforma artificialmente en una imagen comparable.
La cobertura de pruebas actual no representa todos los formatos, efectos
ni juegos. Docker/Lavapipe no reproduce Liverpool ni mide FPS de PS4.

## Reconstruir PS4

```powershell
./scripts/build_parallel_game.ps1
```

El build tiene core, ELF y stage independientes en `build/parallel-game`.
Se verifica ELF, inicializacion diferida, identidad y 28 controles de PKG,
incluida igualdad del payload. Tambien se comprueba el hash de CPU51.
