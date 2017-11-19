//                            TELEFONO DE LINEA
#include "soporteGeneral.h"
#include "soporteTelefono.h"

//Variables GLOBALES
int socketCentral;
int ESTADO; //0 para Apagado y 1 para PRENDIDO
int estoyHablando = 0;
const char* vcodigos[MAXCODIGOS]={"llamar", "cortar","apagar","prende"};
int senial;
char *MIIP;
int contadorID = 0;
char centralID[10];  //Este seria el celular ID, pero lo dejo asi para que sea igual a la central
char ID[15]; 		 //Id de los mensajes...concatenacion de centralID + contador
char *nombreCentral;
char AUXILIAR[11];
nodoNRO* NROS;
char *NROTEL;
char *IP;
char *PORT;
char *MIIP;
char *LISPORT; 
int CONFERENCIA;

/****************************************************************************************/
							//Programa Principal//
/****************************************************************************************/

int main(int argc,char* argv[])
{
	int numbytes=0;
	int sok,i,listener,listenerFile;
	int aux;
	char puerto[6];
	int largo1=sizeof(cabecera);
	int *vsockets;           //Vector de los Sockets que tengo conectados!!!!!!!
	int fdmax;
	char*buffer1;
	fd_set master; 		//Conjunto Master de ficheros
	fd_set readfds; 	//Conjuntos de ficheros de los cuales leer
	int k, j;

	if(argc != 2) 
	{
		printf("Uso: ./tel <Archivo de configuracion>\n");
		exit(-1);
	}
	
	FD_ZERO(&master); 	//Inicializo
	FD_ZERO(&readfds);	//Inicializo
	
	if( cargarTelConfig(	argv[1],
							&NROTEL,
							&IP,
							&PORT,
							&CONFERENCIA,
							&LISPORT,
							&MIIP) == -1)
	{
			logger("Error","Telefono", "No se pudo cargar el archivo de Configuracion", 1);
			return -1;
	}

	strcpy(argv[0], NROTEL);
	argv[1][4] ='\0';
	k=4+1;
	while(argv[1][k]!='\0')
	{
		argv[1][k]='\0';
		k++;
	}

	logger("Informacion","Telefono", "Configuracion Cargada", 1);

	//Creamos el socket y nos conectamos a la direccion pasada por parametro
	if( ( sok = crearSocket(PORT,IP) ) == -1 )
	{	
		logger("Error","Telefono", "Fallo la creacion del socket que nos conecta con la central",1);
		return -1;
	}	
	logger("Socket","Telefono", "Socket que nos conecta con la central creado", 0);
	socketCentral = sok;
	ESTADO = 1; 	//Prendo el Telefono

	if( ( vsockets = calloc( CONFERENCIA, sizeof( int ) )) == NULL  )
	{
		logger("Error","Telefono", "Memoria Insuficiente", 0);
		return -1;
	}

	if( entrarEnRed( NROTEL, sok,LISPORT) == -1 )
	{
		logger("Error", "Telefono", "No se pudo establecer la conexion con la Central", 0);
		return -1;
	}
	
	printf("********************************************\n");
	printf("* NUMERO DE TELEFONO *      %s         *\n", NROTEL); 
	printf("********************************************\n");
	
	if( ( NROS = calloc(CONFERENCIA, sizeof( nodoNRO ) ) ) == NULL )
	{
			logger("Error", "Telefono", "Memoria Insuficiente", 0);
			return -1;
	}
	strcpy(centralID,NROTEL);
	
	inicializarVsockets(vsockets,CONFERENCIA);
	imprimirVsockets(vsockets,CONFERENCIA);
	logger("Informacion","Telefono", "Estructuras Inicializadas", 0);
	
	//Con este socket vamos a escuchar llamados de otro celular
	if( ( listener = crearListener(LISPORT, MIIP) ) == -1)
	{
		logger("Error","Telefono", "Listener Fallo", 0);
		return -1;
	}
	logger("Informacion","Telefono", "Listener Escucha de telefonos creado", 0);
	
	if( ( buffer1=calloc(1,sizeof(cabecera)) ) == NULL )
	{
		logger("Error", "Telefono", "Memoria Insuficiente", 0);
		return -1;
	}

//******************************** SELECT ********************************//
	FD_SET(listener,&master);
	FD_SET(STDIN,&master);
	FD_SET(sok,&master);
	fdmax=listener;
	if( sok > fdmax) 
		fdmax = sok;

//BUCLE PRINCIPAL
	for( ; ; )
	{
		readfds=master;
		//printf( "El tiempo que falta para descargar una rafaga de carga es: %d.\n", tiempo.tv_sec);
		if( select( fdmax+1,&readfds,NULL,NULL, NULL ) == -1)
		{
			perror("Select");
			exit (-1);
		}	
		
		//exploramos las conexiones de readfds
		for(i=0;i<=fdmax;i++)
		{
			if( FD_ISSET(i,&readfds) )
			{ 
					if(i==STDIN) 
						atenderTeclado(i,sok,vsockets);	//Si escribio en Teclado
																						//manejar los datos de un cliente
					else 
					{
							if(i == listener)
							{
								logger("Informacion","Telefono", "Nueva Conexion", 0);
								atenderConexion(i,&fdmax,&master);
							}
							else 
							{
									//buffer1=calloc(1,sizeof(cabecera));
									if((numbytes=recv(i,buffer1,sizeof(cabecera),0))<=0)
									{
										// es un error o conexion cerrada
										logger("Informacion","Telefono", "Conexion Cerrada", 0);
										if( quitarDeVsockets(vsockets,i,CONFERENCIA) == 1 )
										{
											logger("Informacion", "Telefono", "Un telefono se ha ido", 1);
										}
										FD_CLR(i,&master);
										close(i);
										imprimirVsockets(vsockets,CONFERENCIA);
										if(numbytes==0) 
										{	
											if(i == socketCentral)
											{ 
												logger("Informacion","Celular", "Colapso la central", 1);
												senial = OFFLINE;
											}										
											else
											{
												logger("Error","Celular", "Fallo recepcion de datos en el programa principal", 0);
											}
										}
										
									}
									else 
									{ 	
										//No hay errores y hay datos
										atenderEntrada(i,&buffer1,&vsockets,&fdmax,&master, sok);
									}
								}
			
						}		
            
			} //Esta llave cierra el FD_ISSET(i,&master)	
		} //Este cierra el FOR de i=0 a i=fdmax
	if(ESTADO != 0)
	{
		if( senial == OFFLINE )
		{
			if( ( socketCentral = crearSocket(PORT,IP) ) != -1 )
			{	
				logger("Socket","Telefono", "Socket que nos conecta con la central creado", 0);
				if( entrarEnRed( NROTEL, socketCentral,LISPORT) == -1 )
				{
					logger("Error", "Telefono", "No se pudo establecer la conexion con la Central", 1);
				}
			}
			else
			{
				printf("Fallo el intento de reconexion. Se intentara mas tarde automaticamente\n");
			} 
		}
	}
	
	}//Esta llave cierra el for(;;)
	close(sok);
	close(listener);
	free(buffer1);
	free(vsockets);
	free(NROTEL);
	free(IP);
	free(PORT);
	free(LISPORT);
	return 1;
}
