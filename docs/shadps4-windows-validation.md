# Prueba del binario PS4 en Windows

Se probó NPS64 00.64 con shadPS4 SDL 0.19.0 oficial y una NVIDIA RTX 3050 Ti.
La biblioteca abre y Super Mario 64 Europe produce vídeo con el intérprete
cached y el fallback Angrylion. No se modificó el paquete ni la configuración
de la consola.

Dos límites impiden validar el bloqueo de ParaLLEl:

- Ari64 aborta en shadPS4 al proteger su región de código de 32 MiB mediante
  `sceKernelMprotect`: `Failed to change virtual memory protection`.
- Con cached, RADV intenta cargar `/sys/common/lib/libSceGnmDriver.sprx` y
  recibe `0x80020002`. La creación del dispositivo Vulkan falla y el frontend
  recarga el juego con Angrylion. No llega al batch RDP que se bloquea en PS4.

Los logs de esta ejecución están en `build/shadps4/results/ari64-shad.log`,
`ari64-nps64.log`, `cached-shad.log` y `cached-nps64.log`. Son archivos locales
ignorados por Git, al igual que el emulador y las ROMs.

Para repetir desde la raíz en PowerShell:

```powershell
./scripts/test_ps4_emulator_windows.ps1 -Rom './roms/Super Mario 64 (Europe) (En,Fr,De).n64'
```

El script verifica el SHA256 de la release, utiliza un perfil portable bajo
`build/shadps4/runtime/user` y selecciona cached solo en ese perfil. Pulsar N
(Cross) para abrir el juego. Cerrar la ventana al terminar. Requiere el eboot
ya construido y sus archivos de staging; `-Eboot` permite elegir otro staging.

Esta prueba sirve para arranque, integración e interacción del ejecutable real.
Los FPS obtenidos corresponden al PC con CPU fallback. No prueban rendimiento,
coherencia de memoria o sincronización Liverpool. Docker tampoco reproduce ese
hardware. El siguiente requisito para investigar Vulkan en este entorno es
resolver la compatibilidad de carga y las APIs GNM utilizadas por nuestro
winsys; hasta entonces la consola sigue siendo necesaria para ese bloqueo.

Referencia: [release oficial shadPS4 0.19.0](https://github.com/shadps4-emu/shadPS4/releases/tag/v.0.19.0).

## Probe con imports GNM directos

Se construyó y ejecutó un eboot adicional, exclusivamente local, que importa
`sceGnmSubmitCommandBuffers`, `sceGnmSubmitDone` y `sceGnmAddEqEvent` mediante
los stubs del SDK. shadPS4 resuelve estos imports con sus implementaciones HLE.
Se omite la búsqueda de funciones de captura de fallos del firmware en este
probe: no hay un módulo de firmware ni se inventa un handle.

```powershell
docker run --rm --mount "type=bind,source=$($PWD.Path),target=/work" -w /work nps64-gpu-builder:local python3 scripts/build_shadps4_probe.py
./scripts/test_ps4_emulator_windows.ps1 -Eboot 'build/shadps4/probe/stage/eboot.bin'
```

Requiere los objetos y Mesa del build normal ya construidos. El script copia
el staging y reconstruye únicamente el objeto winsys y su archive en
`build/shadps4/probe`; no altera las librerías ni el paquete de consola.

Resultado observado: `virtual query check ok`, `device initialized`, carga
de ParaLLEl y `submit 1` con tres IBs. La importación de RDRAM mediante
`VK_EXT_external_memory_host` falla, por lo que ParaLLEl utiliza su ruta de
copia de memoria; esto no es fallback al rasterizador CPU.

shadPS4 aborta en su procesador GPU con:

```text
Unknown PM4 type 3 opcode 0x43 with count 4
```

El opcode corresponde al paquete `SURFACE_SYNC` emitido por nuestro RADV
GCN7. El siguiente requisito es implementar correctamente ese paquete en
shadPS4; omitirlo no validaría la sincronización. Todavía no hay cuadros ROM
renderizados por ParaLLEl en este entorno ni reproducción del hang de PS4.

Logs locales: `build/shadps4/results/gnm-imports-shad.log` y
`gnm-imports-nps64.log`. El log de NPS64 es acumulativo; este ensayo comienza
en la última aparición de `shadPS4 probe: direct GNM HLE imports`.

## Emulador local adaptado a Mesa (continuación)

Se compiló shadPS4 0.19.0 desde el commit
`c7e065d1b415be16c23e260a21f1dd8bbfc4cb57`, con LLVM 19.1.7 portable,
MSVC/Windows SDK y Ninja. Requiere Visual Studio 2022 con herramientas C++,
CMake, Python y 7-Zip. Fuentes, herramientas y ejecutables quedan en `build/`.

```powershell
./scripts/setup_shadps4_windows.ps1
./scripts/build_shadps4_windows.ps1
docker run --rm --mount "type=bind,source=$($PWD.Path),target=/work" -w /work nps64-gpu-builder:local python3 scripts/build_shadps4_probe.py
./scripts/test_ps4_emulator_windows.ps1 -Eboot 'build/shadps4/probe/stage/eboot.bin' -Emulator 'shadPS4-surface-sync.exe'
```

El parche reproducible implementa `SURFACE_SYNC` mediante una barrera Vulkan
global y espera de finalización, permite inferir dimensiones cuando Mesa no
envía los NOP opcionales de Sony y traduce máscaras dinámicas `V_CMP_CLASS`
mediante los bits IEEE-754, incluyendo ceros con signo, subnormales y NaNs.
La barrera es conservadora: no reproduce las cachés Liverpool ni sirve para
medir rendimiento de PS4. El ejecutable oficial permanece separado.

El probe agrega únicamente en su copia local de Mesa un trailer `OrbShdr`
con longitud y hash del shader compilado, conservando las instrucciones.
Esto satisface el formato de identificación requerido por shadPS4. Los
archives y el PKG de consola no reciben estos cambios.

Resultado de la ejecución: ParaLLEl activo, ROM cargada y envío 5 completado
(`GPU completed 4` al enviar 5). Superó los abortos anteriores por opcode
0x43, metadata de shader, NOP de framebuffer y clasificación de flotantes.
La siguiente compilación de compute se detiene en:

```text
flatten_extended_userdata_pass.cpp:833 FlattenExtendedUserdataPass:
ReadConst base high not from constant memory
```

También aparecen avisos de tamaños de buffers limitados por shadPS4. El
siguiente trabajo requiere compatibilizar su análisis de direcciones y
descriptores con los shaders Mesa. No se omitió esa comprobación ni se
inventó una dirección para continuar. Todavía no se obtuvo un cuadro ROM
Vulkan, una comparación visual ni una reproducción del hang de envío 37.

Evidencia local: `build/shadps4/results/class-mask-shad.log` y
`class-mask-nps64.log`; etapas anteriores `surface-sync-*`, `metadata-*`,
`extent-*` y `ssa-*`. El build nativo e incremental terminó correctamente.
