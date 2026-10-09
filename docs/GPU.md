# Ruta GPU de NPS64: suspendida en PS4 desde 00.13

**Estado actual:** CE-36329-3 y congelamiento confirmados por el usuario. El PKG
normal 00.13 excluye Vulkan/GNM y los builds PS4 tambien bloquean su inicializacion.
Las instrucciones de GPU de revisiones anteriores se conservan como historial.

La CPU emulada continúa en Ari64 x86_64. El RSP HLE produce comandos RDP;
paraLLEl-RDP ejecuta la rasterización y el VI con shaders Vulkan. Se reutiliza
Mesa 26.0.8, ACO y el winsys GNM del commit fijado de DolphinPS4. GNM envía
los command buffers a la GPU de PS4; no se empaquetan módulos propietarios.
El driver se enlaza estáticamente, sin loader Vulkan externo ni Piglet.

El frontend negocia el dispositivo mediante libretro Vulkan, entrega una cola
con exclusión mutua y mantiene un frame en vuelo. Copia la imagen RGBA nativa
a un staging buffer, espera su fence, invalida memoria no coherente cuando
corresponde y convierte los canales al canvas XRGB de VideoOut. Los estados
y dos aperturas independientes se prueban con una ROM original de diagnóstico.
La rasterización es GPU; lectura, escalado y HUD aún utilizan CPU. El siguiente
paso de rendimiento es un compositor GPU directo sobre los buffers VideoOut.

Se conserva resolución interna 1x y se fuerza RDP síncrono para la coherencia de RDRAM. El modo de imagen en pausa controla los
filtros VI AA/bilineal; el modo preciso mantiene los filtros del núcleo.
Los tiempos RDP/VI del log siguen midiendo Angrylion: en Vulkan son cero y
no representan duración de shaders. `core_ms` incluye esperas GPU y transferencia.
Los FPS muestran VI/s frente al objetivo regional de la ROM (PAL ~50, NTSC ~60),
no el número de animaciones únicas por segundo que produce cada juego.

## Compilación reproducible dentro del proyecto

Ejecutar `scripts/build_gpu.ps1` desde la raíz. Usa Docker Linux con clang/LLD
21, libc++/libc++abi 21.1.6 adaptados a musl/FreeBSD de OpenOrbis, glslang 15.1.0,
Mesa 26.0.8 y headers libdrm 2.4.125. Todos los árboles y resultados quedan bajo
`vendor/gpu`, `build/gpu`, `build/core-gpu` y `out`. Los hashes de las fuentes
están en `sources.lock.json`. Los scripts y parches de Dolphin se copian y se
normalizan a LF; el checkout de referencia se conserva.

Cambios específicos sobre el driver de Dolphin: rutas de diagnóstico bajo
`/data/NPS64`, mensajes enviados al descriptor propio del log, alternativa
TLS para qsort, compresión zlib desactivada al no existir libz PS4 en este SDK.
El enlace incluye archivos completos para las funciones weak del dispatch RADV,
sin duplicar objetos runtime/WSI ya incluidos en libvulkan_radeon.a. Se reutilizan
PAID/authinfo públicos del builder GPU de Dolphin para el SELF.

## Qué comprobar en PS4

La secuencia GPU siguiente es historica. Instalar `out/NPS64-00.13.pkg`, que usa CPU; no repetir las pruebas GPU de 00.07-00.12 en consola.
El HUD debe indicar `GPU Vulkan`. En `/data/NPS64/nps64.log` deben aparecer:

```text
[gpu] Vulkan instance ready
[gpu] device=... type=... queue=...
[gpu] renderer=paraLLEl-RDP active
[gpu] first scanout ... fence completed
[load] first_visible_video ...
```

El driver añade líneas `[radv]`. En fallos GPU puede crear `gpu-hang.txt` y
`gpu-fault.txt` dentro de `/data/NPS64`. Conservar también el código CE si la
consola cierra la aplicación. La prueba Linux usa Lavapipe (Vulkan por software):
comprueba API, shaders, colores y ciclo de vida, pero no ejecución en hardware AMD.
No hay PS4 conectada a este entorno y no se han medido FPS de esta versión en PS4.

