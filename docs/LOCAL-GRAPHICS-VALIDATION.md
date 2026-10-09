# Validacion local de graficos

## Comparacion del mismo frame

Ejecutar desde PowerShell en la raiz:

```powershell
docker run --rm -v "${PWD}:/work" -w /work nps64-gpu-builder:local bash scripts/test_hle_compare.sh "Super Mario 64 (Europe) (En,Fr,De).n64" build/hle-comparison
```

Guarda pares CPU/Vulkan sin ejecutar otra VI, antes del HUD del frontend.
Solo se captura al producir un frame GPU nuevo; no los frames repetidos ni
los fallbacks. Se muestrea el primer frame GPU y cada 60 frames GPU.
Los archivos JSON de cada par incluyen frame del core, contador de video,
dimensiones VI, contador GPU y estado del filtro VI.

`report.json` contiene error absoluto RGB medio (0..255) y porcentaje de
pixeles con algun canal diferente por mas de 16. Las imagenes comparison.png
muestran CPU, Vulkan y diferencia, en ese orden. El runner tambien guarda logs.
No devuelve PASS de fidelidad: las metricas son diagnosticas. El test de ciclo
de vida si exige renderizado sostenido, audio, estados y cambios CPU/Vulkan.

Resultado inicial 00.36: 13 pares reproducibles; en frame 2601, error RGB
42.48 y 70.10% sobre umbral. Hay diferencias de encuadre/VI y HUD, ademas de
texturas. El filtro VI de Angrylion y la menor resolucion GPU impiden exigir
igualdad exacta o atribuir todo el error a texturas. Hay que corregir/alinear
el viewport y usar regiones comparables antes de medir fidelidad de muestreo.
Estas pruebas usan Lavapipe: no validan RADV/GNM ni rendimiento de PS4.
La instrumentacion esta excluida del binario PS4 con __ORBIS__.

## shadPS4

Se descargo la release oficial v.0.19.0 a build/shadps4 y el contenedor
linuxserver/shadps4 (digest
sha256:7ca74068f64b9fd9006dd1f4c3512d29c36525bfaeb69bb68c5378ddd3019398).
El contenedor incluye launcher; el ejecutable de emulacion se extrajo de la
AppImage oficial. CLI --help comprobada. Primeros intentos de arrancar ELF y
eboot.bin bajo Xvfb terminaron por timeout; no hay evidencia de ejecutar NPS64.
Logs y configuracion en build/shadps4. Docker actual no expone /dev/dri ni
/dev/dxg; solo Vulkan por software. No se ha validado compatibilidad del port.

Fuentes:
- https://github.com/shadps4-emu/shadPS4/releases/tag/v.0.19.0
- https://github.com/linuxserver/docker-shadps4

Un SDK sirve para compilar. Docker aísla el entorno. shadPS4 traduce hardware y
APIs; ninguno reproduce exactamente Liverpool, su driver y sus bloqueos.

Prueba de integracion reproducible (el runner extrae la AppImage si hace falta):

```powershell
python scripts/setup_ps4_emulator.py
docker run --rm -v "${PWD}:/work" -v "${PWD}/build/shadps4/config:/config" -w /work --entrypoint bash lscr.io/linuxserver/shadps4@sha256:7ca74068f64b9fd9006dd1f4c3512d29c36525bfaeb69bb68c5378ddd3019398 scripts/test_ps4_emulator.sh
```

La investigacion con strace si llego al cargador: reconoce NPSX00064, inicia
Vulkan software y carga eboot.bin. Registra `Unimplemented type Interpreter
Path`, fallback HLE de libSceLibcInternal y numerosas importaciones stub.
No se ha aislado cual impide iniciar NPS64. El script monta una carpeta local
en /data/NPS64 y exige un log del invitado para evitar confundir arranque del
emulador con ejecucion del juego. No se modifico el binario para ocultar fallos.

Actualizacion: los intentos sin strace si ejecutaron el invitado. Los logs
estaban en config/.local/share/shadPS4/data/NPS64, no en el montaje solicitado.
NPS64 inicializa VideoOut, AudioOut, Pad y llega a state=library a ~60 FPS.
El timeout es esperado en el bucle de menu. El runner ahora vacia solo ese log
antes del intento y comprueba que el invitado alcanza el menu. Esto valida
arranque, no ejecucion de ROM ni renderer Vulkan. Los warnings de importaciones
no deben confundirse automaticamente con bloqueos. El script conserva el log
del invitado en build/shadps4/guest.log y el del emulador en integration.log.
