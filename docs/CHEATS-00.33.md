# Cheats por juego — prueba 00.33

Basada en el checkpoint Vulkan 00.32 validado en PS4. No es un release público ni cambia el compositor, el driver o el audio.

## Biblioteca incluida

Se incluye la base `mupencheat.txt` de Mupen64Plus (Gent, GPL-2.0-or-later; procedencia y SHA256 en `assets/mupencheat.source.json`). El compilador genera 32.619 registros, incluidas variantes con opciones, de 558 secciones de ROM. Se omiten 29 entradas con formatos incompatibles/mal formados; no se presentan como códigos funcionales. El emparejamiento usa CRC1, CRC2 y país de la cabecera de ROM, normalizando `.z64`, `.v64` y `.n64`. No se elige un código por parecido del nombre ni se copia una selección a otra ROM. Las modificaciones de ROM con cabeceras originales pueden conservar esos identificadores; los códigos deben corresponder a esa versión.

Los códigos no se activan automáticamente. Los nombres originales están en inglés; la interfaz es bilingüe. Las variantes de un mismo cheat son excluyentes y se permiten hasta 32 activos por ROM.

## Controles

- Biblioteca: R2 → ajustes del juego → Cheats / Trucos.
- Partida: L3 + R3 → Cheats / Trucos.
- X: activar/desactivar. L1/R1 o izquierda/derecha: páginas. Cuadrado: desactivar todos. Círculo: volver.
- R2 durante la partida: botón GameShark, necesario para códigos que lo requieren. No modifica los botones N64 ya asignados.

La selección persiste por hash de contenido de ROM. En la prueba: `/data/NPS64/vulkan-test-cheats/rom-….cfg`; en una compilación pública: `/data/NPS64/cheats/rom-….cfg`. No se cambian el formato de los ajustes o de SRAM ni el archivo ROM. Algunos cheats requieren reiniciar; desactivarlos no deshace todo lo que el juego haya hecho o guardado. Las notas completas originales permanecen en `assets/mupencheat.txt`.

## Núcleo y rendimiento

El parser anterior reservaba el tamaño exacto de un token hexadecimal y escribía el terminador fuera de la reserva. Se sustituye por un parser acotado a 128 pares, sin reservas por token, que valida completamente antes de agregar códigos. Desactivar un código deja intactos los valores de restauración del núcleo, en lugar de reconstruir/destruir los códigos. Se comprueban tamaño real de RDRAM y alineación de accesos de 16 bits; se maneja el fallo de reserva al crear nodos de cheats.

Los cambios del archivo estático se limitan a `libretro.o` y `cheat.o`. Los códigos seleccionados se configuran al iniciar y al cambiar la selección; no se lee la base ni se escribe la selección en cada cuadro. El botón GameShark solo actualiza el estado de entrada del núcleo. Los costes de los códigos activos dependen de esos códigos.

## Validación

- ASan/UBSan: emparejamiento CRC/región, los tres órdenes de bytes, desactivación inicial, variantes excluyentes, persistencia y aislamiento, rollback ante fallo de escritura, límite de 32 activos y rechazo de entradas inválidas.
- Parser real parcheado: múltiples pares, código desactivado sin destruir datos, límite de 128, rechazo de códigos incompletos o largos y botón GameShark.
- Helpers reales de RAM: escritura de 8/16 bits, condiciones, invalidación del código JIT y límites de RAM de 4 MiB.
- Interfaz con y sin actualizador: selección antes de iniciar, carga de selección al jugar, menú desde pausa, desactivar todos y retorno. Capturas revisadas del menú de cheats; se comprueba que los nuevos elementos caben en la pausa.
- Smash USA a 2×, Vulkan por software, sesión de 120 segundos: inyección de un código de prueba en RAM y desactivación con restauración del valor anterior; FullSync conservado. Sin errores ASan ni validación Vulkan, con los 43 avisos UBSan conocidos del núcleo fuera de Vulkan. El núcleo aplica cheats ordinarios después de los primeros 60 VI; la prueba de escritura se realiza en VI 70.
- Cuadro 300 comparado con 00.32: cero diferencias en 819.200 píxeles fuera del HUD. Los archivos del compositor, plataforma, escaladores y cola de audio conservan sus hashes de 00.32.
- PKG: 28 comprobaciones de empaquetado, SFO/icono y carga extraída; base y aviso de licencia incluidos.

La prueba de funcionamiento de códigos concretos en PS4 queda pendiente del usuario. Conservar el PKG 00.32 como referencia. Sin push ni release.
