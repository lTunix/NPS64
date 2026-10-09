# 00.30: captura de Mario 64 en el exterior del castillo

El usuario confirmó Mario texturizado y movimiento en PS4 00.29, con partículas
y otros detalles pendientes. La ROM local de Mario 64 PAL permitió reproducir
un rechazo antes de abrir Vulkan: al pausar, el último VI podía contener cero
triángulos aunque la escena sí tuviera geometría. La versión anterior rechazaba
la prueba usando ese contador, sin capturar un frame nuevo. En la ejecución
local, este rechazo no se debía al límite de polígonos.

Se elimina esa comprobación previa y se capturan hasta seis VI para encontrar
el próximo frame con dibujos. La captura inicial se entrega al renderer sin
avanzar otro frame. Si todavía no hay geometría, se informa explícitamente y
se permite reintentar, pues aún no se han creado recursos Vulkan. Se mantiene
una sesión GPU por inicio de aplicación para conservar la política PS4 de
retención de recursos hasta salir.

El proveedor informa frame, contador de vértices, shutdown y tamaño VI cuando
falla. El renderer registra proveedores inválidos y límites de capacidad. Si
el logger principal falla, se intenta abrir `/data/NPS64/hle-error.log` y se
registra allí el resultado; eso no garantiza escritura si ambos archivos no
son escribibles. Los avisos distinguen ausencia de geometría de fallo Vulkan.

Prueba local reproducible con la ROM suministrada:

    BENCH_SM64=1 BENCH_FRAMES=2600 bash scripts/test_hle_game.sh "Super Mario 64 (Europe) (En,Fr,De).n64"

El backend pulsa Start y A para entrar al primer archivo y avanza hasta el
exterior. Ademas, `BENCH_EXPECT_NO_GEOMETRY=1` con 330 VI prueba dos intentos
en el logo 2D: deben dejar el motivo en el log y no consumir la sesion GPU.
La prueba positiva avanza hasta el
exterior del castillo. Antes de la corrección rechazó la prueba por contador
cero. Después completó 64 frames cambiantes; captura inicial con 599 triángulos,
36 texturas y 599 materiales listos. Los hashes de todos los frames y las
capturas inicial/final coinciden con la referencia sin caché. No hubo errores
Vulkan de validación en Lavapipe. No es una comprobación de fidelidad visual:
se observan huecos y detalles de renderizado todavía pendientes.

La prueba sintética de secuencia incluye un frame con 30.000 vértices y 10.000
triángulos, además de cambios de atlas/scissor y cancelación. Esto verifica el
límite existente en PC, sin aumentarlo a ciegas ni afirmar que cualquier escena
PS4 quede resuelta. OpenOrbis construye el PKG; Liverpool requiere prueba real.

Uso igual: escena 3D, L3+R3, Triángulo; Círculo sale de la secuencia/pantalla.
Probar especialmente en el exterior del castillo. Si aparece otro fallo,
conservar `nps64.log` y, si existe, `hle-error.log`. Continúan pendientes sprites,
partículas, clipping, framebuffer y modos de materiales no cubiertos.
