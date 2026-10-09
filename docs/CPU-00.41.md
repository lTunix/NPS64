# CPU 00.41: continuidad de audio

El usuario confirma que CPU40 llega casi permanentemente a 50 FPS en PS4,
pero oye cortes. El promedio de VI/s no mide continuidad de AudioOut.

La implementacion anterior consumia 256 muestras (5.33 ms) antes de esperar
la terminacion del bloque anterior, sin prebuffer. Cuando una rafaga libretro
llega tarde, emitia silencio aun manteniendo buen promedio de FPS.
Esta es una causa plausible del reporte; faltaban contadores para confirmarla.

## Cambios

AudioOut usa bloques de 1024 muestras (21.33 ms) a 48 kHz estereo S16.
El consumidor espera terminacion antes de consultar la cola. Conserva el
protocolo de OpenOrbis: espera con Output(NULL), luego entrega el bloque.
El arranque acumula 3072 muestras (64 ms); si falta un bloque entero,
conserva las muestras restantes y vuelve a acumular esa reserva.
La pausa/carga/descarga solicita reset: el consumidor descarta la cola
al siguiente limite de bloque, sin que el productor modifique su indice.
El remuestreo y las muestras originales no cambian. La reserva introduce
latencia de audio; no compensa desaceleraciones sostenidas del juego.

Cada dos segundos de produccion aparece:

    [audio] queued_ms=... underruns=... dropped=... block=1024 prefill=3072

underruns cuenta agotamientos despues del arranque; dropped cuenta muestras
rechazadas por cola llena. Los contadores son acumulativos por inicio de app.
Un incremento permite distinguir falta de alimentacion de cola llena.

Se conserva CPU40 PGO y seis trabajadores por defecto. Aumentarlos sin medir
puede competir con audio y frontend. La opcion existente permite probar siete:

    /data/NPS64/diagnostics.cfg: NPS64DIAG5 7 1 1 1 0

El paquete usa seis si no hay override. Siete no se valida como mejor ni se
recomienda como nuevo predeterminado hasta comparar la misma escena en PS4.

## Pruebas

ASan/UBSan: prebuffer, orden de muestras, vaciado, rebuffer, overflow y vuelta
de contadores unsigned. Una simulacion de 20 segundos con batches PAL de
960 muestras cada 20 ms, jitter alternado de 8 ms y salida de 1024 cada
21.333 ms conserva todas las muestras sin underruns ni descartes.
Backend PS4 simulado: cuatro variantes aprueban presentacion y cierre.
Esta prueba del backend desactiva AudioOut; el calendario de audio se prueba
por separado. No reproduce el scheduler o el dispositivo de audio PS4.
OpenOrbis genera PKG de 6619136 bytes, 28 controles aprobados y payload exacto.

No cambia rasterizador ni perfil. Se preserva CPU40 para comparar sonido.
La continuidad real de audio y la latencia necesitan validacion en consola.
