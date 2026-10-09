# 00.29: material de Mario y caché de texeles

El log de PS4 00.28 confirma 64 frames distintos, sin bloqueos de GPU. En la
primera captura hay 813 triángulos: 717 con textura 32x32 y material listo,
96 sin textura soportada. El material del modelo usa mux FC30B261:44664924,
ciclo cero, SHADE no nulo y primitiva blanca. Por tanto, la iluminación no
está simplemente ausente de la captura. No tenemos aquí la imagen ni la ROM
de Mario 64 para reproducir toda la escena.

Se añade una ruta GLSL directa para ese mux: mezcla SHADE y PRIM por TEXEL0,
incluido alfa. Es equivalente a (PRIM-SHADE)*TEXEL0+SHADE y evita la selección
dinámica de entradas del combinador en cada fragmento. Pruebas Vulkan usan el
mux exacto del log y colores conocidos; una textura negra conserva SHADE y
una blanca devuelve PRIM. **La causa definitiva de la apariencia negra en
Liverpool y la corrección visual siguen pendientes de probar en la consola.**

Se decodifican los texeles de cada tile una vez por grupo consecutivo de
triángulos. Cualquier comando de estado, carga o sincronización invalida la
caché antes de ejecutar el handler. Se mantienen ocho tiles de hasta 64x64;
no se crea memoria por frame. Materiales, UV y asociación de comandos siguen
capturándose por dibujo. Angrylion continúa rasterizando como referencia.

Nuevos registros: `texels decoded/cache_hits/last_rejected` describe ahorro
de decodificación y dimensiones/formato/nivel del último rechazo; `pixels`
informa brillo del framebuffer, y `sequence elapsed` mide el tiempo total
de la prueba, incluidas CPU, Vulkan y presentación. Los FPS de pausa no son
FPS del renderer. Los modos y tamaños aún no soportados mantienen diagnóstico.

Docker/Lavapipe valida el material exacto de Mario y regresiones del combinador,
atlas, profundidad, F3DEX2 y secuencia de 120 frames. Mario Kart PAL ejecuta dos
secuencias de 64 frames, con caché y sin ella: compara hashes de todos los
framebuffers y bytes de las capturas inicial/final; exige menor número de
decodificaciones y ausencia de errores Vulkan. Son pruebas de PC; OpenOrbis
construye el paquete PS4, sin emular Liverpool.

En esa secuencia local: 896 decodificaciones con caché frente a 106.426 sin
caché; los 64 hashes y ambas capturas coinciden. Eso mide trabajo evitado,
no el aumento de FPS que conseguirá la PS4.

Uso igual a 00.28: escena 3D, L3+R3, Triángulo. Círculo termina la secuencia;
al finalizar queda el último frame. Una prueba por inicio de aplicación.
Confirmar color de Mario, fondo y velocidad percibida; conservar el nuevo
`/data/NPS64/nps64.log`. No se promete juego continuo ni aceleración completa.
