# Rachas lentas y autoguardado — 00.29

La prueba física de Smash USA en 00.28 todavía cae a 45 FPS y corta audio. El tramo de 44,4 s registró una racha lenta de 10,1 s, 55 intervalos de al menos 1,5 presupuestos, cinco de 50 ms y cuatro de 100 ms. Los vaciados de audio subieron hasta 15. El autoguardado del cuadro 1612 bloqueó 151,090 ms. En intervalos de 45–49 FPS, RDP ocupó aproximadamente 8,6–9,5 ms de los 15,6–17,2 ms del núcleo; FullSync solo representa parte de ese tiempo. No basta con mejorar la media.

## Guardados

Se mantiene una copia de los bytes SRAM conocidos como cargados correctamente o guardados con éxito. Si no cambian y el archivo todavía existe, save_ram no vuelve a escribir ni sincronizar el archivo. Los datos modificados siguen usando file_write_atomic. Un fallo no actualiza la referencia y permite reintentar; las partidas inválidas siguen protegidas. El caché se elimina al cambiar de juego o cerrar la sesión. Si falta memoria para el caché se conserva el guardado normal. La comprobación de existencia permite recrear archivos retirados por FTP, sin basarse en el stat de tamaño que había causado el fallo anterior.

El caso nativo sin cambios pasó por wrote=0 y 0,271 ms. No es una medición de PS4 ni elimina el coste de una escritura necesaria cuando el juego modifica realmente la SRAM. No se cambia el formato de las partidas.

## Cola CPU de comandos RDP

La cola pasa de 4096 a 65536 palabras (16 a 256 KiB). Copia los payloads circulares en hasta dos bloques contiguos, evitando un módulo por palabra. El consumidor conserva su buffer de payload sin reducirlo ni volver a inicializarlo para cada comando; mantiene crecimiento para comandos mayores de 64 palabras. Las publicaciones acquire/release, notificaciones, orden, completed_count, drain y espera FullSync permanecen.

El hilo de comandos solicita la prioridad existente del backend (450), que conserva prioridades ya superiores; audio sigue en 400. Se usa un hook débil para que el núcleo funcione también sin backend PS4. El objetivo es reducir espera de CPU y competir por encima de compilaciones de fondo; su efecto real debe medirse en consola. No se cambian shaders, resolución, filtros ni sincronización RDP.

## Validación

- Backend SRAM real: primer guardado, cambio, omisión por igualdad, bytes exactos de disco, reintento tras fallo, protección, archivo eliminado y reinicio de sesión.
- CommandRing real, tanto previo como modificado: 40000 payloads ordenados, vueltas circulares, presión de consumidor lento, comandos mayores de 64 palabras, drain, reinicio y cierre; capacidades 128/65536. ASan/UBSan.
- Smash 2x durante 60 s y 441 VI con Vulkan validation, sin errores ASan ni de validación. Persisten los 43 avisos UBSan conocidos del núcleo de este juego.
- Captura del cuadro 300: cero píxeles distintos fuera del HUD frente a 00.28.
- Solo cambian command_ring.o y rdp_device.o en la librería PS4; no los objetos de audio o JIT.
- PKG: 28 comprobaciones y regresiones de ajustes/persistencia.

Pendiente físico: repetir el mismo combate a 2x, observar prioridad del trabajador, coste/estado wrote de autoguardados, tiempos RDP y frecuencia/duración de episodios. Los cortes de audio pueden continuar si la producción sigue por debajo del tiempo real; no se declara corregido por una media de FPS mejor. Mantener 00.28 como referencia; sin push ni release.

Validación física: el usuario confirmó mayor rendimiento el 14 de noviembre de 2026 y autorizó guardar este checkpoint. Persisten silencios prolongados de audio a 2x; quedan como siguiente corrección, junto con más optimización de rendimiento. No se declara resuelta la fluidez completa del audio.
