# Continuidad del audio Vulkan 00.21

Corrige el relleno por VI del backend libretro compartido por los juegos. El cálculo anterior añadía ceros cuando un campo entregaba menos audio que su presupuesto DAC, sin compensarlo cuando el siguiente entregaba muestras de más. AI_LEN se entrega en pares de cuadros estéreo, por lo que diferencias pequeñas no representan necesariamente un hueco real.

El nuevo cálculo conserva el balance acumulado y permite dos cuadros de cuantización. Los campos sin DMA y los déficits mayores siguen generando el silencio necesario. Se conservan la fase fraccionaria del DAC, todo el audio DMA, los límites del acumulador y su drenaje de emergencia. El balance se reinicia al inicializar/cerrar o cambiar el divisor DAC. No cambia la temporización de interrupciones AI ni el renderizado.

## Pruebas

- Una regresión de 10000 campos cortos/largos y otra en orden inverso reproducen el relleno artificial con el backend anterior; ambas pasan con el nuevo, conservando un flujo continuo no nulo.
- Las pruebas del backend real pasan ASan/UBSan: DMA/poll, campos parciales y silenciosos, exceso, presupuesto DAC fraccionario, drenaje de emergencia y reinicio.
- Captura nativa de Majora cartucho USA: mismo tramo de 300 campos y mismas 159976 muestras estéreo DMA. Antes: 141 muestras de relleno; después: 3. Esto mide la reducción del relleno, no garantiza ausencia total de chasquidos audibles. El núcleo completo mantiene avisos UBSan preexistentes.
- En el archivo PS4 solo cambia audio_backend_libretro.o; los objetos de CPU y GPU permanecen idénticos a 00.20.
- Mantiene la salida FIFO directa de 00.20, buffer grande y filtro HLE sinc32. No corrige el desajuste de 50/60 Hz específico de Majora Collector’s Edition PAL.
- PKG verificado localmente y por hash tras transferencia FTP. El usuario confirmó en PS4 que desaparecieron los chasquidos. Persiste un ruido tenue de fondo, que queda como trabajo de calidad de audio separado.

No incluye ROMs, no modifica originales y no se publica un release.
