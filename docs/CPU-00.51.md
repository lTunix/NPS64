# CPU 00.51: recuperar tiempo entre VIs de coste desigual

El log real CPU50 contiene caidas de Tennis a 25.9 VI/s y GoldenEye a
31.2 VI/s. Angrylion llega a 24.5 y 26.3 ms respectivamente. GoldenEye
USA tiene un objetivo de 60.02 VI/s; Tennis PAL tiene 50.01. El tiempo
core_ms incluye rdp_ms: no se deben sumar como costes independientes.

## Cambio

El limitador anterior borraba toda deuda cuando terminaba un cuadro mas
de un periodo tarde. El siguiente VI ligero podia dormir aunque todavia
faltara recuperar tiempo. CPU51 conserva hasta un periodo de deuda.
Cuando el trabajo es sostenidamente lento no duerme; tras una pausa larga
no acumula minutos de recuperacion. No cambia la frecuencia de VI ni el
reloj emulado. Este cambio puede recuperar esperas en cargas variables,
pero no reduce el coste por pixel de Angrylion ni garantiza 50/60 VI/s.

Se agregan debt_clamps, core_peak_ms y rdp_peak_ms al log [pacing] para
correlacionar recuperacion y cuadros costosos. Se mantienen el core y PGO
de CPU50, seis trabajadores, calidad grafica, heap y audio de 120 ms.

## Validacion

La prueba con sanitizadores recorre 12000 cuadros a 50 y 60 Hz: carga
ligera respeta el objetivo; sobrecarga continua no duerme; carga alterna
no agrega descansos innecesarios; un bloqueo de 60 segundos conserva
como maximo un VI de deuda. Son simulaciones de reloj, no FPS de PS4.

La regresion del backend PS4, prioridades y audio pasa con sanitizadores.
GoldenEye conserva 7937 imagenes y 266 trazas; Tennis conserva 1785
imagenes y 59 trazas frente a CPU50: 9722 imagenes y 325 estados iguales.
Los archivos del rasterizador, PGO, plataforma y audio siguen identicos.
El PKG pasa 28 controles y su contenido extraido coincide con el stage.

Tambien se evalua -mtune=btver2 para el rasterizador sin ampliar SSE2.
Las 96 comparaciones de color/profundidad pasan, pero Dam cambia de
6.358 a 6.521 ms en el primer par y de 6.881 a 6.536 en el inverso.
La variacion no demuestra una mejora consistente; se descarta esta
candidata. El PKG mantiene las opciones de compilacion CPU50.

Referencias de la investigacion de compilador:
https://llvm.org/docs/CommandGuide/llvm-mca.html
https://llvm.org/devmtg/2014-04/PDFs/Talks/GBedwell_PS4CPUToolchain_EuroLLVM2014_distribution.pdf

La ejecucion local en Docker no reproduce Jaguar, el planificador ni las
capacidades completas de PS4. La mejora real requiere repetir las mismas
escenas en consola y revisar [perf] y [pacing]. CPU50 queda disponible.

## Futuro

El TODO registra un segundo renderizador CPU seleccionable por juego.
Antes de integrarlo hay que evaluar licencia, fidelidad y compatibilidad;
Angrylion continuara como opcion. No se incluye otro renderizador en CPU51.
