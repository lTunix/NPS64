# 00.17: captura del bloqueo, sin corrección confirmada

El usuario confirmó que 00.16 también se bloquea. RSRC1 del tercer
despacho cambia de 002c035f a 002c031f: el límite de ACO se aplicó,
pero no resuelve la espera posterior al rasterizador (marcador 8).

00.17 conserva esa ruta y amplía únicamente el reporte posterior al
bloqueo. No añade despachos ni escrituras de diagnóstico en la GPU.
gpu-hang.txt incluye todos los BO vivos, sus direcciones y tamaños,
y hasta 128 bytes de las direcciones de descriptores de cada despacho.
También guarda hasta 16 KiB por shader en archivos
gpu-hang-shader-<direccion>.bin dentro de /data/NPS64.
Las lecturas se limitan al tamaño restante del BO y exigen mapeo CPU.
Los bytes pueden incluir código posterior al shader; no son una prueba
de que la GPU ejecutara ninguna instrucción concreta.

Paquete: out/NPS64-00.17-vulkan-test.pkg. Es diagnóstico experimental;
se espera que pueda repetir el bloqueo y congelar la consola. El PKG
00.13 estable sigue intacto. No hay validación física local.

Si se ejecuta, conservar gpu-hang.txt, nps64.log y TODOS los archivos
gpu-hang-shader-*.bin de esa ejecución. Obtenerlos por FTP después de
recuperar la consola. No borrar SRAM ni ROMs. Este reporte permitirá
inspeccionar instrucciones GFX7 y descriptores del rasterizador.

Validación local: helper real de captura contra buffers simulados,
ASan/UBSan, direcciones fuera de rango y no mapeadas, archivo binario
exacto; parche idempotente; compilación cruzada y checks del PKG.
Estas verificaciones no demuestran estabilidad de Vulkan en PS4.