Si falla la creación de instancia/dispositivo o la comprobación del renderer,
se recarga la ROM con Angrylion. Un fallo durante un frame devuelve a la biblioteca
cuando el driver permite recuperar el control; un fault del kernel GPU puede
cerrar la app antes de que el frontend intervenga.

Para forzar Angrylion, crear `/data/NPS64/diagnostics.cfg` con:

```text
NPS64DIAG5 4 1 1 1 0
```

Campos: workers CPU, RSP HLE, Ari64 solicitado, VI preciso, Vulkan solicitado.
Eliminar el archivo o poner la última cifra en `1` restaura Vulkan. No cambia
las rutas SRAM. Respaldar los estados anteriores antes de usarlos entre renderers;
las pruebas de estados del nuevo renderer son de host, falta confirmar PS4.

## Revisión 00.08: cierre CE-34878-0 al abrir 00.07

El código CE por sí solo no identifica la causa. Se comprobó que 00.07
contenía tres constructores GPU automáticos antes de main y conservaba Sony
mspace. Dolphin sustituye ese allocator por dlmalloc y evita reservar el heap
en el rango utilizado por sus objetos kernel/pthread. 00.08 incorpora esas
correcciones y aplaza los constructores GPU hasta abrir la primera ROM.

`boot.log` se escribe directamente con libkernel, sin malloc ni stdio:
pre-main, entrada en main, inicialización del heap, plataforma y cada constructor
GPU. Si vuelve a cerrar, recoger `/data/NPS64/boot.log` y `nps64.log`.
El test host con ASan/UBSan verifica 16.384 asignaciones/realocaciones, alineación,
overflow, denegación de reserva y dirección/flags del mapping simulado.
El ELF se verifica para que sólo el trace se ejecute antes de main y para que
no enlace Sony mspace. La resolución de CE-34878-0 requiere prueba PS4 real.


## 00.09: cierre dentro de platform_init

El log de la consola en 00.08 termina en `frontend log/config ready; platform
init begin`. Los constructores GPU todavia no se ejecutan en ese punto.
Se agrega trazado directo y sincronizado del reloj, calloc del canvas, UserService,
Pad, VideoOut, cola de flips, reserva/mapeo/limpieza del scanout, fuente y audio.
Los resultados se escriben en hexadecimal sin asignaciones ni stdio.
La memoria de video usa una reserva virtual alta y MAP_FIXED; se abandona la
inicializacion si no se puede reservar. Los atributos de VideoOut se ponen a cero.

Pruebas locales: backend PS4 simulado con ASan/UBSan, reloj TSC y alternativo,
flips, mandos, copia de video y fallo de reserva controlado; heap con calloc de
720p y reutilizacion ademas de las 16384 asignaciones/reallocaciones existentes.
Ejecutar `bash scripts/test_gpu_platform.sh` y `bash scripts/test_gpu_heap.sh`
dentro del contenedor. Estas pruebas no reproducen los servicios de una PS4 real.

Pendiente: instalar `out/NPS64-00.09.pkg`, comprobar que llega a la biblioteca
y despues cargar una ROM. Si sigue cerrandose, recoger el bloque completo que
empieza en `00.09 before main` de `/data/NPS64/boot.log`. No atribuir el fallo
actual a Vulkan ni declarar resuelto CE-34878-0 antes de comprobarlo en consola.


## 00.10: cargar los modulos del sistema antes de usarlos

El log real de 00.09 alcanza `platform: UserService initialize begin` y se
cierra sin devolver resultado. La GPU sigue aplazada. Se encontro una omision:
`LoadSystemModules()` en `reference/DolphinPS4/patches/dolphin-ps4.patch` carga
SystemService, UserService, VideoOut, AudioOut y Pad. El probe GNM de Dolphin
explica expresamente el cierre por PRX_NOT_RESOLVED_FUNCTION al invocar
`sceUserServiceInitialize` sin cargar su modulo.

00.10 enlaza libSceSysmodule, comprueba los modulos ya cargados y carga los
restantes antes de acceder a esas funciones. Registra resultados en boot.log y
nps64.log. Si una carga falla, no invoca los imports no resueltos.
Corrige tambien el identificador del log principal, que aun indicaba 00.08.

