# 00.38: coordenadas de pantalla y VI de CPU bajo demanda

Incluye la correccion local de huecos negros documentada en
`HLE-SCREEN-SPACE-PERFORMANCE.md`. Shaders de graficos, sin compute.

## Trabajo eliminado

La VI de Angrylion sigue preparando registros, geometria de salida, bordes y
estado temporal en cada campo. Para Vulkan se puede diferir solo el filtro de
pixeles de una VI progresiva normal, sin overscan, sin buffer prestado por el
frontend y con escala interna 1. El contador de campo/gamma se conserva.

Si el cuadro Vulkan se puede presentar, se descarta el filtro pendiente. Si la
geometria esta incompleta, falta el buffer de VI o falla Vulkan, se ejecuta ese
filtro inmediatamente sobre RDRAM y se entrega el cuadro CPU actualizado.
No se reutiliza una imagen CPU obsoleta como fallback. Cerrar/reinicializar VI
cancela cualquier trabajo pendiente. Interlace, otros modos y pruebas de snapshot
conservan la ruta VI completa. Capturas diferenciales fuerzan VI completa.

Los spans RDP CPU siguen escribiendo color, Z y cobertura/RDRAM para coherencia y
fallback. Esta optimizacion retira filtro/copia VI redundantes; no descarga aun
la rasterizacion completa ni garantiza 50 FPS en PS4.

## Prueba con la ROM real

`scripts/test_hle_deferred_vi.sh ROM` compara un recorrido completo optimizado
contra otro con `NPS64_HLE_CPU_VI=1`. Son 1738 hashes identicos, incluyendo GPU,
CPU fallback, transiciones, guardar/cargar y recarga de ROM. Un fallback forzado
en core frame 2839 comprueba expresamente la recuperacion CPU de la escena.
Audio, pausa, cambios de modo y 400+ cuadros GPU nuevos pasan sin errores Vulkan.
Logs: `build/hle-deferred-vi/deferred-core.log` y `reference-core.log`.

La prueba de texturas verifica cada pixel con W variable y perspectiva UV.
La reproduccion de 10 dumps VI seleccionados produce exactamente su scanout GPU.

## Medicion

`scripts/profile_hle_frames.sh` compara CPU, Vulkan con VI CPU completa y Vulkan
con VI bajo demanda. No usa capturas/hashes de diagnostico ni validation layers.
Los tres recorridos deben coincidir en estados del core. Informe:
`build/hle-performance-38/performance.json`. Lavapipe es Vulkan por software en
CPU; los FPS host no representan Liverpool y deben confirmarse en PS4.

Pendientes: filtros VI GPU, cobertura/blender exacto, materiales restantes,
rasterizacion CPU redundante, dependencias framebuffer/RDRAM y presentacion GPU.

### Resultado medido

| Modo | Media ms | P95 ms | Cuadros >20 ms |
|---|---:|---:|---:|
| cpu | 4.797 | 10.272 | 0.031% |
| vulkan-reference | 7.659 | 16.285 | 1.969% |
| vulkan | 7.725 | 17.431 | 1.969% |

3251 cuadros por modo y 133 estados del core identicos. Esta pasada no demuestra
mejora de FPS totales: Vulkan VI completa 7.659 ms medios y VI bajo demanda
7.725 ms. En los tramos finales del log, VI pasa de 0.299..0.387 ms a
0.007..0.011 ms. Se elimina trabajo comprobable de filtro VI, pero el tiempo
RDP CPU y el render Vulkan por software dominan el total. No se promete una
ganancia de FPS en hardware a partir de este resultado.
