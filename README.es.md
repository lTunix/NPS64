**[English](README.md) · Español**

<p align="center">
  <img src="assets/readme-icon.png" alt="NPS64 by lTunix" width="480">
</p>

<h1 align="center">NPS64 by lTunix</h1>

<p align="center">
  <a href="https://www.paypal.com/donate/?hosted_button_id=XMVFTFEQKX8Q8">
    <img src="https://img.shields.io/badge/Donar-PayPal-0070BA?style=for-the-badge&amp;logo=paypal&amp;logoColor=white" alt="Donar con PayPal">
  </a>
</p>

<p align="center">Preservando videojuegos de Nintendo 64 en PlayStation 4.</p>

**NPS64 es una aplicación en desarrollo** para volver a disfrutar juegos de Nintendo 64 en PS4. Su objetivo es contribuir a la preservación de videojuegos y mantener vivas sus historias, mundos y experiencias.

Todavía estamos mejorando la compatibilidad, el rendimiento y la experiencia de uso. Algunos juegos pueden presentar errores o funcionar más lento de lo esperado.

## Descargar

**[Descargar la última versión](https://github.com/lTunix/NPS64/releases/latest)** · [Ver versiones anteriores](https://github.com/lTunix/NPS64/releases)

Descarga el archivo **PKG**. Este enlace siempre abre la versión más reciente. Las versiones publicadas están en desarrollo; tus pruebas nos ayudan a mejorarlas.

## Comenzar a jugar

1. Instala el PKG en una PS4 preparada para ejecutar aplicaciones de la comunidad.
2. Copia tus juegos de Nintendo 64 en `/data/NPS64/games`.
3. Abre **NPS64 by lTunix** y selecciona un juego con **X**.

Se admiten archivos `.n64`, `.z64` y `.v64`. La aplicación no incluye juegos: utiliza tus propias copias.

La biblioteca descarga carátulas desde Libretro y las guarda en `/data/NPS64/covers` para usarlas sin conexión. Si una portada no aparece, puedes copiar allí un PNG de hasta 1024 × 1024 píxeles con el mismo nombre del juego, cambiando únicamente su extensión por `.png`.

Durante una partida, pulsa **L3 + R3** para abrir el menú de pausa. Desde allí puedes guardar o cargar una partida y volver a la biblioteca.

Los juegos compatibles con Rumble Pak pueden hacer vibrar el mando. Está activado por defecto para los cuatro jugadores. En el menú de pausa, **Accesorio** permite elegir Rumble Pak, Memory Pak o ninguno; la elección se guarda. La vibración se detiene al pausar o volver a la biblioteca.

## Idioma

La **00.71** usa **inglés por defecto** (la versión publicada 00.70 usa español). En **Settings → Language**, elige **English** o **Espanol**; el cambio se aplica inmediatamente y se guarda al cerrar la app. Los ajustes antiguos conservan sus opciones y comienzan en inglés. Este ajuste cambia la interfaz de NPS64, no el idioma de las ROMs.


## Trucos (prueba experimental 00.33)

La prueba incluye la base GameShark de Mupen64Plus y busca códigos por CRC de cabecera y región de la ROM, no por nombre de archivo. Abre **Ajustes del juego → Trucos** desde la biblioteca o **Trucos** en la pausa **L3 + R3**. **X** activa/desactiva, **L1/R1** cambia de página, **Cuadrado** desactiva todos y **Círculo** vuelve. Durante la partida, **R2** funciona como botón GameShark para los códigos que lo requieran.

Todos comienzan desactivados. La selección se guarda por ROM; las variantes son excluyentes y se permiten hasta 32 activos. Se incluyen opciones de parámetros y se omiten 29 entradas incompatibles o mal formadas. Algunos códigos requieren reiniciar el juego; desactivarlos no revierte todos los cambios, incluidos los guardados por el juego. No se modifica el archivo de la ROM. Esta función aún no forma parte del PKG público 00.71.

## Actualizaciones

Al abrir la aplicación, NPS64 busca nuevas versiones en GitHub y las descarga automáticamente en `/data/pkg`. También puedes elegir **Buscar actualizaciones** en Configuración o en el menú de pausa.

El menú muestra el progreso de la descarga. Cuando termine, instala el nuevo PKG desde el instalador de tu PS4.

## Funciones actuales

- [x] Biblioteca y menú de pausa en HD, con texto suavizado.
- [x] Carátulas automáticas, guardadas para usarlas sin conexión.
- [x] Carátulas personalizadas en PNG.
- [x] Juegos en formato `.n64`, `.z64` y `.v64`.
- [x] Soporte para hasta cuatro mandos.
- [x] Rumble Pak en juegos compatibles, confirmado en PS4.
- [x] Selección de Rumble Pak, Memory Pak o ningún accesorio.
- [x] Guardado de partidas y ocho ranuras de estados por juego.
- [x] Pausa y regreso a la biblioteca.
- [x] Selección de imagen 4:3 o 16:9 y filtro de imagen.
- [x] Contador de FPS durante la partida.
- [x] Búsqueda automática y manual de actualizaciones.
- [x] Descarga de actualizaciones en `/data/pkg`, con comprobación de integridad.
- [ ] Instalación de actualizaciones desde la propia aplicación.
- [ ] Mejorar la compatibilidad y el rendimiento en juegos exigentes.
- [x] Renderizado Vulkan con Mesa RADV, probado en PS4 física.
- [x] Resolución interna nativa o 2×, guardada por juego.
- [x] Configuración desde la biblioteca y selección de información/FPS en pantalla.

Desde la **00.70**, el PKG público incorpora el renderizador Vulkan probado en PS4. Incluye mejoras de fluidez, presentación a 2× y continuidad del audio. El rendimiento depende del juego y la escena; no se garantizan 60 FPS constantes.

La biblioteca y los menús se muestran en 16:9. El aspecto 4:3 o 16:9, la resolución interna y los demás ajustes de partida se guardan por juego; puedes cambiarlos desde sus ajustes o con **L3 + R3**.

## Compatibilidad: Majora’s Mask

En las pruebas de PS4, **The Legend of Zelda: Majora’s Mask de GameCube Collector’s Edition** presentó audio lento, distorsionado y cortes, además de peor fluidez que la edición de cartucho. **Se recomienda usar un volcado propio de la versión normal de Nintendo 64 (cartucho)**. Las correcciones de audio mejoran la reproducción general, pero no garantizan resolver las particularidades de Collector’s Edition.

Mario Tennis también recibió correcciones de arranque, comprobadas durante las pruebas. La aparición de objetos a distancia conserva el comportamiento del juego; esta versión no incorpora parches de distancia de dibujado.

## Paso desde las versiones de prueba Vulkan

La 00.70 actualiza la aplicación original **NPSX00064** y utiliza `/data/NPS64/saves`, `/data/NPS64/settings.cfg` y `/data/NPS64/game-settings`. Las versiones de prueba **NPSV00064** usaban carpetas `vulkan-test-*`; esas partidas y configuraciones se conservan y no se migran automáticamente. Antes de trasladarlas manualmente, respalda ambas carpetas y cierra la aplicación.

## Mira cómo funciona

Los [clips de la versión 00.67](docs/PRUEBAS-00.67.md) muestran varios juegos con los FPS visibles. Están grabados en un emulador de PS4 en PC; el rendimiento en una consola puede ser diferente.

## Ayúdanos a mejorar

Si encuentras un problema, [abre un reporte](https://github.com/lTunix/NPS64/issues) indicando el juego, la versión de NPS64 y qué ocurrió. Una imagen o un video también ayuda.

## Preservación y comunidad

Preservar un videojuego también significa conservar la posibilidad de jugarlo. NPS64 busca aportar a ese esfuerzo, con un proyecto abierto que pueda seguir creciendo gracias a la comunidad.

Creado por **lTunix**, sobre el trabajo de los proyectos y colaboradores reconocidos en [Créditos](docs/CREDITS.md).

[Licencia](LICENSE) · [Código fuente](https://github.com/lTunix/NPS64)
