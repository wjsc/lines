//                            CELULAR
#include "soporteGeneral.h"
#include "soporteCelular.h"

//Variables Globales
int socketCentral;
int ESTADO; 			//0 para Apagado y 1 para PRENDIDO
int estoyTransfiriendo =0;
int estoyHablando = 0;
int soyMigrado;
const char* vcodigos[MAXCODIGOS] = {"llamar", "cortar","apagar","prende","enviar", "cargar","migrar"};
int senial;
int contadorID=0;
char centralID[10]; //Este seria el celular ID, pero lo dejo asi para que sea igual a la central
char ID[15];  		//Id de los mensajes...concatenacion de centralID + contador
char AUXILIAR[11];
migracion *estados;
char* vMensajes[MENSAJES] = {"Hola!!", "Como va??", "Estoy aca", "No se", "Chau!!"};
int cMensj = 0;
//Variables globales del celular...
char *NROCELULAR;
char *IP;
char *PORT;
char *MIIP;
char *LISPORT; 
char *SENDPORT;
int CARGAACT;
int CARGAMIN;
int CARGAMAX;
int TRANSFERENCIA;
int DUALMODE;
int CONFERENCIA;
char *PATH;
nodoNRO* NROS;
char *nombreCentral;

/****************************************************************************************/
							//Programa Principal//
/****************************************************************************************/

