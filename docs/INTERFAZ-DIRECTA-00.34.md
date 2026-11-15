# HUD y pausa sobre presentación directa — prueba 00.34

El usuario observa corrupción intermitente del menú en partida y de las métricas, especialmente en Super Mario 64. Los juegos funcionan bien: se mantiene el núcleo, los cheats, el driver, el shader de composición, el escalado y FullSync de 00.33.

## Corrección aislada

1. Un callback de vídeo libretro con `data == NULL` significa repetir el cuadro anterior. El frontend borraba `gpu_prepared` antes de devolver, perdiendo la ruta de la imagen válida. Ahora conserva también `gpu_fresh`, la geometría y el almacenamiento anterior. Hay una regresión que llama a la función real con estado GPU preparado y comprueba que permanece intacto.
2. La salida directa usa memoria cacheada para CPU, y HUD/menús se escriben desde CPU después de terminar el shader. El cambio de pantalla solo ejecutaba `SFENCE`, que no publica por sí solo las líneas modificadas de una caché WB hacia un consumidor nativo externo como VideoOut. Se añaden preparación de líneas antes de escribir y publicación/invalidez con `CLFLUSH` y `MFENCE` antes del flip. El efecto visual en PS4 se debe confirmar con el usuario; la simulación local comprueba la propiedad de visibilidad y límites de las regiones.

Solo se registran las regiones que la interfaz modifica: HUD superior, pausa, texto inicial y menú de cheats. La franja de métricas completa usa 3.520 de las 57.600 líneas de caché de un cuadro (cada línea contiene 16 píxeles). Con HUD oculto y sin otra interfaz no se vacía el cuadro completo. Los menús pueden tocar regiones mayores mientras la partida está pausada. No se vuelve a la lectura completa GPU→CPU ni se añade una copia de imagen de juego.

El registro `[overlay-cache]` informa líneas medias, tiempo de preparación y publicación cada 60 presentaciones directas. Su coste real debe contrastarse en la PS4, junto con las métricas de cadencia anteriores.

## Validación

- Simulación de vista CPU cacheada y memoria de pantalla: renovación de contenido previo, publicación de HUD/pausa, conservación de los píxeles de juego y de los bordes de líneas parciales, recorte y límite de 3.520 líneas para HUD completo.
- Frontend real con el perfil de presentación: conservación del cuadro duplicado y navegación de configuración, idiomas, cheats y pausa.
- Backend PS4 con servicios simulados: retención de búferes visibles/pendientes, cambios entre interfaz y juego y retirada antes de liberar.
- La ejecución local con la copia privada de Mario 64 y el empaquetado se registran en `build/overlay-34/`. Vulkan local por software no permite demostrar coherencia con el motor de pantalla físico ni predecir FPS de PS4.

Se conserva 00.33 para comparar y 00.32 como punto anterior validado. No hay push ni release.

Resultado local: Mario 64 PAL completó 90 segundos y 1.870 VI, incluidos el límite de prueba y el retorno a biblioteca. Sin errores ASan ni validación Vulkan; se conservan 40 avisos UBSan del núcleo fuera de Vulkan. El PKG pasó las 28 comprobaciones de empaquetado. Estos resultados no equivalen a confirmar la corrección visual ni el coste de caché en la PS4 física.
