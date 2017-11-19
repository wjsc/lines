Manual de usuario

El trabajo práctico esta conformado por varios archivos:
Archivos “.C”
Archivos “.H”
Archivos “.CFG” (Archivos de configuración)
Archivo Make

Al ejecutar el archivo Make, pulsando ./make, se compilan los 5 procesos (celular, central de celulares, cargador, telefono de línea y central de teléfonos de línea) con sus bibliotecas de variables y funciones tanto globales como locales.

Para ejecutar los diferentes procesos se lo debe hacer de la siguiente manera:
“./nombreDelProceso <archivoDeConfiguracion>”

Nombres de los procesos:
Estos son los nombres de los procesos ejecutables una vez compilados:
Celular				cel
Central de celulares		cen
Cargador			car
Telefono de línea		tel
Central de Tel. de línea	cenL

Archivos de configuración

Para que los procesos se ejecuten normalmente -sin efectos no deseados- los archivos de configuración deben estar construidos de la siguiente manera:

Archivo de configuración del celular
d exp10--numero de celular (d e N )
127.0.0.1--numero de ip de la central
n-- puerto de la central
n--permiso de transferencia de archivos (n e N)
n--conferencia permitida (n e N)
n--carga minima (n e N)
n--carga máxima (n e N) (carga máxima > carga mínima)
1--dual mode (1 = activado, 0 = desactivado)
n--puerto de mi Canal Chat
127.0.0.1--Mi numero de IP 
/dir1/dir2/.../dirn/-- directorio para la transferencia de archivos (debe comenzar y terminar con ‘ / ’)
Nota: al final de cada dato debe haber un doble guión del medio ( ‘ -- ‘)

Archivo de configuración de la central (único para las dos centrales)
127.0.0.1//Numero de IP de la central
n//Numero de puerto de la central
n//cantidad máxima de celulares en red (n e N)
3//cantidad máxima de centrales en red (n e N)
*//IP de la Central a la que me conecto por defecto ( ‘ * ’ si no se conecta a ninguna)
*//PORT de la central a la que me conecto por defecto ( ‘ * ’ si no se conecta a ninguna)
2//Cantidad de Centrales a las que me puedo conectar
n//Cantidad máxima de IDs que se pueden guardar en el vector (n e N)
11111//Nombre de la central
Nota: al final de cada dato debe haber un doble barra (‘ // ‘)

Archivo de configuración del cargador
127.0.0.1//Numero de ip del cargador
n//puerto del cargador
n//capacidad de celulares en carga (n e N)
n//unidades de carga por ráfaga
127.0.0.1//numero de ip de la central a la que se conecta
n//puerto de la central
n//timerafaga, tiempo (segundos) que tarda una ráfaga de carga (n e N)
Nota: al final de cada dato debe haber un doble barra (‘ // ‘)

Archivo de configuración del teléfono
d exp 8//numero de telefono (d e N)
127.0.0.1//numero de ip de la central
n//puerto de la central
n//conferencia máxima permitida (d e N)
n//puerto de mi Canal Chat
127.0.0.1//Mi numero de IP
Nota: al final de cada dato debe haber un doble barra (‘ // ‘)

Operaciones con los procesos

Proceso Teléfono celular

Una vez ejecutado el proceso se puede ingresar por teclado las siguientes operaciones :
-“llamar”: llama a otro teléfono (ya sea celular o de línea) para iniciar una conversación. 
Se deberá ingresar también el numero del teléfono al que se desea llamar, y este debe ser de 10 dígitos si se trata de un celular, y de 8 dígitos si es un teléfono de línea.
-“cortar”: corta la conversación con todos los teléfonos con los que estaba comunicándose.
-“prende”: enciende el celular.
-“apagar”: apaga el celular.
-“enviar”: envía un archivo a otro telefono celular.
Se deberá ingresar también el numero del celular al que se desea enviar un archivo, y este debe ser de 10 dígitos tal como se especificó en la operación “llamar”.
-“cargar”: carga el celular. El proceso se apaga y espera a ser cargado por el cargador. Una vez cargado, se deberá prender manualmente.
-“migrar”: migra el proceso a otra central telefónica (de celulares).

Proceso Cargador

Al comienzo de ejecutar el proceso se pedirá que se ingrese por teclado (numérico) el algoritmo deseado para organizar las peticiones de carga:
-1: el algoritmo que se utilizara será el de FIFO (First In First Out)
-2: el algoritmo que se utilizará será el de RR (Round Robin)
-3: el algoritmo que se utilizará será el de SRT (Shortest Remaining Time)
Si se ingresa algo diferente a lo enunciado anteriormente, se tomara por defecto el algoritmo FIFO.
En cualquier momento durante la ejecución del proceso se podrán ingresar por teclado (alfabético) las siguientes operaciones:
-“prende”: enciende el cargador. Resultando disponible para los celulares.
-“apagar”: apaga el cargador. Al momento de apagarse, se liberan las colas de peticiones de carga, ya sea la cola de carga como la de espera.
-“FIFO”: cambia el algoritmo de administración de peticiones de carga a FIFO (First In First Out).
-“RR”: cambia el algoritmo de administración de peticiones de carga a RR (Round Robin).
-“SRT”: cambia el algoritmo de administración de peticiones de carga a SRT (Shortest Remaining Time).

Proceso Teléfono de línea

Una vez ejecutado el proceso se puede ingresar por teclado las siguientes operaciones :
-“llamar”: llama a otro teléfono (ya sea celular o de línea) para iniciar una conversación.
Se deberá ingresar también el numero del teléfono al que se desea llamar, y este debe ser de 10 dígitos si se trata de un celular, y de 8 dígitos si es un teléfono de línea.
-“cortar”: corta la conversación con todos los teléfonos con los que estaba comunicándose.

