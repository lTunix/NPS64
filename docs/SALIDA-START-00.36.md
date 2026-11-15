# Salida con Start/Options — corrección de prueba 00.36

El botón Options se traduce en BTN_EXIT en la biblioteca y en Start de N64 durante la partida. Se mantiene esa asignación: no se bloquea el botón ni se altera el juego.

El registro de la aplicación pública 00.72 termina después de `raw=00000008 pressed=0800 running=0 paused=0`. El registro fatal separado contiene `signal=0xc` (SIGSYS en la ABI FreeBSD/PS4), `rip=0x8000028bc` y candidatos de pila `0xd55e5a`, `0xd5602e`, `0xd55990`. Los desplazamientos entre esos candidatos son coherentes con el retorno de `_exit` a `_Exit`, el retorno de `_Exit` a `exit` y la continuación de `libc_start_main_stage2` en el runtime enlazado. Son candidatos, no una traza desenrollada. La ruta de código confirma que devolver desde main termina en `_Exit` y la importación `_exit`.

Después de guardar ajustes, cerrar trabajadores, retirar la pantalla, liberar recursos y vaciar/cerrar el log, el frontend invoca un cierre opcional del backend. En PS4 solicita `sceSystemServiceLoadExec("exit", NULL)` y no retorna al CRT. Si la petición retorna, espera un segundo y la reintenta, conservando el control del sistema y evitando volver a la ruta `_exit` que falló. El backend nativo conserva la salida normal.

La prueba del backend PS4 real con servicios simulados verifica la petición `exit`, un primer error devuelto seguido por un segundo cierre simulado y la ausencia de retorno al CRT. La prueba del frontend verifica que la salida de la biblioteca llama al cierre del backend después de terminar. Se conservan las pruebas de retirada de búferes, HUD, controles y ajustes. La confirmación de regreso al menú de PS4 sin CE-34878-0 está pendiente en hardware con el PKG 00.36.
