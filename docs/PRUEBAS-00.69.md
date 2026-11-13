# 00.69 — candidata local, pendiente de rendimiento en PS4

No publicada. Cualquier push, etiqueta o release requiere aprobación explícita
del usuario después de revisar las pruebas. La numeración en `version.h` reserva
la candidata; no declara terminada la optimización.

Esta candidata actualiza el icono desde `icon.jpeg` y añade distribuciones de
tiempo de trabajo y del núcleo. Conserva Ari64/Angrylion y la cuarentena GPU.
No se ha corregido ni habilitado Vulkan para juegos.

## Diagnóstico de GPU

El [README de DolphinPS4](https://github.com/iHaiDeeZ/DolphinPS4) describe
renderizado Vulkan con Mesa/RADV adaptado a GNM. NPS64 ya tiene esa integración
experimental: `gpu_vulkan.c`, `build_gpu.sh` y los parches Mesa. Según
[la investigación existente](GPU-INVESTIGATION.md), hubo un bloqueo de consola
durante los primeros trabajos compute de paraLLEl-RDP. Los marcadores no aíslan
todavía un shader culpable. No basta con retirar el bloqueo de arranque.

También se revisaron `DOLPHIN-GPU.md`, `CATALOGO-GPU.md`,
`ps4/scripts/build.py` y el backend en el proyecto vecino xuperPS4. Su ruta
Vulkan acelera composición de interfaz, texto y vídeo; no implementa el RDP N64.
La documentación de xuperPS4 mantiene pendiente su validación GPU física.

La ruta HLE alternativa de NPS64 evita compute, pero conserva spans CPU para
color/Z/cobertura en RDRAM y después dibuja geometría Vulkan. Suprimir esos
spans requiere resolver lecturas de framebuffer, coherencia y fallback. Las
[mediciones previas](HLE-00.38-DEFERRED-VI.md) no demuestran una mejora total.
La interfaz GPU por sí sola no corrige ese coste durante una partida.

Orden pendiente: aislar el compute mínimo en RADV/GNM y consola; validar memoria
y barreras; comparar paraLLEl-RDP con CPU en la misma escena; comprobar estados,
audio, cambios de juego y limpieza; decidir el backend con esos resultados.

## Comparación en consola

1. En 00.68, usar un juego propio, anotar región, modelo PS4, ajustes de imagen,
   trabajadores y modo CPU. Ejecutar una escena reproducible durante al menos
   dos minutos después del calentamiento y conservar `/data/NPS64/nps64.log`.
2. Instalar el PKG local 00.69 y repetir el recorrido y ajustes. No cambiar
   simultáneamente el filtro VI o el número de trabajadores. Repetir tres veces.
3. Guardar los logs de cada ejecución por separado antes de que roten. Comparar
   `[perf]`: `speed_pct`, `vi_fps`, `rdp_ms`, `vi_ms`, `core_ms`, `draw_ms`,
   `flip_ms`; `[scanout]` separa copia de espera. RDP y VI están incluidos en
   `core_ms`: no sumarlos otra vez.
4. En 00.69, `[frame-times]` informa muestras de juego y tiempos de trabajo sin
   el sleep del limitador. Incluye polling, núcleo, dibujo y presentación,
   incluidas sus esperas; no representa tiempo puro de GPU. p50/p95/p99 son
   cotas superiores de buckets de 0,25 ms por ventana de aproximadamente un
   segundo. Más de 256 ms se agrupa y usa el máximo observado como cota.
   No promediar percentiles para obtener el percentil global.
5. Revisar imagen, audio, pausa, guardar/cargar y dos juegos consecutivos.
   Conservar clips con contador visible. Una mejora de la media que empeore
   tirones, audio o imagen no basta para aceptar el cambio.

Los FPS de VI no son necesariamente cuadros únicos de animación. Comparar
`speed_pct` contra el objetivo de la ROM (PAL/NTSC). Docker, Lavapipe y shadPS4
no certifican rendimiento en Jaguar/Liverpool.

## Evidencia de esta sesión

Las comprobaciones locales y la construcción se registran al terminar en
`build/validation-00.69.txt`. La prueba física queda pendiente del usuario.
No hay aprobación de publicación.

## Primera medición física: Zelda, 29 de octubre de 2026

El usuario instaló la candidata y ejecutó Ocarina of Time (E) (GC).
Se descargaron dos snapshots por FTP, sin modificar la configuración ni detener
el juego. Se aisló la última sesión del log, que contiene también sesiones
antiguas. Versión confirmada: 00.69, Ari64, HLE, Angrylion, seis trabajadores,
VI filtrada, salida nativa 640×240 y objetivo de aproximadamente 50 VI/s.

El segundo snapshot tiene 60 ventanas de juego. Media aritmética de las últimas
diez ventanas (aproximadamente un segundo cada una, no percentiles globales):

| Medida | Resultado |
|---|---:|
| VI/s | 46,55 |
| Velocidad | 93,07 % |
| Núcleo | 19,16 ms |
| RDP, incluido en núcleo | 16,22 ms |
| VI, incluida en núcleo | 0,45 ms |
| Dibujo frontend | 1,22 ms |
| Presentación | 1,08 ms |
| Espera del limitador | 0 ms |

Al final, el trabajo por cuadro ronda 21,5–21,8 ms frente al presupuesto de
20 ms de PAL. RDP consume aproximadamente el 75 % de ese tiempo: es el cuello
de botella dominante en este tramo. El contador acumulado de underruns de audio
sube de 2 en el primer snapshot a 18 en el segundo. Esto indica agotamientos de
la cola; su manifestación audible queda pendiente de confirmar con el usuario.
No aparecen errores explícitos en la última sesión capturada.

La escena inicial alcanzaba aproximadamente 50 VI/s; no se debe extrapolar ese
resultado al recorrido completo. No hay una comparación equivalente con 00.68
ni prueba de mejora atribuible a 00.69. Esta observación confirma que el juego
no mantiene velocidad completa en el tramo final registrado.

Evidencia local: `build/ps4-logs/20261029-113719-zelda/` y
`build/ps4-logs/20261029-113750-zelda/`, con log de sesión y `summary.json`.
No se publicó nada.

## Majora's Mask, misma sesión física

Se aisló la carga de Majora's Mask Collector's Edition (E) (M4) (GC) posterior
a Ocarina, sin reiniciar la aplicación. Ari64/Angrylion, perfil de lanes estándar,
salida 640×288 y objetivo aproximadamente 50 VI/s.

Primera captura: 28 ventanas; las últimas diez promedian 50,01 VI/s y 100 % de
velocidad, núcleo 12,41 ms y RDP 9,84 ms. Segunda captura: 51 ventanas; las últimas
diez promedian 43,35 VI/s y 86,67 %, núcleo 17,23 ms y RDP 13,44 ms. La carga cambia
durante el recorrido: no tratar la primera muestra como velocidad estable del
juego completo. En ventanas finales, p95 de trabajo llega a 52,75 ms y p95 del
núcleo a 50,50 ms; son cotas por ventana, no percentiles globales.

Hay un problema adicional de audio: `dropped` pasa de 0 a 1.873.066 durante esta
carga y la cola se mantiene frecuentemente entre 144 y 171 ms. Los underruns
acumulados de la aplicación pasan de 52 al cargar a 53: no atribuir los 52
anteriores a Majora. Los descartes ya crecen mientras el juego alcanza 100 %;
no se puede explicar todo el problema de audio por falta de velocidad gráfica.
Su causa y manifestación audible requieren investigación adicional.

Capturas locales: `build/ps4-logs/20261029-113927-majoras-mask/` y
`build/ps4-logs/20261029-113951-majoras-mask/`. No se modificó la consola.

El usuario confirmó después que el audio se escucha mal: el síntoma audible
queda confirmado, además de los contadores.

## GoldenEye 007 USA, misma sesión física

Captura de 35 ventanas de juego, posterior a Majora. Ari64/Angrylion con seis
trabajadores, VI filtrada, salida 640×240. Aunque el registro inicial de AV
todavía muestra la tasa del juego anterior, las ventanas de GoldenEye ya
registran el objetivo NTSC correcto de 60,02 VI/s; se usan esas ventanas.

Las últimas diez ventanas promedian 32,61 VI/s, 54,33 % de velocidad, núcleo
28,33 ms, RDP 25,72 ms, VI 0,72 ms, dibujo 1,22 ms y presentación 1,08 ms.
El limitador no duerme. El trabajo por cuadro ronda 30,7 ms, frente a un
presupuesto de 16,66 ms: el RDP CPU representa aproximadamente el 84 % del
trabajo. Ventanas anteriores de baja carga alcanzaban unos 60 VI/s; eso no
representa el tramo posterior.

Aquí el audio muestra agotamientos, a diferencia de los descartes crecientes
registrados en Majora: underruns acumulados pasan de 53 a 84, mientras `dropped`
permanece en 3.928.429. No atribuir ese total de descartes al juego nuevo.
La falta de velocidad y los agotamientos son compatibles con interrupciones
de audio; no se ha probado todavía una corrección.

Evidencia: `build/ps4-logs/20261029-114126-goldeneye/`, con log de la carga
aislada y resumen. No aparecen errores explícitos en ese segmento. No hay
comparación equivalente con otra versión ni autorización de publicación.
