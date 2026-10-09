# CPU 00.48: presentacion con tres buffers

La ultima sesion de nps64.log corresponde a CPU47 y Zelda PAL GC.
Hay dos limites medidos en la PS4 del usuario:

| Muestra | VI/s | RDP ms | Core ms | Dibujo ms | Flip ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| frame 1382 | 40.78 | 10.062 | 13.415 | 1.282 | 6.348 |
| frame 1767 | 42.21 | 18.235 | 21.297 | 1.273 | 1.085 |

RDP esta incluido en core. En la segunda escena, core+dibujo+copia ya
superan el presupuesto PAL de 20 ms: este cambio no garantiza 50 VI/s.
La biblioteca del frontend conserva unos 60 FPS; no es el menu del juego.

## Cambio

CPU47 esperaba al flip anterior antes de copiar cada imagen. CPU48 tiene
tres buffers y admite dos envios pendientes. Antes de reutilizar un buffer,
espera al flip posterior al ultimo envio que uso ese buffer. Asi no modifica
un buffer mostrado ni uno pendiente. Mantiene VSYNC, orden y cola acotada.
Al cerrar espera el ultimo envio antes de liberar memoria. La memoria directa
redondeada crece 4 MiB. La variante GPU aparcada conserva dos buffers.

El audio, heap de 512 MiB, seis trabajadores, filtro VI, perfil PGO y
rasterizador siguen como CPU47. pace_ms, sleep_over_ms y late miden espera
del limitador, exceso del sleep y numero de iteraciones fuera de plazo.

La tabla de reciprocales candidata dio medias nativas de 4.819/4.798 ms
frente a 4.627/4.708 ms de CPU47 en dos pares de orden invertido. Se descarto.
Estas mediciones son de PC, no de PS4.

## Validacion

El backend PS4 real se prueba con servicios VideoOut simulados, reteniendo
y comprobando todos los pixeles de buffers mostrados y pendientes.
Rotacion, cola acotada, reloj TSC, Pad, cierre y AudioOut continuo pasan
ASan/UBSan, incluida la alternativa cuando se rechaza la prioridad de audio.

El modelo de VIs variables alterna costes de 34 y 3 ms, pantalla 60 Hz y
limitador 50 Hz: dos buffers dan 39.98 y tres 50.00 FPS. Demuestra la espera
que puede introducir el algoritmo anterior. No emula tiempos reales de PS4.

La ROM Zelda genera 3485 imagenes completas y 116 estados CPU/VI identicos
a CPU47. El PKG pasa 28 controles, incluido payload extraido igual al stage.
Pruebas locales: build/cpu48-final-qa.log; build/cpu48-ps4-build.log.

Artefacto: out/NPS64-00.48-cpu.pkg. CPU46/47 se conservan. Para confirmar
la mejora hace falta probar las mismas escenas y revisar el nuevo log de PS4.
