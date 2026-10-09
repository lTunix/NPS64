# CPU 00.44: recuperar configuracion de CPU41

El usuario informa unos 10 FPS menos con CPU43 y mejor rendimiento con CPU41.
CPU43 cambio simultaneamente trabajadores y heap: no permite atribuir toda
la regresion a una sola causa. Las pruebas locales de CPU42 ya mostraban
que ocho trabajadores eran mas lentos que seis; no predicen FPS exactos PS4.

CPU44 restaura seis trabajadores predeterminados y heap maximo de 512 MiB.
Conserva Angrylion PGO, escalado SSE2, presentacion CPU40 y audio de 120 ms
solicitado en CPU42. Tambien conserva el reintento de heap menor ante fallos.
Las pruebas ASan/UBSan aprueban heap de 512 MiB y recuperacion a 256 MiB,
reserva de sistema intacta y simulacion de audio de 20 segundos con jitter.

## RDRAM y Expansion Pak

vendor/parallel-n64/mupen64plus-core/src/main/main.c selecciona 0x800000
bytes si DisableExtraMem no esta activo, y 0x400000 si esta deshabilitado.
El log Mario 64 confirma: Initializing 4 RDRAM modules for a total of 8 MB.
Los 8 MiB emulados ya estaban activos, incluidos en las versiones que rinden
mejor. La configuracion Expansion Pak significa 8 MiB totales, frente a 4 MiB
sin expansion. No es el limite del heap del proceso PS4. No se cambia RDRAM
ni se asume que toda referencia a una reserva de 8 MB describa esta opcion.
Si se trata de un cache distinto de Mupen64Plus-Next, requiere identificar
ese ajuste antes de trasladarlo al core paraLLEl actual.

## Configuracion de consola

Si se creo /data/NPS64/diagnostics.cfg con ocho trabajadores, cambiarlo a:

    NPS64DIAG5 6 1 1 1 0

O retirar ese override para recuperar el valor predeterminado. settings.cfg
controla tema, pantalla, slot y VI; no controla trabajadores. Revisar:

    workers=6 requested_workers=6
    [heap] source=direct memory size=536870912 ...
    [audio] ... prefill=5760

La confirmacion de recuperacion del rendimiento y audio requiere PS4 real.
CPU41, CPU42 y CPU43 se conservan para comparar.
