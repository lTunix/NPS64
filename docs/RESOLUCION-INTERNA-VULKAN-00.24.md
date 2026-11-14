# Resolución interna Vulkan 00.24

Checkpoint previo: 2d8aae5, versión 00.23 con suavizado confirmado en PS4.

Esta prueba solicita 2x al rasterizador paraLLEl-RDP: el doble por eje, cuatro veces más píxeles internos. Mantiene Mesa y los shaders incluidos; corrige además un cálculo de límites CPU en el núcleo. El perfil asíncrono existente prepara sus variantes generales nativa y ampliada antes de ejecutar el juego; se mantienen los límites de recursos y el rechazo de pipelines inseguros.

Se fija native texture LOD para conservar la elección de mipmaps de N64 y native TEX_RECT para elementos 2D. No hay reducción de resolución desde el núcleo. La presentación bilineal de 00.23 ahora acepta también la reducción parcial desde el scanout ampliado al rectángulo 4:3 o 16:9 de 720p. Continúan el AA y filtro VI cuando el juego tiene Filtro VI preciso.

Los controles de aspecto, biblioteca y audio permanecen con los ajustes confirmados. El cálculo conservador de rango X usaba abs(int) y desbordaba con INT_MIN, observado durante la sesión 2x de 60 s. Se ensancha a int64_t antes de obtener el valor absoluto, manteniendo el fallback al rectángulo scissor. Solo se recompila rdp_renderer.o; no se cambia código de shaders, temporización ni audio. La prueba extendida exige ahora ausencia de avisos UBSan de ese archivo y del filtro de presentación.

La mayor carga gráfica puede reducir FPS o provocar falta de audio por velocidad insuficiente: requiere comparar en PS4 con 00.23 antes de decidir valores predeterminados o una opción por juego. Las texturas originales conservan su información; aumentar resolución mejora principalmente geometría y bordes.

Validación: referencia matemática del escalador extendida a 1280x960 y 1280x576 hacia 960x720, ambas con ASan/UBSan; PKG con 28 comprobaciones; sesión nativa con Vulkan validation exige uso efectivo de 2x y scanout de anchura superior a 640. El núcleo completo conserva avisos UBSan previos ajenos a Vulkan. Evaluación física de rendimiento y estabilidad pendiente.

Sin push ni release. Mantener 00.23 como referencia.

La primera sesión nativa de 30 s alcanzó solo 108 VI, por debajo de los más de 180 exigidos. Se amplió la duración a 60 s manteniendo esa exigencia; no se relajaron los criterios de estabilidad ni los límites de recursos de PS4.

La sesión final con la corrección pasó los controles ASan/Vulkan y completó el límite de 60 s: [gpu-test] LIMIT reached elapsed_s=60.003 VI=809; no further retro_run until another load Sin avisos UBSan en rdp_renderer.cpp ni bilinear_scaler.h; persisten los avisos previos del resto del núcleo.
