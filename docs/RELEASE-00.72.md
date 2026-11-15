# NPS64 00.72 / v0.72.0

## English

### Improvements since 00.71

- GPU presentation: scaling, image filtering, colour conversion and borders run in a persistent compute compositor, with direct VideoOut presentation on supported PS4 buffers. Removes the previous full-frame CPU readback/scaling path when direct presentation is available; keeps a fallback when buffer registration is unsupported.
- Better smoothness and lower presentation overhead, particularly at 2× internal resolution. Native and 2× remain selectable per game. Hardware testing confirmed a substantial improvement; frame rate still depends on the game and scene.
- Experimental per-game GameShark cheats with an included catalog covering 558 ROM sections and 32,619 entries/variants. Matching uses ROM header CRC and region. Access from library game settings or L3 + R3; selections persist per game and start disabled. Up to 32 active entries; R2 is the GameShark button. Unsupported/malformed entries are excluded.
- Fixes intermittent corruption of the in-game pause menu and performance overlay, including repeated frames in Super Mario 64. Publishes only the CPU-written overlay regions before display, preserving direct GPU presentation.
- Fixes the library status message getting stuck on “already up to date”: ROM loading, save and other application messages now take priority; an explicit update check selects update status again.
- Retains the existing audio processing, save compatibility, English/Spanish interface, automatic covers and per-game settings.

### Install and compatibility

Install **NPS64-by-lTunix-00.72.pkg**. This updates the original **NPSX00064** application, keeps `/data/NPS64` saves/settings and uses tag **v0.72.0** for update detection. The updater downloads to `/data/pkg`; installation remains manual. Isolated NPSV00064 test saves are not migrated automatically.

Cheats are experimental: some require restarting the game; disabling them cannot undo all changes already made or saved by a game. No ROMs are included. For Majora’s Mask, use your own dump of the standard Nintendo 64 cartridge edition; the GameCube Collector’s Edition still has reported audio and performance issues. No draw-distance patches are included. Constant 60 FPS is not guaranteed.

### Validation

The direct GPU presentation path was tested on physical PS4 hardware before this release. Automated checks cover compositor pixel equivalence, buffer lifetime/fallback, overlay visibility, frontend navigation/status, cheats and updater behavior. The release PKG is rebuilt with the original identity, updater enabled and diagnostic session cap removed; packaging, extracted payload and version fields are verified. Long-session and broader game compatibility testing remain ongoing.

## Español

### Mejoras desde 00.71

- Presentación por GPU: escalado, filtrado de imagen, conversión de color y bordes mediante un compositor persistente, con presentación directa a VideoOut cuando los búferes de PS4 lo permiten. Se evita la lectura y el escalado de cada cuadro completo en CPU; se conserva la ruta alternativa si el registro no está disponible.
- Mayor fluidez y menor coste de presentación, especialmente a resolución interna 2×. Se mantienen nativa y 2× por juego. Las pruebas en consola confirmaron una mejora considerable; el rendimiento depende del juego y la escena.
- Trucos GameShark experimentales por juego: catálogo incluido con 558 secciones de ROM y 32.619 entradas/variantes, emparejado por CRC y región. Acceso desde ajustes del juego o L3 + R3, selección persistente y desactivada inicialmente, hasta 32 activos y botón GameShark en R2. Se excluyen entradas incompatibles o mal formadas.
- Corrección de la corrupción intermitente del menú de pausa y las métricas, incluidos los cuadros repetidos de Mario 64. Se publican las regiones de interfaz antes de mostrarlas, conservando la presentación directa por GPU.
- Corrección del mensaje de biblioteca bloqueado en «ya tienes la última versión»: los avisos de carga, guardado y otras acciones toman prioridad. Una nueva comprobación explícita vuelve al estado de actualización.
- Se conservan el procesamiento de audio, las partidas, la interfaz inglés/español, las carátulas automáticas y los ajustes por juego.

### Instalación y compatibilidad


Instala **NPS64-by-lTunix-00.72.pkg**: actualiza **NPSX00064**, conserva `/data/NPS64` y sigue la numeración original con **v0.72.0** para detectar el update. El actualizador descarga a `/data/pkg`; la instalación es manual. Las partidas de NPSV00064 no se migran automáticamente.

Los trucos son experimentales y algunos requieren reiniciar; desactivarlos no revierte todos los cambios o guardados. No se incluyen juegos. Se recomienda un volcado propio del cartucho normal de Majora’s Mask: Collector’s Edition sigue teniendo problemas reportados de audio/rendimiento. No hay parches de distancia de dibujado ni garantía de 60 FPS constantes.

La presentación directa fue validada en PS4 física antes del release. Las comprobaciones automáticas cubren imagen, búferes, interfaz, cheats, mensajes y actualizador; el PKG público conserva la identidad original y elimina el límite diagnóstico. Continúan las pruebas de sesiones largas y compatibilidad.
