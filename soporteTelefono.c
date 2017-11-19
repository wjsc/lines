#include "soporteGeneral.h"
#include "soporteTelefono.h"

//Variables Globales

extern int socketCentral;
extern int ESTADO; 			//0 para Apagado y 1 para PRENDIDO
extern int estoyHablando;
extern const char* vcodigos[MAXCODIGOS];
extern int senial;
extern char *MIIP;
extern int contadorID;
extern char centralID[10];  //Este seria el celular ID, pero lo dejo asi para que sea igual a la central
extern char ID[15]; 
extern char *nombreCentral;
extern nodoNRO* NROS;
extern char *NROTEL;
extern char *IP;
extern char *PORT;
extern char *MIIP;
extern char *LISPORT; 
extern int CONFERENCIA;
extern char AUXILIAR[11];

//FUNCIONES AUXILIARES/////////////////////////////////////////////////////////////////////////
int atenderEntrada(	int sokMov,
					char**buffer1,
					int**vsockets,
					int *fdmax,
					fd_set *master,
					int central)
{

	char *buffer2,*ip,*port;
	int largo2,sokcel,tipo,tipo2, sokCargador, resto;
	int numbytes;
	cabecera *cabe;
	int pos;
	int sokCreado;
	int aux=0;
	char portFile[6];
	char restoCarga[4];
	char *bufData;
	int posicion;	
	//NOTAS: sokMov es el socket que se movio en el select principal(Central o Cel)
	//	 sokcel es un socket que creo para conectarme con un nuevo celular 	
	//       Falta control de errores de CASI todo
	//Recibo la cabecera del mensaje que me enviaron, probablemente 23 bytes

	IDMensaje();
	cabe = (cabecera*)*buffer1;	
	largo2 = cabe->largo;
	switch (cabe->tipo)
	{
		case CALZ:{ 
					// Recibo el contenido del mensaje
							if( (buffer2 = calloc(1, largo2) ) == NULL )
							{
								logger("Error","Telefono", "Memoria insuficiente", 0);
								return -1;
							}
					if( ( numbytes = recv(sokMov,buffer2,largo2,0) ) <0)
					{
						logger("Error","Telefono", "Fallo la recepcion de datos de atenderEntrada", 0);
						close(sokMov);
						return -1;
					}
					//printf("Mensaje: %s\n",buffer2);	
					if( ( ip = calloc(1,16) ) == NULL )
					{
						logger("Error","Telefono", "Memoria insuficiente", 0);
						return -1;
					}
					ip = strtok(buffer2,"*");
					if( ( port = calloc(1,6) ) == NULL )
					{
						logger("Error","Telefono", "Memoria insuficiente", 0);
						return -1;
					}
					port = strtok(NULL,"*");		
					sokcel = crearSocket(port,ip);		
					if( enviarCame(YAMA,NROTEL,&sokcel,0,1,0,ID) == -1 )
					{
						logger("Error", "Telefono", "Fallo el enviarCame de YAMA", 0);
						return -1;
					}
					//free(ip);	
					FD_SET(sokcel,master);
					if(sokcel>*fdmax) 
						*fdmax=sokcel; 
					} break;
				
		case YAMZ: {			
					logger("Informacion","Telefono", "Comunicacion establecida", 1);
					if( ( buffer2 = calloc(1,largo2) ) == NULL )
					{
						logger("Error", "Telefono", "Memoria Insuficiente", 0);
						return -1;
					}	
					if( ( numbytes = recv(sokMov,buffer2,largo2,0) ) < 0 )
					{
						logger("Error","Telefono", "Fallo la recepcion de datos en atenderEntrada", 0);
						close(sokMov);
						return -1;
					}
					buffer2[largo2]='\0';
					if( ( pos = buscarLibreVsockets(*vsockets,CONFERENCIA) ) == -1 ) 
						return -1;	
					agregarAVsockets(*vsockets,sokMov,pos, buffer2);
					imprimirVsockets(*vsockets,CONFERENCIA);
					}break;
		case CALX: {	
					logger("Error","Telefono", "El telefono solicitado no esta conectado a la central o esta apagado", 1);
					}break;
	   	case FINA: {
					if( enviarCame(FINZ,"\0",&sokMov,0,1,0,ID) == -1)
					{
						logger("Error", "Telefono", "Fallo el enviarCame de FINZ", 0);
					}
					quitarDeVsockets(*vsockets,sokMov,CONFERENCIA);
					FD_CLR(sokMov,master);
					close(sokMov);	
					logger("Informacion","Telefono", "Conversacion finalizada", 1);
					imprimirVsockets(*vsockets,CONFERENCIA);
					}break;
		case FINZ: {
					FD_CLR(sokMov,master);
					logger("Informacion","Celular", "Conversacion finalizada", 1);
					quitarDeVsockets(*vsockets,sokMov,CONFERENCIA);
					imprimirVsockets(*vsockets,CONFERENCIA);
					close(sokMov);
					}break;
		case YAMA: { 		
					logger("Informacion","Telefono", "LLamada entrante", 1);
					if( ( buffer2 = calloc(1,largo2) ) == NULL )
					{
						logger("Error", "Telefono", "Memoria Insuficiente", 0);
						return -1;
					}
					if((numbytes=recv(sokMov,buffer2,largo2,0))<0)
					{
						logger("Error","Telefono", "Fallo la recepcion de datos en atenderEntrada", 0);
						close(sokMov);
						return -1;
					}
					buffer2[largo2]='\0';
					if( ESTADO == 0 ) //VER SI ESTA BIEN ESTO?
					{
						if( enviarCame(YAMX,"\0",&sokcel,0,1,0,"\0") == -1 )
						{
							logger("Error", "Celular", "Fallo el enviarCame de YAMX", 0);
							return -1;
						}
						FD_CLR(sokMov,master);
						close(sokMov);
						return 1;
					}
					if( ( pos = buscarLibreVsockets(*vsockets,CONFERENCIA)) == -1 ) 
					{
						//En este caso ya estoy conectado con maximo de telefonos
						if( enviarCame(YAMX,"\0",&sokMov,0,1,0,"\0") == -1)
						{
							logger("Error", "Telefono", "Fallo el enviarCame de YAMX", 0);
							return -1;
						}
						FD_CLR(sokMov,master);
						close(sokMov);
						return 1;
					}	
					agregarAVsockets(*vsockets,sokMov,pos, buffer2);
					printf("\tLLamada Aceptada.\n");
					imprimirVsockets(*vsockets,CONFERENCIA);
					if( enviarCame(YAMZ,NROTEL,&sokMov,0,1,0,"\0") == -1 )
					{
						logger("Error", "Telefono", "Fallo el enviarCame de YAMZ", 0);
						return -1;
					}
					printf("Comunicacion Establecida\n");
				}break;
				
		case YAMX:{
					contadorID--; //emparejo
					logger("Informacion","Telefono", "El celular esta ocupado", 1);
					FD_CLR(sokMov,master);
					close(sokMov);
					}break;	
   	case SMS: {
   						// Recibo el contenido del mensaje(Lo que el otro celular me dice)	
							if( ( buffer2 = calloc(1,largo2+1) ) == NULL )
							{
								logger("Error","Celular", "Memoria Insuficiente", 0);
								return -1;
							}
							if( ( numbytes = recv(sokMov,buffer2,largo2,0) ) < 0 )
							{
								logger("Error","Celular", "Fallo la recepcion de datos en atenderEntrada", 1);
								close(sokMov);
								return -1;
							}
							buffer2[largo2]='\0';
							for(posicion=0;posicion<CONFERENCIA;posicion++) 
							{
								if(sokMov == (*vsockets)[posicion])
								printf("%s",NROS[posicion].NRO);
							}
							printf(" - %s\n",buffer2);
							quitarDeVsockets(*vsockets,sokMov,CONFERENCIA);
							if( enviarCame(SMS,buffer2,*vsockets,CONFERENCIA,cabe->TTL,cabe->HOPS,ID) == -1)
							{
								logger("Error","Celular", "Fallo el enviarCame de SMS", 0);
								return -1;
							}
							if( ( pos = buscarLibreVsockets(*vsockets,CONFERENCIA)) == -1)
								logger("Error","Celular", "Deficiencia en el programa", 0);	
							agregarAVsockets(*vsockets,sokMov,pos, AUXILIAR);
							free(buffer2);
					}break;		

	}
	return 1;
}

