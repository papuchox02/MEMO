# MEMO
MEMO es un robot de escritorio que mediante ciertas funciones, aporta a disminuir la procrastinación del usuario. Las funciones principales de MEMO, son: Pomodoro, para la temporización de bloques de estudio; Chatbot, para consultar información puntual del estudio; Agente de calendario, para la organización documentada para la organización. 

## Prototipo en el navegador

Abre [MEMO.html](MEMO-local/MEMO.html) para probar el calendario, Pomodoro y la navegación con joystick. El chatbot y la voz están simulados. Consulta [las instrucciones](MEMO-local/LEEME.txt) para más detalles.

## Interfaz inicial del ESP32-S3

Abre [MEMO_Interfaz_Inicial.ino](MEMO_Interfaz_Inicial/MEMO_Interfaz_Inicial.ino) en Arduino IDE y selecciona **ESP32S3 Dev Module**. Esta versión está preparada para el montaje con pantalla MAR4018 de 800 × 480 en modo de 8 bits. Incluye inicio, menú con joystick, Pomodoro de 25 minutos y una prueba que graba y reproduce audio con el pulsador dedicado.

Calendario en el robot, conexión a la API del chatbot y sincronización con iPhone están pendientes.
