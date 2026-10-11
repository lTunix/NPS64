# ParaLLEl 00.61: descarte vertical mediante predicado

Los logs de 00.60 confirman que el shader leyó el scissor correcto
`[0, 0, 640, 480]` y completó el despacho. Aun así, el caso 7 falló en
la misma palabra de máscara que los casos anteriores. Cambiar a cargas
vectoriales no resolvió el error. Los datos capturados permiten comprobar
los resultados sin asumir que un fallo de comparación es un bloqueo GPU.

El caso 8 conserva entradas volátiles, subida GPU, lectura de scissor,
atómicos y el stress de 0..256 primitivas. Cambia únicamente el descarte
vertical de `bin_primitive`: guarda `end_y >= start_y` como predicado,
acota el rango de interpolación y combina el predicado con el resultado
final. Así evita el retorno anticipado cuando un tile queda fuera del
scissor. El shader original y los casos anteriores permanecen disponibles.

La hipótesis es un problema en esa ruta de cálculo/control de flujo en
PS4. No se declara demostrada la causa ni corregida la ejecución real.
La referencia sigue excluyendo los mismos tiles incorrectos; no se ha
relajado para aceptar la salida de la consola.

Validación local: 512 despachos con fences, validación Vulkan sin errores,
todos los conteos y ambos patrones de scissor, máscaras/entradas/guardas
completas y lectura de scissor correctas. RADV/ACO para BONAIRE/GFX7
compila los ocho casos. La estabilidad en PS4 queda pendiente del caso 8.

Ejecutar **caso 8**, seleccionado al abrir 00.61. Si falla, conservar logs
y `tile-fail-case7-buffer*.bin` de `/data/NPS64-Parallel/`.

Para analizar la captura de 00.60:

```sh
python scripts/analyze_tile_failure.py logs build/parallel-tile-probe/ps4-60-analysis
```
