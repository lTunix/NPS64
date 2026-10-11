# ParaLLEl 00.60: lecturas vectoriales y scissor observado por el shader

00.59 reprodujo el mismo error que 00.58 incluso subiendo las entradas
mediante comandos GPU. Los cinco buffers capturados tienen sus guardas
intactas. El scissor en memoria es el esperado: `[0, 0, 640, 480]` en
unidades de cuarto de píxel. Las máscaras contienen 640 tiles inesperados:
las filas 16..31 están activas en todas las columnas, aunque el cuadrante
superior termina antes de ellas. La fila 15 está vacía y la región
superior tiene el límite X inclusivo correcto. Coarse coincide con la
misma cobertura errónea. No hay un bloqueo GPU.

La prueba nueva conserva las funciones originales de binning, el patrón
de stress y la referencia. Declara las entradas como `volatile readonly`.
En la salida nativa de RADV/ACO BONAIRE/GFX7, la variante anterior usa
`s_buffer_load_*`; la nueva utiliza `buffer_load_*` y no contiene cargas
escalares de buffer ni instrucciones LDS. Es una hipótesis controlada
sobre la ruta de lectura, no una causa ya demostrada en PS4.

Además, el shader guarda el scissor observado al inicio de cada grupo
en el buffer de diagnóstico. La CPU verifica esos valores antes de
comparar máscaras. Se mantienen las capturas completas al fallar.

Validación local: 448 despachos con fences y validación Vulkan, todos los
conteos/patrones de stress, guardas y scissor leído por GPU correctos.
RADV/ACO compiló siete casos para GFX7. El PKG pasó 28 comprobaciones.
Docker no ejecuta el hardware PS4; queda pendiente el caso 7 en consola.

Ejecutar **caso 7**, seleccionado al inicio. Si falla, conservar los logs
y `tile-fail-case6-buffer*.bin` de `/data/NPS64-Parallel/`. La línea
`GPU scissor group0=... expected=...` permite distinguir una lectura
incorrecta del dato de un fallo posterior en el cálculo.

Análisis reproducible de la captura anterior:

```sh
python scripts/analyze_tile_failure.py logs build/parallel-tile-probe/ps4-59-analysis
```

Genera un informe JSON y un SVG con máscara esperada, observada y diferencias.
No cambia la referencia para aceptar el resultado incorrecto ni modifica
la versión CPU o el renderizador usado por las ROMs.