//------------------------------------------------------------------------------------

int atenderTeclado(int teclado,int central, int*vsockets )
{
	char *buffer;
	int numbytes=0;
	int sok,pos;
	int senvio;
	char*ipPort;

	//printf("Ejecutando atenderTeclado\n");
	if( (buffer = calloc(1,MAXDATASIZE)) == NULL)
	{
		logger("Error","Telefono", "Memoria insuficiente", 0);
		return -1;
	}
	if( (numbytes = read(teclado,buffer,MAXDATASIZE) ) <=0 )
	{
		logger("Error","Telefono", "Fallo recepcion de datos al leer teclado en atenderTeclado", 0);
		return -1;
	}
	IDMensaje();
	buffer[numbytes-1]='\0';
	if( (strlen(buffer) ) == 6)
	{
		if( ( pos = buscarEnVector(buffer) ) ==-1 )
		{
			//logger("Informacion","Celular", "No se encontro el comando en el vector", 0);
			pos=-1;
		}
	}
  else 
		pos=-1;
	if((ESTADO == 0) && (pos!=3) ) 
		return 1;
	switch(pos) 
	{	
		case -1:{ 	//Voy a Enviar UN MSJ POR TECLADO		
					if(ESTADO==0) 
					{
						printf("El Telefono esta apagado. Primero Debe prenderlo\n");
						free(buffer);
						return 1;
					}
					if( enviarCame(SMS,buffer,vsockets,CONFERENCIA,SMSTTL,0,ID) == -1)
					{
						logger("Error","Telefono", "Fallo el enviarCame de SMS", 0);
						return -1;
					}
				}break;
		case 0: { 	//RECIBI "LLAMAR" POR TECLADO
					if(ESTADO==0) 
					{
						printf("El Telefono esta apagado. Primero Debe prenderlo\n");
						free(buffer);
						return 1;
					}
					if(senial == OFFLINE)
					{
						logger("Informacion","Telefono", "Sin senial", 1);
						free(buffer);
						return 1;
					}
					if( buscarLibreVsockets(vsockets,CONFERENCIA)==-1)
					{
						logger("Informacion","Telefono", "Limite de conferencia alcanzado", 1);
						free(buffer);
						return 1;
					}
					printf("A quien deseas llamar?\n");
					if(	(numbytes=read(teclado,buffer,MAXDATASIZE) ) <=0 )
					{
						logger("Error","Telefono", "Error al leer teclado en atenderTeclado", 1);
						free(buffer);
						return -1;
					}
					buffer[numbytes-1]='\0';	
					if( (strcmp(buffer,NROTEL) ) == 0) 
					{
						printf("Ese es tu numero!\n");
						free(buffer);
						return 1;
					}
					if( enviarCame(CALA,buffer,&central,0,1,0,ID) == -1)
					{
						logger("Error","Telefono", "Fallo el enviarCame de CALA", 0);
						return -1;
					}
					printf("Buscando...\n");
				}break;
		case 1: {	//RECIBI "CORTAR" POR TECLADO
					logger("Informacion","Telefono", "Cortando todas las comunicaciones", 1); 
					if( enviarCame(FINA,"\0",vsockets,CONFERENCIA,1,0,ID) == -1 )
					{
						logger("Error","Telefono", "Fallo el enviarCame de FINA", 0);
						return -1;
					}
				}break;
		case 2: {	//RECIBI "APAGAR" POR TECLADO 
					if(ESTADO==0) 
					{
						printf("El Telefono ya se encuentra apagado\n");
						free(buffer);
						return 1;
					}
					ESTADO = 0; //Apago el Telefono
					logger("Informacion","Telefono", "Telefono apagado", 1);;
					if(senial == OFFLINE)
					{
						logger("Informacion","Telefono", "Sin senial", 1);
						free(buffer);
						return 1;
					}
					free(nombreCentral);
					if( enviarCame(APGA,"\0",&central,0,1,0,ID) == -1)
					{
						logger("Error","Telefono", "Fallo el enviarCame de APGA", 0);
						return -1;
					}
					IDMensaje();
					if( enviarCame(FINA,"\0",vsockets,CONFERENCIA,1,0,ID) == -1 )
					{
						logger("Error","Telefono", "Fallo el enviarCame de FINA", 0);
						return -1;
					}
				}break;
		case 3: {	//RECIBI "PRENDE" POR TECLADO
					if(ESTADO == 1)
					{
						printf("El telefono ya está encendido\n");
						free(buffer);
						return 1;
					}
					ESTADO = 1; //Prendo el Telefono
					logger("Informacion","Telefono", "Telefono encendido", 1);
					if( senial  == OFFLINE )
					{
						if( ( socketCentral = crearSocket(PORT,IP) ) != -1 )
						{	
							logger("Socket","Telefono", "Socket que nos conecta con la central creado", 0);
							if( entrarEnRed( NROTEL, socketCentral,LISPORT) == -1 )
							{
								logger("Error", "Celular", "No se pudo establecer la conexion con la Central", 1);
							}
						}
						else
						{
							printf("Fallo el intento de reconexion. Se intentara mas tarde automaticamente\n");
						} 
					}
					else
					{
						if( entrarEnRed(NROTEL,central,LISPORT) == -1)
						{
							logger("Error","Telefono", "No se pudo establecer la conexion con la central", 0);
						}
					}
				}break;	
					
	}	
	free(buffer);
	return 1;
}