int main(int argc,char* argv[])
{
	int numbytes = 0;
	int sok,i,listener,listenerFile;
	int aux,aux666=0;
	int largo1 = sizeof(cabecera);
	int fdmax;
	int sokCelOriginal;
	cabecera*cabe;
	char*buffer1;
	char*buffer;
	fd_set master; 		//Conjunto Master de ficheros
	fd_set readfds; 	//Conjuntos de ficheros de los cuales leer
	struct timeval tiempo;
	int *vsockets;           //Vector de los Sockets que tengo conectados!!!!!!!
	int j;
	int k,w;
	
	if(argc != 2 && argc!= 4) 
	{
		logger("Error","Celular", "Debo usar:./cel <Archivo de configuracion> ", 1);
		exit(-1);
	}
	
	//Inicializo los ficheros de descriptores
	FD_ZERO(&master); 	//Inicializo
	FD_ZERO(&readfds);	//Inicializo
	
	if(argc==2)
	{
		soyMigrado=0;
		if( cargarCeluConfig( 	argv[1],
								&NROCELULAR,
								&IP,
								&PORT,
								&TRANSFERENCIA,
								&CONFERENCIA,
								&CARGAMIN,
								&CARGAMAX,
								&DUALMODE,
								&LISPORT,
								&MIIP,
								&PATH) == -1)
		{
			logger("Error","Celular", "No se pudo cargar el archivo de Configuracion", 1);
			return -1;
		}
		CARGAACT = CARGAMAX;
	}
	if(argc==4)
	{
		soyMigrado=1;
		printf("SoyMigrado\n");
		printf("ipCentral: %s\nportCentral: %s\n",argv[0],argv[1]);
		printf("ipCel: %s\nportCel: %s\n\n",argv[2],argv[3]);
		sokCelOriginal=crearSocket(argv[3],argv[2]);
		enviarCame(PARA,"\0",&sokCelOriginal,0,1,0,"\0");
		if( (buffer = calloc( 1, sizeof(cabecera) ) ) == NULL)
		{
			logger("Error","Cargador", "Memoria Insuficiente", 0);
			return -1;
		}
		if( recv( sokCelOriginal, buffer, sizeof(cabecera), 0 ) == -1 )
		{
			logger("Error","Cargador", "Fallo al recibir datos en AtenderConexionCelular", 0);
			close(sokCelOriginal);
			return -1;
		}
		cabe = (cabecera*)buffer;
		if( cabe->tipo == PARZ )
		{	
			if( (buffer = realloc( buffer, cabe->largo ) ) == NULL )
			{
				logger("Error","Cargador", "Memoria Insuficiente.", 0);
				return -1;
			}
		if( ( numbytes = recv(sokCelOriginal,buffer,cabe->largo,0) ) <0)
		{
			//Log Error en el Recv
			logger("Error","Cargador", "Fallo al recibir datos en AtenderConexionCelular", 0);
			close(sokCelOriginal);
			return -1;
		}
		estados=(migracion*)buffer;
		CONFERENCIA=estados->CONFERENCIA;
		DUALMODE=estados->DUALMODE;
		CARGAACT=estados->CARGAACT;
		CARGAMIN=estados->CARGAMIN;
		CARGAMAX=estados->CARGAMAX;
		TRANSFERENCIA=0;
		contadorID=estados->contadorID;
		NROCELULAR = malloc(strlen(estados->NROCELULAR)+1);
		strcpy(NROCELULAR,estados->NROCELULAR);
		IP = malloc(strlen(argv[0])+1);
		strcpy(IP,argv[0]);
		PORT=malloc(strlen(argv[1])+1);
		strcpy(PORT,argv[1]);
		MIIP=malloc(strlen(argv[0])+1);
		strcpy(MIIP,argv[0]);
		LISPORT=malloc(strlen(argv[1])+1);
		sprintf(LISPORT,"%d",atoi(argv[1])+111);
		printf("VARIABLES RECIBIDAS!\n");
		printf("CONFERENCIA= %d\n",CONFERENCIA);
		printf("TRANSFERENCIA= %d\n",TRANSFERENCIA);
		printf("DUALMODE= %d\n",DUALMODE);
		printf("contadorID= %d\n",contadorID);
		printf("NROCELULAR = %s\n",NROCELULAR);
		}//Cierra el cabe->tipo==PARZ 
	}
	
	//PUNTO DE ENCUENTRO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//CAMBIAR EL NOMBRE DEL PROCESO!!!
	
	
	if(!soyMigrado)
	{
	strcpy(argv[0], NROCELULAR);
	argv[1][4] ='\0';
	k=4+1;
	while(argv[1][k]!='\0')
		{
		argv[1][k]='\0';
		k++;
		}
	}
		
	if(soyMigrado) 	
	{
		k=strlen(argv[0]);
		k=strlen(NROCELULAR)-k;
		strcpy(argv[0], NROCELULAR);
		argv[0][strlen(NROCELULAR)] ='*';
		w=1;
		//k=strlen(NROCELULAR)-10;
		
		while(	w<argc)
			{
		
		while(argv[w][k]!='\0')
		{
			argv[w][k]='\0';
			k++;
		}
		
		w++;
		k=0;
			}		
	}
 			
	logger("Informacion","Celular", "Configuracion Cargada", 1);
	
	
	tiempo.tv_sec = TIMEDESC;
	tiempo.tv_usec = 0;

	//MANEJO DE SENIALES
	seniales(SIGTERM,sigHandler);
	seniales(SIGHUP,sigHandler);
	seniales(SIGCHLD,sigHandler);
	seniales(SIGUSR1,sigHandler);
	//FIN DE MANEJO DE SENIALES
	if( ( SENDPORT = malloc(strlen(PORT)+1) ) == NULL )
	{
		logger("Error", "Celular", "Memoria Insuficiente", 0);
		return -1;
	}
	sprintf(SENDPORT,"%d",atoi(LISPORT)+1);
	
	//Creamos el socket y nos conectamos a la direccion pasada por parametro
	if( ( sok = crearSocket(PORT,IP) ) == -1 )
	{	
		logger("Error","Celular", "Fallo la creacion del socket que nos conecta con la central",1);
		return -1;
	}	
	logger("Socket","Celular", "Socket que nos conecta con la central creado", 0);
	socketCentral = sok;
	ESTADO = 1; 	//Prendo el Celular
	estoyTransfiriendo = 0;
	if( ( vsockets = calloc( CONFERENCIA, sizeof( int ) )) == NULL  )
	{
		logger("Error","Celular", "Memoria Insuficiente", 0);
		return -1;
	}
	
	//Pongo el flag de Transferencia 0, es decir No Estoy transfieriendo
	printf("********************************************\n");
	printf("* NUMERO DE CELULAR *     %s        *\n", NROCELULAR); 
	printf("********************************************\n");
	if( DUALMODE == 0 )
		printf("*    DUALMODE       *          NO           *\n");
	else
		printf("*    DUALMODE       *          SI           *\n");   
	printf("********************************************\n");
	if( TRANSFERENCIA == 0)
		printf("*   TRANSFERENCIA   *          NO           *\n");
	else
		printf("*   TRANSFERENCIA   *          SI           *\n");
	printf("********************************************\n");
	

	if( ( NROS = calloc(CONFERENCIA, sizeof( nodoNRO ) ) ) == NULL )
	{
			logger("Error", "Celular", "Memoria Insuficiente", 0);
			return -1;
	}
	strcpy(centralID,NROCELULAR);
	inicializarVsockets(vsockets,CONFERENCIA);
	imprimirVsockets(vsockets,CONFERENCIA);
	logger("Informacion","Celular", "Estructuras Inicializadas", 0);
	//Con este socket vamos a escuchar llamados de otro celular
	
	while( ( listener = crearListener(LISPORT, MIIP) ) == -1)
	{
		if(!soyMigrado)
			{
		logger("Error","Celular", "Listener Fallo", 1);
		return -1;
			}
		sprintf(LISPORT,"%d",atoi(LISPORT)+1);
		sprintf(SENDPORT,"%d",atoi(LISPORT)+1);
		
	}
	
	if( entrarEnRed( NROCELULAR, sok,LISPORT) == -1 )
	{
		logger("Error", "Celular", "No se pudo establecer la conexion con la Central", 0);
		return -1;
	}
	logger("Informacion","Celular", "Listener Escucha de celulares creado", 0);
	if( ( listenerFile = crearListener(SENDPORT, MIIP) ) == -1)
	{
		logger("Error","Celular", "ListenerFile Fallo", 0);
		return -1;
	}
	logger("Informacion","Celular", "Listener escucha por archivos Creado ", 0);
	if( ( buffer1=calloc(1,sizeof(cabecera)) ) == NULL )
	{
		logger("Error", "Celular", "Memoria Insuficiente", 0);
		return -1;
	}
	
	
	if(soyMigrado)
		{
		enviarCame(KILL,"\0",&sokCelOriginal,0,1,0,"\0");
		}
//******************************** SELECT ********************************//
	FD_SET(listener,&master);
	FD_SET(listenerFile,&master);
	if( !soyMigrado ) 
		FD_SET(STDIN,&master);
	FD_SET(sok,&master);
	fdmax = listener;
	if( listenerFile > fdmax) 
		fdmax = listenerFile;
	if( sok > fdmax) 
		fdmax = sok;

//BUCLE PRINCIPAL
	for( ; ; )
	{
		readfds=master;
		//printf( "El tiempo que falta para descargar una rafaga de carga es: %d.\n", tiempo.tv_sec);
		if( select( fdmax+1,&readfds,NULL,NULL, &tiempo ) == -1)
			select_controlar(&readfds,listenerFile,&master);
		
		//exploramos las conexiones de readfds
		for(i=0;i<=fdmax;i++)
		{
			if( FD_ISSET(i,&readfds) )
			{ 
				//Hay datos en el socket 			
				if(i==listenerFile) 
				{
					estoyHablando = hayAlguien (vsockets,CONFERENCIA );
					receptor(listenerFile,SENDPORT, &master,fdmax,estoyHablando);
				}
				else      
				{
					if( i==STDIN) 
						atenderTeclado(i,sok,vsockets);	//Si escribio en Teclado
					//manejar los datos de un cliente
					else 
					{
						if(i == listener)
						{
							logger("Informacion","Celular", "Nueva Conexion", 0);
							atenderConexion(i,&fdmax,&master);
						}
						else 
						{
							//buffer1=calloc(1,sizeof(cabecera));
							if((numbytes=recv(i,buffer1,sizeof(cabecera),0))<=0)
							{
								// es un error o conexion cerrada
								logger("Informacion","Celular", "Conexion Cerrada", 0);
								if( quitarDeVsockets(vsockets,i,CONFERENCIA) == 1 )
								{
									logger("Informacion", "Celular", "Un celular se ha ido", 1);
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
								}
								else
								{
									logger("Error","Celular", "Fallo recepcion de datos en el programa principal", 0);
								}
							}
							else 
							{ 	
								//No hay errores y hay datos
								atenderEntrada(i,&buffer1,&vsockets,&fdmax,&master, sok);
							}
						}
					}		
            	}  //Esta llave cierra el else de i==listenerFile   
			} //Esta llave cierra el FD_ISSET(i,&master)	
		} //Este cierra el FOR de i=0 a i=fdmax
		if( tiempo.tv_sec == 0 )
		{
			tiempo.tv_sec = TIMEDESC;
			tiempo.tv_usec = 0;
			if(ESTADO != 0)
			{
				if( senial == OFFLINE )
				{
						if( ( socketCentral = crearSocket(PORT,IP) ) != -1 )
						{	
							logger("Socket","Celular", "Socket que nos conecta con la central creado", 0);
							if( entrarEnRed( NROCELULAR, socketCentral,LISPORT) == -1 )
							{
								logger("Error", "Celular", "No se pudo establecer la conexion con la Central", 1);
							}
						}
						else
						{
							printf("Fallo el intento de reconexion. Se intentara mas tarde automaticamente\n");
						} 

				}
				if( hayAlguien(vsockets, CONFERENCIA ) || estoyTransfiriendo )
					{
					//printf("CONFERENCIA %d\n",CONFERENCIA);
					//imprimirVsockets(vsockets,CONFERENCIA);
					//printf("hayAlguien dio: %d\n",hayAlguien(vsockets, CONFERENCIA ));
					//printf("estoyTransfiriendo vale: %d\n",estoyTransfiriendo);
					CARGAACT = CARGAACT - 3;
					//printf("desconte 3 \n");
					}
				else 
				{
					
					CARGAACT--;
					//printf("desconte 1 \n");
					if( CARGAACT <= CARGAMIN ) //CONTROLA SI TENGO BATERIA
					{
						logger("Informacion","Celular", "Bateria baja, el celular sera cargado", 1);
						if( enviarCame(CRGA, "\0", &socketCentral,0,1,0,"\0") == -1 )
						{
							logger("Error", "Celular", "Fallo el enviarCame de CRGA", 0);
							return -1;
						}
					}
				}
				printf( "Carga Actual: %d\n", CARGAACT);
			}
		} 
	}//Esta llave cierra el for(;;)


	close(sok);
	close(listener);
	free(buffer1);
	free(vsockets);
	free(NROCELULAR);
	free(IP);
	free(PORT);
	free(LISPORT);
	free(SENDPORT);
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
