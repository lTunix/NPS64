# Validación de NPS64 00.06

## HUD y presentación — 00.06

Informe del usuario: 00.05 mejoró algo en PS4, pero continúa lento y los FPS
se ocultan. 00.06 mantiene HUD de FPS/objetivo/velocidad/CPU/workers durante
toda la partida. Los trabajadores mostrados son los creados realmente.
Se conservan contadores separados al cambiar entre biblioteca, pausa y juego.
El log registra RDP/VI, que forman parte del tiempo del núcleo.

La presentación deja un flip pendiente mientras emula el siguiente frame;
lo espera antes de reutilizar scanout y antes de apagar. Se conserva doble
buffer, canvas en RAM caché y sfence. El callback retiene video nativo y
escala directamente a canvas, sin otro buffer/copias de 720p. El reloj TSC
evita llamadas repetidas al kernel; tiene fallback para frecuencia inválida.
Imagen precisa/rápida se selecciona y conserva desde pausa.

Pasaron backend PS4 simulado con flips acotados y 60 frames de 10/20 ms,
reloj TSC/fallback, input, almacenamiento, escalado y núcleo real. Pasó
guardar/cargar y dos sesiones con cambio de filtro durante pausa, CPU cached
y Ari64, síncrono y cuatro trabajadores. El test de HUD espera 200 frames
en biblioteca y comprueba FPS, objetivo y velocidad durante una partida
sostenida; otro exige HUD en ambas sesiones después de haber aparecido video.

Mario Kart completó título/menús/carrera con cuatro trabajadores preciso,
dos/cuatro/seis rápido y una variante -O3 descartada. El renderer preciso
coincide píxel por píxel con 00.05 fuera del HUD. Resultados en PERFORMANCE.md.
El host no muestra una mejora consistente del núcleo entre esas variantes:
se conservan -O2 y cuatro trabajadores. No hay prueba de FPS de PS4 00.06.

PKG: out/NPS64-00.06.pkg, 6 619 136 bytes.
SHA-256: 7f27070ba0039991912430a814e12d8de05692ad35b90a51bd1f418fe67e4398.
Pasó ELF/OELF, 28 checks de PKG y comparación del payload extraído con stage.
00.05 se conserva con su nombre original y en out/history. La ROM comercial
se usa sólo localmente; no está en PKG ni fuentes.

## JIT y renderizado paralelo — 00.05

El usuario confirmó 00.04 en PS4: interfaz rápida y Mario Kart ejecutándose,
con una estimación de 15 FPS o menos. La versión 00.05 incorpora Ari64 x64,
NASM, regeneración de offsets, cache en BSS y sceKernelMprotect con páginas
de 16 KiB. Selección de CPU condicionada al permiso ejecutable y fallback
al intérprete. Perfil predeterminado: Ari64, RSP HLE, cuatro trabajadores.

Pasó scripts/test.sh: entrada, almacenamiento, backend PS4 simulado,
escalado y núcleo real. Las nuevas pruebas verifican permisos/alineación/
overflow bajo ASan/UBSan y guardar/cargar estados y dos sesiones con CPU
intérprete, JIT síncrono y JIT con cuatro trabajadores.
Mario Kart completó intro, menús y carrera en los tres perfiles: las capturas
del frame 3570 son idénticas píxel por píxel. El tiempo de carrera baja de
13,948 a 4,966 ms en Docker; detalles y límites en PERFORMANCE.md.

El PKG 00.05 pasó checks ELF/OELF, 28 checks de paquete y comparación del
payload extraído con stage. Usa el mismo TitleID y módulos públicos de
OpenOrbis. La ROM comercial no está incluida en el paquete ni las fuentes.
Entregable: out/NPS64-00.05.pkg, 6 619 136 bytes.
SHA-256: 4de025047ee3432209eea0ea3ca00a071cf6c4d3be10ffd1cddbc8215f9553db.
00.04 se conserva en out/history y también con su nombre original.
La memoria ejecutable, los cuatro trabajadores y los FPS de 00.05 todavía
requieren prueba en PS4 real. No se considera el benchmark Linux prueba
del hardware, ni cobertura completa de todas las instrucciones N64.

