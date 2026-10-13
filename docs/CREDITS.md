# Créditos y licencias

NPS64 es un frontend experimental nuevo, GPL-2.0-or-later, enlazado con
paraLLEl-N64/Mupen64Plus y libretro-common. Los avisos y licencias originales
se conservan en vendor/parallel-n64. Consulta sources.lock.json para el código
fuente exacto utilizado. No se incluyen juegos comerciales ni BIOS.

DolphinPS4, de iHaiDeeZ y sus colaboradores, se estudió como referencia.
src/third_party/ps4_malloc.c y scripts/ps4-link.x proceden de ese repositorio,
commit cf2c55229e8b7b96ee12aa7dff1d2c7ef574c57a, bajo GPL-2.0.
El allocator deriva del trabajo de PSChrome mencionado en el propio archivo.
Se modificaron el nombre del heap y su límite a 512 MiB. La corrección
pthread_once de src/runtime_ps4.c adapta la de port/ps4_runtime.cpp.
No se copiaron su marca, icono, biblioteca de juegos ni frontend Dolphin.

OpenOrbis aporta headers, libc y herramientas públicas de PS4; los módulos
libSce*.so del SDK son stubs de enlace y no se redistribuyen en el PKG.
La licencia del toolchain está en vendor/OpenOrbis/LICENSE.
El PKG 00.02 sí incluye los módulos públicos de ejemplo de OpenOrbis
libc.prx, libSceFios2.prx y sce_sys/about/right.sprx, tomados del SDK v0.5.4.
Sus fuentes están en vendor/OpenOrbis/src/modules y se incluyen en el archivo
de fuentes; docs/OpenOrbis-LICENSE.txt conserva la licencia del proyecto.

La fuente rasterizada deriva de DejaVu Sans Mono. FONT-LICENSE.txt dentro del
PKG incluye el aviso de Bitstream/DejaVu. El icono se dibuja con Pillow;
no se usan logos o imágenes de Nintendo/Sony. El nombre Nintendo 64 describe
el sistema emulado y no implica afiliación.

Antes de distribuir un binario GPL, acompáñalo con su código fuente
correspondiente completo, parches, scripts y licencias. scripts/source_bundle.sh
genera un archivo de fuentes local para acompañar al PKG.

00.07 incorpora Mesa 26.0.8 / RADV / ACO y el winsys PS4 MIT de DolphinPS4,
libdrm 2.4.125 (headers), y libc++/libc++abi LLVM 21.1.6
(Apache-2.0 WITH LLVM-exception). glslang 15.1.0 se usa como herramienta host.
Se conservan sus avisos y licencias en vendor/gpu y en las fuentes correspondientes.
ParaLLEl-RDP / Granite conserva los avisos MIT de Hans-Kristian Arntzen;
el núcleo completo sigue GPL-2.0-or-later. No se incluye ninguna ROM en el PKG
ni en el archivo de fuentes. El driver GNM se obtiene de la propia PS4 al arrancar.

00.08 usa dlmalloc 2.8.6 de Doug Lea (MIT-0), conservando su texto de licencia,
con la configuración mspace de DolphinPS4; las fuentes se incluyen en src/third_party.

Carátulas: [Libretro Thumbnails](https://github.com/libretro-thumbnails/Nintendo_-_Nintendo_64). Las imágenes se descargan al usar la biblioteca y pertenecen a sus respectivos titulares.

Decodificador PNG: [stb_image, Sean Barrett y colaboradores](https://github.com/nothings/stb), licencia MIT / dominio público incluida en el archivo fuente.
