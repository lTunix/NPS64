# NPS64 by lTunix — 00.68

Una versión en desarrollo dedicada a mejorar la biblioteca, facilitar las actualizaciones y añadir vibración a los juegos compatibles.

## Novedades desde 00.67

- **Interfaz HD:** biblioteca y pausa renovadas en 720p, texto suavizado, portada destacada, miniaturas y navegación por páginas.
- **Carátulas automáticas:** descarga desde Libretro y almacenamiento en `/data/NPS64/covers` para usarlas sin conexión. También admite portadas personalizadas en PNG de hasta 1024 × 1024 píxeles.
- **Actualizaciones automáticas:** busca nuevas versiones al abrir la aplicación y descarga el PKG en `/data/pkg`. Puedes buscar manualmente con **L2** en la biblioteca o desde **Buscar actualizaciones** en pausa.
- **Descargas verificadas:** muestra el progreso y comprueba el tamaño y la integridad antes de guardar el PKG. La instalación se realiza desde el instalador de la PS4.
- **Rumble Pak:** vibración activada por defecto para los cuatro jugadores en juegos compatibles, confirmada por pruebas en PS4 física.
- **Accesorio seleccionable:** desde pausa puedes elegir Rumble Pak, Memory Pak o ninguno para los cuatro mandos. La selección se conserva al volver a abrir la aplicación. La vibración se detiene al pausar, desconectar el mando o salir del juego.
- **Configuración compatible:** conserva los ajustes de versiones anteriores y añade la selección del accesorio.
- **README renovado:** enlace permanente a la última versión y checklist de funciones disponibles y pendientes.

## Descargar y jugar

Descarga **NPS64-by-lTunix-00.68.pkg**, instálalo y copia tus juegos en `/data/NPS64/games`. No se incluyen juegos.

Durante la partida, **L3 + R3** abre el menú de pausa. Para probar la vibración, selecciona **Accesorio: Rumble Pak** y usa un juego compatible, como GoldenEye 007.

**[Descargar siempre la última versión](https://github.com/lTunix/NPS64/releases/latest)** · [Funciones e instrucciones](https://github.com/lTunix/NPS64#funciones-actuales)

## Validación y desarrollo

El PKG pasó las 28 comprobaciones de empaquetado. Se verificaron localmente los motores de vibración, desconexión, arranque, pausa y estados guardados; el PKG arrancó GoldenEye en shadPS4. La vibración fue confirmada por el usuario en PS4 física.

Esta versión mantiene el renderizado por CPU. No incorpora Vulkan para uso general ni promete mejoras nuevas de FPS respecto a 00.67. Seguimos mejorando la compatibilidad y el rendimiento.

Las fuentes correspondientes y las sumas SHA256 acompañan al PKG. Los [clips anteriores de 00.67](https://github.com/lTunix/NPS64/blob/v0.67.0/docs/PRUEBAS-00.67.md) siguen disponibles como referencia; no se grabaron clips nuevos para esta versión.

[Reportar un problema](https://github.com/lTunix/NPS64/issues) · [Créditos](https://github.com/lTunix/NPS64/blob/v0.68.0/docs/CREDITS.md)
