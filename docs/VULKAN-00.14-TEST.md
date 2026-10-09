# NPS64 00.14 — prueba Vulkan en PS4

Paquete: `out/NPS64-00.14-vulkan-test.pkg`.
Esta compilación experimental habilita Vulkan de forma explícita. La ruta
habitual sigue siendo 00.13 con Angrylion. No hay validación en consola
de estos cambios; todavía puede repetirse el bloqueo del sistema.

## Cambios respecto a 00.12

- RADV PS4 espera el fin de cada despacho de cómputo y sincroniza cachés
  vectorial, escalar y L2 antes de permitir el siguiente trabajo.
- GNM mantiene como máximo una submission pendiente: espera su fence
  antes de asignar y enviar otra. La espera libera el mutex del dispositivo.
- paraLLEl-RDP separa preparación de spans y clasificación de tiles con
  una barrera explícita, también en resolución escalada.
- Se mantienen publicación SFENCE de escrituras WC, ausencia de prefetch
  por autocopia de shaders, marcadores de progreso, inicialización C++
  diferida y carga explícita de módulos del sistema.

Son medidas conservadoras para eliminar solapamiento y hacer más precisa
la traza. Pueden reducir rendimiento. No prueban la causa original ni
ofrecen recuperación de una GPU física bloqueada.

El renderizador sigue siendo paraLLEl-RDP Vulkan sobre Mesa RADV/ACO y
libSceGnmDriver, adaptado de DolphinPS4. La emulación de CPU N64 usa Ari64.
El framebuffer Vulkan aún se lee a RAM para la composición y VideoOut.

## Prueba

Instalar el PKG como actualización. No incluye ROMs ni modifica partidas.
Abrir Mario Kart y comprobar imagen y FPS. Si existe `diagnostics.cfg`,
su último valor controla GPU; para solicitar Vulkan se puede usar:

```text
NPS64DIAG5 4 1 1 1 1
```

El log debe identificar `00.14 EXPERIMENTAL`,
`conservative compute: serialized setup and single-flight submissions`
y `renderer=paraLLEl-RDP active`. Si aparece Angrylion, se produjo fallback
o la configuración pidió CPU: no es una prueba Vulkan satisfactoria.

Para analizar un fallo se necesitan `/data/NPS64/nps64.log`,
`/data/NPS64/gpu-hang.txt` y `/data/NPS64/boot.log` del mismo intento.
Los números de submission cambiarán; importa el último marcador alcanzado
y qué despacho precede a la espera. No repetir un bloqueo de consola sin
analizar primero la traza nueva. Conservar `out/NPS64-00.13.pkg` para volver
a la compilación funcional.

## Reconstrucción

Con SDK, dependencias e imagen Docker ya preparados en este directorio:

```powershell
.\scripts\build_vulkan_test.ps1
```

Las pruebas de host usan Lavapipe, no el controlador PS4. La prueba del
parche verifica orden de fences, sincronización e idempotencia. La
compilación cruzada verifica el código PS4 y el paquete se valida y extrae.
La confirmación de que el juego funciona con Vulkan sigue pendiente en
hardware real.

## Validación de entrega

PKG: 30 605 312 bytes, APP_VER/VERSION 00.14, 28 comprobaciones correctas.
SHA-256:
`3462731528810d15f6cb60e6ffadfb87e727430688564f852ddc70e1b1d8a861`.

ELF/OELF alineados a 16 KiB y sin segmentos solapados; sólo el registro
temprano se ejecuta antes de main. Inspección del ELF confirma Vulkan
habilitado, ambos controles de serialización, núcleo RDP parcheado y Ari64.

En host pasan los tres probes compute, dos sesiones con imagen/estados,
fallbacks de inicialización y 360 frames de Mario Kart (primera imagen
visible en frame 53, cierre limpio, sin errores de validación Vulkan).
La prueba del build habitual confirma que sigue bloqueando GPU y puede
ejecutar dos sesiones CPU. RADV PS4 fue recompilado; los tests Lavapipe no
ejecutan ese controlador cruzado ni demuestran que el bloqueo esté resuelto.
