# Lectura SRAM — 00.16

Checkpoint de iconos 00.15: commit local `9fad7ac`, sin push.

Los nueve archivos SRAM existentes en PS4 tienen 296960 bytes, que corresponde al formato del núcleo. Se descargó una copia local de cada uno con SHA-256 para verificar la lectura; no se alteraron los originales.

La comprobación anterior dependía de `stat().st_size`. Ahora valida los bytes reales del archivo y exige EOF exacto, sin depender del tamaño de metadatos de libc. La carga de SRAM se realiza en un buffer temporal: si el archivo está truncado, sobran bytes o falla la lectura, no modifica la memoria del núcleo y mantiene bloqueada la sobrescritura del original. Un archivo ausente se distingue de uno ilegible. No hay conversiones ni borrado de partidas.

El registro muestra resultado, tamaño esperado y ruta de cada carga. Al volver a biblioteca el mensaje indica el resultado del guardado, en lugar de conservar el aviso de inicio. Un fallo real conserva una advertencia, sin recomendar retirar archivos por FTP.

Pruebas ASan/UBSan: tamaño de stat deliberadamente erróneo; archivos completos, cortos, largos, vacíos o ausentes; memoria intacta ante error; lectura de las nueve copias PS4. Pruebas existentes de almacenamiento/estados y navegación del frontend. PKG comprobado y núcleo Vulkan idéntico al anterior. Pendiente verificar carga y guardado en PS4 con 00.16. Audio y Mario Tennis se investigan por separado.
