# NPS64 00.75 / v0.75.0

## English

### Interface improvements

- **3D cartridge library:** animated carousel with cover art on the cartridges, perspective, lighting and floor reflections.
- **Refreshed visual design:** dark backgrounds, application emblem, rounded panels, colored action cards and updated controller button hints.
- **Library navigation:** D-pad navigation, L1/R1 paging and shortcuts to global and per-game settings. Action-card sizing accommodates English and Spanish labels.
- **Update dialog:** an available release offers **Download** or **Later** before downloading. Progress, cancellation, completion and retry actions are displayed in the interface, with an availability indicator in the library.
- **Verified downloads:** packages still require the expected size and SHA256 before becoming ready in `/data/pkg`. Installation remains manual; the interface does not install PKGs automatically.
- **Cover rendering:** reference-counted access lets the cartridge renderer use cached cover art while the asynchronous cover worker continues operating.
- Settings, pause and cheat screens retain their functions with the refreshed styling. English remains the default interface language, with Spanish available in settings.

### Installation and validation

Install **NPS64-by-lTunix-00.75.pkg** to update the original **NPSX00064** application. Tag **v0.75.0** continues the updater's public version sequence. Public saves and preferences remain under `/data/NPS64`; isolated **NPSV00064** test data is not migrated automatically. No games are included.

The user confirmed that the new interface works correctly on PS4. Frontend/settings, updater and asynchronous cover regression tests pass, including download confirmation before fetching the package. The public package is checked for ELF/OELF mapping, package integrity, extracted payload, icon and SFO. Diagnostic frame capture and GPU timestamps are excluded. The emulation core is retained from 00.74; this release does not claim an additional game FPS improvement.

### Remaining work

Army Men still has intermittent stutters and effect-heavy drops at 2×. Conker's intro performance and demanding Mario Tennis replays/particles remain pending. Continue broader and longer-session testing; constant 60 FPS is not guaranteed. For Majora’s Mask, the standard Nintendo 64 cartridge edition remains recommended over the GameCube Collector’s Edition because of its reported audio/performance issues.

## Español

### Mejoras de interfaz

- **Biblioteca de cartuchos 3D:** carrusel animado con carátulas sobre los cartuchos, perspectiva, iluminación y reflejos en el suelo.
- **Diseño renovado:** fondos oscuros, emblema de la aplicación, paneles redondeados, tarjetas de acciones de colores e indicadores de botones actualizados.
- **Navegación de biblioteca:** cruceta, cambio de página con L1/R1 y accesos a ajustes generales y por juego. Las tarjetas se adaptan a las etiquetas en inglés y español.
- **Ventana de actualización:** ofrece **Descargar** o **Más tarde** antes de descargar una versión disponible. Muestra progreso, cancelación, resultado y acciones de reintento, con un indicador de disponibilidad en la biblioteca.
- **Descargas verificadas:** se comprueban el tamaño y SHA256 antes de dejar el PKG listo en `/data/pkg`. La instalación sigue siendo manual; la interfaz no instala PKGs automáticamente.
- **Carátulas en el carrusel:** el acceso con referencias permite utilizar las imágenes de caché mientras sigue trabajando la descarga asíncrona.
- Los menús de ajustes, pausa y trucos conservan sus funciones con el nuevo estilo. El idioma predeterminado sigue siendo inglés y se puede seleccionar español en ajustes.

### Instalación y validación

Instala **NPS64-by-lTunix-00.75.pkg** para actualizar la aplicación original **NPSX00064**. El tag **v0.75.0** mantiene la secuencia del actualizador. Se conservan partidas y preferencias públicas en `/data/NPS64`; los datos de prueba **NPSV00064** no se migran automáticamente. No se incluyen juegos.

El usuario confirmó el funcionamiento correcto de la nueva interfaz en PS4. Pasan las regresiones de interfaz/ajustes, actualizador y carátulas asíncronas, incluida la confirmación antes de descargar el paquete. El PKG público verifica el mapeo ELF/OELF, integridad, extracción, icono y SFO. Excluye capturas diagnósticas y consultas de tiempo GPU. Conserva el núcleo de emulación de 00.74; esta versión no declara una mejora adicional de FPS de los juegos.

### Pendientes

Army Men conserva tirones y bajones con muchos efectos a 2×. Siguen pendientes el rendimiento de la intro de Conker y las repeticiones/partículas exigentes de Mario Tennis. Continuar las pruebas prolongadas y de más juegos; no se garantizan 60 FPS constantes. Para Majora’s Mask se mantiene la recomendación de la edición normal de cartucho de Nintendo 64 frente a GameCube Collector’s Edition, por sus problemas reportados de audio/rendimiento.
