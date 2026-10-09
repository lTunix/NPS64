# CPU 00.43: ocho trabajadores y heap de hasta 1 GiB

Cambio solicitado por el usuario: CPU PGO usa ocho trabajadores por defecto.
La configuracion diagnostics.cfg existente conserva prioridad. Si contiene
un valor anterior, para obtener ocho se debe usar en /data/NPS64/diagnostics.cfg:

    NPS64DIAG5 8 1 1 1 0

El log debe indicar workers=8 requested_workers=8. Las mediciones de CPU42
no mostraron mejora local frente a seis; este valor se adopta por preferencia
explicita del usuario y requiere comparar en PS4 sonido y rendimiento.

## RAM de la aplicacion

El limite del heap directo CPU aumenta de 512 MiB a 1024 MiB. El allocator
consulta el mayor bloque libre, conserva 2560 MiB de reserva y limita la
asignacion a 1 GiB. Si falla la asignacion o el mapeo, intenta cantidades
menores antes de las alternativas flexible/mmap existentes. Un mapeo fallido
libera la asignacion fisica antes de reintentar.

Se aumenta la capacidad del heap del emulador, no la RAM de la PS4 o del SO.
La asignacion directa reserva esa capacidad al inicializarse cuando hay memoria
suficiente; los objetos usan malloc desde ese heap. No se llena memoria sin
necesidad. No cambia la RDRAM N64 ni el Expansion Pak, y no se promete mas FPS
solo por reservar mas heap. El limite de las variantes GPU queda en 512 MiB.

En una PS4 con memoria suficiente se espera:

    [heap] source=direct memory size=1073741824 ...

Un valor menor es valido si la disponibilidad o asignacion obliga a reducirlo.

## Audio y validacion

Se conserva la reserva de 5760 muestras (120 ms), bloques de 1024 y el
remuestreo CPU42. La prueba de 20 segundos con jitter y pausas de entrega
aprueba sin perdidas ni agotamientos.

Prueba ASan/UBSan del allocator CPU real con servicios kernel/mspace simulados:
heap de 1024 MiB; fallo de asignacion y fallo de mapeo con recuperacion a 512 MiB;
solo 256 MiB asignables respetando la reserva. Alineacion y log de tamano correctos.
El mspace simulado usa malloc del host: esta prueba comprueba la politica de
asignacion y la recuperacion, no la disponibilidad fisica real de PS4.

Reproducir: bash scripts/test_cpu_heap.sh en el contenedor de build.
OpenOrbis compila el paquete; disponibilidad y rendimiento se verifican en consola.
CPU42 y las versiones anteriores quedan preservadas.
