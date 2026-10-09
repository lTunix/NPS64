# Prueba Vulkan 00.21

La captura real de 00.20 completa los 64 envíos, lee los píxeles y registra
PASS para triángulos y profundidad. El proceso se cierra después de ese
mensaje, antes de mostrar el resultado. No reproduce el bloqueo compute
de paraLLEl-RDP en el envío 37.

00.21 conserva los recursos Vulkan durante la pantalla de resultado en PS4,
evitando la ruta de destrucción que se ejecutaba inmediatamente después del
PASS. Es un aislamiento del cierre, no una causa demostrada del fallo.
La prueba nativa conserva la liberación completa y la validación Vulkan.
Los recursos retenidos pertenecen a este proceso y no crecen por frame.

Muestra una pantalla inicial antes de iniciar Vulkan y registra la vuelta
de la prueba y la primera presentación del resultado. El registro queda
en /data/NPS64/hle-probe.log. Todavía no ejecuta ROMs ni implementa el
renderer HLE de N64. Usa NPSX00064 y reemplaza la instalación anterior;
conservar NPS64 00.13 para jugar.

Validación en host: 64 envíos, todos los píxeles del framebuffer y ausencia
de errores de validación. La corrección del cierre requiere prueba en PS4.
