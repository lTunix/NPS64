> Resultado en PS4: sigue bloqueando en submission 37, marcador 8. La captura nueva contiene el shader reducido (s_endpgm a 0x25c4; 104 VGPR). Las mascaras son coherentes y la reproduccion completa en Lavapipe vuelve a pasar. La 00.19 no resuelve el fallo. En la raiz recibida falta nps64.log; boot.log no identifica la version del paquete.

# NPS64 00.19 â€” prueba Vulkan

Ruta nueva de rasterizado sin texturas, habilitada en el perfil experimental PS4. El procesador comprueba todos los estados del lote: si alguno usa texel0, texel1, texel1 en pipeline, LOD o modo copia, conserva el shader original. La variante conserva geometrÃ­a, combinadores, profundidad, mezcla y escritura de memoria; elimina las rutas de texturas mediante constantes en el cÃ³digo GLSL. Usa el mismo layout explÃ­cito de recursos de Granite que el shader original.

La captura 00.18 alcanza el marcador 8 y se detiene antes del 9, en la espera posterior al tercer despacho. Un marcador posterior al despacho no demuestra que el shader haya terminado. Las mÃ¡scaras de las 1.200 tiles activas coinciden y los datos capturados permiten reproducir ese despacho. El estado de rasterizado no usa texturas. Los avisos NOT DONE de marcadores anteriores significan que la misma posiciÃ³n de memoria ya fue sobrescrita, no que se omitieran esas escrituras.

ValidaciÃ³n: reproducciÃ³n con Lavapipe y capa de validaciÃ³n Vulkan; ambos shaders completan el despacho y producen exactamente los mismos 16 MiB de VRAM y 4 MiB de memoria auxiliar. Los datos provienen de la captura posterior al bloqueo: esta comparaciÃ³n demuestra equivalencia para esos datos, no reconstruye necesariamente el estado anterior al despacho. Pruebas funcionales de vÃ­deo, conversiÃ³n RGBA, estados, dos sesiones y retorno a CPU pasan en PC.

El PKG es experimental. No estÃ¡ probado en PS4 y no se puede afirmar que resuelva CE-36329-3. Los lotes con texturas siguen usando el shader original y el controlador mantiene los cambios de sincronizaciÃ³n y captura anteriores. Conserva el PKG 00.13 de CPU que funciona. Si falla, recuperar nps64.log y gpu-hang.txt junto con los binarios gpu-hang-* de /data/NPS64, manteniendo juntos los archivos de la misma ejecuciÃ³n.

Compilar: scripts/build_vulkan_test.ps1. Prueba reproducible de captura: tests/replay_gpu_capture.py (requiere los archivos de captura en la raÃ­z y los shaders generados). El paquete de fuentes no incluye ROMs ni capturas de memoria del juego.

Mario Kart PAL completó 360 frames en Lavapipe; el registro confirma selección de la ruta sin texturas. Los píxeles del juego en los frames 300 y 330 coinciden exactamente con la referencia anterior (HUD excluido). No equivale a una prueba de estabilidad o rendimiento de PS4. El PKG 00.13 conserva su SHA256 original.
