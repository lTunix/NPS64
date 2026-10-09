# ParaLLEl 00.57: aislar el primer bloqueo GPU

## Evidencia de PS4 00.56

La primera prueba ejecutada corresponde al **caso 0 / 32 hilos**. Las trazas
confirman que SPIR-V, NIR, ACO y la carga del shader completan. El primer
despacho se bloquea: `waiting for 1, completed 0, submitted 1`. La espera
devuelve `-13` y la aplicación detiene los envíos sin cerrarse.
Esto no confirma que la compilación del caso de 64 hilos esté corregida:
ese caso no aparece ejecutado en estos logs.

Los hashes de los buffers capturados de triángulo y scissor coinciden con
la captura preparada. Las máscaras de tiles y coarse permanecen en cero.
Los cinco descriptores apuntan a las asignaciones previstas y el reporte
no detecta solapamientos físicos. Esto reduce las causas posibles, pero
no identifica por sí solo una instrucción GPU defectuosa.

## Casos nuevos

Se conservan los dos casos originales. Se añaden:

3. **Direct tiles / no shared LDS / atomics**: cada invocación evalúa las
   primitivas de su grupo directamente con las mismas funciones originales
   `load_triangle_setup`, `load_scissor_state` y `bin_primitive`. Elimina
   el filtro de metatiles, la máscara compartida y sus barreras; conserva
   los atómicos de coarse.
4. **Direct tiles / stores / one-group fixture only**: igual al caso 3,
   sustituye los atómicos por escrituras de coarse. Sólo es válido para
   esta captura con un grupo de primitivas y un escritor por tile. No
   sirve como reemplazo general del renderizador.

La aplicación selecciona inicialmente el caso 3. Si pasa, probar el 4.
Si aparece un bloqueo, reiniciar la consola antes de probar el 4: esta
aplicación no implementa un reset de la GPU. Conservar `boot.log`,
`tile-probe.log` y los archivos `gpu-hang*` cuando se produzca un fallo.

## Validación local

- 256 despachos con fence en Lavapipe: los cuatro casos verifican las
  mismas máscaras, entradas y guardas de las cinco asignaciones, sin
  errores de validación Vulkan.
- RADV/ACO Mesa 26.0.8 para BONAIRE/GFX7 compila los cuatro casos.
- En la salida ISA nativa, el caso 3 no contiene instrucciones `ds_*`
  ni barreras; el caso 4 tampoco contiene atómicos de buffer. Los
  originales sí contienen operaciones LDS.
- El PKG PS4 00.57 pasa las 28 comprobaciones de estructura/payload.

Docker no ejecuta la GPU PS4. Estos resultados validan las variantes y
el paquete; la estabilidad del despacho sigue pendiente del hardware.
No se cambió el núcleo CPU ni se habilitaron estas variantes para ROMs.

Reproducir con las dependencias y la captura preparada existentes:

```sh
python3 scripts/prepare_tile_direct_probe.py
python3 scripts/test_prepared_parallel_tile.py
bash scripts/test_radv_tile_compiler.sh
NPS64_TILE_USE_PREPARED=1 bash scripts/build_parallel_tile_probe.sh
```
