# Proyecto Final PS 2026-2027

## Servidor Concurrente TCP de Calificaciones IMDb — "A Knight of the Seven Kingdoms"

### Integrantes

- Carolina Pérez Juvier
- Dora Leonor Rodríguez Galpert

### Descripción

Servidor concurrente TCP en C que escucha en el puerto `6666`. Cada conexión es atendida por un proceso hijo creado con `fork()`. El hijo consulta la base de datos empaquetada en `libmonitor.a` y devuelve la información del episodio solicitado.

### Archivos

- `Makefile`: automatiza la compilación.
- `monitor.c` / `monitor.h`: base de datos y utilidades de formato.
- `libmonitor.a`: biblioteca estática generada por `make all`.
- `server.c` / `server`: servidor concurrente TCP.
- `client.c` / `client`: cliente de prueba.
- `screenshot/`: capturas de funcionamiento.

### Compilación

```bash
make all
```
