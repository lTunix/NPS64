# Cierre al cargar ROM — corrección 00.38

El usuario informa un cierre al cargar ROM con la prueba 00.37. Se descargaron los registros de la consola por FTP. Dos fallos registrados coinciden: SIGSEGV, RIP relativo `0x1e7866` y dirección de escritura `0xd004210`. La descompilación del ELF exacto de 00.37 sitúa esa instrucción en `volkGenLoadInstance`, guardando el resultado de una búsqueda de función en la variable global `vkCreateDevice`. Ocurre durante el arranque de Vulkan, antes de cualquier cambio de resolución en vivo.

La variable tiene dirección relativa `0xcc04210`. El ELF original reserva hasta `0xcc044c4`, pero el OELF convertido para PS4 solo declara memoria escribible hasta `0xcc03958`. La dirección de la escritura queda en la siguiente página de PS4, sin reserva. El conversor no había incluido íntegramente el hueco de alineación entre `.data` y `.bss`. El control anterior verificaba alineación y solapamiento de segmentos, pero no la cobertura completa de BSS; por eso no rechazó este paquete. Una ejecución nativa no atraviesa esa conversión.

## Corrección

- El script de enlace incluye el relleno de alineación dentro de `.data` y alinea y completa `.bss` a páginas PS4 de 16 KiB. También conserva los símbolos COMMON dentro de BSS.
- El verificador compara las extensiones reales de las secciones NOBITS asignadas del ELF original con los segmentos escribibles del OELF; excluye TLS, cuya asignación es independiente. Si falta memoria, el empaquetado se detiene.
- La prueba 00.37 archivada falla con la comprobación nueva: reproduce exactamente la reserva incompleta.
- En 00.38, tanto el ELF como el OELF reservan la región escribible hasta `0xcc08000`. La variable que falló ahora está en `0xcc05210`, dentro del rango. El núcleo tiene el mismo SHA-256 que 00.37: no se altera el rasterizador, el cambio de resolución en vivo o el procesamiento de audio.

La prueba 00.38 pasa la cobertura de BSS, la comprobación de arranque diferido y las 28 comprobaciones del PKG, extracción, icono y SFO. La validación del arranque de ROM y de las alternancias en PS4 queda pendiente del usuario. Se conserva 00.36 como paquete anterior funcional. Sin push ni release.

## Validación posterior en PS4 para 00.73

Se recuperó una sesión física 00.38 con Smash USA: primera imagen visible en VI 43, cambio a nativa en VI 177 (6,662 ms) y regreso a 2× en VI 403 (176,575 ms), con dimensiones 640×240 y 1280×480 respectivamente. El juego finaliza en VI 2.949 tras 59,078 s, vuelve a biblioteca y registra la limpieza de salida. El log fatal 00.38 está vacío. Los tiempos de cambio se midieron durante la pausa y no se extrapolan a otros juegos. Se conserva esta evidencia antes de preparar el release público 00.73; las validaciones pendientes anteriores describen el estado de la primera entrega del PKG de prueba.
