# Version activa: 00.51 CPU

Ari64 + Angrylion, seis trabajadores y audio continuo de CPU46.
El build normal (`scripts/build.sh`) genera `out/NPS64-00.51-cpu.pkg`.
CPU51 corrige las esperas posteriores a VIs costosos, manteniendo hasta
un cuadro de retraso para recuperar tiempo sin una rafaga ilimitada.
El rasterizador, perfil PGO y reparacion de Tennis son los de CPU50.
Ver [cambios, validacion y limites](docs/CPU-00.51.md).

Repositorio: [lTunix/NPS64](https://github.com/lTunix/NPS64).
`main` conserva la version CPU; las pruebas de ParaLLEl-RDP se realizan en
`experimental/parallel-rdp-ps4`. Ver [ramas y preparacion del clon](docs/REPOSITORY.md).

En esta rama, seguir el [plan de pruebas ParaLLEl-RDP](docs/PARALLEL-RDP-PLAN.md).
Primer paquete independiente: [00.52, pruebas compute para PS4](docs/PARALLEL-00.52-COMPUTE.md).

Los apartados siguientes son historial de las pruebas anteriores.

> 00.19 experimental: variante de rasterizado sin texturas, con selección conservadora por lote y comparación completa de memoria en PC. Ver [prueba Vulkan](docs/VULKAN-00.19-TEST.md). Pendiente validación en PS4; 00.13 CPU sigue siendo la versión estable.

Prueba actual: **00.18 diagnÃ³stico de descriptores y datos**.
La captura 00.17 omitiÃ³ los descriptores 0xa0/0xb0 usados por el rasterizador.
Ver [datos necesarios y lÃ­mites](docs/VULKAN-00.18-TEST.md).

Prueba actual: **00.17 diagnÃ³stico**, sin correcciÃ³n confirmada.
00.16 tambiÃ©n falla con marcador 8 aunque RSRC1 cambiÃ³ a 002c031f.
Ver [instrucciones y lÃ­mites](docs/VULKAN-00.17-TEST.md).

Prueba Vulkan actual: **00.16**, asignaciÃ³n conservadora de registros ACO.
00.15 sigue fallando tras el tercer despacho (marcador 8).
[Detalles y lÃ­mites de la prueba](docs/VULKAN-00.16-TEST.md).

# NPS64 para PS4 â€” 00.13 estable / 00.15 Vulkan experimental

00.14 sigue bloqueÃ¡ndose en PS4. La traza localiza el fallo en clasificaciÃ³n
de tiles. 00.15 sustituye ese shader por una ruta independiente por tile,
sin LDS, atÃ³micos ni barreras internas. Instrucciones actuales en
[docs/VULKAN-00.15-TEST.md](docs/VULKAN-00.15-TEST.md). ValidaciÃ³n PS4 pendiente.

La prueba Vulkan anterior estÃ¡ en `out/NPS64-00.14-vulkan-test.pkg`.
00.14 habilita Vulkan con sincronizaciÃ³n por despacho, una submission
pendiente y separaciÃ³n de spans/tiles. CompilaciÃ³n y pruebas Linux pasan;
la estabilidad PS4 sigue pendiente y puede repetirse el bloqueo del sistema.
Instrucciones y reconstrucciÃ³n en [docs/VULKAN-00.14-TEST.md](docs/VULKAN-00.14-TEST.md).

00.12 sigue bloqueando la GPU y el usuario reporta congelamiento de la consola
con CE-36329-3. 00.13 restaura Ari64 + Angrylion y el PKG principal no incluye
Vulkan, Mesa ni GNM. La ruta Vulkan de PS4 queda bloqueada tambien en el codigo:
una configuracion antigua no puede volver a activarla en la compilaciÃ³n habitual.
00.14 la habilita Ãºnicamente con `NPS64_GPU_EXPERIMENTAL=1`. Los FPS permanentes y
las mejoras CPU anteriores se conservan. La ruta GPU sigue pendiente de arreglo;
esta entrega restaura funcionamiento y evita enviar la carga GPU que congela PS4.

Port de Nintendo 64 con frontend para DualShock 4, inspirado en DolphinPS4
v03.10. La versiÃ³n 00.07 aÃ±ade paraLLEl-RDP sobre Vulkan y el driver
Mesa RADV/GNM de DolphinPS4: rasterizaciÃ³n y VI en la GPU AMD de PS4,
CPU N64 en Ari64 y RSP HLE. Conserva Angrylion como alternativa automÃ¡tica
cuando Vulkan no puede inicializarse.

El usuario confirmÃ³ que 00.06 funciona mucho mejor en consola. La ruta GPU
00.07 se prueba en Linux con Lavapipe y necesita validaciÃ³n en una PS4 real;
las pruebas del PC no demuestran FPS ni estabilidad de la GPU de PS4.
El HUD permanente indica `GPU Vulkan` cuando la ruta GPU estÃ¡ activa y
`CPU Angrylion` cuando se usa la alternativa. La imagen nativa se transfiere
a RAM para el escalado, HUD y VideoOut existentes; la presentaciÃ³n todavÃ­a
consume CPU. No se promete 60 FPS: esta ROM PAL tiene objetivo de ~50 VI/s.

El estudio del port de Wii/GameCube estÃ¡ en [docs/DOLPHINPS4-STUDY.md](docs/DOLPHINPS4-STUDY.md).
Las tareas pendientes y la secuencia de pruebas estÃ¡n en [TODO.md](TODO.md).
Los resultados comprobados se registran en [docs/VALIDATION.md](docs/VALIDATION.md).

La versiÃ³n 00.01 se instalÃ³ en la PS4 del usuario, pero fue rechazada al arrancar
con el mensaje de datos daÃ±ados. La 00.02 corrige una omisiÃ³n del paquete:
incluye los tres mÃ³dulos pÃºblicos de OpenOrbis que tambiÃ©n empaqueta DolphinPS4.
Su arranque en consola fue confirmado. Instalar `out/NPS64-00.13.pkg`
como reemplazo de la versiÃ³n anterior; `/data/NPS64/` contiene las ROMs y partidas.
Si vuelve a fallar, recoger cÃ³digo CE exacto, firmware, versiÃ³n de GoldHEN/loader
y, si existe, `/data/NPS64/nps64.log`.

TriÃ¡ngulo abre la prueba de mando. CÃ­rculo vuelve y Options sale desde la
biblioteca. El menÃº muestra FPS, lectura del mando, dibujo, flip y estado del log.
00.04 dibuja en RAM cachÃ© y copia secuencialmente al scanout; utiliza reloj y
sleep nativos de PS4, escribe el log con descriptor propio y optimiza el escalado.
00.05 usa cuatro trabajadores; el modo sÃ­ncrono y cxd4 siguen disponibles.
Consulta [docs/PS4-DIAGNOSTICS.md](docs/PS4-DIAGNOSTICS.md) para las pruebas.
Las medidas y lÃ­mites del benchmark estÃ¡n en [docs/PERFORMANCE.md](docs/PERFORMANCE.md).

## Compilar con Docker desde este directorio

Docker Desktop debe estar iniciado y usar contenedores Linux. En PowerShell:

```powershell
.\scripts\build.ps1
```

El script normal usa la imagen base, OpenOrbis y el renderer CPU Angrylion.
Las versiones y hashes estan fijados en `sources.lock.json`. No requiere SDK
oficial de Sony. `build_gpu.ps1` conserva la compilacion de investigacion, pero
su paquete `NPS64-00.13-gpu-disabled.pkg` tambien bloquea Vulkan en PS4.
Las pruebas GPU de PC no son validacion de GNM ni autorizan volver a probar
la ruta que produjo el congelamiento de consola.
Todos los archivos del proyecto se escriben en este directorio. El SDK
estÃ¡ en `toolchain/`, las fuentes en `vendor/`, la referencia Dolphin en
`reference/`, los objetos en `build/` y los entregables en `out/`.
Los paquetes de compilaciÃ³n se instalan dentro de la imagen Docker.

Resultado esperado de una compilaciÃ³n correcta:

```text
out/IV0000-NPSX00064_00-NPS64PS400000000.pkg
out/NPS64-00.13.pkg
out/SHA256SUMS
build/nps64.elf
build/nps64.oelf
build/stage/eboot.bin
```

Para ejecutar las pruebas de almacenamiento y la integraciÃ³n del nÃºcleo real
con una ROM de diagnÃ³stico original, sin ROMs comerciales:

```powershell
.\scripts\build.ps1 -Tests
```

Los logs del nÃºcleo estÃ¡n en `build/core-build.log`, `build/native-core.log`
y `build/testdata/nps64.log`. El test nativo usa el mismo frontend con una
plataforma simulada: no ejecuta VideoOut, AudioOut ni el allocator de PS4.

Para acompaÃ±ar el binario con sus fuentes correspondientes:

```powershell
docker run --rm --mount "type=bind,source=$($PWD.Path),target=/work" -w /work nps64-builder:local bash scripts/source_bundle.sh
```

## Instalar y probar en PS4

1. Usar una PS4 con un entorno homebrew que permita instalar fake PKG.
   Anotar modelo, firmware y versiÃ³n del loader/GoldHEN en el informe de pruebas.
2. Copiar el PKG a un USB o a `/data/pkg/` mediante el mÃ©todo que ya uses.
   Instalarlo desde el instalador de paquetes de la consola.
3. Iniciar NPS64 una vez: crea `/data/NPS64/` y sus subdirectorios.
4. Copiar tus ROMs a `/data/NPS64/games/` por FTP. Se aceptan archivos sin
   comprimir `.z64`, `.v64` y `.n64`, con cabecera vÃ¡lida, de 4 KiB a 64 MiB.
   El escaneo es de un nivel y admite hasta 512 entradas.
5. Abrir NPS64; pulsar Cuadrado para actualizar la biblioteca y X para jugar.
6. Pulsar L3 + R3 para pausa, estados o regreso a la biblioteca. Salir del
   juego desde ese menÃº para guardar la partida. TambiÃ©n hay guardado
   automÃ¡tico cada 30 segundos de tiempo real.
7. Recoger `/data/NPS64/nps64.log` si hay problemas. Un cierre forzado puede
   perder los cambios posteriores al Ãºltimo guardado.

Si no hay juegos, debe mostrarse la biblioteca vacÃ­a con instrucciones;
eso es la primera prueba en consola. La ROM propia de diagnÃ³stico creada
por las pruebas estÃ¡ en `build/testdata/games/NPS64-test.z64`; debe mostrar
una superficie turquesa al ejecutarse. No requiere un juego comercial.

## Controles

| DualShock 4 | Biblioteca / pausa | Nintendo 64 |
|---|---|---|
| Cruceta | Seleccionar | Cruceta |
| X | Confirmar / jugar | A |
| Cuadrado | Actualizar biblioteca | B |
| CÃ­rculo | AtrÃ¡s; permanece en biblioteca | Sin asignaciÃ³n |
| TriÃ¡ngulo | Abrir/cerrar prueba del mando | Sin asignaciÃ³n |
| L1 / R1 | Cambiar pÃ¡gina | L / R |
| L2 | â€” | Z |
| OPTIONS | Salir desde biblioteca | Start |
| Stick izquierdo | â€” | Stick analÃ³gico |
| Stick derecho | â€” | Botones C |
| L3 + R3 | Abrir/cerrar pausa | Atajo del frontend |
| Izquierda / derecha | Tema; ranura en pausa | SegÃºn el juego |

Los mandos 2 a 4 se asignan a usuarios conectados; el usuario que inicia la
aplicaciÃ³n conserva el puerto 1. La conexiÃ³n de cuatro mandos estÃ¡ implementada
y pendiente de prueba en consola. No hay vibraciÃ³n ni reasignaciÃ³n grÃ¡fica.

## Datos persistentes

```text
/data/NPS64/
  games/         ROMs del usuario
  saves/         .srm y .slot0.state a .slot7.state
  system/        directorio de sistema para libretro, vacÃ­o en esta versiÃ³n
  settings.cfg   tema, aspecto y ranura
  nps64.log      registro de arranque/nÃºcleo/errores
```

Las partidas se identifican con un hash FNV-1a de 64 bits del contenido de
la ROM, normalizado a big endian. Renombrar o cambiar entre los tres Ã³rdenes
de bytes conserva la identidad. Modificar una ROM cambia la identidad.
El hash evita mezclas accidentales; no es una garantÃ­a criptogrÃ¡fica.
Los estados aÃ±aden versiÃ³n de formato, commit del nÃºcleo, tamaÃ±o y checksum;
se rechazan los vacÃ­os, incompatibles o corruptos antes de pasarlos al nÃºcleo.
Se escribe a un archivo temporal, se sincroniza y se reemplaza por `rename`.
Una SRAM existente con tamaÃ±o incorrecto o ilegible se protege contra
sobrescritura durante esa sesiÃ³n; respaldarla y retirarla por FTP permite
crear una partida nueva.

## Alcance de esta primera versiÃ³n

Incluye biblioteca en lista, tres temas, video 4:3 o 16:9 estirado, pausa,
ocho ranuras y persistencia. No implementa todavÃ­a el cover-flow/XMB completo,
carÃ¡tulas descargables, ajustes por juego, cheats, Rumble Pak, Transfer Pak,
64DD, ROMs comprimidas o actualizaciones automÃ¡ticas de DolphinPS4.
El modo 16:9 no modifica la cÃ¡mara ni activa un widescreen hack.

La CPU usa Ari64 cuando sceKernelMprotect acepta el cache ejecutable de
32 MiB; si falla, vuelve al intÃ©rprete con cachÃ© y registra el motivo.
La rasterizaciÃ³n GPU requiere validar el nuevo driver en consola.
La alternativa Angrylion y el intÃ©rprete con cachÃ© siguen disponibles. Consulta diagnostics.cfg para comparar
perfiles o recuperar el modo sÃ­ncrono de 00.04.

Licencia y atribuciones: [LICENSE](LICENSE) y [docs/CREDITS.md](docs/CREDITS.md).

Para probar tambiÃ©n Vulkan, estados, dos sesiones y las alternativas de arranque:

```powershell
.\scripts\build_gpu.ps1 -Tests
```

La documentaciÃ³n de la ruta GPU y sus lÃ­mites estÃ¡ en
[docs/GPU.md](docs/GPU.md). El perfil de recuperaciÃ³n por FTP es
`NPS64DIAG5 4 1 1 1 0` en `/data/NPS64/diagnostics.cfg`; la Ãºltima cifra
En 00.13 ambos valores usan Angrylion en PS4; el valor GPU de diagnostico se ignora mientras la ruta esta bloqueada.
