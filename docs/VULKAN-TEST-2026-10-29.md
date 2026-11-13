# Pruebas Vulkan en PS4 — 29 de octubre de 2026

**Estado actual:** el paquete de juegos 00.01 bloqueó la GPU en PS4 y fue
retirado de `/data/pkg`. El usuario confirmó bloqueos con reinicio de consola.
No volver a ejecutarlo. La investigación posterior se realiza fuera de la
consola, exclusivamente sobre Vulkan; el trabajo CPU/audio queda en pausa.

## Cómputo básico confirmado en hardware

Aplicación separada `NPSG00064`, versión 00.01. El usuario ejecutó las tres
pruebas y los logs descargados por FTP confirman `AMD Liverpool (PS4)
(RADV BONAIRE)`, vendor 1002, device 9920:

| Prueba | Resultado físico |
|---|---|
| Storage buffer, 64 hilos | PASS, repetida tres veces |
| LDS, atomicOr, barreras y bitmask, 32 hilos | PASS |
| LDS, atomicOr, barreras y bitmask, 64 hilos | PASS |

Cada caso hace dos dispatches con dependencia, espera un fence y compara cada
palabra, incluida la zona de guarda. Los resultados son correctos y no aparecen
timeouts en la captura. Se crean y destruyen dispositivo y recursos en cada
caso; la captura se tomó con la aplicación todavía abierta, por lo que no
certifica el cierre del proceso. Los 181–389 ms registrados incluyen creación
del contexto, compilación y liberación: no son tiempos de ejecución del shader
ni un benchmark de juegos.

Se utiliza Mesa 26.0.8 preparada por xuperPS4 01.19 y DolphinPS4
`69360a158a9619d7c877d4229c2b37b0a34982dc`, más reciente que la integración
inicial de NPS64. El objeto GNM se recompila cambiando únicamente sus rutas de
log a NPS64. Los archivos externos y miembros de archivos thin se verifican
por SHA-256 en `build/compute-probe-ps4/manifest.json`; el árbol xuperPS4 no
se modifica. No se interpreta este resultado como una reparación demostrada
del bloqueo de la integración anterior.

El mismo código de prueba pasa en Lavapipe con la capa Khronos efectivamente
cargada, sin VUID ni errores. El PKG pasa 28 controles, verificación ELF/OELF,
constructores diferidos, comparación del payload extraído y del icono embebido.

Evidencia física: `build/ps4-logs/20261029-115311-compute-probe/`.
Fuente compartida host/PS4: `tests/gpu_compute_probe.c`.

## Prueba de juegos

Título separado `NPSV00064`, `NPS64 - Vulkan Game Test`. Conserva 00.69 y el
diagnóstico de cómputo. Usa el RDP completo; las opciones antiguas de binning
alternativo y rasterizado de solo color están desactivadas. Se comienza con
ubershader, sin subgroups ni tipos pequeños y con dependencia entre setup y
binning. No se retira la cuarentena de las compilaciones normales.

Esta ruta sustituye la rasterización Angrylion por paraLLEl-RDP; no dibuja una
segunda copia de la escena CPU como el experimento HLE. Mantiene el JIT MIPS y
RSP HLE en CPU, y el scanout Vulkan todavía vuelve a RAM para escalado/VideoOut.
Es una primera prueba funcional; sincronización, shaders, readback y compilación
de pipelines pueden limitar su rendimiento. La prueba pequeña no cubre todos
los shaders ni dependencias de un juego real.

- Lee las ROMs existentes de `/data/NPS64/games`.
- Usa `/data/NPS64/vulkan-test-saves`, sin compartir guardados o estados CPU.
- Configuración propia: `vulkan-test-settings.cfg` y `vulkan-test-diagnostics.cfg`.
- Log: `/data/NPS64/vulkan-game-test.log`.
- Log del mismo backend GNM validado: `/data/NPS64/gpu-probe-driver.log`.
- Sin actualizador automático ni publicación en GitHub.

Debe confirmarse `renderer=paraLLEl-RDP active`, imagen correcta, sonido y cierre
antes de comparar `[perf]` contra CPU en el mismo recorrido. Si aparece fallback
Angrylion, no contar la ejecución como una medición GPU. El primer arranque
incluye compilación de shaders; separar esa fase del recorrido caliente.

El paquete de juego ya se construyó, verificó y subió por FTP a `/data/pkg/`.
SHA-256: `4681ec8549e4b0b1d248740a5f20efbd15b61d9e554501621189a33b075a0b58`.
Pasaron dos sesiones GPU con ROM sintética, imagen, pausa, estados y cierre;
Majora completó además 600 VIs con imagen visible y audio. Se usó el mismo
perfil de shaders completo en Lavapipe con capa Khronos, sin errores de
validación. La ejecución local incluye un pico de compilación de 14 segundos:
no se usa su media para predecir FPS de consola. Falta la prueba física del juego.

## Corrección de audio incluida para evaluación

El núcleo anterior añade un campo completo de silencio cuando la entrega final
de AI devuelve cero, incluso si los eventos DMA o lecturas AI_LEN ya entregaron
audio en ese mismo campo. Majora acumula así descartes aun a velocidad completa.
Se contabilizan todas las entregas y se rellena únicamente el déficit; las
fracciones de muestra del DAC se conservan. Se limpia la frecuencia exacta al
reiniciar el backend entre juegos. No se cambia la duración de DMA ni sus IRQ.

Prueba local de Majora, 1800 VIs y mismos inputs: antes 2.116.546 frames estéreo
(66,151839 s de audio), después 1.131.794 (35,373709 s), para unos 36 segundos
emulados incluyendo arranque. Ambos entregan 1.392.298 valores no nulos. La
prueba del backend real con sanitizadores cubre DMA completado, lecturas,
campos parciales/silenciosos, exceso, fracción del DAC y vaciado de emergencia.
Esto no corrige los underruns debidos a emulación a media velocidad: siguen
dependiendo del rendimiento. La escucha y los contadores físicos están pendientes.

## Reproducir la construcción con las dependencias locales

```sh
python3 scripts/build_compute_probe.py
python3 scripts/build_gpu_game_test.py
```

Ambos aceptan `--reference-work` y `--sdk`. No instalan herramientas globales,
no modifican las fuentes de referencia y no suben ni publican paquetes.
Los scripts presuponen el árbol de dependencias preparado de xuperPS4 01.19;
no sustituyen su bootstrap. La aprobación de publicación sigue pendiente.

## Fallo físico de la prueba de juegos

Majora llegó a inicializar paraLLEl-RDP, seleccionar Liverpool y completar el
fence del primer scanout visible. Después, el driver informa `waiting for 110,
completed 109, submitted 111`. Esto confirma trabajo detenido; no identifica
por sí solo el dispatch causante. Los tiempos iniciales incluyen compilación
y no se consideran un benchmark válido de rendimiento sostenido.

El intento de importar RDRAM mediante external-memory-host falla y el núcleo
usa su alternativa de copia. El backend GNM devuelve ENOSYS para userptr;
ese mensaje no demuestra que sea la causa del bloqueo. Se desactiva esa
importación en los siguientes diagnósticos para evitar un intento no soportado.

El volcado detallado no se guardó porque faltaba `/data/NPS64/gpu-probe`.
La preparación del diagnóstico ahora exige crear y comprobar esa carpeta y
`flight` antes de iniciar GPU, y habilita el flight recorder existente de
DolphinPS4. La prueba de carpetas comprueba creación, reutilización, colisión
con un archivo y rutas largas con ASan/UBSan. El analizador de registros IBRC
decodifica comandos PM4 sin ejecutarlos y rechaza capturas truncadas.

Captura del fallo: `build/ps4-logs/20261029-120308-vulkan-game-freeze/`.
No existe un dump que permita reconstruir exactamente la submission 110.

## Análisis de kernels fuera de la consola

