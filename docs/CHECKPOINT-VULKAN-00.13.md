# Checkpoint local: Vulkan Game Test 00.13

Estado de referencia antes de optimizar texturas, antialiasing y audio.
El usuario confirma en PS4 una mejora general y ausencia de los tirones
anteriores. GoldenEye baja ocasionalmente de 60 a unos 55 FPS; la causa
precisa de esas caídas no se ha medido.

La versión Vulkan de prueba es **00.13**, título **NPSV00064**. La versión
CPU de referencia continúa siendo **00.69**; no confundir ambas numeraciones.

## Comportamiento conservado

- Rasterizador split paraLLEl-RDP, con shader general en GPU y variantes
  especializadas compiladas en segundo plano, sin omitir dibujos.
- `ACO_DEBUG=nosched`, shader combinado desactivado y compilador con pila
  de 8 MiB. Guards: SGPR <=96, VGPR <=128, scratch/spills=0, LDS <=512.
- Readback con memoria CPU cached, tres buffers VideoOut y pausa a 600 s.
- Sin restaurar binarios RADV ni precargar recetas aprendidas de otros
  arranques. Los archivos existentes se preservan.
- Partidas de prueba aisladas y registros de rendimiento/cierre.
- Icono actualizado y herramientas de construcción, diagnóstico y pruebas.

## Evidencia y artefacto

La comparación nativa sin historial coincide en 503 hashes de imagen con
la ruta síncrona. Los 24 pipelines capturados pasan la compilación protegida
BONAIRE virtual: máximo 88 SGPR/64 VGPR, sin scratch ni spills. La validación
nativa no sustituye la ejecución PS4; permanecen 39 avisos UBSan del core
ajenos al código Vulkan en esa sesión.

El PKG pasa validación ELF, constructores, extracción, icono, SFO y 28 checks.
Artefacto local: `out/NPS64-Vulkan-Game-Test-00.13.pkg` (no incluido en Git).
SHA256: `f03cb3448ee50ab01b36ee9432f8348fa59de1e3e8836224ae81303636c694f8`.

La entrega FTP fue verificada descargando los bytes remotos. El commit
conserva fuentes y scripts, excluyendo SDK, dependencias descargadas, ROMs,
partidas, capturas, logs y binarios generados.

## Pendiente

Texturas, antialiasing y calidad de audio no se modifican en este checkpoint.
El arranque de Mario Tennis VC sigue pendiente. No hay push, tag ni release.
El historial técnico y las limitaciones están en
[VULKAN-TEST-2026-10-29.md](VULKAN-TEST-2026-10-29.md).
