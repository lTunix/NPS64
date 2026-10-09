# Vulkan 00.16: prueba de asignación de registros

00.15 falla en PS4 con marcador 8. Las esperas de spans y tiles se
completan; queda bloqueada la espera del rasterizador principal.
Una escritura después de DISPATCH confirma recepción del comando,
no finalización del shader. Los marcadores comparten una dirección:
los valores anteriores aparecen sobrescritos, no necesariamente omitidos.

00.16 conserva el shader directo de tiles y limita a 96 los SGPR
direccionables que ACO puede asignar en PS4/GFX7. Esto deja margen
para registros especiales y redondeo; el compilador reasigna o derrama
registros. No se recorta RSRC1 después de compilar.
La traza anterior reserva 112 SGPR en el rasterizador (RSRC1 002c035f).
Este exceso es una hipótesis de trabajo, no una causa demostrada.

Paquete: out/NPS64-00.16-vulkan-test.pkg. Es experimental y todavía
puede congelar la consola. 00.13 estable permanece disponible.
No hay PS4 conectada para validar el cambio. La compilación y las
comprobaciones del paquete no demuestran estabilidad en consola.

En una nueva traza comprobar RSRC1 del tercer despacho y el marcador
posterior a su espera (9). Si vuelve a fallar, conservar gpu-hang.txt
y el registro de arranque. No borrar ROMs ni SRAM para esta prueba.
