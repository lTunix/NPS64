# 00.54: aislar el bloqueo de tiles del RDP original

00.53 carga Mario pero no produce el primer cuadro visible en PS4. El log
entregado confirma `waiting for 331, completed 330, submitted 331`.
Las VIs iniciales devolvieron 1x1; `visible=0` antes del bloqueo.
El numero 331 corresponde a esta ejecucion con CXD4; un numero mayor que
37 no implica que el primer rasterizado del juego haya completado.

En `gpu-hang.txt`, el ultimo marcador es 5, inmediatamente despues del
despacho de tile binning. La barrera `CS_PARTIAL_FLUSH` posterior no avanza
al marcador 6. WRITE_DATA no demuestra finalizacion del shader: la etapa
queda acotada a binning o su finalizacion/sincronizacion. El shader capturado
usa LDS, swizzles y atomicos. No se ha probado una causa definitiva.

## Prueba independiente en PS4

Instalar `out/NPS64-00.54-parallel-tile-probe.pkg` (TITLE_ID `NPSP00054`).
No es una nueva version que ejecute juegos ni una correccion confirmada.
No cambia CPU51 o el paquete 00.53.

- Al abrir aparece seleccionado el caso 2, **64 lanes**. Pulsar X.
- Debe completar 64 fences y verificar las mascaras, entradas y guardas.
- Si pasa, guardar el log. El caso 1 permite contrastar el shader original
  de **32 lanes**; conviene recoger el resultado del caso 2 antes de probarlo.
- Recoger `/data/NPS64-Parallel/tile-probe.log` y `/data/NPS64/boot.log`.
  Si hay bloqueo, recoger tambien `gpu-hang.txt` y los dumps actualizados.
- El dispositivo se retiene hasta cerrar la app; no se intenta reiniciarlo
  despues de un fallo fisico de GPU.

## Que se reproduce

Se compila el shader upstream `tile_binning_combined.comp` con UBERSHADER=1, SMALL_TYPES=0 y SUBGROUP=0. Usa los
buffers triangle y scissor capturados del envio 331, resolucion 320x239 y
una primitiva. La segunda variante cambia solamente el grupo local 32->64
y ajusta la dimension Z de grupos 8->4 para cubrir los mismos tiles.
Se mantienen LDS y atomicos del algoritmo original, texturas no se sustituyen
y no se usa el shader directo sin LDS de los experimentos anteriores.

Cada binding tiene una asignacion independiente; se comparan los 5 MiB
completos despues del fence, incluidas las regiones de guarda y entradas
que el shader no debe modificar. La referencia se genera en Lavapipe y
se comprueba contra la expectativa rectangular independiente: 1200 tiles
activos, bit de primitiva cero, resto de mascaras a cero.

Ambas variantes completan 128 envios en total con resultados identicos,
SPIR-V valido y sin errores de validacion Vulkan/sincronizacion en Docker.
Eso no prueba que alguna complete en Liverpool. Las entradas son snapshots
posteriores al bloqueo: la prueba omite el despacho previo de span setup,
no conserva las direcciones GPU originales ni ejecuta el ISA capturado.

Si 32 falla y 64 pasa, habra evidencia para integrar y comparar la variante
64 en el juego. Si ambas pasan, hay que investigar la dependencia de span
setup, las barreras y el contexto completo. Si ambas fallan, la investigacion
se concentra en el shader, sus accesos y el controlador.

## Reproduccion local

```bash
python3 scripts/prepare_parallel_tile_probe.py /work/logs
bash scripts/build_parallel_tile_probe.sh /work/logs
```

Dentro de `nps64-gpu-builder:local`, con el proyecto montado en `/work`.
Los datos capturados y las cabeceras generadas permanecen fuera de Git.
El manifiesto SHA256 y los resultados estan en `build/parallel-tile-probe`.
La reconstruccion necesita los dumps externos; el repositorio no contiene
bytes de ROM ni los buffers extraidos de la consola.
