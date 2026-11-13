# Configuración y biblioteca Vulkan 00.14

Parte del checkpoint local `8247b26` (00.13). No cambia el núcleo ni el perfil de shaders, sincronización o renderizado Vulkan. No se publica en GitHub.

- Biblioteca HD de la rama principal, fuente suavizada y carátulas descargadas/caché local. Interfaz fija 1280×720 (16:9), independiente del juego.
- Izquierda abre la barra lateral; arriba/abajo selecciona Biblioteca o Configuración; X/derecha abre. L2 abre Configuración directamente y R2 los ajustes del juego seleccionado. Círculo vuelve.
- Configuración general: información superior oculta / FPS / FPS y detalles, tema, filtro VI, accesorio y ranura predeterminados. Actualizar carátulas vuelve a solicitar las imágenes de la biblioteca.
- Ajustes por juego: aspecto 4:3 o 16:9, filtro VI, accesorio para los cuatro mandos y ranura de estados. Restablecer elimina únicamente la configuración de ese juego. El aspecto 16:9 escala la imagen; no es un parche de campo de visión.
- L3+R3 mantiene los estados y acceso a biblioteca, muestra el aspecto actual y permite cambiarlo, cambiar filtro/accesorio/ranura y alternar la información superior. Esta última opción es global; el resto se guarda por juego.
- Juegos sin configuración arrancan en 4:3. Los ajustes utilizan una clave del contenido normalizado, independiente del nombre y del orden de bytes de la ROM. Cambiar un juego no cambia los demás.
- Generales: `/data/NPS64/vulkan-test-settings.cfg` (`NPS64CFG4`); juegos: `/data/NPS64/vulkan-test-game-settings/rom-*.cfg`. Migración de CFG1/2/3 conserva tema, filtro, accesorio y ranura; descarta el antiguo aspecto global para evitar aplicarlo a todos los juegos. SRAM y estados no se migran ni eliminan.
- La cola de carátulas deja de iniciar trabajos mientras se juega; una descarga ya iniciada puede terminar. Al volver a biblioteca se reanuda.

## Validación local

- `scripts/test_settings.sh`: ASan/UBSan, migración, persistencia, separación entre juegos, restablecer y rechazo de archivos inválidos.
- `scripts/test_frontend_settings.sh`: navegación real del frontend con núcleo nativo y dos ROMs sintéticas originales; configuración general, ficha por juego, carga, pausa, regreso, persistencia, barras 4:3/16:9 y los tres modos de información. Sin errores ASan/UBSan. Capturas de biblioteca/configuración verificadas visualmente. Prueba ScePad incluye R2.
- `scripts/test_covers.sh`: catálogo/regiones, caché, PNG truncado, dibujo recortado y cierre del trabajador.
- `scripts/test_vulkan_game_trace.sh`: más de 180 VI, shaders asíncronos, límite de sesión y bloqueo de reanudación; sin errores ASan ni validación Vulkan. Quedan 40 reportes UBSan fuera de Vulkan en el núcleo; esta ejecución completa no se considera limpia de UBSan.
- PKG: ELF/constructores, 28 comprobaciones, extracción idéntica, icono y SFO. Fuente HD y catálogo incluidos. Hash del núcleo idéntico al manifiesto 00.13.

Pendiente: probar el PKG 00.14 en PS4, navegación/carátulas, persistencia tras cerrar la app y rendimiento con información oculta/visible. Los resultados nativos no certifican el rendimiento físico. Texturas, antialiasing y calidad de audio quedan para la siguiente etapa.