//------------------------------------------------------------------------------------


int atenderConexion(int listener,int *fdmax,fd_set *master)
{
	struct sockaddr_in cliente; //esctructura con la info de un Celular que me llama
	int addrlen;
	int new_fd;

	//Hay una llamada entrante
	//printf("Ejecutando atenderConexion Celular\n");
	addrlen=sizeof(cliente);
	if((new_fd=accept(listener,(struct sockaddr *)&cliente,&addrlen))==-1)	
		logger("Error","Telefono", "Error en el accept de atenderConexion", 1);
	else
		{ 
			//O sea que no hubo error  de esta nueva conexion
			logger("Informacion","Telefono", "Nueva conexion", 0);
		}
	FD_SET(new_fd,master);
	if(new_fd > *fdmax) 
		*fdmax=new_fd; 
	return 1;					
}


//--------------------------------------------------------------------------

int buscarEnVector(char*buffer)
{
	int i=0;

	//printf("Ejecutando buscarEnVector\n");
	while((i<MAXCODIGOS)&&(strcmp(buffer,vcodigos[i])!=0)) i++;
	if(i==MAXCODIGOS) 
	{
		logger("informacion","Celular","Comando Desconocido",0);
		return -1;
	}
	return i;
}

//--------------------------------------------------------------------------