Pruebas reproducibles: `scripts/test_gpu_platform.sh` con ASan/UBSan exige que
los cinco modulos esten cargados antes de UserService; cubre modulos ya cargados,
falta de UserService sin invocarlo, reserva de video denegada, TSC y reloj kernel,
mandos, presentacion y cierre. Validacion del PKG y ELF sigue obligatoria.

Pendiente en PS4: instalar `out/NPS64-00.10.pkg`; confirmar biblioteca visible,
mandos y audio; despues cargar Mario Kart y comprobar renderer/FPS. Si falla,
recoger el bloque `00.10 before main` completo de `/data/NPS64/boot.log` junto
con `/data/NPS64/nps64.log`. Las pruebas del PC no certifican los servicios PS4.


## 00.11: pantalla negra despues de activar GPU Vulkan

Confirmado por el usuario: 00.10 inicia correctamente; carga los cinco modulos,
abre mandos/video/audio y negocia un dispositivo Vulkan AMD Liverpool.
El reporte de Mario Kart muestra envios 1-36 completados, scanout inicial 1x1,
y bloqueo del envio 37; la GPU queda en 36 con 39 enviados. El stream 37
contiene clear-indirect, binning, rasterizer con DISPATCH_INDIRECT, resolve y VI.
El reporte no incluye marcadores que permitan saber en cual shader se detiene;
no atribuir el bloqueo exclusivamente al paquete indirecto ni al compilador.

Cambio: usar PARALLEL_RDP_UBERSHADER=1, PARALLEL_RDP_SUBGROUP=0,
PARALLEL_RDP_SMALL_TYPES=0 y PARALLEL_RDP_FORCE_SYNC_SHADER=1 en PS4.
El ubershader rasteriza y mezcla en GPU con dispatch directo, evitando la lista
de rasterizadores especializados y los contadores de despacho indirecto.
Los enteros de 32 bits ya eran el valor por defecto para RADV; se hacen explicitos.
El mismo perfil se activa en los tests y benchmark de PC mediante
NPS64_TEST_PS4_GPU_PROFILE. Ari64 y la ruta de copia de scanout se conservan.
Este es un perfil de compatibilidad, no una optimizacion de FPS demostrada.

Pruebas con VK_LAYER_KHRONOS_validation: ROM sintetica con pixeles esperados,
SRAM/estado, dos sesiones GPU y retorno a biblioteca; driver Vulkan ausente y
dispositivo no apto vuelven a Angrylion. Sin errores de validacion Vulkan.
Mario Kart local: 360 frames, imagen visible desde frame 53, scanout 640x288,
capturas frame 300/330 y cierre limpio. Lavapipe es software: esos tiempos no
predicen rendimiento ni comportamiento del driver RADV/GNM en PS4.

Pendiente: instalar `out/NPS64-00.11.pkg`, comprobar imagen despues de compilar
los shaders iniciales; probar carrera y FPS. Confirmar en el log el perfil de
GPU, `Overriding ubershader = 1`, `Allow subgroups = 0`, geometria mayor a 1x1
y `first_visible_video`. Si persiste: recoger nps64.log y gpu-hang.txt nuevos.
Volver a habilitar rasterizadores especializados/subgrupos solo despues de
validar cada cambio en consola. El fallo al importar VK_EXT_external_memory_host
usa la alternativa de copia y no demuestra la causa del hang. La advertencia
SRAM ilegible se conserva: no sobrescribir ni eliminar las partidas protegidas.


## 00.12: el despacho directo tambien bloquea la GPU

El reporte de consola de 00.11 confirma el perfil nuevo (ubershader=1,
subgroup=0) pero vuelve a detenerse en 37, completado 36/enviado 39. El stream
ahora contiene tres DISPATCH_DIRECT y no DISPATCH_INDIRECT. No hay solapamiento
fisico entre los 47 BO. Esto descarta el despacho indirecto como causa unica;
no hay marcadores que permitan saber cual de los tres shaders se detiene.

Cambios del driver, aplicados reproduciblemente por scripts/patch_gpu_driver.py:
- SFENCE con barrera de compilador justo antes de dev->submit: publica las
  escrituras WC de CPU, incluidas las subidas al BO de codigo de shader.
- Desactivar radv_emit_shader_prefetch solo bajo __PS4__: evita el DMA_DATA de
  copia del codigo sobre la misma direccion; no elimina despachos ni shaders.
