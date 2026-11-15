# Pantalla ancha experimental — prueba 00.36

Prueba local basada en el release 00.72. No reemplaza el release público ni cambia el renderizador Vulkan, FullSync, la resolución interna o el audio.

## Uso

En los ajustes de un juego y en la fila **Aspecto** de la pausa L3 + R3:

- **4:3**: proporción original.
- **16:9 (estirado)**: presentación ancha sin cambiar la proyección del juego.
- **16:9 experimental**: parche de proyección y presentación ancha, únicamente en las versiones compatibles.

X o derecha avanza e izquierda retrocede. En la pausa, izquierda/derecha sobre esta fila ya no cambia la ranura de guardado. La selección se aplica al continuar la partida, sin cerrar o recargar la ROM, y se recuerda por juego. No aparece una entrada adicional en Cheats; «desactivar todos los trucos» no desactiva el formato. La biblioteca y el menú mantienen su proporción 16:9.

## Primer catálogo

| Juego | CRC1 / CRC2 de cabecera | País / revisión | Tamaño |
|---|---|---|---|
| Super Mario 64 USA | 635A2BFF / 8B022326 | 45 / 00 | 8 MiB |
| Super Smash Bros. USA | 916B8B5B / 780B85A4 | 45 / 00 | 16 MiB |

Se normalizan los órdenes de bytes z64, v64 y n64. No se selecciona un parche por nombre. Las versiones PAL, japonesas y otras revisiones quedan excluidas. Una ROM modificada que conserve cabecera y tamaño puede conservar estos identificadores; no se certifica compatibilidad con modificaciones.

Los códigos de proyección proceden de las publicaciones originales de gamemasterplc: [Mario USA](https://sites.google.com/site/gamemasterplcscheatcodes/nintendo-64/super-mario-64-ntsc-u) y [Smash USA](https://sites.google.com/site/gamemasterplcscheatcodes/nintendo-64/super-smash-bros-ntsc-u). Se incluyen solo los códigos Widescreen, no códigos de desbloqueo, velocidad ni los enablers de hardware GameShark.

## Aplicación y reversión en vivo

Cada perfil se registra una sola vez en un índice interno reservado (4096), fuera del catálogo visible (0–4095). Activar/desactivar cambia únicamente su estado habilitado y conserva las capturas de valores originales; no reconstruye todos los cheats ni reinicia el núcleo. La aplicación y restauración ocurren en la siguiente VI al continuar.

Las escrituras del perfil esperan memoria inicializada (no capturan 0000/FFFF como original). Después solo se escriben valores que coincidan con el original capturado o el parche aplicado. Al desactivar se restaura una dirección únicamente si aún contiene el valor del parche: no se pisa una superposición que el juego haya reemplazado. Se conservan las comprobaciones de límites, alineación e invalidación del recompilador. Los cheats normales conservan su comportamiento anterior.

Los estados incluyen RAM y pueden contener modificaciones: la prueba de restauración de valores de estados cubre una misma sesión con originales ya capturados; no garantiza limpiar un estado parcheado cargado como primera imagen de RAM en una sesión nueva. Para la primera validación, iniciar normalmente y usar las partidas del juego. La ampliación de vista puede exponer recorte de objetos, elementos 2D o HUD estirados; su corrección y el coste en PS4 se validan por juego.

## Validación

- Ajustes GAME3 con tres formatos y migración de GAME1/GAME2, que solo admitían 0/1; aislamiento por ROM y rechazo de valores fuera de rango.
- Perfiles: CRC, país, revisión, tamaño, tres órdenes de bytes y sintaxis acotada.
- Motor real de aplicación/restauración extraído del núcleo parcheado, con ASan/UBSan: ambos perfiles, veinte ciclos, cambios repetidos durante pausa, RAM sin inicializar, reemplazo de overlays y valores de estados dentro de la misma sesión.
- Frontend real: ciclo de tres formatos en ambas direcciones, persistencia, conservación de ranura y catálogo, exclusión de ROM no compatible; se mantienen HUD, pausa y estados del actualizador.
- Integración con ROM privada Smash USA: 780 VI y cambios desde la pausa en VI 300, 420, 540 y 660. Las cinco medias palabras coinciden con el parche al activar y con los originales al desactivar. Vulkan con validación local; no equivale a rendimiento ni validación visual de PS4.
- PKG aislado NPSV00064, versión 00.36, con actualizador de prueba y compositor directo. Las partidas de NPSX00064 quedan separadas. Confirmación visual y de rendimiento en PS4 pendiente.

Pruebas: `bash scripts/test_settings.sh`, `bash scripts/test_widescreen.sh`, `bash scripts/test_overlay_cache.sh`, `bash scripts/test_cheats.sh`. La integración requiere un núcleo nativo preparado con `patch_cheats.py` recompilado y un volcado privado de Smash USA; ver `scripts/test_widescreen_game.sh`.
