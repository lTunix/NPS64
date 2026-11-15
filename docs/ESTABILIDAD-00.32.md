# Revisión de estabilidad — 00.32

Fecha: 2026-11-15. Punto de retorno: `152d090`, rama `perf/gpu-presentation-00.72`. No se cambia el ejecutable ni se propone otra optimización tras esta revisión.

## Evidencia en PS4

El usuario confirma mejora considerable, cercana a 60 FPS sostenidos en los juegos probados, y considera satisfactorio el resultado. Se descargaron los registros por FTP de solo lectura. La sesión 00.32 contiene tres juegos ejecutados consecutivamente a 2× con filtro VI y FullSync activo. Todos registran `enabled=1 direct_videoout=1`, registro de VideoOut exitoso y cero intervalos de lectura antigua (`[vulkan-times]`). La copia CPU del compositor se registra como 0,000 ms.

| Juego | Duración observada | Cadencia media de presentación | Mayor episodio lento después de 15 s |
|---|---:|---:|---:|
| Super Smash Bros. (USA) | 53.8 s | 59.12/s | 175.1 ms |
| Legend of Zelda, The - Majora's Mask (USA) | 61.4 s | 58.99/s | 120.9 ms |
| 007 - GoldenEye (USA) | 199.0 s | 59.73/s | 125.6 ms |

La cadencia se mide entre retornos de presentación: no cuenta fotogramas únicos mostrados físicamente ni sustituye una captura de pantalla sincronizada. Hay cuadros repetidos según el juego. Los 15 segundos solo separan una ventana inicial; no demuestran que toda compilación/carga termine ahí. Un episodio lento puede abarcar varios cuadros y no equivale a un bloqueo.

No se encontraron errores de envío GPU, timeouts de VideoOut ni fallos de retirada. El registro fatal de 00.32 estaba vacío. Los tres juegos terminaron con cierre de trabajadores; volver a registrar los búferes en el juego siguiente también funcionó. Estas observaciones no prueban ausencia de fallos en todas las ROM o en sesiones largas.

## Tirones y audio

Persisten picos breves: los máximos de intervalos individuales después de 15 s fueron 137,3 ms en Smash, 88,0 ms en Majora y 125,6 ms en GoldenEye. Los picos iniciales son mayores. No se atribuyen a un componente concreto sin una medición causal adicional. Los autoguardados registrados duraron 0,79–0,84 ms y no escribieron SRAM sin cambios; no explican esos picos grandes.

Los contadores de recuperación de audio aumentan durante la sesión (son acumulativos, también atraviesan cargas y cambios de juego). El total final de salida en recuperación fue 1.329,979 ms y el máximo 174,958 ms. No permiten atribuir todos los cortes a la partida ni medir calidad subjetiva; el procesamiento de audio no cambió respecto al punto anterior.

## Revisión del código

Se revisaron las barreras y la espera de fence antes del acceso CPU, la conservación de FullSync, la reutilización de cada búfer solo después del flip siguiente a su última presentación, la restauración del canvas CPU y la sustitución de la imagen externa antes de desregistrar/liberar su memoria. El cambio de imagen invalida la caché de descriptores para evitar reutilizar handles destruidos. El rechazo del registro conserva la ruta anterior en PS4. Los fallos de sincronización detienen la ruta; no se elimina una espera para ganar FPS.

No se encontró un nuevo defecto que justifique cambiar el código validado. Los ensayos locales anteriores cubren igualdad de píxeles, cambios de aspecto/filtro, limpieza de HUD, cambios entre menú CPU y presentación externa, retirada y rechazo de registro. Permanecen 43 avisos UBSan conocidos del núcleo fuera de Vulkan en las pruebas locales; esta revisión no los convierte en una ejecución UBSan limpia.

## Criterio para conservar este punto

Mantener el ejecutable probado y el checkpoint. Quedan como ampliación de validación: sesiones de 30–60 minutos, repetición de guardar/cargar estados, cambios de aspecto/filtro en pausa y varios ciclos biblioteca→juego→biblioteca. No hay un nuevo fallo confirmado en esta muestra. No afirmar «totalmente estable» solo con unos cinco minutos de tres juegos. No se modifica el recompilador ni se añaden más cambios sin evidencia de necesidad. Sin push ni release.