int buscarLibreVsockets(int*vsockets,int CONFERENCIA)
{
	//Esta funcion detecta la primer posicion que contenga -1 en el vsockets 
	int i=0;	
	
	//printf("Ejecutando buscarLibreVsockets\n");
	while((vsockets[i]!=0)&&(i<CONFERENCIA)) i++;
	if(i==CONFERENCIA) 
	{
		printf("No hay espacio en el vector\nLlegaste al tope de CONFERENCIA\n");
		return -1;
	}
	return i;
} 

//--------------------------------------------------------------------------
 
int agregarAVsockets(int*vsockets,int socket,int pos, char* nro)
{
	//Esta0Funcion agrega un Socket a la posicion de pos
	//printf("Ejecutando agregarAVsockets\n");
	vsockets[pos]=socket;
	strcpy(NROS[pos].NRO, nro);
}

//--------------------------------------------------------------------------

int quitarDeVsockets(int*vsockets,int socket,int CONFERENCIA)
{
	//Esta funcion quita del vector vsockets el socket y pone -1 en su psicion
	int i=0;

	//printf("Ejecutando quitarDeVsockets\n");
	while((vsockets[i]!=socket)&&(i<CONFERENCIA)) 
		i++;
	if(i == CONFERENCIA) 
	{
		//printf("No se encuentra el socket en el vector\n");
		return -1;
	}
	vsockets[i] = 0;
	strcpy(AUXILIAR,NROS[i].NRO);
	NROS[i].NRO[0]='\0';	
	return 1;
}

