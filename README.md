Versión Inicial 0.0.1
Funcionalidades

    >Guardado de Credenciales y URLs en Memoria NVS:El sistema guarda las credenciales de la red Wi-Fi y las URLs del servidor en la memoria no volátil (NVS) para su persistencia.

    >Servidor Web:El servidor web se ejecuta a través de HTTP y sirve la página web del cliente desde el sistema de archivos SPIFFS.La página web es accesible mediante la dirección IP actual del dispositivo.

    >Task de Botones con Debounce:Implementa una task dedicada para la lectura de los botones.Aplica técnicas de debounce para asegurar la lectura precisa de los botones, evitando falsos positivos.

    >Inicialización del Display:El sistema inicializa y configura el display LCD para mostrar mensajes y estados del dispositivo.

    >Conexión a Wi-Fi:El dispositivo se conecta a la red Wi-Fi utilizando las credenciales guardadas en la NVS.En caso de pérdida de conexión, intenta reconectarse automáticamente.

    >Conexión a Firebase:El dispositivo se conecta a Firebase para sincronizar y actualizar datos.Actualiza la información en Firebase cada 60 segundos, manteniendo los datos en el servidor siempre actualizados
