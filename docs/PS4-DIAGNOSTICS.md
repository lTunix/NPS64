## Estado actual 00.13

La GPU queda bloqueada en PS4 por congelamiento de consola CE-36329-3. Instalar
out/NPS64-00.13.pkg (Ari64 + Angrylion). Los perfiles que soliciten GPU no pueden
activarla, incluido NPS64DIAG5 con ultimo valor 1. No repetir en consola las
comparaciones GPU antiguas que aparecen abajo; se conservan como historial.

## Perfil GPU 00.07

El predeterminado ahora es Vulkan. El HUD muestra `GPU Vulkan` o `CPU Angrylion`.
Consultar también [GPU.md](GPU.md). Para comparar en PS4:

| diagnostics.cfg | Renderer |
|---|---|
| `NPS64DIAG5 4 1 1 1 1` | Vulkan / paraLLEl-RDP, Ari64, HLE, VI preciso |
| `NPS64DIAG5 4 1 1 0 1` | Vulkan, VI rápido |
| `NPS64DIAG5 4 1 1 1 0` | Angrylion, cuatro workers, Ari64, HLE |

Los formatos anteriores siguen siendo válidos, pero no desactivan Vulkan;
los perfiles Angrylion de las versiones anteriores requieren la última cifra
`0` del formato nuevo. Cambiar el archivo con la app cerrada.

# Diagnóstico de carga y mando — 00.06

00.06 conserva el contador durante toda la partida, arriba a la izquierda.
FPS mide frames de VI emulados por segundo de tiempo real, no frames únicos
de animación. El objetivo se obtiene de la ROM: Mario Kart PAL indica 50.
25/50 y 50% significa que la emulación corre a mitad de velocidad.
La segunda línea muestra core, RDP, VI, dibujo y flip. RDP/VI forman parte
de core: no sumar esos tiempos al total. VI incluye sincronización/filtrado.
workers muestra el pool realmente creado; requested_workers del log muestra
lo solicitado. CPU identifica Ari64 o cached según el permiso ejecutable.

L3 + R3 abre pausa. La última opción «Imagen» permite comparar precisa y
rápida. Se conserva al reiniciar en NPS64CFG2, compatible con ajustes antiguos.
El modo rápido omite filtrado VI y puede cambiar nitidez y márgenes; no
omite emulación ni fotogramas. 00.06 mantiene imagen precisa por defecto.

00.03 arranca pero el menú responde lento, Mario Kart queda negro y el log
no se escribe. 00.04 cambia el framebuffer, temporización, log y RSP.

1. Instalar out/NPS64-00.06.pkg como reemplazo de 00.05.
2. Pulsar Triángulo en biblioteca para probar el mando. Pulsar cada botón
   por separado: la línea X/Círculo/Cuadrado/Triángulo debe coincidir.
   El valor ScePad debe mostrar respectivamente 00004000, 00002000,
   00008000 y 00001000. Círculo o Triángulo cierran esta prueba.
3. Cuadrado busca ROMs e informa la cantidad; X abre el juego seleccionado.
   Círculo permanece en biblioteca. Options sale desde la biblioteca.
4. Probar primero out/NPS64-test.z64 en /data/NPS64/games/.
   Debe dibujar una superficie turquesa. Probar después Mario Kart 64.
5. Después de cada cierre, recoger /data/NPS64/nps64.log por FTP.
   El log se añade al anterior: buscar la última línea «NPS64 00.06».
   Anotar firmware, GoldHEN/loader, modelo, tamaño/formato de la ROM y
   si apareció alguna imagen antes del error. No hace falta compartir la ROM.

## Interpretación del último marcador

| Última etapa sin la siguiente | Área a investigar |
|---|---|
| hash_begin sin hash_ok | Lectura/identificación del archivo |
| retro_init_begin | Inicialización del núcleo y configuración |
| retro_load_game_begin | Lectura del core y apertura de cartucho |
| frame=0 begin sin end | Primer arranque CPU/RSP/plugins |
| first_video antes del cierre | Ya hubo video; revisar las etapas posteriores |

No son diagnósticos definitivos: el log delimita el punto antes del cierre.
La línea [heap] muestra si se obtuvo el heap directo esperado o un fallback.
Cada dos segundos se escribe [perf] con FPS y tiempos de lectura del mando,
núcleo, dibujo y flip. La biblioteca también muestra estas medidas y «log OK»
o «log ERROR». Durante un arranque sin imagen se muestra el contador de frames;
first_visible_video registra cuándo apareció contenido distinto de negro.
Si el archivo sigue vacío, informar del indicador log y de los valores del menú.
El inicio comercial tiene una fase negra: no interpretar una imagen negra
aislada como bloqueo si el contador de fotogramas sigue avanzando.

## Comparar renderer sin recompilar

00.06 solicita Ari64, RSP HLE y cuatro trabajadores Angrylion. Buscar
[jit] requested=1 executable=1 selected=Ari64 y workers=4 al cargar.
Si mprotect falla, el log muestra el error hexadecimal del kernel y
selected=cached interpreter. El perfil anterior sigue disponible.

Opcionalmente crear /data/NPS64/diagnostics.cfg como texto con una línea:

| Contenido | CPU solicitada | RSP | Angrylion |
|---|---|---|---|
| NPS64DIAG3 4 1 1 | Ari64 (predeterminado) | HLE | Cuatro trabajadores |
| NPS64DIAG3 0 1 1 | Ari64 | HLE | Síncrono |
| NPS64DIAG3 4 1 0 | Intérprete | HLE | Cuatro trabajadores |
| NPS64DIAG3 0 1 0 | Intérprete, perfil 00.04 | HLE | Síncrono |
| NPS64DIAG3 0 0 0 | Intérprete | cxd4 | Síncrono |

NPS64DIAG1 y NPS64DIAG2 se aceptan y solicitan JIT, pero conservan el número
de trabajadores del archivo. Retirar diagnostics.cfg antiguo para probar
el nuevo perfil completo. El formato nuevo es workers, hle, cpu: workers
0..8 (0=síncrono), hle 0=cxd4/1=HLE, cpu 0=intérprete/1=Ari64 comprobado.
NPS64DIAG4 añade un cuarto número: filtro VI 0=rápido, 1=preciso.
Ejemplos: NPS64DIAG4 4 1 1 1 reproduce el predeterminado preciso;
NPS64DIAG4 4 1 1 0 prueba rápido; NPS64DIAG4 6 1 1 0 prueba seis trabajadores.
El archivo de diagnóstico tiene prioridad al arrancar sobre el filtro guardado;
retirarlo permite usar el ajuste persistente de pausa.

Cerrar y abrir la aplicación después de cambiar ese archivo. Comparar una
sola opción por prueba y guardar cada log antes de la siguiente.
Retirar diagnostics.cfg restaura el perfil predeterminado.
Los estados creados con perfiles diferentes aún necesitan validación;
durante el diagnóstico cargar la ROM directamente y no cargar un estado.
