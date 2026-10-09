# 00.15 Vulkan: sustitución del shader de tiles

El usuario confirma que 00.14 sigue bloqueándose. Su traza muestra
`submitted 37` y el destino de marcadores contiene 5. La cola single-flight
está activa. El marcador 3 está después de la barrera de spans: ese trabajo
superó la espera. El marcador 5 está después de despachar tiles, pero antes
de esperar su finalización; 6 no se alcanza. El rasterizador posterior no
se despachó. Esto localiza el bloqueo en tiles o su finalización, sin probar
si la causa es LDS, atómicos, barreras internas o sus accesos a memoria.

## Cambio de 00.15

`src/gpu_shaders/tile_binning_direct.comp` reemplaza la clasificación con
reducciones por una invocación independiente por tile. Cada hilo calcula
las máscaras de todos los grupos de primitivas y escribe su máscara gruesa.
Se mantienen los predicados geométricos y estructuras MIT de paraLLEl-RDP.
No contiene memoria compartida, atómicos, operaciones de subgrupo ni
barreras internas. Usa 64 hilos por grupo, despachando una dimensión X fija
de 1 y ajustando las filas de meta-tiles a grupos de 8x8 tiles.

La reflexión de recursos se conserva explícitamente: este núcleo no incluye
reflexión SPIR-V dinámica. El shader mantiene los bindings y constantes que
consume la ruta ubershader. El bytecode se genera con glslang 15.1.0; el
generador audita que no haya variables Workgroup ni instrucciones de barrera
o atómicos. El header generado se incluye en las fuentes correspondientes.

Se mantienen los fences y sincronización de 00.14. La ruta GPU sigue siendo
Vulkan sobre RADV/GNM; CPU N64 usa Ari64. El cambio puede hacer más trabajo
geométrico cuando hay muchas primitivas. Es una ruta de compatibilidad
experimental; la estabilidad en PS4 no está confirmada.

## Validación y prueba

La comparación directa en Vulkan contra el shader original cubre 1,31,32,
33,64,255 y 256 primitivas, varias máscaras por tile, scissor y resolución
321x241 con tiles parciales. Las máscaras finas y gruesas coinciden exactamente
en Lavapipe; no aparecen errores de validación. Esta prueba no usa Liverpool.

Instalar `out/NPS64-00.15-vulkan-test.pkg`. En `nps64.log` deben aparecer
`00.15 EXPERIMENTAL`, `NPS64 direct tile binning = 1; local=64, no LDS/atomics/barriers`
y `renderer=paraLLEl-RDP active`. Si la configuración solicita GPU=0, cambiar
el último valor de `diagnostics.cfg` a 1:

```text
NPS64DIAG5 4 1 1 1 1
```

Si falla, conservar `nps64.log`, `gpu-hang.txt` y `boot.log` del mismo intento.
En la nueva traza de tiles se espera local_size_x=64 y LDS_SIZE=0. El shader
puede cambiar de dirección y el número de submission no identifica la causa.
Todavía puede bloquear la consola; conservar 00.13 para restaurar la ruta CPU.

Reconstrucción con dependencias Docker/SDK preparadas:

```powershell
.\scripts\build_vulkan_test.ps1
```

Oráculo de máscaras en Docker:

```sh
python3 scripts/compile_gpu_shaders.py
python3 tests/test_gpu_tile_equivalence.py
```

## Resultado de entrega

PKG: 30 605 312 bytes, APP_VER/VERSION 00.15 y 28 checks correctos.
SHA-256: `1a02941bfb92751052b161e087e8da6f1264875c3f03ee623399c8600a386cd7`.
ELF/OELF alineados y sin segmentos solapados; constructores GPU diferidos.

Pasan en host la comparación de máscaras de ambos shaders, dos sesiones
Vulkan con imagen y estados, fallbacks y 360 frames de Mario Kart. Las
imágenes de juego de los frames 300 y 330 son idénticas píxel a píxel a la
ruta original (excluyendo HUD y barra de estado). El build habitual mantiene
la cuarentena GPU. La confirmación PS4 de 00.15 sigue pendiente.

Evidencia: `build/gpu-tile-equivalence/results.log`,
`build/gpu-tile-equivalence/image-results.log`,
`build/gpu-functional-test.log`, `build/gpu-bench-results.log`.