- Activar RADV_PS4_TRACE=1 en el perfil: usa los marcadores ligeros ya existentes
  de Dolphin para el flight recorder y gpu-hang.txt. No activa RADV_DEBUG=hang
  ni su sincronizacion costosa de cada shader. El nombre de version es 00.12.

Validacion local: Mesa RADV/GNM recompilado, parches idempotentes comprobados,
barrera antes del handoff verificada, GPU frontend con validacion Vulkan,
pixeles de ROM sintetica, dos sesiones, estados y alternativas Angrylion.
El benchmark de Mario Kart de 00.11 valida el perfil directo en Lavapipe;
NO valida estos cambios de GNM ni elimina la necesidad de probar hardware.
El empaquetado de fuentes sustituye en la cache los dos archivos Mesa editados
para que correspondan a esta compilacion y no incluya codigo antiguo del driver.

Pendiente en PS4: instalar out/NPS64-00.12.pkg y abrir Mario Kart. Si se bloquea,
recoger los archivos nuevos nps64.log y gpu-hang.txt completos. Buscar WRITE_DATA,
valores memory/done/NOT DONE y trace ids en execution check para distinguir
preparacion, binning y rasterizacion. No prometer recuperacion a CPU despues de
un GPU hang: se mantienen las esperas originales, pues liberar BO aun usados
por la GPU sin un reset confirmado puede causar corrupcion. Confirmar imagen,
carrera y FPS antes de declarar esta incidencia resuelta.


## 00.13: restaurar CPU tras congelamiento de consola CE-36329-3

00.12 sigue bloqueando y el usuario informa congelamiento de PS4/CE-36329-3.
El marcador compartido llega a 3; las escrituras 1/2 figuran NOT DONE porque
el mismo destino contiene el ultimo valor, 3. Eso NO prueba que no ocurrieran.
El siguiente marcador, 4, esta DESPUES de CS_PARTIAL_FLUSH/PFP_SYNC_ME y no se
alcanza. La GPU se detiene esperando trabajo anterior al tercer despacho:
span_setup (64 hilos) o tile_binning_combined (32 hilos, LDS/barriers). Los
WRITE_DATA del ME no garantizan finalizacion de compute. No culpar todavia al
ubershader, que queda despues de la barrera sin evidencia de que haya comenzado.

Entrega principal: out/NPS64-00.13.pkg, software sin Mesa/Vulkan/GNM, con
Ari64, RSP HLE, cuatro trabajadores Angrylion, canvas rapido y HUD permanente.
Mismo title/content id; conserva ROMs y partidas. La configuracion grafica vieja
no se borra, pero requested_gpu se fuerza a cero en PS4 despues de leerla.
Ademas nps64_gpu_begin devuelve false antes de constructores/loader/dispositivo
en PS4. Ningun PKG normal de esta revision puede activar la ruta GPU defectuosa.
El script GPU genera un paquete separado `00.13-gpu-disabled`, no una nueva
prueba de hardware. Las fuentes y los parches de investigacion se conservan.

Test scripts/test_gpu_quarantine.sh: configura GPU=1 tanto en diagnostics.cfg
como en BENCH_GPU y sustituye nps64_gpu_begin por abort. El frontend no lo
invoca; dos sesiones Angrylion producen imagen y guardan/cargan estados.
La comprobacion final del ELF principal exige ausencia de RADV/GNM/Vulkan.

Pendientes para retomar GPU sin usar otra ROM como primer experimento:
- Aislar span_setup y tile_binning, diseminar/disensamblar sus shaders ACO gfx7.
- Preparar reproduccion minima de compute con grupos 64 y 32, LDS, atomicas,
  barreras y descriptores; verificar los datos emitidos y registros Liverpool.
- Investigar mapa/coherencia de BO y diferencias del contexto GNM respecto a
  Linux RADV; confirmar antes de culpar al paquete indirecto, precarga o shaders.
- No tratar Lavapipe como sustituto de validacion de hardware. Rehabilitar GPU
  solo cuando exista evidencia del arreglo y un protocolo que no repita el
  bloqueo global. No prometer recuperacion de un GPU hang mediante timeout CPU.
- Validar en PS4 que 00.13 vuelve a mostrar el juego y conserva controles/FPS.
La mejora solicitada de rendimiento GPU permanece sin resolver.
