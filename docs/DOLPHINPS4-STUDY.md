# Estudio previo: DolphinPS4 v03.10

Referencia consultada el 5 de octubre de 2026:
https://github.com/iHaiDeeZ/DolphinPS4/releases/tag/v03.10

La API de GitHub informó `published_at=2026-10-05T06:21:26Z`.
Se descargó `main` y se fijó el commit
`cf2c55229e8b7b96ee12aa7dff1d2c7ef574c57a` en `sources.lock.json`.
La copia completa está en `reference/DolphinPS4`.

## Qué hace el port de Wii/GameCube

No es una conversión del emulador a otro sistema emulado. Es Dolphin
modificado para ejecutarse como aplicación nativa x86_64 de PS4.
El núcleo sigue emulando CPU PowerPC, memoria y dispositivos de GC/Wii.

| Área | Evidencia revisada | Aplicación en NPS64 |
|---|---|---|
| Build separado del upstream | BUILDING.md, patches/BASE.txt, scripts/configure-dolphin.sh | Fuentes fijadas, copias de build y parche acotado, sin alterar vendor |
| Toolchain | toolchain/env.sh, ps4-love-style.cmake, ps4-dolphin.cmake | Target x86_64-pc-freebsd12-elf y OpenOrbis dentro de Docker |
| Modernización C++ | build-libcxx.sh, build-runtime.sh | No se necesita reconstruir libc++ 21 para el renderer software elegido |
| Heap y ABI | port/ps4_runtime.cpp, toolchain/runtime/ps4_malloc.c | Allocator mspace adaptado y wrapper pthread_once |
| Inicio y libc | port/ps4_crt1.S | Mantener CRT estándar inicialmente; verificar presión del heap de módulos en consola |
| Loader ELF | ps4-love-style.cmake, ps4-link.x, check-oelf.cmake | Conservar datos/TLS por símbolo; comprobar segmentos ELF/OELF |
| GPU | patches/mesa-ps4.patch, port/mesa/ac_ps4_drm.c, scripts/build-mesa.sh | Pendiente: no usar Vulkan de Linux como sustituto del backend GNM |
| Núcleo y frontend | patches/dolphin-ps4.patch, README.md, xmb/ | Frontend propio libretro con biblioteca, DS4, pausa, estados y temas |
| PKG | scripts/make-pkg.sh, package-dolphin.sh | SFO, content ID, ELF → OELF/SELF → GP4 → PKG |
| Pruebas de hardware | probe/, probe-vk/, probe-gnm/ | Primera prueba NPS64: biblioteca vacía, seguida por ROM propia |

## Detalles que sí influyen en la implementación

El runtime de Dolphin identifica que el allocator precompilado de OpenOrbis
intenta mapear 2.5 GiB inicialmente y puede fallar en consolas retail. No se
debe confiar en que un enlace exitoso pruebe ese allocator. NPS64 enlaza la
variante C `ps4_malloc.c` antes de libc, con límite de 512 MiB y nombre propio.
La memoria del núcleo usa su modo comprimido en PS4 para evitar reservar
otros 512 MiB para un mapa físico lineal. El heap y el consumo total todavía
se deben medir en la consola.

La prueba de abrir dos ROMs consecutivas detectó además problemas del
nuevo núcleo N64 independientes de Dolphin: CoreShutdown no se llamaba,
el latch g_real_stop persistía y la ruta de reentrada no completaba la
liberación del intérprete/dispositivos. Se corrigen en patch_core.py y se
verifican con dos arranques que producen imagen. También se amplía la tabla
de índices de paks para admitir PLUGIN_BIO_PAK=6 sin escribir fuera de límites.

`pthread_once_t` es un `int` de cuatro bytes en los headers de musl usados
por OpenOrbis; Dolphin documenta que la implementación del kernel puede
escribir dieciséis bytes. Se adapta su solución de estados atómicos
0/1/2 y se enlaza con `--wrap=pthread_once`.

El linker de OpenOrbis deja secciones `.data.*`, `.tdata.*`, `.tbss.*` y
constructores priorizados fuera de las reglas originales. Se adapta el
script de Dolphin y se recogen esas secciones explícitamente. Además se
usa `-z max-page-size=0x4000` y se comprueba que el OELF no tenga segmentos
PT_LOAD duplicados, un fallo de arranque señalado por Dolphin.

Para gráficos acelerados, Dolphin usa Mesa 26.0.8 parcheado y RADV con un
winsys que envía trabajo mediante GNM. Esa adaptación es considerable:
no basta con compilar un núcleo N64 con `HAVE_VULKAN=1`. NPS64 inicialmente
usa Angrylion, entrega frames CPU a VideoOut y mantiene la ruta GPU en TODO.

## Qué no se puede reutilizar directamente

El JIT PowerPC de Dolphin no recompila MIPS R4300/RSP. Para N64 se necesita
el recompilador del núcleo N64 y una adaptación propia del cache de código,
memoria ejecutable, invalidación y ABI. Lo mismo ocurre con guardados,
cheats y controles: los dispositivos GameCube/Wii son distintos.

El perfil RetroArch/Piglet del SFO de Dolphin tiene autoridad y atributos
específicos para conseguir contexto EGL. NPS64 no necesita Piglet, por lo
que utiliza el perfil básico de OpenOrbis y PAID `0x3800000000000011`.
No copia los SPRX de Piglet/shaders ni incorpora archivos de un SDK privado.

El script scripts/make-pkg.sh de Dolphin también copia los módulos públicos
libc.prx, libSceFios2.prx y sce_sys/about/right.sprx de los ejemplos OpenOrbis.
NPS64 00.01 los omitió: esa omisión se corrige en 00.02. Estos módulos son
distintos de los módulos privados Piglet/shaders. La verificación de hashes
del fake PKG no comprueba que esté completo para el cargador de la consola.

## Diferencias deliberadas y siguientes pasos

Esta entrega conserva el esquema de aplicación homebrew y los elementos
básicos de biblioteca, mando, pausa y persistencia; no pretende igualar el
frontend maduro de Dolphin v03.10. Primero se valida el núcleo software en
la consola; después, JIT y renderer acelerado; por último, cover-flow/XMB,
carátulas y ajustes por juego sobre una base estable.