//-----------------------------------------------------------------------------

int inicializarVsockets(int*vsockets,int CONFERENCIA)
{
	//Esta Funcion inicializa vsockets con todo -1
	int i;

	//printf("Ejecutando inicializarVsockets\n");
	for(i=0;i<CONFERENCIA;i++) 
		{
		vsockets[i]=0;
		NROS[i].NRO[0]='\0';
		}
	return 1;
}

//-----------------------------------------------------------------------------

void imprimirVsockets(int*vsockets,int CONFERENCIA)
{
	//Esta funcion muestra en un momento dado el contenido de todo vsockets
	int aux;

	//printf("Ejecutando imprimirVsockets\n");
	//printf("Este es mi vsockets actual: \n");
	printf("********************************************\n");
	printf("*       TELEFONOS CONECTADOS               *\n"); 
	printf("********************************************\n");
 	printf("* POS * SOK *        NUMERO                *\n");
	printf("********************************************\n");
	for(aux=0;aux<CONFERENCIA;aux++) 
	{
		printf("*  %d  *  %d  *",aux,vsockets[aux]);
		if(NROS[aux].NRO[0] != '\0')
			printf("      %s              *\n",NROS[aux].NRO);
		else
			printf("          LIBRE               *\n");  
	printf("*------------------------------------------*\n");
	}
	printf("********************************************\n");
	return;
}

//--------------------------------------------------------------------------------------

