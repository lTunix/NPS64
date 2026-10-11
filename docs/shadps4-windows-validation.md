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
