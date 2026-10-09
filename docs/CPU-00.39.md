# CPU 00.39: Ari64 + Angrylion

Vulkan queda fuera del build normal por las regresiones reportadas en PS4.
Se conserva la rasterizacion Angrylion, sus texturas, cobertura, particulas,
blender y filtro VI. No se aplica la VI diferida de la variante Vulkan.
Las preferencias GPU antiguas no activan ningun renderer alternativo.

## Cambio de rendimiento

`src/software_scaler.h` convierte y escala filas de 32 bits mediante SSE2
para relaciones exactas 3:2, 2x, 3x y 4x (incluidos 320->960 y 640->960).
Reutiliza la copia de filas repetidas existente. Otros tamanos y formatos
siguen por la rutina escalar. El cambio no modifica la imagen ni el core.

## Validacion

- Prueba escalar contra SSE2, RGB/RGBA, direcciones sin alinear, canarios
  y ASan/UBSan: PASS.
- Mario 64 PAL, 4000 iteraciones con la misma entrada, Ari64 y 4 workers:
  3952 hashes de imagen completa y 133 muestras CPU/VI identicos.
- OpenOrbis: ELF valido; PKG de 6619136 bytes, 28 controles aprobados y
  contenido extraido identico al stage. No se enlaza Mesa/RADV/Vulkan.
- La version estable 00.13 conserva su SHA256 original.

Reproducir la prueba de filas:

```sh
clang-14 -std=c11 -O1 -g -Wall -Wextra -Werror \
  -fsanitize=address,undefined -Isrc tests/software_scaler_test.c \
  -o build/software-scaler-test
build/software-scaler-test
```

Para comparar el frontend nativo, ejecutar `scripts/bench.sh` con
`BENCH_JIT=1 BENCH_CORE=build/native-jit BENCH_WORKERS=4 BENCH_CPU=jit
BENCH_SM64=1 BENCH_FRAMES=4000`. La referencia usa `NPS64_CPU_SCALAR=1`;
la version optimizada omite esa variable. `NPS64_CPU_FRAME_HASHES=1`
registra las imagenes antes de dibujar la UI. `BENCH_CPU_PROFILE=1`
registra tiempos; medir por separado sin hashes.

## Prueba en PS4

Instalar `out/NPS64-00.39-cpu.pkg`. El log debe comenzar con
`NPS64 00.39 CPU ONLY` y `[renderer] CPU Angrylion; Vulkan not included`.
Comparar la misma escena con 00.13: `draw_ms`, `core_ms`, `rdp_ms`,
`vi_ms`, `vi_fps` y `speed_pct`. Las mediciones Linux no certifican
50 FPS en PS4. La siguiente optimizacion dependera del coste dominante
observado en consola, manteniendo fidelidad grafica.

## Medicion local sin hashes

Linux/Docker, misma entrada y 4 workers; 3251 muestras por ejecucion,
frames 650..3900. Segunda pareja en orden inverso. Draw es la media de
ventanas de juego de frames 1000..3900. No hay limitador ni vsync real.

| Pareja | Rutina | Muestras | Total medio ms | P95 ms | Draw medio ms |
|---|---|---:|---:|---:|---:|
| perf | scalar | 3251 | 6.965 | 15.273 | 0.744 |
| perf | simd | 3251 | 5.854 | 12.538 | 0.451 |
| repeat | scalar | 3251 | 5.896 | 13.153 | 0.628 |
| repeat | simd | 3251 | 6.511 | 15.741 | 0.577 |

El escalado reduce draw en ambas parejas. El tiempo total varia y la
segunda pareja no confirma una mejora total; no se atribuye al cambio
una ganancia estable de FPS. Los spans RDP y la VI conservan su coste.
Logs y muestras locales: `build/cpu39-*-perf.*` y `build/cpu39-*-repeat.*`.