## Correcciones de rendimiento, log y ROM real — 00.04

Informe del usuario de 00.03: arranca y carga la ROM sin cerrar, pero el menú
tarda alrededor de un segundo en responder, el juego queda negro y no se
escribe el log. Se reproduce una fase negra del arranque de la ROM local en
Linux; una prueba más larga alcanza título y carrera, por lo que una fase
negra aislada no demuestra un bloqueo permanente del núcleo.

00.04 añade canvas en memoria CPU con caché y copia al scanout; reemplaza
stderr/freopen por archivo de log independiente; usa reloj/sleep de kernel;
inicia flipArg en 1; reduce divisiones y reutiliza filas del escalado.
Se añade perfil periódico y telemetría visible para verificar la consola.
RSP HLE pasa a ser predeterminado tras probar título, menús y carrera; se
conserva cxd4 seleccionable en diagnostics.cfg para comparar compatibilidad.

Pasaron pruebas ScePad; almacenamiento ASan/UBSan; backend PS4 con servicios
simulados y logger de archivo; escalado en tres formatos con padding y cambio
de geometría; integración real con ROM propia, estados y dos arranques en
ambos perfiles de trabajadores. Las pruebas nativas usan ahora memoria
comprimida como PS4. El benchmark del juego usa la ROM entregada por el
usuario y partidas de prueba separadas en build/benchdata.

Resultados, capturas verificadas y límites de medición: docs/PERFORMANCE.md.
El resultado del host no demuestra la latencia ni FPS del hardware PS4.
Posteriormente el usuario confirmó interfaz rápida y juego arrancando en 00.04.
El build PS4 final pasó ELF/OELF y las 28 comprobaciones del PKG; se extrajo
y comparó el payload con build/stage, incluyendo los módulos OpenOrbis.
Entregable: out/NPS64-00.04.pkg, 6 619 136 bytes; hash en out/SHA256SUMS.
El paquete 00.03 se conserva en out/history/NPS64-00.03.pkg.

## Prueba de consola de 00.02 y cambios de diagnóstico de 00.03

El usuario confirmó que 00.02 arranca y detecta juegos en la biblioteca.
Al intentar cargar Mario Kart 64 tarda y se cierra con CE-34878-0.
Informó posible confusión del mapeo: Cuadrado muestra «Biblioteca actualizada»;
el código corresponde a esa acción. No se cambian arbitrariamente los bits
de X/Círculo/Cuadrado: coinciden con los valores ScePad del SDK.

00.03 añade una prueba de mando con Triángulo y registra botones recibidos.
Círculo vuelve; salir desde biblioteca pasa a Options. La búsqueda informa
el número de ROMs. La carga muestra un aviso antes de comenzar y escribe
marcas de hash, retro_init, retro_load_game, AV y primeros frames/video.
Se registra el tamaño/origen del heap. El buffer de hash en stack baja de
64 KiB a 4 KiB. Angrylion se configura síncrono por defecto como medida de
aislamiento, sin afirmar que sus trabajadores causaran el cierre.
diagnostics.cfg permite comparar el renderer síncrono y cuatro trabajadores.

Pruebas Linux: mapeo con bits físicos ScePad, ejes, gatillo y desconexión;
almacenamiento ASan/UBSan con hash de ROM de varios bloques; integración
con núcleo real, prueba del mando sin lanzar ROM por accidente, búsqueda,
dos arranques, video y guardar/cargar estado. Todas pasaron.
Se prueban por separado ambos perfiles gráficos. El modo CPU puro no se
ofrece en esta entrega: su prueba de integración adicional no completó
la ejecución y su adaptación queda pendiente.

