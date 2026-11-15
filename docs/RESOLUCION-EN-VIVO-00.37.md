# Resolución en vivo y botón Options — prueba 00.37

Basada en la prueba 00.36 confirmada por el usuario. Continúan pendientes de validación en PS4 los cambios de esta prueba. No hay push ni release nuevo.

## Interfaz

La biblioteca incluye el icono **OPTIONS** y la acción **Exit / Salir**, según el idioma, junto a los demás controles inferiores. Conserva la salida por el servicio de sistema de 00.36. La máscara del botón se genera una vez y se reutiliza; no se añaden archivos ni asignaciones durante el dibujo.

En **L3 + R3 → Resolución**, X, izquierda o derecha alternan **nativa / 2×** sin cerrar la ROM ni reiniciar el núcleo. El menú muestra la resolución aplicada, sin «próximo inicio». La selección se guarda por juego después de un cambio exitoso; si el cambio falla se conserva el modo anterior. La imagen nueva aparece al continuar la partida. Cambiar formato 4:3/16:9 y sus perfiles experimentales sigue siendo independiente.

## Rasterizador

La API del núcleo solo admite factor 1 o 2 y exige una frontera sin comandos RDP parciales. Drena el anillo y la línea de tiempo, conserva FullSync y espera el trabajo del dispositivo. No reconstruye el procesador RDP, la ROM, el recompilador o el contexto Vulkan.

Se conservan RAM canónica, RAM oculta, TMEM, teselas, constantes, framebuffer y estado de comandos. Antes de ampliar se sincroniza la RAM y se reservan recursos en handles temporales: un fallo de reserva mantiene los recursos activos. Se amplían únicamente los búferes de trabajo que lo necesitan. Su capacidad máxima se retiene para las siguientes alternancias, liberándose al cerrar el juego; nativa rasteriza y despacha realmente a 1×, sin renderizar a 2× para reducir después.

Al entrar a 2× se copian los valores canónicos a cada plano de muestras, incluidos color, profundidad y cobertura oculta. No se usa un valor centinela para invalidar colores: también se conservan 00000000 y FFFFFFFF. Se espera la copia con barreras y fence antes de activar el factor. Se preparan los shaders generales de ese factor durante la pausa, conservando las especializaciones asíncronas. Si esa preparación no mantiene disponible la ruta general, se restablecen las capacidades anteriores y la interfaz no cambia de modo.

La imagen VI previa y los búferes de VideoOut permanecen vivos hasta su sustitución habitual. No se registran nuevas pantallas ni se retiran los búferes visibles al cambiar resolución.

## Validación local

- Frontend real: aplicación inmediata, persistencia por juego, retroceso de selección fallida y ausencia de ejecución/reinicio del núcleo desde el ajuste. Se conservan los controles de widescreen, idioma, pausa, actualizador y salida de 00.36.
- Revisión visual del pie de biblioteca en inglés y español; OPTIONS y Exit/Salir completos y dentro de pantalla.
- Smash USA y Mario 64 PAL: 840 VI por sesión, seis alternancias por juego, doce cambios en total. Una sola carga de ROM por sesión. El hash completo de los 8 MiB de RAM y el de SRAM permanecen idénticos antes/después de cada llamada; se conservan sus direcciones y el contador del núcleo no avanza durante la llamada.
- Dimensiones VI observadas en escenas válidas: Smash 640×240 / 1280×480; Mario PAL 640×288 / 1280×576. Los intervalos de blanking pueden seguir entregando imágenes 1×1, como en el funcionamiento normal del VI.
- Sin errores ASan, UBSan ni de validación Vulkan detectados en esas dos integraciones. El frontend está instrumentado; el núcleo nativo utilizado es release -O2, sin instrumentación completa del núcleo. Son pruebas nativas con Vulkan por software y no predicen el rendimiento de PS4.
- PKG aislado NPSV00064, APP_VER 00.37, con 28 comprobaciones de empaquetado, contenido extraído, icono y SFO verificados. La numeración pública permanece 00.72.

Repetir la integración con un núcleo nativo preparado y recompilado: `NPS64_RESOLUTION_ROM=/ruta/privada/al/juego.z64 NATIVE_GPU_DIR=build/native-gpu-asan bash scripts/test_live_resolution_game.sh`. Preparación: `scripts/patch_vulkan_live_resolution.py`; el constructor GPU de PS4 y el constructor nativo la aplican a copias de trabajo, nunca al vendor original.

En PS4, alternar durante una partida y confirmar imagen, profundidad, texturas, audio al continuar, guardados y regreso a biblioteca. La primera transición puede tardar brevemente mientras se reservan recursos y preparan shaders; las pruebas de consola determinarán ese coste y la continuidad posterior.

## Confirmación posterior de 00.38 en PS4

Después de corregir la cobertura de BSS del ejecutable, los registros físicos de Smash USA confirman dos cambios en la misma sesión: 2× → nativa en VI 177 y nativa → 2× en VI 403, sin otra carga de ROM. La sesión llega a VI 2.949 y vuelve a biblioteca; no hay registro fatal. Esta muestra confirma esa ruta en consola y no amplía por sí sola la validación a todos los juegos o sesiones largas.
