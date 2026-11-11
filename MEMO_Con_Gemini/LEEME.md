# MEMO con Gemini

Este paquete contiene el firmware para la MAR4018 en 8 bits y un servidor local para Windows. Usa el mismo cableado del programa MEMO_Interfaz_Inicial. La pantalla compone la imagen en PSRAM y envía solo los bloques que cambian. El Pomodoro sigue siendo local.

## Preparar el servidor

1. Si descargaste el repositorio de GitHub, copia Configuracion.ejemplo.h como Configuracion.h en esta carpeta. Si usas el ZIP de MEMO con Gemini, extrae el paquete completo. Conserva el nombre de la carpeta MEMO_Con_Gemini.
2. Crea una clave en https://aistudio.google.com/apikey. Revisa la cuota y las condiciones de tu proyecto, incluyendo el uso de audio. Cada pregunta grabada se envía a Google.
3. Abre Iniciar-servidor.cmd. Si indica que falta Node.js, instala Node.js 20 o posterior desde https://nodejs.org/ y vuelve a abrirlo.
4. Introduce la clave de Gemini y elige un código privado de al menos 12 caracteres, distinto de la clave de Gemini. Deja abierta la ventana. Si Windows solicita acceso de red para Node.js, habilítalo solo para redes privadas. Usa el servidor dentro de una red de confianza.
5. Anota la dirección IPv4 que aparece para la red que comparten el PC y MEMO. No uses 127.0.0.1.

## Cargar en Arduino IDE

1. Abre MEMO_Con_Gemini.ino. En la pestaña Configuracion.h, escribe el nombre y la clave del Wi-Fi, sustituye la IP del servidor y pon el mismo código privado en TOKEN_MEMO. Conserva :8787/chat al final de la URL. La clave de Gemini se introduce únicamente en el servidor.
2. Selecciona ESP32S3 Dev Module y Herramientas > PSRAM > OPI PSRAM. Si tu IDE ofrece Flash Size, selecciona 16 MB para el N16R8.
3. Desconecta la alimentación externa antes de programar por USB, como en la prueba anterior. Conecta el ESP32 por USB, selecciona el puerto COM y pulsa Subir. Deja el joystick quieto al arrancar.
4. Abre Chatbot y pulsa el botón dedicado (o el centro del joystick). Cuando diga GRABANDO, haz una pregunta breve. Tienes tres segundos. La respuesta aparece como texto.

## Si algo falla

- SIN WIFI: revisa el nombre y la clave de la red. Deja unos segundos al ESP32 para conectarse.
- SERVIDOR MEMO NO DISPONIBLE: verifica la IP, que el PC y el ESP32 estén en la misma red y que el servidor siga abierto. Desde otro equipo en esa red puedes abrir http://IP_DEL_PC:8787/salud para comprobar que es accesible.
- REVISA EL TOKEN: el código de Configuracion.h debe coincidir con el del servidor.
- REVISA LA CLAVE O EL MODELO: revisa la clave y que tu proyecto tenga acceso a gemini-3.8-flash. El servidor permite cambiar el modelo mediante GEMINI_MODELO antes de iniciarlo.
- CUOTA DE GEMINI AGOTADA: revisa los límites del proyecto en Google AI Studio.
- Si el monitor serie indica que falta PSRAM, revisa que esté seleccionado OPI PSRAM. El firmware usa dibujo directo como alternativa.

Esta versión muestra respuestas en pantalla. El calendario en el robot, el historial de conversación y la salida de voz de Gemini quedan para una etapa posterior.
