# Huecos negros y rendimiento local

Cambios posteriores a 00.37, incluidos en 00.38; PKG anterior preservado.
Sin validacion PS4. La medicion de esta pagina es anterior a VI bajo demanda;
la medicion de tres modos actual esta en HLE-00.38-DEFERRED-VI.md.

El diagnostico ahora guarda geometria/materiales/atlas por framebuffer y reproduce
el seleccionado por VI_ORIGIN. Antes adjuntaba la geometria del siguiente cuadro.
`tests/hle_scene_replay.c` permite reproducirlo sin ejecutar la CPU N64.
Los dumps incluyen `scene_frame`, `display_target` y scanout RGBA original.

Quitar profundidad no eliminaba los huecos negros. Rasterizar coordenadas de
pantalla sin W de clip si los eliminaba, pero UV afines deformaban las texturas.
Los shaders dividen la posicion por W y rasterizan con W=1. UV/W y 1/W se
interpolan sin perspectiva y el fragment shader recupera UV por su cociente.
Profundidad y colores usan interpolacion de pantalla. La nueva prueba compara
cada pixel contra una ecuacion CPU con W variable 1234/2468: cobertura completa
sin perder perspectiva de textura.

`build/hle-screen-space/frame-2839-comparison.png`: CPU / Vulkan / diferencia.
Los huecos desaparecen. Error RGB medio 14.8483 -> 13.2098 (0..255).
Pixeles con diferencia maxima >16: 42.9375% -> 38.6003%. Todavia hay diferencias
de filtrado VI, cobertura, bordes y materiales; no es equivalencia pixel a pixel.

## Medicion limpia

`scripts/profile_hle_frames.sh` usa el frontend de `scripts/test_hle_game.sh`.
Misma ROM PAL e inputs; rango de cuadros 650..3900, 3251 muestras por modo.
Sin capturas, validation layers ni limitador; atlas completo al cambiar como PS4.
Se excluyen arranque, activacion Vulkan y escritura del informe en cada muestra.
Los 133 registros de estado del core coinciden exactamente entre los dos modos.
Resultados: `build/hle-performance/performance.json`.

| Modo | Media ms | Mediana ms | P95 ms | P99 ms | Cuadros >20 ms |
|---|---:|---:|---:|---:|---:|
| CPU | 4.084 | 2.837 | 8.241 | 10.345 | 0.031% |
| Vulkan + referencia CPU | 6.552 | 3.691 | 13.059 | 16.750 | 0.554% |

50 Hz requiere 20 ms por cuadro. Lavapipe ejecuta Vulkan por software en CPU:
estos valores no representan Liverpool ni garantizan FPS de PS4. Vulkan conserva
la rasterizacion completa Angrylion/VI para referencia, fallback y coherencia
RDRAM; suma 2.469 ms medios aqui en vez de descargar la CPU.
El recorrido Vulkan registra 1642 cuadros GPU nuevos, 4 fallback CPU y VI repetidas.

Para retirar trabajo redundante hay que omitir spans CPU solo cuando el cuadro GPU
pueda completarse, conservar estado/TMEM y resolver lecturas posteriores de
framebuffer/Z en RDRAM. Tambien hace falta replay CPU para fallback. Desactivar
Angrylion sin estas dependencias rompe escenas y texturas.

## Validacion

PASS: ROM real continua (400+ cuadros GPU nuevos, audio, pausa, estados, dos
sesiones y cambios CPU/Vulkan); texturas con oracle por pixel y W variable;
combinadores, alpha/cobertura, 128 glifos y profundidad. Sin errores Vulkan de
validacion. La reproduccion del scanout comprueba dumps contra RGBA capturado.