No se ha reproducido el cierre de Mario Kart en este entorno: no hay consola
conectada ni una ROM de ese juego en el workspace. Falta el log del usuario.
La versión 00.03 requiere prueba en PS4; ver docs/PS4-DIAGNOSTICS.md.
La cross-compilación y conversión ELF/OELF pasaron. El PKG 00.03 se valida
con 28 comprobaciones y extracción/comparación de todo el payload, incluidos
los módulos públicos. Los perfiles nativos verificados tienen logs en
build/native-profile-0.log y build/native-profile-4.log.

## Corrección tras la prueba en consola

El usuario confirmó que 00.01 se instala, pero al iniciar aparece el mensaje
de que no se puede continuar usando la aplicación porque sus datos están dañados.
Código CE, firmware y versión del loader todavía no se han proporcionado.

La comparación con reference/DolphinPS4/scripts/make-pkg.sh detectó una
omisión en scripts/package.sh: faltaban los módulos públicos OpenOrbis
sce_module/libc.prx, sce_module/libSceFios2.prx y sce_sys/about/right.sprx.
00.02 los incorpora desde el SDK público v0.5.4 y actualiza APP_VER/VERSION
a 00.02 manteniendo TITLE_ID y CONTENT_ID.

Se recompilaron frontend y ejecutable PS4 y se generó el paquete 00.02.
Las 28 comprobaciones PKG pasaron; los ocho archivos de payload, incluidos
los tres módulos, se compararon byte por byte después de extraer el PFS.
El verificador nuevo rechazó correctamente el PKG 00.01 extraído con los
tres módulos ausentes y aceptó 00.02. Log: build/pkg-regression.log.

Entregable nuevo: out/NPS64-00.02.pkg (también con nombre CONTENT_ID).
El original se conserva en out/history/NPS64-00.01.pkg.
Hash de la nueva entrega: out/SHA256SUMS.
Fuentes correspondientes: out/NPS64-00.02-source.tar.gz.

La omisión está corregida y el usuario confirmó posteriormente el arranque
de 00.02. La carga de juegos comerciales sigue fallando, según el informe anterior.

## Validación previa de 00.01

Entorno: Windows, Docker Desktop con contenedores Linux, workspace NPS64.
Fecha: 5 de octubre de 2026. Toolchain público OpenOrbis v0.5.4,
clang/LLVM 14 de Ubuntu 22.04. Núcleo y referencia fijados en sources.lock.json.

## Comprobado hasta ahora

- Pruebas de almacenamiento con AddressSanitizer y UndefinedBehaviorSanitizer:
  lectura de tres órdenes de bytes, ROM inválida, escaneo limitado, escritura
  atómica, tamaño exacto, identidad de ROM y rechazo de estados corruptos/de otra ROM.
- Prueba de integración nativa con el núcleo real y ROM original de diagnóstico:
  lectura, ejecución R4300, imagen turquesa Angrylion, pausa, guardar/cargar estado,
  retorno a biblioteca, segundo arranque con imagen, persistencia SRAM y salida.
  No utiliza juegos comerciales. El unload/reload también libera el core y el intérprete.
- Cross-compilación y enlace de frontend/core/runtime para PS4.
- Conversión a eboot.bin; ELF/OELF x86_64 con segmentos de carga de 16 KiB,
  sin duplicación ni solapamiento.

## Paquete 00.01 original

`out/IV0000-NPSX00064_00-NPS64PS400000000.pkg`: 6 619 136 bytes.
Se completó la entrada PowerShell scripts/build.ps1 y se repitió la compilación
tras los cambios de ciclo de vida e índice de paks.

PkgTool.Core completó 28 comprobaciones de tamaños, hashes y firmas del fake
PKG. Se extrajo el contenido PFS y se compararon eboot.bin, font.bin y
las tres licencias/atribuciones byte por byte con build/stage.
Se confirmó presencia de las entradas PARAM_SFO e ICON0_PNG.
Los registros están en build/pkg-validation.log, pkg-entries.log y pkg-extraction.log.

