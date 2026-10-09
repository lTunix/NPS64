# CPU 00.45: remuestreo de audio filtrado

El usuario confirma recuperacion de FPS con CPU44, pero oye sonido sucio y
microcortes. La conversion anterior repetia la muestra mas cercana al pasar
la frecuencia DAC N64 a 48 kHz, introduciendo componentes espectrales espurias.
Esto explica una fuente de distorsion; no demuestra que todos los cortes del
reporte tengan esa causa. El log disponible no incluye contadores recientes.

## Cambio

Filtro FIR sinc causal de 32 taps y tabla de 256 fases, ventana Blackman,
normalizacion DC y saturacion S16. Mantiene historia y posicion fraccionaria
entre batches de cualquier tamano. El core entrega frecuencia DAC exacta;
solo se reconstruye la tabla cuando cambia. El cutoff baja para frecuencias
por encima de 48 kHz, para limitar alias en la reduccion de frecuencia.
El retardo del filtro es 15 muestras de entrada (0.47 ms a 32040 Hz).
No hay asignaciones dinamicas por bloque; las tablas ocupan 32 KiB.
Al pausar/cargar se reinicia historia; no se reutilizan muestras del juego
anterior. Cambiar frecuencia tambien reinicia historia y fase.

Se conservan seis trabajadores, Angrylion PGO, heap de 512 MiB, bloques
AudioOut de 1024 muestras y reserva de 5760 (120 ms). No modifica velocidad
del juego ni compensacion de reloj. Falta sostenida de produccion de audio
puede seguir causando agotamientos; subir el buffer no los resuelve siempre.

## Validacion

ASan/UBSan: conversion identica por muestra entre un batch completo y batches
variables, frecuencias 8000, 22037.94, 32000, 32040, 44100, 48000, 96000 y
192000 Hz; numero de muestras dentro de una muestra del valor teorico.
DC conserva exactamente ambos canales. Reset no retiene historia previa.
Seno de 1 kHz: error RMS 5.22 unidades S16 frente a la senal ideal retardada.
Seno de 10 kHz a 32040 Hz convertido a 48 kHz: componente imagen a 22040 Hz
respecto a fundamental 0.000071 frente a 0.453833 con la rutina anterior.
Es una medida sintetica de calidad de conversion, no una grabacion PS4.

Backend PS4 simulado: cuatro variantes de presentacion/cierre aprobadas.
Este backend desactiva AudioOut; no valida hardware de sonido ni scheduler.
OpenOrbis compila el paquete. Prueba en PS4 pendiente.

Reproducir en Docker: compilar tests/test_audio_resampler.c con -Isrc -lm.
La prueba se integra en scripts/test.sh.

## Comprobacion en consola

    [audio] queued_ms=... underruns=... dropped=... block=1024 prefill=5760 resampler=sinc32 input_hz=...

Comparar la misma musica/escena con CPU44. Revisar si underruns/dropped aumentan
al oir cortes: permite distinguir distorsion de conversion de falta o exceso
de muestras. Si hay diagnostics.cfg debe pedir seis trabajadores. CPU44 y
versiones anteriores se preservan para comparar.
