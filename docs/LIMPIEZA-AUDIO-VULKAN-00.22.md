# Calidad de voces Vulkan 00.22

Checkpoint estable: 390d31f, versión 00.21 validada en PS4 sin chasquidos.

Se investiga el ruido tenue residual por separado. Una prueba con la rutina real alist_resample y una voz silenciosa mantiene idénticos sus cuatro valores de historial y cuatro muestras de entrada. Solo cambia el contenido vecino de trabajo. La interpolación original produce 0 en ambos casos; la extensión de 32 taps cambia de 0 a -1128. Su ventana ampliada consume muestras que esa llamada no garantiza pertenezcan a la voz. La prueba es un diagnóstico reproducible del filtro existente, no una garantía sobre todo el audio de todos los juegos.

La 00.22 desactiva esa extensión en el perfil de prueba y usa la interpolación del microcódigo original. Mantiene el remuestreo de salida sinc32 a 48 kHz, la salida FIFO directa, el buffer grande y el balance acumulado que eliminó los chasquidos. No modifica ni enmascara el audio mediante una puerta de ruido o ecualización. El núcleo y todos sus objetos son idénticos a 00.21; solo cambia la selección de la opción en el frontend.

Validación: prueba de aislamiento con ASan/UBSan; captura nativa de 600 VI de Majora cartucho con PCM crudo y remuestreado idénticos a la referencia de interpolación original; 28 comprobaciones de PKG. El usuario confirmó en PS4 que desapareció el ruido tenue residual y autorizó el checkpoint. El núcleo nativo completo conserva avisos UBSan preexistentes.

La salida de 48 kHz no recupera detalle ausente de los samples comprimidos del juego original. La mejora buscada aquí es evitar contaminación adicional del emulador. Sin push ni release.
