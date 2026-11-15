# Mensajes de biblioteca — prueba 00.35

En producción, el estado no vacío del actualizador tenía prioridad incondicional sobre el mensaje de aplicación. «Ya tienes la última versión» ocultaba los avisos de lectura de ROM y guardado, aunque esos avisos se actualizaban internamente.

El frontend selecciona explícitamente el canal mostrado. Al iniciar o pedir actualizaciones se muestra el estado de GitHub. Cualquier `notify` de aplicación (carga, guardado, errores, ajustes o cheats) selecciona el mensaje local y un resultado posterior del worker no lo sobrescribe. Solo una nueva petición explícita vuelve al canal del actualizador. Si el estado está vacío, se conserva el mensaje local. La misma selección se utiliza en biblioteca HD, biblioteca básica y configuración. La pausa ya tiene filas separadas y las conserva.

No se añaden temporizadores ni se bloquea/interrumpe el actualizador. Tampoco cambia el núcleo, el renderizador, el audio, la presentación directa ni la corrección de caché de 00.34.

La prueba 00.35 se construye con `--gpu-present --test-updater` para reproducir el escenario de producción en NPSV00064, con la identidad y carpetas de prueba. La numeración pública sigue siendo 00.71; esta prueba no se publica ni reemplaza NPSX00064.

Validación: frontend real con actualizador simulado siempre no vacío; lectura y guardado visibles; un resultado asíncrono posterior no desplaza el aviso local; una petición manual vuelve a mostrar la comprobación; estado vacío no borra el aviso. También pasa la variante sin actualizador, y se conserva la navegación de idiomas, cheats, pausa y ajustes. El PKG pasa 28 comprobaciones de empaquetado. La confirmación de uso en PS4 queda a cargo del usuario. Sin push ni release.
