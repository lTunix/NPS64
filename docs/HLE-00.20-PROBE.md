# Primera prueba del backend gráfico HLE: 00.20

Este PKG prueba el rasterizador gráfico de PS4 con shaders de vértices y fragmentos Vulkan. No incluye el núcleo N64 ni permite abrir juegos. Es una prueba previa a conectar un renderer HLE de juegos; no es una conversión completa de NPS64 ni una corrección confirmada del bloqueo anterior.

La aplicación crea una imagen RGBA de 256 por 256 y un buffer de profundidad. Dibuja primero un triángulo rojo cercano y después otro azul más lejano en la misma posición. El rojo debe conservarse, demostrando que funciona la prueba de profundidad. Repite 64 envíos, espera cada fence y comprueba el centro y el fondo. No graba vkCmdDispatch ni inicializa paraLLEl-RDP. El controlador RADV/ACO y el puente GNM siguen compartidos con la ruta anterior; sus operaciones internas no equivalen necesariamente a las órdenes explícitas de la aplicación.

En PC se verificaron también los 65.536 píxeles: 20.808 rojos y 44.728 negros, sin píxeles azules. Lavapipe y la capa de validación Vulkan completaron la prueba sin errores. Esto no verifica la ejecución en PS4.

Al instalar y abrir, la pantalla debe mostrar un triángulo rojo sobre fondo negro y PASS. Círculo sale. Si se detiene, recuperar `/data/NPS64/hle-probe.log` y los archivos de captura GPU. El último frame registrado ayuda a localizar la submission que se detiene. La prueba aún puede bloquear la consola porque comparte el controlador experimental; un timeout no puede reiniciar la GPU.

El paquete usa el mismo título de aplicación NPSX00064 y sustituye la instalación actual. Conservar `out/NPS64-00.13.pkg`, que no se modifica, para volver al emulador estable.

Compilar dentro de la imagen nps64-gpu-builder: `bash scripts/build_hle_probe.sh`. Prueba PC: `bash scripts/test_hle_probe.sh`. Los shaders y las tablas de carga Vulkan se generan con `scripts/compile_hle_probe.py`.

Pendiente para juegos: seleccionar/adaptar un frontend HLE de display lists, aportar vértices y texturas N64, implementar combinadores y mezcla, gestionar framebuffer y conectar la presentación. Esta primera prueba conserva la copia CPU para mostrar el resultado; todavía no prueba presentación directa ni rendimiento de juegos.

Los renderers glN64/Rice del núcleo usan OpenGL. Piglet del ejemplo público OpenOrbis usa shaders PSSL precompilados; no se ha verificado aquí compilación de GLSL en PS4. Activar esos renderers sobre Vulkan o Piglet sin un backend compatible no es un port funcional. Esta prueba utiliza SPIR-V generado antes de empaquetar y evita esa suposición.
