# Resolución por juego y presentación Vulkan 00.25

Tras confirmar 2x en PS4, el usuario prioriza optimizarla y ofrecer nativa/2x. El rasterizador usa factores enteros de potencia de dos; no se ofrece un 1,5x obtenido desde 2x porque no reduciría su coste de renderizado.

## Selección persistente

La configuración general incluye resolución predeterminada y la ficha de cada ROM incluye resolución interna nativa/2x. El menú L3+R3 permite guardar otra resolución para ese juego y avisa que se aplicará al volver a abrirlo. Durante la sesión conserva el modo activo: cambiar otras opciones no fuerza un cambio de resolución en caliente. Se mantienen las proporciones por juego, la biblioteca 16:9 y los ajustes de audio validados.

Los formatos CFG5 y GAME2 agregan el campo de resolución y leen las versiones previas. La migración conserva 2x como valor heredado de 00.24. Valores fuera de rango o registros incompletos se rechazan. Los ajustes antiguos de PS4 se descargaron a build/ps4-settings-before-25 para facilitar una vuelta atrás; las versiones anteriores no leen los formatos nuevos.

## Optimización

El callback de vídeo utiliza directamente los píxeles del staging Vulkan cuyo fence ya terminó. Ese buffer se reutiliza solo en el siguiente callback del mismo hilo y permanece válido durante dibujo y pausa. Se elimina la copia completa hacia native_frame; el frontend nunca libera el buffer del driver. La ruta de vídeo software conserva su copia propia. Un readback fallido invalida los píxeles prestados y el cierre limpia la referencia.

En una salida 2x de 1280x576 se evita copiar unos 2,95 MB por cuadro. En este PC el tiempo mediano registrado de esa copia pasó de 0,6965 ms a 0. La captura del cuadro 300 coincide exactamente con 00.24 fuera del HUD. Esto no garantiza un aumento concreto de FPS en PS4. No se cambia el núcleo, los shaders ni el audio respecto de 00.24.

## Validación

- ASan/UBSan: migración de configuración, campos inválidos, aislamiento de resolución entre ROMs y persistencia.
- Frontend real: selección antes de cargar, conservación del modo activo al guardar otra resolución en pausa, aspecto, HUD y regreso a biblioteca.
- Vida útil del vídeo: reutilización en pausa, cambio de tamaño, fallo de readback, copia independiente de callbacks software y cero copias adicionales Vulkan.
- Vulkan validation: 2x durante 60 s/869 VI y nativa durante 30 s/1026 VI; ambas sin errores ASan ni de validación Vulkan. Persisten 40 avisos UBSan del núcleo previamente identificados, sin avisos en el rasterizador corregido o el escalador.
- PKG: 28 comprobaciones. Rendimiento y uso de los nuevos ajustes en PS4 pendientes de validación física.

Sin push ni release. Mantener 00.24 para comparación.
