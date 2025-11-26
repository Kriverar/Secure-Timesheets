Secure Timesheet

Descripción del proyecto

Este proyecto es una mini aplicación de consola en C++ que simula un sistema de ponche para empleados y supervisores.

Instrucciones

- Registrar usuarios con rol employee o supervisor
- Iniciar sesión con username y password
- El empleado puede crear, editar y enviar timesheets
- El supervissor puede ver, aprobar o rechazar timesheets
- Guardar todos los datos en archivos de texto dentro de la carpeta de data
- Guardar un log de acciones en data/log.txt
  
Medidas de seguridad

- Validación de entradas
- Detección de patrones peligrosos
- Hashing de contraseñas con una implementación de SHA-256
- Archivo de log para auditoría de acciones

Ejemplo de entrada y salida

Menu Principal

1. Registrarse
2. Inciar Sesion
3. Salir

Elige una opcion: 1          <- (entrada del usuario)

=== Registro de Usuario ===

Username (no puede tener espacios y maximo 20 caracteres): juan

Password (no puede tener espacios y maximo 20 caracteres): 1234

Rol Employee (e) o Supervisor (s): e

Usuario registrado.


Menu Principal

1. Registrarse
2. Inciar Sesion
3. Salir

Elige una opcion: 2          <- (entrada del usuario)

=== Iniciar Sesion ===

Username: juan
-Password: 1234
-Bienvenido juan (employee)


=== Menu Employee ===

1. Crear timesheet (draft)
2. Ver mis timesheets
3. Editar timesheet en draft
4. Enviar (submit) timesheet
5. Cerrar sesion

Opcion: 1                   <- (entrada del usuario)

Horas trabajadas (1 a 24): 8

Fecha (yyyy-mm-dd): 2025-11-26

Descripcion corta de la tarea (sin espacios y maximo 30 caracteres): caja

Timesheet creado con ID 1

Autores

Dariel Vilches Negron | dvilches8644@interbayamon.edu

Kristian Rivera Rios  | krivera6833@interbayamon.edu

Universidad Interamericana de Puerto Rico – Recinto de Bayamón
