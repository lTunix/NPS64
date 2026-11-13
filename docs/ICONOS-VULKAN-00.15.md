# Iconos del mando — Vulkan 00.15

La configuración y biblioteca 00.14 quedan guardadas en el commit local `7438201`, sin push.

La biblioteca, configuración y pausa muestran símbolos de cruz, círculo, cuadrado, flechas y botones L1/R1/L2/R2/L3/R3. Los símbolos tienen contornos suavizados y una máscara pequeña calculada una sola vez por icono. No se añaden archivos gráficos ni trabajo al renderizado del juego. Se mantienen etiquetas de acción; cuadrado indica **Buscar ROMs**.

Se mantienen el núcleo Vulkan y las opciones de renderizado de 00.14/00.13. La nueva versión únicamente cambia la presentación, textos y número de versión del frontend.

Validación: prueba de navegación, persistencia y aspectos con ASan/UBSan; ScePad; inspección visual de biblioteca, configuración y pausa a 1280×720. PKG verificado con 28 comprobaciones, extracción, icono y SFO; núcleo comparado por SHA-256 con 00.14. La prueba Vulkan nativa de 00.14 sigue como referencia, no se atribuye una nueva medición de rendimiento a esta edición visual. Pendiente comprobación visual en PS4. Sin publicación en GitHub.