SHA-256 del binario entregado: consultar `out/SHA256SUMS`, generado por el build.
El proceso es repetible; los timestamps del paquete pueden cambiar el hash
entre compilaciones aunque no se modifique el código.

Se creó `out/NPS64-00.01-source.tar.gz`, se extrajo en `build/rebuild-check`
y se recompiló desde esas fuentes con el mismo SDK local montado dentro
del workspace. Esa reconstrucción volvió a producir ELF/OELF válidos y
un PKG con las 28 comprobaciones correctas y payload verificado.
No se exige identidad binaria entre builds incrementales y limpios:
el orden de miembros del archivo estático también puede cambiar el layout.

ROM de diagnóstico original para probar la consola:
`out/NPS64-test.z64` (4096 bytes), copia del archivo generado por el test.

Quedan warnings de macros redefinidas del upstream; no hay errores de build.
La advertencia de escritura fuera de límites de l_pak_type_idx se corrigió.

## No probado

No hay una PS4 conectada a esta sesión. La instalación de 00.01 fue confirmada
por el usuario; su arranque falló. El arranque de 00.02 está confirmado por
el usuario; también confirmó interfaz rápida y Mario Kart en 00.04.
Falta validar JIT y cuatro trabajadores de 00.05, otros firmwares y juegos,
y medir rendimiento de consola con la telemetría del log.
El test nativo reemplaza la plataforma PS4 por un backend simulado;
un resultado positivo en Linux no valida esas APIs de consola.

La ROM propia ejercita el arranque CPU y scanout de framebuffer por VI;
no es una prueba de todo el pipeline RDP, audio/RSP o microcódigos de juegos.
JIT forma parte de 00.05. Vulkan se integra en 00.07 y falta probar el hardware PS4.

## 00.07 — Vulkan / paraLLEl-RDP

- Mesa 26.0.8 + ACO/RADV/GNM y libc++/libc++abi 21.1.6 compilados para PS4.
- Núcleo con Ari64, GPU y alternativa Angrylion enlazados en el mismo SELF.
- ELF y OELF x86_64, alineación 16 KiB, segmentos de carga sin solapamiento.
- PKG generado, 28 comprobaciones correctas y payload extraído idéntico al stage.
- Suite software anterior pasa: input real, VideoOut simulado, reloj, SRAM,
  estados, intérprete/JIT, cuatro workers y HUD FPS sostenido.
- ROM original de diagnóstico en Vulkan/Lavapipe: colores RGBA correctos,
  estados guardados/cargados y dos sesiones independientes.
- Driver Vulkan inexistente: retorno a Angrylion sin perder funcionalidad.
- Renderer rechazado después de crear dispositivo/contexto: libera la ruta GPU,
  recarga con Angrylion y funciona en las dos sesiones.
- VK_LAYER_KHRONOS_validation no reporta VUID ni errores en estas pruebas.
- Mario Kart PAL local llega a carrera por Vulkan en una ejecución de 2400 VI;
  la ROM se mantiene fuera del PKG y del archivo de fuentes.

Lavapipe ejecuta Vulkan por software en el PC. Estas pruebas no verifican GNM,
coherencia real de la memoria AMD, instalación, arranque ni FPS de 00.07 en PS4.
La confirmación del usuario corresponde a 00.06, que funciona mucho mejor.

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


Resultados locales 00.13: test_gpu_quarantine pasa con GPU solicitado y abort
si se intentara inicializar Vulkan; test_jit pasa en cached/Ari64 y trabajadores
0/4, SRAM/estado, dos sesiones y HUD sostenido. Mario Kart PAL con Ari64/HLE y
cuatro trabajadores: 360 cuadros, imagen real y cierre correcto en PC. Estos
resultados no son mediciones de FPS de PS4. ELF de entrega comprobado: Ari64
presente, sin radv_*, ac_drm_*, vk_icd* ni libSceGnmDriver. PKG 6619136 bytes,
28 comprobaciones correctas. La mejora GPU sigue pendiente.