Se exportaron SPIR-V, layouts y constantes de especialización del recorrido
real de Majora en Lavapipe. Los hashes de cinco pipelines coinciden con los
compilados inmediatamente antes del bloqueo físico. Se vuelven a compilar
para un dispositivo virtual Bonaire, sin enviar ningún comando GPU, usando
RADV host 26.2.4 y AMDGPU noop drm-shim. Es la técnica de compilación utilizada
por [Mesa para CI](https://docs.mesa3d.org/ci/drm-shim.html).

El shim procede de Mesa 26.0.8. Una copia local anuncia DRM 3.54 en vez de 3.49
para que el RADV instalado acepte el dispositivo virtual; no se modifica
xuperPS4 ni se ejecuta trabajo en hardware. El compilador host no es el mismo
binario que Mesa 26.0.8 PS4: sus estadísticas son una hipótesis verificable,
no prueba de la causa del reset ni equivalencia de ISA física.

| Perfil del RDP | Pipelines capturados | SGPR máx. | VGPR máx. | Scratch máx. por wave | VGPR derramados máx. |
|---|---:|---:|---:|---:|---:|
| Ubershader | 7 | 112 | 256 | 5120 bytes | 79 |
| Etapas separadas | 23 | 72 | 48 | 0 | 0 |

Los pipelines `5fb308f708ff1198` y `e03a18dca5918de5` del log físico son dos
especializaciones del mismo ubershader; ambos generan la presión alta en
el compilador host. Las pruebas pequeñas de cómputo no usan scratch. Eso
justifica evaluar etapas separadas y el manejo de memoria temporal, pero no
permite afirmar todavía que se corrigió el bloqueo.

Herramientas nuevas: `capture_vulkan_pipelines.py`,
`compile_captured_pipelines.py`, `tests/compile_captured_pipeline.c` y
`analyze_ps4_flight.py`. El compilador de pipelines no contiene dispatches ni
envíos de cola. Informes: `build/vulkan-offline/bonaire-pipelines/report.json`
y `build/vulkan-offline/bonaire-split/report.json`.

Se compararon 190 hashes de imagen del recorrido Vulkan con ubershader y con
etapas separadas: coinciden exactamente en ese recorrido local. No es todavía
una prueba de compatibilidad completa ni de velocidad en PS4.

También se descargó la revisión actual de DolphinPS4
`879b3e57f3d3180bcb5f2234db3903688b28cd7d` (25 de octubre). El archivo
[`port/mesa/ac_ps4_drm.c`](https://github.com/iHaiDeeZ/DolphinPS4/blob/879b3e57f3d3180bcb5f2234db3903688b28cd7d/port/mesa/ac_ps4_drm.c)
y `patches/mesa-ps4.patch` son idénticos a la referencia 69360a1 usada por
xuperPS4. SHA-256 del archivo GNM original:
`8a2a387a56405c87ea2af8f0eef5fa07863c3a7073a00586fcdf6e68f18fb37d`.
La capacidad de Vulkan/RADV en PS4 no se pone en duda; se investiga la carga
y la integración particulares de paraLLEl-RDP.

## 00.02 — únicamente compilación y consulta de recursos

Se preparó `NPS64-Vulkan-Compile-Test-00.02.pkg`, actualización del título de
diagnóstico NPSG00064. No es otra versión de la prueba de juegos fallida.
Empaqueta dos especializaciones del ubershader y 23 pipelines separados,
exportados del mismo recorrido. Compila cada pipeline y consulta los recursos
reales de Mesa 26.0.8 PS4 mediante VK_KHR_pipeline_executable_properties.

La función compartida con el host no crea buffers de comandos, no obtiene
colas y no hace dispatches ni submits. La tabla de funciones generada se
comprueba para excluir esas entradas. El resultado solo valida compilación
y muestra estadísticas; no declara que los kernels terminen al ejecutarse.

Log: `/data/NPS64/pipeline-compile.log`. En la interfaz, X compila el grupo
seleccionado, arriba/abajo cambia entre ubershader y etapas separadas, y
círculo sale. Los tiempos incluyen inicialización y compilación, no FPS.
Pasaron la compilación cruzada, verificaciones ELF y 28 controles de PKG;
se compararon payload extraído, los 25 fixtures, icono y SFO. El mismo
compilador de prueba pasa la capa Khronos en el host.

Reproducción con los pipelines capturados disponibles:

```sh
python3 scripts/build_compute_probe.py --compile-only
```

El paquete de juego 00.01 sigue retirado. No se subió otra prueba de ejecución
completa de RDP ni se publicó nada en GitHub.


### Resultado físico de 00.02

Los logs conservados en `build/ps4-logs/20261029-123413-pipeline-compile`
confirman 25/25 compilaciones en AMD Liverpool / RADV BONAIRE, Mesa 26.0.8.
Los dos ubershaders requieren 256 VGPR, 79 VGPR derramados y 5120 bytes de
scratch por wave. Los 23 pipelines separados usan como máximo 72 SGPR,
48 VGPR y 512 bytes de LDS, sin derrames ni scratch. Esto confirma en PS4
las estadísticas locales; no demuestra todavía ejecución estable ni FPS.

Se añadió una barrera de admisión de pipelines para la prueba de juegos:
consulta los recursos antes de entregar el pipeline al renderer y rechaza
scratch, derrames de VGPR, más de 96 SGPR, 128 VGPR o 512 bytes de LDS.
También rechaza estadísticas ausentes o fallidas y bloquea reintentos tras
un rechazo. Pasaron las pruebas con ASan/UBSan de límites, errores y limpieza
de handles. Los 23 pipelines separados fueron aceptados y el ubershader
rechazado usando la API Vulkan con el dispositivo virtual Bonaire, sin submits.
Esta comprobación no detecta todos los posibles errores de memoria o sincronización.

## 00.03 — ejecución mínima entre waves

Antes de volver al RDP completo, `NPS64-Vulkan-Multiwave-Test-00.03.pkg`
prueba cinco casos: almacenamiento de 64 hilos y memoria compartida con
32, 64, 128 y 256 hilos por grupo. Una máscara leída en ejecución evita que
el compilador elimine la comunicación; los casos 128/256 usan 512 bytes LDS
en la compilación virtual Bonaire. Cada caso comprueba dos dispatches,
su dependencia, resultados exactos y palabras de guarda. No carga juegos.

Los cinco casos pasan en Lavapipe con validación Khronos y compilan para
Bonaire sin scratch ni derrames. La prueba física sigue pendiente. Aunque
la carga sea pequeña, un fallo de driver durante su ejecución puede bloquear
la GPU; no es una garantía de que el problema anterior esté corregido.

Instalar sobre el diagnóstico NPSG00064, recorrer los cinco casos con
arriba/abajo y ejecutar cada uno con X. Log: `/data/NPS64/compute-probe.log`,
identificado por el encabezado 00.03. El paquete de juego fallido sigue retirado.


### Resultado físico 00.03 y candidato de juego 00.02

Logs en `build/ps4-logs/20261111-231218-multiwave`: 5/5 resultados cero,
con salida exacta y guardas intactas en Liverpool. Incluye 128 y 256 hilos,
por lo que la comunicación LDS entre waves funciona en estos kernels mínimos.
No prueba todavía que toda la sincronización del renderer sea correcta.

El candidato `NPS64-Vulkan-Game-Test-00.02.pkg` usa etapas separadas,
la comprobación de recursos, captura de comandos antes de enviarlos y
copia de RDRAM en lugar de importación externa. No cambia el título estable.
No permite continuar por CPU si falla el arranque Vulkan. Se pausa a los
180 VI por carga; intentar reanudar no ejecuta más VI. El límite se aplica
entre llamadas al núcleo, no puede interrumpir un dispatch que ya se bloquee.
La siguiente prueba física debe comenzar con Majora, que produjo el bloqueo
anterior. Todavía no se declara resuelto el bloqueo ni validado el rendimiento.


Validación local del candidato 00.02: Majora en Lavapipe con capa Khronos,
scanout visible y renderer paraLLEl-RDP activo, ubershader=0. Alcanzó exactamente
el aviso de límite a 180 VI; el backend intentó reanudar posteriormente y la
pausa se mantuvo. Ejecución finalizó con código cero y sin mensajes VUID ni
Validation Error. Evidencia: `build/vulkan-game-00.02-host.log` y
`build/gpu-game-test/host-validation-00.02.log`. Las comprobaciones ELF,
constructores diferidos, 28 verificaciones PKG, payload extraído, icono y SFO
pasaron. La prueba del guard con ASan/UBSan volvió a pasar. La consulta de
recursos del guard se probó por separado con Bonaire virtual; Lavapipe en
esta ejecución no reproduce el controlador PS4 ni mide su rendimiento.


### Fallo físico del candidato de juego 00.02

Usuario informa CE-34878-0 al cargar Majora. Evidencia preservada en
`build/ps4-logs/20261111-231636-game-00.02-crash`. La ROM sí cargó: primer
scanout 640x288 completado, imagen visible y último informe con 31 VI.
No hay rechazo del guard. Tras el cambio a scanout 1x1 se registran pipelines
7dec72a83506cb7f (64 SGPR/32 VGPR), eb9caba993e1e289
(32 SGPR/48 VGPR/512 LDS) y 9f567813741f38af (24 SGPR/8 VGPR), todos sin
scratch ni derrames y compilados correctamente según los mensajes disponibles.

El flight recorder conservó envíos 102–109; hay dispatches directos en 103,
106 y 109. El último progreso registra completed=109 y submitted=109.
Esto no localiza la instrucción causante ni descarta un fallo GPU posterior,
pero no reproduce la evidencia anterior de espera 110 con completed=109.
No se obtuvo dirección de excepción ni backtrace. `/data/sce_coredumps`
está vacío; `sce_logs/log.bin` se conservó como binario, sin texto útil.
No se afirma que el cierre sea un fallo CPU ni que los shaders estén exentos
de errores. El paquete 00.02 queda retirado de `/data/pkg` conservando el
artefacto, ELF y capturas locales para investigación. No hay release publicado.


## Juego 00.03 — puntos de entrada y retorno

Se añaden checkpoints Vulkan con identificadores de llamada para
CreateComputePipelines, Begin/EndCommandBuffer, CmdBindPipeline,
CmdBindDescriptorSets, CmdPushConstants, CmdDispatch/Indirect, QueueSubmit,
QueueSubmit2/KHR y WaitForFences. Conservan argumentos y resultados, incluso
VK_ERROR_DEVICE_LOST, y no anuncian funciones ausentes. El wrapper se aplica
sobre el guard de recursos: no lo sustituye. Es diagnóstico de un dispositivo
activo a la vez y no debe reiniciarse mientras el renderer esté trabajando.

La copia de compilación del núcleo registra el hash de shader/pipeline antes
y después de compilar y el comienzo/final de preparación de cada dispatch.
El frontend registra entrada/salida de cada VI de la prueba. No se modifican
los shaders ni se suprime trabajo para obtener un resultado favorable. Se
mantienen etapas separadas, importación externa desactivada, guard y límite
180 VI. El nuevo logging afecta los tiempos; este paquete no mide rendimiento.

`analyze_gpu_checkpoints.py` empareja llamadas incluso si se intercalan y
selecciona la última sesión del log acumulado. Una entrada sin salida localiza
un intervalo, pero no demuestra por sí sola fallo CPU o GPU. El test con
ASan/UBSan del wrapper valida funciones ausentes, ambos nombres de Submit2,
offset de 64 bits, argumentos sin alterar y propagación de error del driver.
El test del analizador valida intercalado, error devuelto, llamada pendiente,
aislamiento de sesiones y pares incorrectos.


### Validación local final de 00.03

Se repitió Majora con núcleo y frontend ASan/UBSan, Lavapipe y capa Khronos.
El rasterizador CPU no utilizado se reutilizó sin sanitizer; el driver Mesa
es precompilado. La compilación inicial encontró un memcpy con puntero nulo
y tamaño cero en `RenderPass::setup_subpasses`. `patch_vulkan_safety.py`
evita ambas copias de attachments cuando su conteo es cero, conservando el
resultado inicializado de la estructura. No se atribuye el CE-34878-0 a ese
hallazgo, que ocurre también en el host que consigue terminar.

Tras corregirlo, el recorrido finaliza con código cero a 180 VI, incluyendo
intentos posteriores de reanudar. Todas las llamadas Vulkan registradas
retornan (11.015 entradas/salidas en la ejecución final), sin errores ASan ni de validación Vulkan,
y sin avisos UBSan en `parallel-rdp/vulkan`. Quedan 37 avisos UBSan en el
núcleo/JIT, controlador y emisión CPU de comandos RDP. Se conservan completos
en el informe; no se declara ejecución limpia de UBSan y no se modificó esa
lógica en este trabajo. No se reprodujo el cierre de PS4 en el host.

Reproducción del test con la compilación sanitizer y ROM local preparadas:
`bash build/run-local.sh bash scripts/test_vulkan_game_trace.sh`.
Evidencia: `build/vulkan-game-00.03-asan.log`,
`build/gpu-game-test/host-00.03-core.log`, `host-00.03-checkpoints.json`
y `manifest-00.03.json`. El diagnóstico no sirve para comparar FPS por el
coste de los checkpoints y sanitizers. La prueba física queda pendiente.


### Fallo físico del juego 00.03: llamada acotada

Evidencia en `build/ps4-logs/20261111-233408-game-00.03-crash`.
Hay 950 entradas y 949 retornos. La única llamada pendiente es
vkCreateComputePipelines (id=950), pipeline 52bbce7140cf109d,
SPIR-V b657f09ca9e10227, dentro del VI 36. La preparación de dispatch
indirecto anterior (id=949) retorna y queda registrada. El último progreso
GPU sigue completed=109/submitted=109; el command buffer que se preparaba
no alcanza el checkpoint de envío. Esto acota el cierre a la llamada envuelta,
pero no distingue todavía creación del driver de consultas del guard ni
elimina la posibilidad de una terminación asíncrona.

El mismo pipeline 52bb sí compiló en la PS4 con el diagnóstico 00.02:
64 SGPR, 40 VGPR, cero scratch/derrames, 219.232 ms. Ese diagnóstico usa un
dispositivo limpio por caso y VK_NULL_HANDLE como pipeline cache, mientras
el juego conserva dispositivo/cache y ocupa memoria con el núcleo y la ROM.
No se atribuye aún el fallo a pila, heap, cache ni a una llamada concreta
del driver. Se conserva el ELF/PKG 00.03 local y se retira su PKG de la PS4.

## Diagnóstico 00.04 — comparación del guard en el pipeline 52bb

`NPS64-Vulkan-Guard-Compare-00.04.pkg`, título NPSG00064, no es otro juego.
Contiene dos fixtures idénticos del pipeline 52bb ya capturado; grupo 0
compila sin el wrapper de recursos y grupo 1 con el wrapper. No se resuelven
APIs vkCmd ni vkQueue, no se cargan ROMs y no se envían comandos GPU.
Se añaden checkpoints internos de creación/consulta y un informe de la
región virtual de pila en PS4 antes de llamar al driver. La consulta usa
la estructura ABI de OpenOrbis; las direcciones se registran para comparar,
no para concluir por sí solas que hay desbordamiento.

Ambos modos pasan con RADV y Bonaire virtual en el host. El test ASan/UBSan
del guard volvió a pasar. La comparación física sigue pendiente. Esta prueba
solo aísla el wrapper; no reproduce toda la memoria/cache/contexto del juego.
Uso: seleccionar cada una de las dos filas y X. Log acumulado
`/data/NPS64/pipeline-compile.log`, encabezado Guard Compare 00.04.


### Resultado físico Guard Compare 00.04

Logs `build/ps4-logs/20261111-234348-guard-compare`: ambos casos terminan
con resultado cero. El guard registra retorno correcto de creación del driver,
consulta de propiedades, conteo de estadísticas y lectura de estadísticas.
Los recursos coinciden: 64 SGPR, 40 VGPR, cero scratch, derrames y LDS.
La región de pila consultada mide 2 MiB. Ambos casos usan cache nula y un
dispositivo nuevo. Esto valida el guard en ese contexto, sin demostrar todavía
que la pila, cache o memoria del juego expliquen el cierre.

## Juego 00.04 — contexto de compilación aislado

Se conserva shader, especialización, dispositivo, VkPipelineCache, argumentos
y configuración de RDP. Cada llamada de creación compute se ejecuta de forma
síncrona en un hilo joinable con pila explícita de 8 MiB. El caller espera
pthread_join antes de consultar recursos o usar el resultado. Un error de
creación de hilo se propaga; no se reintenta sobre la pila anterior. Un fallo
de join termina el proceso para no dejar que el hilo use datos de pila caducados.
No se atribuye aún el problema a la pila: también cambia el contexto de hilo.
Se registran regiones de pila en caller/worker y retornos de cada fase del
guard. El malloc de esta variante escribe un marcador en boot.log si una
asignación falla. Se conserva la pausa obligatoria a 180 VI; no es benchmark.

El test unitario ASan/UBSan verifica ejecución en otro hilo, tamaño de pila,
cache y argumentos intactos, resultado publicado después del join y propagación
de éxito/OOM/device-lost. El guard pasa también con worker habilitado, y el
pipeline 52bb compila con guard+worker en Bonaire virtual sin envíos GPU.
Majora pasa localmente los 180 VI con 23 workers terminados; sin errores ASan
o de validación Vulkan. Persisten 37 avisos UBSan del núcleo/emisión ajenos al
código Vulkan. El test nativo usa un adaptador en la tabla API para entregar
la creación al worker: Lavapipe no ofrece el guard de estadísticas de PS4.
Esta limitación se conserva en manifest-00.04.json. La ejecución física de
esta variante está pendiente. El ELF/manifest del juego 00.03 se archivó antes
de reconstruir en `build/gpu-game-test/archive-00.03`.


### Fallos físicos del juego 00.04 y pila real de callbacks

Logs en `build/ps4-logs/20261111-235343-game-00.04-crashes`. Majora sigue
cerrándose dentro del VI 36, pero el pipeline 52bbce7140cf109d retorna
correctamente con el worker y supera también todas las consultas del guard.
Llega a enviar ese command buffer. La llamada pendiente cambia al pipeline
73ffd61f6a9c3dfd (id=962). Mario 64 Europa muestra el mismo patrón en VI 109
(id=1155). La entrada de API se registra, pero no el primer checkpoint interno
del guard. La pila del callback registrada en ambos juegos mide 64 KiB, frente
a los 2 MiB del diagnóstico aislado; el worker sí mide los 8 MiB pedidos.
La cache que pasa la creación en estos logs es NULL; la hipótesis anterior de
una cache distinta no está respaldada para estas llamadas. No se registró
el marcador de malloc fallido.

## Juego 00.05 — corregir el consumo de pila del guard

El guard que añadimos reservaba estadísticas para 64 entradas y un batch para
16 pipelines en la pila, incluso con count=1. El objeto PS4 00.04 reserva
0x8f18 bytes locales, además de registros salvados. Se trasladan ambos arrays
acotados al heap. Se libera el batch después del retorno/join y la tabla tras
las consultas; ante fallo de asignación se rechaza la operación, se destruyen
los pipelines ya creados y se anulan las salidas. El worker de 8 MiB se mantiene.
La compilación PS4 informa ahora 328 bytes en guarded_create_compute y añade
un límite obligatorio de 4096 bytes por frame para guard/worker.

Se creó un reproducer sintético: hilo con pila real de 64 KiB y 32 KiB ya
ocupados por el llamador. El guard anterior termina con SIGSEGV (exit=-11);
el corregido pasa con worker habilitado y deshabilitado. Este reproducer
confirma el riesgo y la corrección local, pero no sustituye un backtrace de
la consola ni garantiza que no exista otro fallo GPU. Se preservan fuente,
objeto y ELF anteriores en `build/gpu-game-test/archive-00.04`.

`bash build/run-local.sh bash scripts/test_gpu_guard.sh` verifica umbrales,
rechazos, asignaciones fallidas antes/después de crear el pipeline, limpieza
sin fugas con ASan/UBSan, bloqueo de reintentos y regresión de pila pequeña.
La compilación de los 23 pipelines separados con guard+worker pasa en Bonaire
virtual. Majora vuelve a completar 180 VI localmente con el adaptador nativo
de worker, sin error ASan ni de validación Vulkan; siguen los 37 avisos UBSan
previos ajenos al código Vulkan. El paquete pasó ELF, constructores diferidos,
28 controles PKG, extracción, icono y SFO. La ejecución física 00.05 sigue
pendiente y no se declara corregido todavía el cierre completo del juego.


### Resultado físico del juego 00.05: ambos alcanzan el límite

Evidencia `build/ps4-logs/20261112-000357-game-00.05-pass`.
Mario 64 Europa y Majora PAL GC alcanzan exactamente el límite de 180 VI:
última llamada frame=179 end y un único marcador LIMIT por sesión. No hay
rechazos del guard. Mario termina 12 compilaciones/joins y vuelve a biblioteca
con ROM cerrada; Majora termina 23 compilaciones/joins y continúa en pausa.
La pausa observada es la protección programada del diagnóstico, no un bloqueo.
Esto valida en hardware que el recorrido inicial antes fallido consigue avanzar
tras reducir la pila del guard; no valida juego prolongado ni rendimiento final.

Siguiente validación prevista: recorridos prolongados con interacción y cambios
de escena en Mario/Majora; luego GoldenEye. Para medir velocidad hace falta
reducir los checkpoints detallados manteniendo errores, guard y métricas de VI,
percentiles de tiempo por frame y underruns de audio. No publicar hasta contar
con mediciones físicas comparables y aprobación explícita del usuario.


## Juego 00.06 — sesión prolongada y registros compactos

Se sustituye la pausa de 180 VI por un máximo de 600 segundos de reloj por
carga de ROM, desde que finaliza su carga. Incluye pausas manuales. Una vez
vencido, intentar reanudar no ejecuta más retro_run; salir y cargar otra ROM
inicia otro plazo. El límite se comprueba entre VI y no interrumpe un dispatch
que se haya bloqueado. Guard, arrays en heap y compiler worker de 8 MiB se
mantienen. El guard ocupa ahora 296 bytes de pila en la compilación PS4.

Por defecto se eliminan las entradas/salidas por API, registros por dispatch
y la captura continua de IB/flight recorder. Se conservan errores, métricas
por segundo, percentiles por frame, audio y un par breve por compilación con
hash de shader/pipeline. NPS64_GPU_API_TRACE=1 permite el modo detallado en
una compilación de diagnóstico; no hay opción nueva en la interfaz. Los
archivos flight antiguos no se actualizan en el modo compacto y no deben
interpretarse como capturas de esta sesión.

Las métricas vi_fps/speed_pct muestran ritmo real; gpu_ms=0 no mide la duración
de los kernels compute en esta ruta. La espera/readback Vulkan ocurre dentro
de retro_run y contribuye a core_ms. No se infiere ocupación GPU de ese cero.
Las comparaciones CPU/GPU deben usar el mismo juego, región y escena, separando
el arranque/compilación del recorrido estable.

Validación nativa con ASan/UBSan y Khronos: se usó un plazo reducido de 30 s
solo en el ejecutable de test. Majora alcanzó 1070 VI y se pausó a 30.039 s.
El backend intentó reanudar después del vencimiento y el contador al salir
permaneció en 1070. No hay errores ASan ni VUID, ni registros detallados por
API/dispatch, y se conservan las métricas. Se registran 40 avisos UBSan del
núcleo/emisión ajenos al código Vulkan en este recorrido más largo; no se
declara ejecución limpia de UBSan. El adaptador nativo de worker y el driver
precompilado conservan las limitaciones documentadas en versiones anteriores.
Pasaron también regresión de pila de 64 KiB, fallos de asignación, handoff de
worker y test de tracing: modo compacto silencia llamadas normales y conserva
errores devueltos. ELF, límite de pila por función, constructores y 28 checks
PKG con payload/icono/SFO extraídos pasan. La prueba de 600 s en PS4 está
pendiente. Se conserva PKG 00.05 como respaldo del recorrido físico validado;
no se publica ningún release ni se realizan cambios en GitHub.


### Sesión física 00.06 con varios juegos

Evidencia en `build/ps4-logs/20261112-002047-game-00.06-playing`.
Se descargó el log inicialmente y después solo los bytes nuevos con FTP REST
para reducir tráfico durante la prueba. El seguimiento terminó antes de la
siguiente variante. Cinco cargas cierran normalmente: Mario 9949 VI/253.967 s,
Mario Kart 1114/46.009 s, Ocarina 929/29.795 s, Majora 1692/52.633 s y
GoldenEye 4159/117.585 s. No hay rechazos del guard. Son 17843 VI y unos
500 s sumados entre sesiones; ninguna sesión completa el límite de 600 s.
No se declara por ello validación de una partida continua de diez minutos.

Las medias aritméticas de vi_fps por intervalo, descartando los primeros cinco
intervalos de cada carga, fueron 39.68, 39.11, 37.90, 37.75 y 38.59,
respectivamente. Incluyen cambios de escena y posibles compilaciones nuevas:
no son un benchmark controlado ni una media ponderada por tiempo. GoldenEye
USA tiene objetivo 60; los otros juegos de la muestra son PAL con objetivo 50.
El usuario informa que todos se perciben más fluidos que la versión CPU,
aunque reporta entre 17 y 40 en pantalla. No se cuantifica una mejora frente
a CPU sin comparar la misma escena y región.

Los underruns de audio crecen en cada sesión (deltas 356/29/21/57/166).
El contador de dropped es acumulativo: Mario Kart añade 753; ese valor se
hereda en juegos posteriores, donde no hay otro incremento en la muestra.
El rendimiento y el audio todavía necesitan optimización.

## Juego 00.07 — memoria de lectura con caché y desglose de tiempos

La elección anterior de memoria de readback tomaba el primer tipo visible y
coherente. Mesa 26.0.8 enumera un GTT write-combined antes de los GTT cached;
el backend GNM mapea VRAM o CPU_GTT_USWC como WC garlic y el resto como WB
onion. Se prioriza ahora HOST_CACHED entre los tipos admitidos por
memoryTypeBits, después COHERENT dentro del mismo nivel. Si no hay cached,
se conserva el fallback visible/coherente; si se escoge no coherente, se
conserva InvalidateMappedMemoryRanges. Las imágenes y el resto de buffers
del renderer no cambian de política. No cambian filtros ni shaders.

`[gpu-readback]` registra tipo, flags, cached/coherent y tamaño. Cada segundo,
`[vulkan-times]` mide preparación, envío, espera de fence, invalidación,
readback total y memcpy hacia la copia nativa, en ms de reloj por loop.
core_other_ms resta readback y snapshot de core_ms, pero sigue incluyendo
emulación, creación/emisión de comandos, compilación y otras esperas del
renderer. No representa CPU pura ni duración de kernels GPU. readback_ms es
la suma de sus cuatro fases; no debe sumarse otra vez a esas fases. Se deben
separar intervalos de arranque/transición al analizar el rendimiento.

El selector pasa ASan/UBSan con máscaras, memoria ausente, bit31 y fallback
coherente/no coherente. Majora pasa el test temporizado nativo, cap y bloqueo
de reanudación, con validación Khronos y sin errores ASan ni VUID; permanecen
los 40 avisos UBSan previos ajenos al código Vulkan. Las cuentas de las fases
se verificaron en 17 intervalos. Se compararon 609 hashes de imágenes con el
ejecutable 00.06, usando estados locales aislados: todos coinciden. Lavapipe
ya ofrecía memoria cached, así que esta comparación no prueba el cambio de
tipo ni su coherencia/rendimiento en el driver PS4. También pasan scanout,
conversión RGBA, save/load y dos sesiones GPU independientes en el test
funcional sintético. ELF, constructores, límite de pila y 28 checks de PKG,
payload extraído, icono y SFO pasan. La mejora física queda pendiente. Se
conserva 00.06 como referencia y se retira el PKG 00.05 tras verificar la
subida de 00.07. No hay publicación en GitHub.


## 00.07: resultados físicos y preparación de 00.08 (2026-11-12)

El usuario confirmó una mejora general en todos los juegos con el readback
cached. Los logs físicos seleccionan memory_type=5, flags=0xe, cached=1 y
coherent=1. Mario PAL y Kart llegan a una mediana próxima a 50 VI/s; Ocarina
también tiene intervalos sostenidos a 50. Majora sigue con intervalos por
debajo del objetivo y audio lento/distorsionado. Estas sesiones usan escenas
diferentes, por lo que no constituyen un benchmark comparativo controlado.

Los tirones coinciden con compilaciones nuevas de aproximadamente 240–250 ms;
varias en un intervalo alcanzan unos 500–750 ms. El readback físico ya ronda
0.25 ms y la copia de snapshot 0.24 ms, con fence de unos 0.15 ms. En Majora
se observan además esperas VideoOut de 6–8 ms. Los tiempos son de reloj CPU;
no son medidas de ejecución de kernels en la GPU.

00.08 crea un VkPipelineCache también sin GRANITE_VULKAN_FILESYSTEM y guarda
su contenido al destruir el dispositivo al salir del juego. Directorio:
/data/NPS64/pipeline-cache. Se conserva el formato interno Granite y se añade
un contenedor de identidad de GPU/driver/UUID, límite de 32 MiB, SHA256 y
reemplazo atómico. Una caché incompatible, truncada o corrupta se descarta.
No se guardan ROMs ni partidas en ella. Es necesario salir normalmente del
juego para persistir lo compilado; cerrar abruptamente puede perder la caché
nueva. Un driver diferente genera otra identidad de caché.

El test de compilación sin envíos GPU con RADV y BONAIRE virtual guarda
4868 bytes y recarga ese blob: 5.933 ms en frío frente a 0.286 ms en caliente.
Los controles de recursos continúan activos en ambos casos. Esto comprueba
la reutilización de un pipeline en ese compilador del host, no una ganancia
medida en PS4. Lavapipe solo serializa su cabecera vacía de caché; su prueba
se usa para integridad, carga y equivalencia de imagen, no para medir rapidez.

El título de juego pasa de dos a tres buffers VideoOut, igual que el camino
CPU. Conserva la espera por finalización antes de reutilizar cada buffer y
permite como máximo dos flips pendientes. Se registra el número de buffers
y los tiempos de espera/copia. El efecto sobre tirones y audio necesita
medición física de 00.08. No se aumenta indiscriminadamente la cola de audio.
Los tests del resampler sinc y del backend de campos pasan con ASan/UBSan:
RMS 5.220 y alias relativo 0.000071 en la señal de prueba del resampler.
Estos tests no demuestran que el audio de Majora esté resuelto en consola.

Se evaluó la compilación asíncrona original del renderer split, sin activar
el ubershader que bloqueó la consola. Una ejecución nativa captura el fallback
real 386b345ad2a2a7dd: su compilación para BONAIRE requiere 112 SGPR, frente
al límite validado de 96. No tiene spills ni scratch, pero el guard lo rechaza.
Por ello 00.08 conserva FORCE_SYNC_SHADER=1 por defecto y no relaja límites.
El modo asíncrono queda optativo para investigación local. La caché puede
reducir compilaciones al repetir sesiones, pero no elimina la primera
compilación de una variante desconocida. Los probes de caché con la garantía
FAIL_ON_PIPELINE_COMPILE_REQUIRED no crean un hilo nuevo; las compilaciones
reales mantienen el hilo de 8 MiB. El test verifica ambos caminos.

Mario Tennis (E) (VC), digest d40fb0e87996af902eff9c194027fc3b, no está
corregido. Su sesión física emite scanout 1x1 sin imagen visible. Se identificó
una instrucción reservada 0x7c000000 en el bootstrap. El experimento de sustituir
solo esa palabra en RAM permite avanzar más allá de la excepción, pero sigue
sin imagen/audio tras 15000 VI con JIT y 600 con intérprete cached. Se retiró
ese experimento del PKG: se conserva únicamente el ajuste CRC previo,
limitado a su identidad y payload verificados. No se acelera a ciegas el reloj
VI ni se toca el archivo ROM. El contador de VI no prueba que el juego cargue.

00.08 continúa como PKG local de prueba NPSV00064, con partidas aisladas y
límite de 600 segundos por carga. No hay publicación, push, tag ni release.

Validación final 00.08: sesión nativa de Majora con 1024 VI y pausa a
30.019 s, con intento de reanudación bloqueado; sin error ASan ni VUID.
Se conservan los 40 avisos UBSan conocidos ajenos al código Vulkan.
Dos ejecuciones del mismo binario con SRAM restaurada validan inicio sin
caché, guardado, recarga y 1010 hashes de imagen iguales.
Lavapipe guarda 56 bytes internos; no se interpreta la diferencia de VI
entre esas sesiones como beneficio de caché. Pasan los tests de guard con
pila real de 64 KiB, fallos de asignación, worker de 8 MiB y probes directos;
también almacenamiento corrupto/interrumpido, compatibilidad ROM acotada,
resampler y backend de audio. Pasan ELF, constructores diferidos, pila,
payload extraído, icono actualizado, SFO y 28 comprobaciones de PKG.

PKG 00.08: `40788ad632692fce8589c888bda997eac6447e5fda6f8ccb8d70b35d64863ece` (31326208 bytes).
Prueba física pendiente: comparar Majora al primer arranque y tras volver a
biblioteca para guardar la caché y abrirlo de nuevo, recorriendo la misma
zona. Revisar tiempos de compilación, cache loaded/bytes, VideoOut wait,
VI/s y underruns, además de la percepción de audio y tirones del usuario.

Subida FTP 192.168.1.14:2121 completada a /data/pkg; descarga remota
verificada por SHA256 antes de renombrar el PKG temporal. Se retiró solo
NPS64-Vulkan-Game-Test-00.06.pkg y se conservan 00.07, CPU 00.69 y xuperPS4.
No se modificaron ROMs, partidas ni logs remotos.


## 00.08 físico: tirones al cambiar de cámara; revisión 00.09

El usuario confirma menos tirones en Mario 64, pero caídas bruscas a unos
29 FPS; Zelda durante la intro y GoldenEye presentan el mismo patrón,
especialmente al cambiar cámara/pantalla. La instantánea física
20261112-212249 conserva Mario completo, Ocarina completo y GoldenEye parcial.
Sus medianas por intervalo son 49.95, 49.95 y 59.94 VI/s respectivamente;
las regiones PAL/NTSC tienen objetivos distintos. Mario compila 38 pipelines
por 7196 ms acumulados, Ocarina 49 por 10008 ms y GoldenEye parcial 41 por
8017 ms. Son tiempos de reloj de compilación, no tiempo GPU de renderizado.
En el intervalo de Mario de 29.61 VI/s aparecen dos compilaciones de unos
214 ms cada una. Tres buffers están activos, pero no evitan que el hilo de
render espere a un pipeline nuevo.

Se encontró una deficiencia adicional de 00.08 en consola: guarda cachés
(143512 bytes al salir de Mario y 257008 tras Zelda), pero la siguiente
inicialización sigue indicando loaded=0. Por tanto no se ha demostrado aún
la reutilización física y no se atribuye la mejora subjetiva a la caché.
Se descargó el archivo guardado: longitud, identidad y SHA256 válidos. El
nuevo lector nativo con ASan/UBSan recupera sus 257008 bytes correctamente.
La causa exacta del rechazo en consola todavía no se conoce: el cargador
anterior no registraba la rama de fallo.

00.09 cambia el lector para validar primero la cabecera acotada, leer el
payload declarado y comprobar EOF con fread/ferror; elimina la dependencia
de stat y de fgetc para validar el tamaño. Se mantienen identidad, máximo
32 MiB, SHA256, rechazo de truncamiento y datos sobrantes, y escritura
atómica. Se registran motivo y ruta de cada fallo, además de cada hit.
Al salir del juego, fuera del bucle de frames, se vuelve a leer el archivo
recién guardado y se registra readable/bytes/expected. Esa comprobación no
ocurre en los cambios de cámara. El formato de caché no cambia.

Esta revisión busca reparar o localizar el fallo de recuperación antes de
ampliar la compilación en paralelo. Un buffer de presentación adicional no
sustituye un pipeline que todavía no está compilado. La compilación ya es
CPU; el modo asíncrono genérico sigue desactivado y el guard conserva sus
límites, tras la variante de 112 SGPR encontrada en la prueba anterior.
La efectividad de 00.09 queda pendiente de loaded=1 y tiempos de recompilación
en la consola; aún puede haber pausas al encontrar variantes desconocidas.


Validación local 00.09: el lector revisado carga la caché física descargada
(257008 bytes) bajo ASan/UBSan. Pasan identidad/SHA256, archivos truncados,
datos sobrantes, límites, escrituras inválidas y temporal interrumpido.
El test temporizado de Majora alcanza 1160 VI, pausa a 30.005 s y bloquea
la reanudación. La lectura de verificación tras guardar informa readable=1.
No aparecen errores ASan ni VUID. Hay 41 avisos UBSan fuera de Vulkan;
comparado por ubicación con 00.08 se observa además un overflow firmado en
rdp_emit_bridge.c:62, fuente no modificada por esta revisión. Se conserva
el registro para investigar; no se declara que el core esté limpio de UBSan.
El SDK confirma marcos de pila de 1400 bytes en carga y 1192 en guardado
(<4 KiB), y pasa ELF, constructores, extracción, icono, SFO y 28 checks PKG.

Las dos ejecuciones del mismo binario 00.09, con SRAM restaurada, validan
inicio sin caché, guardado, lectura inmediata y recarga; coinciden
1079 hashes de imagen. La caché Lavapipe es una cabecera vacía; esto no
mide el rendimiento PS4. PKG: `31b12b6dd787ced3ef33a3b22df05e6430d51585724a848143e480685a1f9129`
(31326208 bytes). La próxima prueba física debe reutilizar la misma
zona de Mario o la misma intro de Zelda tras volver a biblioteca.

00.09 subida a /data/pkg en 192.168.1.14:2121; bytes remotos verificados
por SHA256 antes del renombrado final. Se retiró solo el PKG 00.07;
se conservan 00.08, CPU 00.69 y xuperPS4. No se alteró la caché remota
ni ROMs, partidas o logs. No hubo publicación en GitHub.


## CE-34878-0 físico con 00.09; diagnóstico aislado 00.10

El usuario informa que Ocarina no inicia y muestra CE-34878-0. La instantánea
20261112-213408 conserva el log de app, driver, caché y log de sistema.
00.09 sí recupera 547072 bytes: loaded=1, ready=1. Termina la carga del core,
el primer pipeline 2016ea5dbb9f30ea se crea en 2.226 ms y pasa el guard
(16 SGPR/8 VGPR/sin scratch ni spills). El último marcador es la selección
del buffer readback 16 bytes cached tipo5. No se completa el primer scanout.
Esto sitúa el cierre después de recuperar la caché, pero no identifica la
instrucción que falla ni demuestra un fallo GPU. El directorio de coredumps
está vacío y el log de sistema no contiene una traza de app aprovechable.

El aviso de SRAM ilegible ya estaba presente en las tres sesiones 00.08;
no se modifica ni elimina ninguna partida para intentar corregir este cierre.
Se examinaron marcos de pila del driver de caché con las opciones de compilación
existentes, sin modificar sus objetos ni bibliotecas. Esa inspección no prueba
la causa del cierre. No se relajan guards ni se cambia Mesa.

00.10 es diagnóstica, no una corrección confirmada de la caché caliente.
Con X normal crea una caché Vulkan vacía en RAM, omite recuperación y guardado
a disco y conserva el archivo previo para reproducir el fallo. L2+X al cargar
en biblioteca habilita explícitamente la ruta de caché persistente; puede
volver a cerrar la app. No se habilita esa ruta si falla la instalación de
los cinco manejadores fatales. Se mantienen tres buffers, readback cached,
compilación síncrona con pila de 8 MiB, guard y límite de sesión de 600 s.

Se añade /data/NPS64/vulkan-game-crash-00.10.log, con descriptor preabierto y
escritura acotada sin heap, stdio ni bloqueo del logger en el manejador. Captura
señal, dirección de fallo, registros, RIP relativo al ELF cuando corresponde
y hasta 16 palabras de pila dentro de la región consultada; son candidatos,
no una traza de llamadas. Se usan los offsets ABI FreeBSD/PS4 documentados
en ps4_crashlog.cpp de DolphinPS4 69360a1. Se restablece automáticamente la
acción fatal: no se intenta continuar tras el error. No es un watchdog GPU,
no captura resets y no garantiza registrar un desbordamiento de pila.
Hay marcadores acotados solo en el primer readback para distinguir asignación,
bind, map, grabación, envío y espera. No generan tráfico continuo por frame.

El test nativo provoca SIGSEGV real en un hijo, verifica dirección 0x1,
PC relativo y terminación por SIGSEGV, y comprueba el fallo de apertura.
El manejador compilado PS4 usa 1656 bytes de pila; init usa 72 (<4 KiB).
Majora nativo pasa ambos modos con validación Khronos, sin ASan ni VUID,
pausa temporizada y reanudación bloqueada. Persistente: 783 VI, pausa a
30.013 s; nueva: 794 VI, pausa a 30.013 s. Coinciden 783 hashes con SRAM
restaurada; el modo nuevo preserva contenido y mtime de la caché en disco.
Permanecen los 40 avisos UBSan del core ajenos a Vulkan. Las pruebas nativas
no reproducen la caché PS4 ni validan el manejador ABI PS4 en hardware.
ELF, constructores, extracción, icono, SFO y 28 checks PKG pasan.

PKG 00.10: `da68c587ca9c129453e581c569841e7ea2fbb5853935a5cb782a536bb85ecddc` (31326208 bytes).
La prueba física necesaria es abrir primero Ocarina con X; si funciona,
volver a biblioteca y reproducir una vez con L2+X para obtener la dirección
exacta del cierre. 00.08 permanece como respaldo. No hay publicación GitHub.

00.10 subida por FTP a /data/pkg y descarga remota verificada por SHA256.
Se retiró únicamente el PKG 00.09; se conservan 00.08, CPU 00.69 y xuperPS4.
No se alteraron cachés, ROMs, partidas ni logs remotos.


## 00.10 físico: prueba A/B confirma cierre al recuperar caché

El usuario confirma que Zelda abre con X y cierra con CE-34878-0 con L2+X.
Instantánea 20261112-214917: el modo fresco ejecutó 1440 VI en 38.423 s y
salió normalmente a biblioteca, omitiendo guardado. El modo persistente
recuperó 547072 bytes, creó el primer pipeline en 184.375 us y pasó el guard.
Completó asignación, bind, map y grabación del primer readback. El último
marcador es `[readback-start] submit begin`; no vuelve del envío.

El archivo fatal está vacío a pesar de handlers=5/5. No se obtuvo RIP ni
se puede asignar el fallo a una instrucción. No hay gpu-fault.txt. Los IB y
progress existentes corresponden al recorder anterior: 00.10 no activó un
recorder para esta reproducción, por lo que no se usan como evidencia actual.
El ensayo A/B identifica la ruta de recuperación como disparador, pero aún
no distingue una señal CPU, un trap del driver o una terminación GPU/sistema.
No se modifica Mesa ni se introduce una espera especulativa como corrección.

00.11 conserva el modo X fresco y habilita API trace y RADV_PS4_TRACE solo
con L2+X en PS4. Se usa el recorder existente del winsys, que escribe los IB
antes de entrar a GNM y sigue el progreso de finalización. El detalle API
se desactiva tras tres VI completos; el recorder sigue activo en el proceso.
La prueba persistente se limita a 30 segundos y luego queda pausada; la ruta
normal conserva 600 segundos. Es una prueba diagnóstica, no un benchmark ni
una reparación declarada: el tráfico de registro puede alterar el timing.
Se debe terminar la aplicación tras este ensayo antes de otras comparaciones.

Se amplían las señales capturadas a siete, incluyendo SIGTRAP (5) y SIGSYS
(12 en FreeBSD). Un trap de breakpoint se reanuda después de su instrucción;
por eso el manejador ahora vuelve a emitir la señal con su acción fatal ya
restaurada, en lugar de permitir que el programa continúe. El test ejecuta
SIGSEGV por acceso a 0x1 e int3 reales en hijos, comprueba ambas capturas y
terminaciones fatales. El marco PS4 del manejador es 1672 bytes (<4 KiB).
La captura sigue sin garantizar registros ante kills del kernel/GPU o pila
inaccesible. El nuevo archivo es vulkan-game-crash-00.11.log.

La comparación nativa con Lavapipe pasa antes del ajuste de presupuesto:
sin ASan/VUID, con cap y reanudación bloqueada; persisten 40 avisos UBSan del
core. No reproduce el contenido del cache RADV PS4. Se vuelve a comprobar
la cota persistente con el presupuesto de producción de 600 segundos y la
salida nativa a los 32 s. La versión solo cambia diagnóstico y presupuesto;
renderer, compilador y hashes de las bibliotecas permanecen iguales a 00.10.

Comprobación final del presupuesto 00.11: 1053 VI,
pausa a 30.0 s y reanudación bloqueada; el binario nativo usa
NPS64_GAME_TEST_SECONDS=600 y la ruta persistente informa wall_limit_s=30.000.
Sin ASan ni VUID; 40 avisos UBSan ajenos a Vulkan conservados. Pasan ELF,
constructores, extracción, icono, SFO y 28 checks PKG. Hash PKG:
`e097220fe8b1be17153dd566196521dc6a9637c45f1797db2f69f50ad8e544da` (31326208 bytes).

Precisión del marcador 00.10: submit begin precede a lock_queue y a la
llamada Vulkan. Todavía no distingue un fallo en el bloqueo de uno dentro
de vkQueueSubmit. El wrapper API de 00.11 permite separar esos puntos.

00.11 subida a /data/pkg en 192.168.1.14:2121 y descarga remota verificada
por SHA256. Se retiró solo el PKG 00.08; se conservan 00.10, CPU 00.69
y xuperPS4. No se modificaron cachés, ROMs, partidas ni logs remotos.
No hubo publicación en GitHub.


## 00.11 físico y alternativa 00.12: precarga de parámetros

El usuario informa el mismo cierre. La instantánea 20261112-221040 contiene
la captura de esta reproducción. El wrapper deja pendiente vkQueueSubmit
id=44; por tanto ya se entró a Vulkan después de adquirir el bloqueo de cola.
El recorder confirma finalización hasta el envío 5 y registra el 6 antes
de GNM, sin finalización registrada del 6. Sus IB incluyen cómputo interno
8x8 del driver durante la preparación/lectura de imagen; el shader de app
2016ea5dbb9f30ea había pasado su guard y se registra en el envío previo.
Esto no identifica una instrucción CPU/GPU culpable. Los archivos de señales
siguen vacíos y no hay gpu-fault.txt. Los IB de secuencias 727/728 del mismo
directorio son restos anteriores del anillo; solo 1–6 pertenecen al progress
actual. Se conservan los datos y la decodificación en flight.json.

00.12 evita completamente la recuperación de binarios RADV en la interfaz.
La causa interna del cierre no se declara reparada. Siempre crea un cache
Vulkan vacío en RAM y no lee ni reemplaza los archivos binarios antiguos.
L2 deja de activar la ruta problemática; todos los arranques normales usan
la alternativa siguiente, manteniendo el renderer Vulkan.

Se guardan únicamente hashes SPIR-V/pipeline, máscara de especializaciones
y hasta 12 valores uint32. Al volver a biblioteca se persisten por ROM en
/data/NPS64/pipeline-recipes/<ROM key>, con identidad GPU/driver/UUID, kind=3,
SHA256 y reemplazo atómico. El payload tiene formato NPS64RC1, máximo 1024
registros y rechazo de versión, tamaño, máscara, constantes inactivas y
duplicados inválidos. No contiene código máquina, punteros ni partidas.
El almacenamiento binario RADV previo (kind=1) permanece intacto y sin uso.

Después del primer readback completado, el frontend suspende la ejecución
de nuevos VI y recompila las variantes conocidas a partir del ShaderBank
actual. Se reconstruye y comprueba el hash de cada pipeline; versiones de
shader incompatibles se omiten. Cada creación conserva el worker de 8 MiB,
el guard de recursos y el registro en el mapa de pipelines del programa.
No se dibujan frames N64 adicionales durante la precarga. El puente es de
un solo uso por dispositivo, con limpieza al destruirlo y registro protegido
por mutex. Se informa la preparación en pantalla; el presupuesto de precarga
es de 20 segundos (más la compilación en curso). Las variantes nuevas todavía
pueden bloquear al encontrarlas. La primera sesión aprende, la siguiente
precarga lo aprendido. Se necesita salir a biblioteca para guardar.

El test del formato pasa ASan/UBSan: roundtrip, límites de cantidad/máscara,
constantes inactivas, duplicados, corrupción con contenedor válido, separación
de kind y ciclo de vida del callback. Los marcos SDK del módulo C son de
88 bytes en carga/guardado. La sesión nativa inicial de Majora alcanzó
694 VI y pausó a 30.007 s, con reanudación bloqueada, sin ASan ni VUID;
se conservan 40 avisos UBSan ajenos al código Vulkan.

Una comparación del mismo binario con SRAM y recetas iniciales restauradas
muestra 23 compilaciones durante ejecución sin precarga. Con precarga se
preparan 23 variantes: 21 compilaciones se adelantan y quedan 2 en el arranque,
con 557 hashes de imagen iguales. La precarga tardó
1261.082 ms en el host; no predice su duración en PS4.
El cache binario previo conserva contenido y mtime en ambas sesiones.
El test reproducible es tests/test_vulkan_recipes_session.py, con --data,
--binary y --output para una copia de prueba aislada previamente preparada
por scripts/test_vulkan_game_trace.sh. La ROM de prueba es local del usuario
y no se incluye. Pasan ELF, constructores, extracción, icono, SFO y 28 checks PKG.

PKG 00.12: `30ccfdb435a96b65a02e99e636951f1f13d023d57fb3a15c66d51ef305fdd136` (31326208 bytes).
Validación física pendiente: abrir Zelda con X normal, jugar la intro o una
zona durante aproximadamente un minuto, volver a biblioteca y abrir de nuevo
con X. Comparar la misma cámara/pantalla. El segundo arranque puede tardar más
por la precarga; no debe confundirse esa preparación con un tirón en juego.
Comprobar init/save de recipes, prepared/skipped, compilaciones posteriores
y percepción del usuario. No se afirma que todos los tirones estén resueltos.
No hay publicación, tag ni push a GitHub.

00.12 subida a /data/pkg en 192.168.1.14:2121 y descarga remota verificada
por SHA256. Listado final: NPS64-Vulkan-Game-Test-00.12.pkg y el paquete
xuperPS4-01.20-console.pkg, que no se modificó. El script solo contempla
retirar 00.11 si existe; no elimina otros PKGs. No se alteraron ROMs,
partidas, cachés o logs remotos.


## 00.13: render híbrido para el primer recorrido (2026-11-13)

El usuario confirma que 00.12 funciona, pero precisa que la precarga de una
segunda sesión no cumple su objetivo: evitar tirones en tiempo real desde
el primer recorrido sin una carga larga. Se cambia el enfoque a una variante
general del rasterizador split en GPU y especializaciones compiladas en CPU
en segundo plano. No se adivinan variantes futuras ni se omiten dibujos.
Es el principio descrito por Dolphin en su modo híbrido:
https://dolphin-emu.org/blog/2017/07/30/ubershaders/ . No se atribuye esta
implementación concreta a emuladores de Switch sin verificación adicional.

El impedimento anterior era el shader general real 386b345ad2a2a7dd, con
112 SGPR. En compilación host RADV/BONAIRE, ACO_DEBUG=nosched reduce ese shader
a 88 SGPR/64 VGPR, con cero scratch y spills. No se relaja el guard. Desactivar
solo ILP no reduce los 112 SGPR. VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT
produce 262144 bytes de scratch y es rechazado: esa alternativa no se usa.
NOSCHED conserva las otras optimizaciones y las pasadas de waitcnt/NOP,
pero puede cambiar el rendimiento de todos los shaders; la medición física
es necesaria. En Mesa 26.0.8 el flag afecta codegen y hace que RADV desactive
su cache de binarios; la reutilización de pipelines vivos de Granite se mantiene.

Al recibir el ShaderBank se prepara la variante general conocida del renderer
(una para escala normal, otra si se habilita escalado). No depende de ROMs,
recetas previas ni código máquina guardado. Se reconstruye y comprueba el hash
usando el helper existente; se crea con el mismo guard y pila de compilador de
8 MiB. Si el guard rechaza la creación inicial, la app rehúsa iniciar el juego.
El ubershader combinado que bloqueó la consola permanece desactivado.

En cada estado nuevo de rasterizado se consulta el mapa de pipelines vivos.
Si falta la variante especializada, se encola una única compilación en el
worker original y se dibuja con el general ya preparado. No se crean ni prueban
pipelines especializados desde ese recorrido del hilo de render. El trabajo
completa la variante y el mapa permite usarla posteriormente. El log del nodo
se escribe antes de transferir su propiedad al worker. Los demás stages del
renderer conservan su creación habitual: esta revisión no promete eliminar
toda compilación de primer uso fuera del rasterizador. El shader general
puede costar más GPU temporalmente; NOSCHED global también necesita comparar FPS.

Se desactivan carga/guardado/precarga de recetas por juego y recuperación del
cache binario en todas las aperturas. Los archivos existentes se preservan.
No se necesita una segunda carga. Se mantienen tres buffers, readback cached,
partidas aisladas, logs compactos, cap de 600 s y captura fatal. El código de
recetas permanece disponible para los tests históricos, pero la interfaz no
habilita ese modo. No cambia la lógica de audio ni se corrige aquí Tennis VC.

Pruebas: los 24 pipelines de la captura asíncrona anterior y los 24 de la
captura real 00.13 compilan y pasan el guard en BONAIRE virtual con NOSCHED:
máximos 88 SGPR/64 VGPR, scratch=0, spills=0 y LDS=0. Esto inspecciona código
compilado, no ejecución en la PS4. El guard físico sigue siendo obligatorio.
Majora nativo con Lavapipe arranca en un directorio sin historial de shaders,
prepara el general antes del juego y encola 15 especializaciones. Completa
503 VI, pausa a 30.062 s y bloquea reanudación; no aparecen ASan ni VUID.
Permanecen 39 avisos UBSan ajenos al código Vulkan. Los 503 hashes de imagen
comunes coinciden con 00.12 síncrona usando SRAM idéntica. Las bibliotecas
Mesa PS4 permanecen iguales; solo se selecciona su opción de compilación.
Se evita reescribir headers idénticos al aplicar parches para no recompilar
todo el core innecesariamente. Pasan ELF, constructores, extracción, icono,
SFO y 28 checks PKG.

PKG 00.13: `f03cb3448ee50ab01b36ee9432f8348fa59de1e3e8836224ae81303636c694f8` (31326208 bytes).
Prueba física pendiente: abrir con X y probar directamente las cámaras/zonas
que provocaban caídas, sin recorrido previo ni segunda carga. Confirmar el
general listo y sus recursos físicos, trabajos encolados, tiempos de frame,
VI/s y underruns. La prueba software no mide rendimiento de Liverpool ni
valida su ejecución GNM. No hay publicación en GitHub; 00.12 se conserva como
referencia estable hasta contar con resultados de esta variante.

00.13 subida a /data/pkg en 192.168.1.14:2121 y descarga remota verificada
por SHA256. Se conserva 00.12 junto a 00.13; xuperPS4-01.20 no se modificó.
No se alteraron cachés, recetas, ROMs, partidas ni logs remotos.
No se publicó ningún release ni se hizo push a GitHub.


## Confirmación del usuario y checkpoint local 00.13 (2026-11-13)

El usuario confirma que todos los juegos funcionan mucho mejor y que ya no
percibe los tirones anteriores. GoldenEye presenta ocasionalmente unos 55 FPS
frente a su objetivo de 60. Esa observación no demuestra por sí sola que la
causa sea mayor exigencia gráfica; queda pendiente medir los intervalos.
Esta confirmación es una prueba física reportada por el usuario, no un
benchmark controlado ni una sesión de 600 segundos verificada en logs.

Se solicita guardar este estado en un commit local sin push, antes de tocar
texturas, antialiasing y calidad de audio. Esas mejoras siguen pendientes.
No se cambia el renderer ni se genera un nuevo PKG para este checkpoint.
