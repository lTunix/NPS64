# Presentación suave Vulkan 00.23

Checkpoint previo de audio limpio: 44da134, versión 00.22 confirmada en PS4.

El usuario describe principalmente píxeles y bordes dentados. Vulkan sigue renderizando a 1x con AA y filtrado VI existentes. El frontend ampliaba esa imagen a 960x720 (4:3) o 1280x720 (16:9) con vecino más cercano. Esta prueba sustituye esa última ampliación por bilineal centrado cuando Filtro VI está en preciso, respetando la selección por juego. El modo rápido mantiene la presentación anterior. La biblioteca, los textos y los iconos se dibujan después y no se suavizan.

No cambia el contenido de las texturas, la resolución interna ni los shaders RDP. No reconstruye detalles ausentes de las texturas originales ni pretende corregir errores de geometría. No modifica el audio ni el núcleo.

La interpolación de presentación se ejecuta en CPU/SSE2, donde ya se hacía la ampliación anterior; la rasterización RDP sigue en Vulkan. El filtro separable usa dos filas de trabajo reutilizables, un mapa horizontal cacheado, interpolación entera y SSE2 horizontal y vertical. El estado grande es estático; no se reserva memoria por cuadro. Se mantiene la conversión RGBA/XRGB, las franjas 4:3 y los límites de imagen. Se usa la ruta anterior para otros formatos y para reducción de tamaño.

Validación: referencia matemática independiente en ambos órdenes de canales (máximo 2 LSB), tamaños unitarios/pares/impares, bordes, color constante, stride y límites con ASan/UBSan; 28 comprobaciones de PKG. La sesión nativa Vulkan con validación pasó el límite temporal, más de 180 VI, salida/reentrada y cierre de compilador sin errores ASan ni de validación Vulkan; conserva 40 avisos UBSan preexistentes ajenos a Vulkan. Prueba aislada sin sanitizadores: aproximadamente 1,6 ms por cuadro en este PC (640x480 a 960x720). Los costes nativos no representan la CPU Jaguar de PS4: rendimiento y preferencia visual requieren comparación física con 00.22. Aún no se ha incrementado la resolución interna para evitar mezclar el coste de rasterización con esta prueba de presentación.

Sin push ni release.

Validación física: el usuario confirmó una mejora visible y autorizó guardar este checkpoint antes de probar 2x de resolución interna.
