# NPS64 00.73 / v0.73.0

## English

### Improvements since 00.72

- **Live internal resolution:** switch between native and 2× from **L3 + R3 → Resolution** without reloading the ROM. The game remains paused while GPU resources are prepared, then continues with the selected resolution. The choice is saved per game. A failed switch retains the previous setting.
- **Experimental widescreen:** the aspect-ratio row now offers **4:3**, **16:9 (stretched)** and, for compatible ROMs, **16:9 experimental**. Initial profiles cover **Super Mario 64 USA Rev 0** and **Super Smash Bros. USA Rev 0**. Selection is saved per game and can be enabled or disabled live. It is separate from the Cheats menu and checks header CRC, region, revision and ROM size.
- **Safer live patch restoration:** experimental widescreen preserves original values, waits for initialized memory and avoids overwriting locations replaced by the game. Unsupported ROM versions keep the normal display options. Ocarina of Time is not included in this initial catalog; some scenes or HUD elements may still have visual issues.
- **Options / Exit:** the library footer now shows the OPTIONS button and localized **Exit / Salir** action. Exiting uses the PS4 system service, correcting the crash encountered when returning through the C runtime. Options remains the Nintendo 64 Start button during gameplay.
- **ROM-startup fix:** corrects incomplete writable-memory coverage found during testing. Data and BSS alignment are preserved in the packaged executable, and packaging now verifies the entire original BSS region before accepting a PKG.
- **Documentation refresh:** English and Spanish READMEs have download, release notes, issue and PayPal buttons, plus a lightweight original pixel animation.
- Existing audio processing, GPU presentation, cheats, cover downloads and saves are retained.

### Install and compatibility

Install **NPS64-by-lTunix-00.73.pkg**. It updates the original **NPSX00064** application and preserves `/data/NPS64` saves and preferences. Existing per-game settings are read and upgraded to support the third aspect mode. Tag **v0.73.0** continues the original version sequence so older public builds detect the update. The updater downloads to `/data/pkg`; installation is manual.

Isolated **NPSV00064** test saves/settings are not migrated automatically. Experimental widescreen is limited to the listed ROM revisions and does not imply support for modified ROMs retaining their original header. Some cheats require restarting, and disabling them cannot undo every change already saved by the game.

For Majora’s Mask, use your own dump of the normal Nintendo 64 cartridge edition; the GameCube Collector’s Edition retains reported audio/performance issues. No games are included. Performance depends on the game and scene; constant 60 FPS is not guaranteed.

### Validation

Physical PS4 logs from test **00.38** confirm Smash USA loads and presents, changes from 2× to native and back to 2×, and returns to the library. The separate fatal log is empty. That session contains 2,949 VI over approximately 59 seconds, including pauses; it is not a benchmark for every game or a long-session stability guarantee.

Native Vulkan integration tests completed six live resolution changes each in Smash USA and Mario 64 PAL, with one ROM load per session and unchanged RAM/SRAM hashes during each switch. Frontend tests cover persistence, failed-change rollback, experimental widescreen, translations and exit. The original failing executable is rejected by the new memory-coverage check; the corrected executable passes. The public PKG is rebuilt with updater enabled, original application identity and no diagnostic session cap, then verified through ELF/OELF checks, package validation and payload extraction. Broader hardware compatibility testing remains ongoing.

## Español

### Mejoras desde 00.72

- **Resolución interna en vivo:** alterna entre nativa y 2× desde **L3 + R3 → Resolución**, sin recargar la ROM. El juego permanece pausado mientras se preparan los recursos de GPU y continúa con la resolución seleccionada. Se guarda por juego; si el cambio falla se conserva la selección anterior.
- **Widescreen experimental:** la fila de aspecto ofrece **4:3**, **16:9 (estirado)** y, en versiones compatibles, **16:9 experimental**. Los primeros perfiles son **Super Mario 64 USA Rev 0** y **Super Smash Bros. USA Rev 0**. Se activa y desactiva en vivo, se recuerda por juego y está separado del menú Trucos. Comprueba CRC de cabecera, región, revisión y tamaño de ROM.
- **Restauración del parche:** conserva los valores originales, espera memoria inicializada y evita sobrescribir direcciones reemplazadas por el juego. Las versiones no compatibles mantienen las opciones normales. Ocarina of Time todavía no está incluido; algunas escenas o elementos del HUD pueden presentar problemas visuales.
- **Options / Salir:** la franja inferior muestra el botón OPTIONS y la acción **Exit / Salir** según el idioma. La salida usa el servicio de PS4, corrigiendo el cierre con error del runtime. Durante la partida, Options conserva la función Start de Nintendo 64.
- **Corrección de arranque de ROM:** se corrige la reserva incompleta de memoria escribible detectada en las pruebas. El empaquetado conserva la alineación de datos/BSS y verifica toda la región original antes de aceptar el PKG.
- **README renovados:** botones de descarga, novedades, reportes y PayPal, con una animación pixelada original y ligera en ambos idiomas.
- Se conservan el procesamiento de audio, la presentación por GPU, los trucos, las carátulas y las partidas.

### Instalación y validación

Instala **NPS64-by-lTunix-00.73.pkg**: actualiza **NPSX00064**, conserva `/data/NPS64` y sigue la numeración original con **v0.73.0** para detectar el update. Los ajustes por juego anteriores se leen y actualizan al formato de tres opciones de aspecto. La descarga va a `/data/pkg` y la instalación es manual. Las partidas y ajustes de NPSV00064 no se migran automáticamente.

El widescreen experimental se limita a las revisiones indicadas; no certifica ROMs modificadas que conserven su cabecera. Los trucos pueden requerir reiniciar y desactivarlos no revierte todos los cambios guardados. Se recomienda la edición normal de cartucho de Majora’s Mask; Collector’s Edition conserva problemas reportados de audio/rendimiento. No se incluyen juegos. No se garantizan 60 FPS constantes.

Los logs de PS4 de **00.38** confirman carga de Smash USA, presentación, cambio 2× → nativa → 2× y regreso a biblioteca, con el log fatal vacío. La sesión registra 2.949 VI en unos 59 segundos, incluidas pausas; no equivale a una prueba prolongada o de todos los juegos. Las pruebas nativas verifican doce cambios de resolución entre Smash y Mario PAL sin recargar las ROM ni alterar los hashes de RAM/SRAM. El PKG público pasa la cobertura de memoria, validación de arranque, comprobaciones de paquete y extracción. Continúan las pruebas de compatibilidad en consola.