int entrarEnRed( char* nroCel, int socket,char*puerto )
{
	char buffer[5];
	char*buffer2;

	if( (buffer2 = calloc(1,strlen(nroCel)+1+strlen(MIIP)+1+strlen(puerto)+2)) == NULL)
	{
		logger("Error", "Telefono", "Memoria Insuficiente", 0);
		return -1;
	}	
	strcpy(buffer2,nroCel);
	buffer2[strlen(nroCel)]='*';
	strcpy(&buffer2[strlen(nroCel)+1],MIIP);
	buffer2[strlen(nroCel)+1+strlen(MIIP)]='*';
	strcpy(&buffer2[strlen(nroCel)+1+strlen(MIIP)+1],puerto);
	buffer2[strlen(nroCel)+1+strlen(MIIP)+1+strlen(puerto)]='*';
	buffer2[strlen(nroCel)+1+strlen(MIIP)+1+strlen(puerto)+1]='\0';
	//printf("buffer2: %s\n",buffer2);
	IDMensaje();
	if( enviarCame(CONA,buffer2,&socket,0,1,0,ID) == -1)
	{
		logger("Error", "Telefono", "Fallo el enviarCame de CONA", 0);
		return -1;
	}
	if( recv( socket, buffer, 5, 0 ) == -1 )
	{
		logger("Error", "Telefono", "Fallo la recepcion de datos en EntrarEnRed", 0);
		close(socket);
		free(buffer2);
		return -1;
	}
	buffer[4] = '\0';
	if( strcmp("CONX", buffer) == 0)
	{
		logger("Informacion", "Telefono", "Central LLena. Esperando por espacio en la red.", 1);
		if( recv( socket, buffer, 5, 0 ) == -1 )
		{
			logger("Error", "Telefono", "Fallo la recepcion de datos en EntrarEnRed", 0);
			close(socket);
			free(buffer2);
			return -1;
		}
		buffer[4] = '\0';
	}
	if(strcmp("CCEL",buffer) == 0 ) /* VER PARAMETROS */ 
	{
		logger("Informacion","Telefono", "La central es para telefonos Celulares", 1);
		close(socket);
		free(buffer2);
		return -1;
	}
	if(strcmp("CONZ",buffer) != 0 ) /* VER PARAMETROS */ 
	{
		logger("Error", "Telefono", "Fallo la sincronizacion", 0);
		close(socket);
		free(buffer2);
		return -1;
	}
	nombreCentral=calloc(1,6);
	if( recv( socket, nombreCentral, 6, 0 ) == -1 )
		{
			logger("Error", "Telefono", "Fallo la recepcion de datos en EntrarEnRed", 0);
			close(socket);
			free(buffer2);
			return -1;
		}
	logger("Informacion", "Telefono", "Conexion Establecida con la Central. Telefono En Red", 1);
	printf("********************************************\n");
	printf("*  CONECTADO CON LA CENTRAL:  %s        *\n",nombreCentral);
	printf("********************************************\n");
	senial = ONLINE;
	free(buffer2);
	return 1;
}

//************************************************************************************************
//-----------------------------------------------------------------------------------------------------

int cargarTelConfig( 	char* ruta,
			char** num,
			char** ip,
			char** port,
			int* conf,
			char** portcent,
			char** MIIP)
{
	FILE* arch;
	char* linea; 
	char* palabra;
	int cont = 0;

	if( (arch = fopen( ruta, "r" ) ) == NULL )
	{ 
		printf("No se pudo abrir el archivo %s\n",ruta);
		return -1;
	}
	if((linea = malloc( MAXLINEA )) == NULL)
	{
		printf("Memoria insuficiente\n");
		return -1;
	}
	while( linea = fgets( linea, MAXLINEA+1+1, arch ) )
	{
		palabra = strtok(linea,"//" );
		switch(cont)
		{
			case 0:{
					if((*num = malloc( strlen(palabra) + 1 )) == NULL)
					{
						printf("Memoria insuficiente\n");
						return -1;
					}
					strcpy( *num, palabra );
		
				}break;
			case 1:	
					if((*ip = malloc( strlen(palabra) + 1 )) == NULL)
					{
						printf("Memoria insuficiente\n");
						return -1;
					}
					strcpy( *ip, palabra);
				break;
			case 2:	
					if((*port = malloc( strlen(palabra) + 1 )) == NULL)
					{
						printf("Memoria insuficiente\n");
						return -1;
					}
					strcpy( *port,palabra);
				break;
			case 3:	
					*conf = atoi(palabra);
				break;
			case 4: 
					if((*portcent = malloc( strlen(palabra) + 1 )) == NULL)
					{
						printf("Memoria insuficiente\n");
						return -1;
					}
					strcpy( *portcent,palabra);	
				break;
			case 5:
					if((*MIIP = malloc( strlen(palabra) + 1 )) == NULL)
					{
						printf("Memoria insuficiente\n");
						return -1;
					}
					strcpy( *MIIP,palabra);	
				break;
		}
	palabra = strtok( NULL , "\0\n" );
	cont++;
	}
	if( ferror(arch) != 0 )
	{
		printf("ERROR DE LECTURA");
		return -1;
	}	
	fclose(arch);
	return 1;
}

//------------------------------------------------------------------------------------
