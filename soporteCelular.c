#include "soporteGeneral.h"
#include "soporteCelular.h"

//Variables Globales
extern int socketCentral;
extern int ESTADO;			//0 para Apagado y 1 para PRENDIDO
extern int estoyTransfiriendo;
extern int estoyHablando;
extern int soyMigrado;
extern const char* vcodigos[MAXCODIGOS];
extern int senial;
extern int contadorID;
extern char centralID[10];	//Este seria el celular ID, pero lo dejo asi para que sea igual a la central
extern char ID[15];  		//Id de los mensajes...concatenacion de centralID + contador
extern char *nombreCentral;
extern migracion *estados;
extern char* vMensajes[MENSAJES];
extern int cMensj;
//Variables globales del celular...
extern char *NROCELULAR;
extern char *IP;
extern char *PORT;
extern char *MIIP;
extern char *LISPORT; 
extern char *SENDPORT;
extern int CARGAACT;
extern int CARGAMIN;
extern int CARGAMAX;
extern int TRANSFERENCIA;
extern int DUALMODE;
extern int CONFERENCIA;
extern char* PATH;
extern nodoNRO* NROS;
extern char AUXILIAR[11];
//extern int *vsockets;           //Vector de los Sockets que tengo conectados!!!!!!!


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
	//printf("recibi tipo: %d\n",cabe->tipo);
	switch (cabe->tipo)
	{
		case CALZ:{ 
					// Recibo el contenido del mensaje
					if( (buffer2 = calloc(1, largo2) ) == NULL )
					{
						logger("Error","Celular", "Memoria insuficiente", 0);
						return -1;
					}
					if( ( numbytes = recv(sokMov,buffer2,largo2,0) ) <0)
					{
						logger("Error","Celular", "Fallo la recepcion de datos de atenderEntrada", 0);
						close(sokMov);
						return -1;
					}
					//printf("Mensaje: %s\n",buffer2);	
					if( ( ip = calloc(1,16) ) == NULL )
					{
						logger("Error","Celular", "Memoria insuficiente", 0);
						return -1;
					}
					ip = strtok(buffer2,"*");
					if( ( port = calloc(1,6) ) == NULL )
					{
						logger("Error","Celular", "Memoria insuficiente", 0);
						return -1;
					}
					port = strtok(NULL,"*");		
					sokcel = crearSocket(port,ip);		
					if( enviarCame(YAMA,NROCELULAR,&sokcel,0,1,0,ID) == -1 )
					{
						logger("Error", "Celular", "Fallo el enviarCame de YAMA", 0);
						return -1;
					}
					//free(ip);	
					FD_SET(sokcel,master);
					if(sokcel>*fdmax) 
						*fdmax=sokcel; 
					} break;
				
		case YAMZ: {			
					logger("Informacion","Celular", "Comunicacion establecida", 1);
					if( ( buffer2 = calloc(1,largo2) ) == NULL )
					{
						logger("Error", "Celular", "Memoria Insuficiente", 0);
						return -1;
					}	
					if( ( numbytes = recv(sokMov,buffer2,largo2,0) ) < 0 )
					{
						logger("Error","Celular", "Fallo la recepcion de datos en atenderEntrada", 0);
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
					logger("Error","Celular", "El telefono solicitado no esta conectado a la central o esta apagado", 1);
					}break;
	   	
		case FINA: {
					if( enviarCame(FINZ,"\0",&sokMov,0,1,0,ID) == -1)
					{
						logger("Error", "Celular", "Fallo el enviarCame de FINZ", 0);
					}
					quitarDeVsockets(*vsockets,sokMov,CONFERENCIA);
					FD_CLR(sokMov,master);
					close(sokMov);	
					logger("Informacion","Celular", "Conversacion finalizada", 1);
					imprimirVsockets(*vsockets,CONFERENCIA);
					if( CARGAACT <= CARGAMIN ) //COMPROBACION DE BATERIA
					{
						if( enviarCame(FINA,"\0",*vsockets,CONFERENCIA,1,0,"\0") == -1 )	// le aviso a los otros cel
						{
							logger("Error", "Celular", "Fallo el enviarCame de FINA", 0);
							return -1;
						}
						if( enviarCame(CRGA, "\0", &central,0,1,0,"\0") == -1)
						{
							logger("Error", "Celular", "Fallo el enviarCame de CRGA", 0);
							return -1;
						}
					}
					}break;
		
		case FINZ: {
					FD_CLR(sokMov,master);
					logger("Informacion","Celular", "Conversacion finalizada", 1);
					quitarDeVsockets(*vsockets,sokMov,CONFERENCIA);
					imprimirVsockets(*vsockets,CONFERENCIA);
					close(sokMov);
					if( CARGAACT <= CARGAMIN ) //COMPROBACION DE BATERIA
					{
						if( enviarCame(FINA,"\0",*vsockets,CONFERENCIA,1,0,"\0") == -1 )	// le aviso a los otros cel
						{
							logger("Error", "Celular", "Fallo el enviarCame de FINA", 0);
							return -1;
						}
						if( enviarCame(CRGA, "\0", &central,0,1,0,"\0") == -1)
						{
							logger("Error", "Celular", "Fallo el enviarCame de CRGA", 0);
							return -1;
						}
					}
					}break;
		
		case YAMA: { 		
					logger("Informacion","Celular", "LLamada entrante", 1);
					if( ( buffer2 = calloc(1,largo2) ) == NULL )
					{
						logger("Error", "Celular", "Memoria Insuficiente", 0);
						return -1;
					}
					if((numbytes=recv(sokMov,buffer2,largo2,0))<0)
					{
						logger("Error","Celular", "Fallo la recepcion de datos en atenderEntrada", 0);
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
					if( !DUALMODE)
					{
						if( estoyTransfiriendo )
						{
							if( enviarCame(YAMX,"\0",&sokMov,0,1,0,"\0") == -1)
							{
								logger("Error", "Celular", "Fallo el enviarCame de YAMX", 0);
								return -1;
							}
							FD_CLR(sokMov,master);
							close(sokMov);
							return 1;
						}
					}
					if( ( pos = buscarLibreVsockets(*vsockets,CONFERENCIA)) == -1 ) 
					{
						//En este caso ya estoy conectado con maximo de celulares
						if( enviarCame(YAMX,"\0",&sokMov,0,1,0,"\0") == -1)
						{
							logger("Error", "Celular", "Fallo el enviarCame de YAMX", 0);
							return -1;
						}
						FD_CLR(sokMov,master);
						close(sokMov);
						return 1;
					}	
					agregarAVsockets(*vsockets,sokMov,pos, buffer2);
					printf("\tLLamada Aceptada.\n");
					imprimirVsockets(*vsockets,CONFERENCIA);
					if( enviarCame(YAMZ,NROCELULAR,&sokMov,0,1,0,"\0") == -1 )
					{
						logger("Error", "Celular", "Fallo el enviarCame de YAMZ", 0);
						return -1;
					}
					printf("Comunicacion Establecida\n");
				}break;
				
		case YAMX:{
					contadorID--; //emparejo
					logger("Informacion","Celular", "El celular esta ocupado", 1);
					FD_CLR(sokMov,master);
					close(sokMov);
					}break;	
   		case SMS: {
   					//printf("Has recibido un SMS\n");
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
					//ACA MOSTRAR EL QUE ME MANDO EL MENSAJE
					
					for(posicion=0;posicion<CONFERENCIA;posicion++) 
						{
							if(sokMov == (*vsockets)[posicion])
							printf("%s",NROS[posicion].NRO);
							
						}
					
					//printf("LLEGUE ACA pos: %d\n",posicion);				
					//printf("%s",NROS[posicion].NRO); 
					//printf("\nNO LLEGUE ACA\n");	
					printf(" - %s\n",buffer2);
					
					
					
					//printf("TTL= %d\n",cabe->TTL);
					quitarDeVsockets(*vsockets,sokMov,CONFERENCIA);
					if( enviarCame(SMS,buffer2,*vsockets,CONFERENCIA,cabe->TTL,cabe->HOPS,ID) == -1)
					{
						logger("Error","Celular", "Fallo el enviarCame de SMS", 0);
						return -1;
					}
					if( ( pos = buscarLibreVsockets(*vsockets,CONFERENCIA)) == -1)
						logger("Error","Celular", "Deficiencia en el programa", 0);	
					agregarAVsockets(*vsockets,sokMov,pos, AUXILIAR);
					if(soyMigrado)
						robot( *vsockets, CONFERENCIA );
					free(buffer2);
					}break;	
   		case ENVX: {
					contadorID--;
					logger("Error","Celular", "El celular solicitado no se encuentra en la red", 1);
					}break;			
  		case ENVZ: {
   					if( ESTADO == 0 ) //VER SI ESTA BIEN ESTO?
					{
						if( enviarCame(CALX,"\0",&sokcel,0,1,0,"\0") == -1 )
						{
							logger("Error","Celular", "Fallo el enviarCame de CALX", 0);
							return -1;
						}
						return 1;
					}
					//printf("Se Recibio ENVZ!\n");
   					if( ( buffer2 = calloc(1, largo2) ) == NULL )
					{
						logger("Error","Celular", "Memoria Insuficiente", 0);
						return -1;
					}
					if( ( numbytes = recv(sokMov,buffer2,largo2,0) ) < 0 )
					{
						logger("Error","Celular", "Fallo la recepcion de datos en atenderEntrada", 0);
						close(sokMov);
						return -1;
					}
					if( ( ip = calloc(1,16) ) == NULL )
					{
						logger("Error","Celular", "Memoria Insuficiente", 0);
						return -1;
					}
					ip = strtok(buffer2,"*");
					if( ( port = calloc(1,6) ) == NULL )
					{
						logger("Error","Celular", "Memoria Insuficiente", 0);
						return -1;
					}
					port = strtok(NULL,"*");
					aux = atoi(port);
					sprintf(portFile ,"%d",aux+1);
					if( estoyTransfiriendo == 0 )
					{
						estoyTransfiriendo=1;		
						if( ( enviador(ip,portFile,master,*fdmax) ) == -1 )
						{
							estoyTransfiriendo=0;
							logger("Error","Celular", "Fallo el Envio de Archivo", 1);
							return 0;	
						}  
					}
					else 
						logger("Error","Celular", "El canal de transferencia esta ocupado.No se pudo enviar", 1);				
					//free( ip );
					//free( port );
					//free( buffer2);
					}break;
		case CRGZ:	{
					if( ( buffer2 = calloc(1, largo2) ) == NULL )
					{
						logger("Error","Celular", "Memoria Insuficiente", 0);
						return -1;
					}
					if( ( numbytes = recv(sokMov,buffer2,largo2,0) ) < 0 )
					{
						logger("Error","Celular", "Fallo la recepcion de datos en atenderEntrada", 0);
						close(sokMov);
						return -1;
					}
					if( ( ip = calloc(1,16) ) == NULL )
					{
						logger("Error","Celular", "Memoria Insuficiente", 0);
						return -1;
					}
					ip = strtok(buffer2,"*");
					if( ( port = calloc(1,6) ) == NULL )
					{
						logger("Error","Celular", "Memoria Insuficiente", 0);
						return -1;
					}
					port = strtok(NULL,"*");
					sokCargador = crearSocket(port,ip);
					//printf("El cargador es el socket: %d\n", sokCargador);
					FD_SET(sokCargador, master);
					if( sokCargador > *fdmax) 
						*fdmax = sokCargador;
					resto = CARGAMAX - CARGAACT;
					sprintf(restoCarga, "%d", resto);
					if( ( bufData = calloc( 1, strlen(restoCarga)+1+strlen(NROCELULAR)+1+1) ) == NULL )
					{
						logger("Error","Celular", "Memoria Insufciente", 0);
						return -1;
					}
					strcpy(bufData, restoCarga);
					bufData[strlen(restoCarga)] ='*';
					bufData[strlen(restoCarga)+1] ='\0';
					strcat( bufData, NROCELULAR);
					bufData[strlen(restoCarga)+1+strlen(NROCELULAR)+1] = '*';
					bufData[strlen(restoCarga)+1+strlen(NROCELULAR)+1+1] ='\0';
					if( enviarCame(GBBA, bufData, &sokCargador, 0, 1, 0,"\0") == -1 )
					{
						logger("Error","Celular", "Fallo el enviarCame de GBBA", 0);
						return -1;
					}	
					//free( ip );
					//free( port );
					//free( buffer2);
					}break;
		case CRGX:	{
						logger("Informacion","Celular", "No se encuentra el cargador.Intente mas tarde", 1);
						if( ESTADO != 0 )
						{
							ESTADO = 0; //Apago el Celular
							logger("Informacion","Celular", "Celular apagado", 1);
							if( enviarCame(APGA,"\0",&central,0,1,0,ID) == -1 )
							{
								logger("Error","Celular", "Fallo el enviarCame de APGA", 0);
								return -1;
							}
							IDMensaje();
						}
					}break;
		case GBBZ:	{
					logger("Informacion","Celular", "El celular comenzo a cargarse y se mantendra apagado", 1);
					ESTADO = 0; //Apago el Celular
					printf("Apagando el Celular\n");
					if( enviarCame(APGA,"\0",&central,0,1,0,ID) == -1 )
					{
						logger("Error","Celular", "Fallo el enviarCame de APGA", 0);
						return -1;
					}	
					IDMensaje();
					if( enviarCame(FINA,"\0",*vsockets,CONFERENCIA,1,0,ID) == -1 )
					{
						logger("Error","Celular", "Fallo el enviarCame de APGA", 0);
						return -1;
					}
					}break;
		case GBBX:	{
						logger("Informacion","Celular", "El cargador esta apagado", 1);
						if( ESTADO != 0 )
						{
							ESTADO = 0; //Apago el Celular
							logger("Informacion","Celular", "Celular apagado", 1);
							enviarCame(APGA,"\0",&central,0,1,0,ID);
							IDMensaje();
						}
					}break;
		case GCCA: {
					logger("Informacion","Celular", "Carga del celular completada.Puede encenderlo", 1);
					CARGAACT = CARGAMAX;
					}break;
		case GCCX:	{
					if( ( buffer2 = calloc(1, largo2) )== NULL )
					{
						logger("Error","Celular", "Memoria Insuficiente", 0);
						return -1;
					}
					if( ( numbytes = recv(sokMov,buffer2,largo2,0) ) < 0 )
					{
						logger("Error","Celular", "Fallo la recepcion de datos en atenderEntrada", 0);
						close(sokMov);
						return -1;
					}
					CARGAACT = CARGAMAX - atoi(buffer2);
					logger("Informacion","Celular", "El cargador se ha apagado. No se ha podido completar la carga", 1);
					printf("La carga actual es: %d\n", CARGAACT);
					//printf("Para completar la carga vuelva a cargarlo mas tarde\n");
					}break;
		case MGRZ: {
					printf("\tMIGRACION EN PROCESO!!\n");
		
	
				}break;
		case MGRX: {
					printf("\tMIGRACION CANCELADA!!!\n");
			
		
		
				}break;
				
		case PARA: {
				estados = calloc( 1, sizeof( migracion) );
				strcpy(estados->NROCELULAR, NROCELULAR);
				estados->DUALMODE = DUALMODE;
				estados->CARGAACT = CARGAACT;
				estados->CARGAMIN = CARGAMIN;
				estados->CARGAMAX = CARGAMAX;
				estados->CONFERENCIA = CONFERENCIA;
				estados->TRANSFERENCIA = 0;
				estados->contadorID = contadorID;
				
				enviarCame( PARZ, (char*)estados, &sokMov,0,1,0,"\0"); 
				
				}break;
				
		case KILL:	{
				enviarCame(APGA,"\0",&central,0,1,0,ID);
				enviarCame(MGGO,NROCELULAR,*vsockets,CONFERENCIA,1,0,"\0");
				logger("Informacion","Celular", "Migracion Exitosa", 1);
				exit(1);
		
				}break;
		case MGGO:	{
				if( ( buffer2 = calloc(1, largo2) )== NULL )
				{
					logger("Error","Celular", "Memoria Insuficiente", 0);
					return -1;
				}
				if( ( numbytes = recv(sokMov,buffer2,largo2,0) ) < 0 )
				{
					logger("Error","Celular", "Fallo la recepcion de datos en atenderEntrada", 0);
					close(sokMov);
					return -1;
				}
				printf("Voy a ReLlamar a : %s\n",buffer2);
				if( enviarCame(CALA,buffer2,&central,0,1,0,ID) == -1)
				{
					logger("Error","Celular", "Fallo el enviarCame de CALA", 0);
					return -1;
				}
				printf("Buscando...\n");
				
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
		logger("Error","Celular", "Memoria insuficiente", 0);
		return -1;
	}
	if( (numbytes = read(teclado,buffer,MAXDATASIZE) ) <=0 )
	{
		logger("Error","Celular", "Fallo recepcion de datos al leer teclado en atenderTeclado", 0);
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
	if((ESTADO==0) && (pos!=3) && (pos!=5) ) return 1;
	switch(pos) 
	{	
		case -1:{ 	//Voy a Enviar UN MSJ POR TECLADO		
					if(ESTADO==0) 
					{
						printf("El celular esta apagado. Primero Debe prenderlo\n");
						free(buffer);
						return 1;
					}
					if( enviarCame(SMS,buffer,vsockets,CONFERENCIA,SMSTTL,0,ID) == -1)
					{
						logger("Error","Celular", "Fallo el enviarCame de SMS", 0);
						return -1;
					}
				}break;
		case 0: { 	//RECIBI "LLAMAR" POR TECLADO
					if(ESTADO==0) 
					{
						printf("El celular esta apagado. Primero Debe prenderlo\n");
						free(buffer);
						return 1;
					}
					if( CARGAACT <= CARGAMIN ) //CONTROLA SI TENGO BATERIA
					{
						logger("Informacion","Celular", "Bateria Baja. El celular sera cargado", 1);
						if( enviarCame(CRGA, "\0", &central,0,1,0,"\0") == -1 )
						{
							logger("Error","Celular", "Fallo el enviarCame de CRGA", 0);
							return -1;
						}	
						free(buffer);
						return 1;
					}
					if(senial == OFFLINE)
					{
						logger("Informacion","Celular", "Sin senial", 1);
						free(buffer);
						return 1;
					}
					if( DUALMODE == 0 )
					{
						if( estoyTransfiriendo )
						{
							logger("Informacion","Celular", "No soporta DUALMODE", 1);
							free(buffer);
							return 1;
						}
					}
					if( buscarLibreVsockets(vsockets,CONFERENCIA)==-1)
					{
						logger("Informacion","Celular", "Limite de conferencia alcanzado", 1);
						free(buffer);
						return 1;
					}
					printf("A quien deseas llamar?\n");
					if(	(numbytes=read(teclado,buffer,MAXDATASIZE) ) <=0 )
					{
						logger("Error","Celular", "Error al leer teclado en atenderTeclado", 1);
						free(buffer);
						return -1;
					}
					buffer[numbytes-1]='\0';	
					if( (strcmp(buffer,NROCELULAR) ) == 0) 
					{
						printf("Ese es tu numero!\n");
						free(buffer);
						return 1;
					}
					if( enviarCame(CALA,buffer,&central,0,1,0,ID) == -1)
					{
						logger("Error","Celular", "Fallo el enviarCame de CALA", 0);
						return -1;
					}
					printf("Buscando...\n");
				}break;
		case 1: {	//RECIBI "CORTAR" POR TECLADO
					logger("Informacion","Celular", "Cortando todas las comunicaciones", 1); 
					if( enviarCame(FINA,"\0",vsockets,CONFERENCIA,1,0,ID) == -1 )
					{
						logger("Error","Celular", "Fallo el enviarCame de FINA", 0);
						return -1;
					}
					if( CARGAACT <= CARGAMIN ) //CONTROLA SI TENGO BATERIA
					{
						logger("Informacion","Celular", "Bateria Baja. El celular sera cargado", 1);
						if( enviarCame(CRGA, "\0", &central,0,1,0,"\0") == -1)
						{
							logger("Error","Celular", "Fallo el enviarCame de CRGA", 0);
							return -1;
						}
						free(buffer);
						return 1;
					}	
				}break;
		case 2: {	//RECIBI "APAGAR" POR TECLADO 
					if(ESTADO==0) 
					{
						printf("El celular ya se encuentra apagado\n");
						free(buffer);
						return 1;
					}
					ESTADO = 0; //Apago el Celular
					logger("Informacion","Celular", "Celular apagado", 1);;
					if(senial == OFFLINE)
					{
						logger("Informacion","Celular", "Sin senial", 1);
						free(buffer);
						return 1;
					}
					free(nombreCentral);
					if( enviarCame(APGA,"\0",&central,0,1,0,ID) == -1)
					{
						logger("Error","Celular", "Fallo el enviarCame de APGA", 0);
						return -1;
					}
					IDMensaje();
					if( enviarCame(FINA,"\0",vsockets,CONFERENCIA,1,0,ID) == -1 )
					{
						logger("Error","Celular", "Fallo el enviarCame de FINA", 0);
						return -1;
					}
					if( CARGAACT <= CARGAMIN ) //CONTROLA SI TENGO BATERIA
					{
						logger("Informacion","Celular", "Bateria Baja. El celular sera cargado", 1);
						if( enviarCame(CRGA, "\0", &central,0,1,0,"\0") == -1)
						{
							logger("Error","Celular", "Fallo el enviarCame de CRGA", 0);
							return -1;
						}
						free(buffer);
						return 1;
					}		
				}break;
		case 3: {	//RECIBI "PRENDE" POR TECLADO
					if(ESTADO == 1)
					{
						printf("El celular ya esta encendido\n");
						free(buffer);
						return 1;
					}
					if( CARGAACT <= CARGAMIN ) //CONTROLA SI TENGO BATERIA
					{
						logger("Informacion","Celular", "Bateria Baja. El celular sera cargado", 1);
						if( enviarCame(CRGA, "\0", &central,0,1,0,"\0") == -1 )
						{
							logger("Error","Celular", "Fallo el enviarCame de CRGA", 0);
							return -1;
						}
						free(buffer);
						return 1;
					}	
					ESTADO = 1; //Prendo el Celular
					logger("Informacion","Celular", "Celular encendido", 1);
					if( senial  == OFFLINE )
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
					else
					{
						if( entrarEnRed(NROCELULAR,central,LISPORT) == -1)
						{
							logger("Error","Celular", "No se pudo establecer la conexion con la central", 0);
						}
					}
				}break;	
		case 4: { 	//RECIBI "ENVIAR" POR TECLADO
					if(ESTADO==0) 
					{
						printf("El celular esta apagado. Primero Debe prenderlo\n");
						free(buffer);
						return 1;
					}
					if( CARGAACT <= CARGAMIN ) //CONTROLA SI TENGO BATERIA
					{
						logger("Informacion","Celular", "Bateria Baja. El celular sera cargado", 1);
						if( enviarCame(CRGA, "\0", &central,0,1,0,"\0") == -1 )
						{
							logger("Error","Celular", "Fallo el enviarCame de CRGA", 0);
							return -1;
						}	
						free(buffer);
						return 1;
					}
					if( TRANSFERENCIA == 0)
					{
						printf("El celular no puede enviar archivos\n");
						free(buffer);
						return 1;
					}
					if(senial == OFFLINE)
					{
						logger("Informacion","Celular", "No hay senal", 1);
						free(buffer);
						return 1;
					}	
					if( DUALMODE == 0 )
					{
						if( hayAlguien( vsockets, CONFERENCIA ) )
						{
							logger("Informacion","Celular", "No soporta DUALMODE", 1);
							free(buffer);
							return 1;
						}
					}	
					//printf("Se Va a Enviar un Archivo\n");
					printf("A quien deseas Enviar?\n");
					if( (numbytes = read(teclado,buffer,MAXDATASIZE) ) <=0 )
					{
						logger("Error","Celular", "Error al leer teclado en atenderTeclado", 0);
						free(buffer);
						return -1;
					}
					buffer[numbytes-1]='\0';	
					if( (strcmp(buffer,NROCELULAR) ) == 0) 
					{
						printf("Ese es tu numero!\n");
						free(buffer);
						return 1;
					}
					if( enviarCame(ENVA,buffer,&central,0,1,0,"\0") == -1)
					{
						logger("Error","Celular", "Fallo el enviarCame de ENVA", 0);
						return -1;
					}
				}break;
		case 5:	{  //RECIBI "CARGAR" POR TECLADO
					if(senial == OFFLINE)
					{
						logger("Informacion","Celular", "No hay senal", 1);
						free(buffer);
						return 1;
					}
					if( enviarCame(CRGA, "\0", &central,0,1,0,"\0") == -1)
					{
						logger("Error","Celular", "Fallo el enviarCame de CRGA", 0);
						return -1;
					}
				}break;
		
		case 6:	{	//Recibi "migrar" por teclado
				printf("Decidiste migrar, te avisare apenas migres!\n");
				ipPort=calloc(1,strlen(MIIP)+1+strlen(LISPORT)+1+1);
				strcpy(ipPort,MIIP);
				ipPort[strlen(MIIP)]='*';
				ipPort[strlen(MIIP)+1]='\0';
				strcat(ipPort,LISPORT);
				ipPort[strlen(ipPort)+1]='\0';
				ipPort[strlen(ipPort)]='*';
				//printf("el buffer dice: %s\n",ipPort);
				enviarCame(MGRA, ipPort, &socketCentral,0,1,0,ID);
				free(ipPort);
			} break;
					
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
		logger("Error","Celular", "Error en el accept de atenderConexion", 1);
	else
		{ 
			//O sea que no hubo error  de esta nueva conexion
			logger("Informacion","Celular", "Nueva conexion", 0);
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
	printf("*       CELULARES CONECTADOS               *\n"); 
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
		logger("Error", "Celular", "Memoria Insuficiente", 0);
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
		logger("Error", "Celular", "Fallo el enviarCame de CONA", 0);
		return -1;
	}
	if( recv( socket, buffer, 5, 0 ) == -1 )
	{
		logger("Error", "Celular", "Fallo la recepcion de datos en EntrarEnRed", 0);
		close(socket);
		free(buffer2);
		return -1;
	}
	buffer[4] = '\0';
	if( strcmp("CONX", buffer) == 0)
	{
		logger("Informacion", "Celular", "Central LLena. Esperando por espacio en la red.", 1);
		if( recv( socket, buffer, 5, 0 ) == -1 )
		{
			logger("Error", "Celular", "Fallo la recepcion de datos en EntrarEnRed", 0);
			close(socket);
			free(buffer2);
			return -1;
		}
		buffer[4] = '\0';
	}
	if(strcmp("CLIN",buffer) == 0 ) /* VER PARAMETROS */ 
	{
		logger("Informacion","Celular", "La central es para telefonos de linea", 1);
		close(socket);
		free(buffer2);
		return -1;
	}
	if(strcmp("CONZ",buffer) != 0 ) /* VER PARAMETROS */ 
	{
		logger("Error", "Celular", "Fallo la sincronizacion", 0);
		close(socket);
		free(buffer2);
		return -1;
	}
	nombreCentral=calloc(1,6);
	if( recv( socket, nombreCentral, 6, 0 ) == -1 )
		{
			logger("Error", "Celular", "Fallo la recepcion de datos en EntrarEnRed", 0);
			close(socket);
			free(buffer2);
			return -1;
		}
	logger("Informacion", "Celular", "Conexion Establecida con la Central. Celular En Red", 1);
	printf("********************************************\n");
	printf("*  CONECTADO CON LA CENTRAL:  %s        *\n",nombreCentral);
	printf("********************************************\n");
	senial = ONLINE;
	free(buffer2);
	return 1;
}

//************************************************************************************************/

int enviador(char*ip,char*puerto,fd_set *master,int fdmax)
{
	int socket;
	char bufPRM[5];
	char buffer[BUFFENVIO];
	FILE*Archivo;
	int i;
	char *path;
	char *nombre;
	int flag=1;
	int numbytes,num2;
	int*tamanio;
	pid_t child;
	int j; 
	char buff[3];

	//printf("Ejecutando enviador\n");
	if( (nombre = calloc(1,TAMMAXNOMBRE)) == NULL)
	{
		logger("Error","Celular", "Memoria Insuficiente", 0);
		return -1;
	}
	printf("Que archivo queres enviar?\n");
	if((numbytes=read(STDIN,nombre,TAMMAXNOMBRE))<=0)
	{
		logger("Error","Celular", "Fallo recepcion de datos en Enviador", 0);
		free(nombre);
		return -1;
	}
	nombre[numbytes-1]='\0';
	if( ( path = calloc(1,strlen(PATH)+strlen(nombre)+1)) == NULL)
	{
		logger("Error","Celular", "Memoria Insuficiente", 0);
		free(nombre);
		return -1;
	}
	strcpy(path,PATH);
	strcpy(&path[strlen(PATH)],nombre);
	printf("Se enviar el archivo ubicado en: %s\n",path);
	numbytes = -1;
	if( (tamanio = malloc(sizeof(int) ) ) == NULL)
	{
		logger("Error","Celular", "Memoria Insuficiente", 0);
		free(nombre);
		free(path);
		return -1;
	}
	if( ( Archivo = fopen(path,"r") ) == NULL )
	{
		logger("Error","Celular", "No se pudo abrir el archivo", 1);
		free(tamanio);
		free(path);
		free(nombre);
		return -1;	
	}
	estoyTransfiriendo=1;
	if( (child=fork()) == 0 )
	{
		logger("Error","Celular", "Se ha creado el hijo que atendera el envio de archivos", 0);
		//Este es el hijo
		for(j=0;j<=fdmax;j++)
		{
			if(FD_ISSET(j,master))
			{	
				close(j);
				FD_CLR(j,master);
			}
		}
		socket = crearSocket(puerto,ip);
		//Habria que hacer un send para consultar al receptor si tiene dualmode y si puede recibir
		if( recv( socket,bufPRM, 5, 0 ) == -1 )
		{
			logger("Error","Celular", "Fallo la recepcion de datos en Enviador", 0);
			close(socket);
			free(path);
			free(nombre);
			free(tamanio);
			return -1;
		}
		bufPRM[4] = '\0';
		//printf("se recibio %s\n",bufPRM);
		if( strcmp("PRMZ",bufPRM) == 0)
		{	
			*tamanio = strlen(nombre);
			send(socket,(char*)tamanio ,sizeof(int),0);
			//printf("%d\n",*((int*)tamanio));
			send(socket,nombre,*((int*)tamanio),0);
			logger("Informacion","Celular", "Envio en Progreso...", 1);
			while(flag)
			{
				if( ESTADO == 0)
				{
					flag=0;
				}
				numbytes=fread(buffer,1,BUFFENVIO,Archivo);
				if(numbytes != 0) 
				{
					if( write(socket,buffer,numbytes) != numbytes )
					{
						printf("\tFallo el Envio\n");
						free(path);
						free(nombre);
						free(tamanio);
						fclose(Archivo);
						close(socket);
						printf("\tTransferencia Cancelada\n");
						exit(EXIT_SUCCESS);
					}
				}
				if(numbytes == 0) 
					flag=0;
		 	}
			printf("\tTransferencia Completa\n");
			logger("Informacion","Celular", "Envio Finalizado", 1);
		}
			else 	
		{
			if( strcmp("PRMX",bufPRM) == 0)
				logger("Informacion","Celular", "El celular no puede recibir archivos", 1);
			else 
				printf("Mala recepcion de la confirmacion de envio\n");
		}	
		free(path);
		free(nombre);
		free(tamanio);
		fclose(Archivo);
		close(socket);
		exit(EXIT_SUCCESS);
	}
	
return 1;
}

//------------------------------------------------------------------------------------------

int receptor(int listenerFile,char*SENDPORT, fd_set *master,int fdmax,int estoyHablando)
{
	int socket;
	struct sockaddr_in cliente; //esctructura con la info de un Celular que me llama
	int addrlen;
	int numbytes=50;
	int num2;
	char bufPRM[5]; 
	char buffer[BUFFENVIO];
	int flag=1;
	char*nombre;
	FILE* Archivo;
	char* tamanio;
	int j;
	pid_t child;

	if( ( tamanio = malloc( sizeof(int) ) ) == NULL)
	{
		logger("Error","Celular", "Memoria Insuficiente", 0);
		return -1;
	}
	addrlen = sizeof(cliente);
	if((socket = accept(listenerFile,(struct sockaddr *)&cliente,&addrlen))== -1)
	{
		logger("Error","Celular", "Fallo el accept del receptor", 0);
		return -1;
	}
	  
	if( (child=fork()) == 0 )
	{
		logger("Informacion","Celular", "Se ha creado el hijo que atendera la recepcion del archivo", 0);
		//printf("Nuevo Canal del Puerto de Archivos de: %s\n",inet_ntoa(cliente.sin_addr));	
		for(j=0;j<=fdmax;j++)
		{
			if(FD_ISSET(j,master))
			{	
				close(j);
				FD_CLR(j,master);
			}
		}
		//**********CON ABUSO DE NOTACION********//	
		if((!TRANSFERENCIA)||( !( DUALMODE || ( (!estoyTransfiriendo) && (!estoyHablando) ) ) ) )
			send(socket,"PRMX" ,sizeof("PRMX"),0);
		else 
		{
			
			send(socket,"PRMZ" ,sizeof("PRMZ"),0);
			//Habria que recibir la consulta, verificar el dualmode y responderle si puedo o no recibir el archivo
			if( ( nombre = calloc(1, TAMMAXNOMBRE) ) == NULL )
			{
				logger("Error","Celular", "Memoria Insuficiente", 0);
				return -1;
			}
			recv(socket,tamanio,sizeof(int),0);
			//printf("%d\n",*((int*)tamanio));
			recv(socket,nombre,*((int*)tamanio),0);
			if( ( Archivo = fopen(nombre,"w") ) == NULL )
			{
				logger("Error","Celular", "Memoria Insuficiente", 0);
				return -1;
			}
			printf( "Se recibira el archivo: %s\n", nombre);
			logger("Informacion","Celular", "Recibiendo Archivo...", 1);
			while(flag)
			{
				numbytes=read(socket,buffer,BUFFENVIO);
				if (numbytes!=0)
				{
					fwrite(buffer,1,numbytes,Archivo);
				}
				if(numbytes==0) 
				{
					flag=0;
				}
				if(numbytes < 0) 
				{
					printf("Transferencia cancelada\n");
					fclose(Archivo);
					close(socket);
					exit(EXIT_SUCCESS);
				}
			}
			printf("\tTransferencia Completa\n");
			logger("Informacion","Celular", "Recepcion Finalizada", 1);
		}
		fclose(Archivo);
		close(socket);
		exit(EXIT_SUCCESS);
	}
	else
	{ 
		// Si sos el padre
		estoyTransfiriendo=1; //NO TOCAR ESTO!	
		close(socket);	
	} 
	return 1;
}

//-----------------------------------------------------------------------------------------------------
int cargarCeluConfig( 	char* ruta,
			char** num,
			char** ip,
			char** port,
			int* transf,
			int* conf,
			int* cargamin,
			int* cargamax,
			int* dualmode,
			char** portcent,
			char** MIIP,
			char** PATH
			)
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
		palabra = strtok(linea,"--" );
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
					*transf = atoi(palabra);
				break;
			case 4:	
					*conf = atoi(palabra);
				break;
			case 5: 
					*cargamin = atoi(palabra);
				break;
			case 6: 
					*cargamax = atoi(palabra);
				break;
			case 7: 
					*dualmode = atoi(palabra);
				break;
			case 8: 
					if((*portcent = malloc( strlen(palabra) + 1 )) == NULL)
					{
						printf("Memoria insuficiente\n");
						return -1;
					}
					strcpy( *portcent,palabra);	
				break;
			case 9:
					if((*MIIP = malloc( strlen(palabra) + 1 )) == NULL)
					{
						printf("Memoria insuficiente\n");
						return -1;
					}
					strcpy( *MIIP,palabra);
				break;
			case 10:	
					if((*PATH = malloc( strlen(palabra) + 1 )) == NULL)
					{
						printf("Memoria insuficiente\n");
						return -1;
					}
					strcpy( *PATH,palabra);
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
	free(linea);
	return 1;
}

//------------------------------------------------------------------------------------
void sigHandler(int senial)
{
char *ipPort;
	switch(senial)
	{
		case SIGTERM: 
					//Es un Kill sin nada, solo pid
					//Log
					break;
		case SIGCHLD:
					estoyTransfiriendo = 0;
					logger("Informacion","Celular", "Matando al hijo", 0);
					wait(NULL);
					//Log
					break;
		case SIGHUP: 
					IDMensaje();
					enviarCame(CRGA, "\0", &socketCentral,0,1,0,"\0");
					break;
		case SIGUSR1:		
					IDMensaje();
					ipPort=calloc(1,strlen(MIIP)+1+strlen(LISPORT)+1+1);
					strcpy(ipPort,MIIP);
					ipPort[strlen(MIIP)]='*';
					ipPort[strlen(MIIP)+1]='\0';
					strcat(ipPort,LISPORT);
					ipPort[strlen(ipPort)+1]='\0';
					ipPort[strlen(ipPort)]='*';
					enviarCame(MGRA, ipPort, &socketCentral,0,1,0,ID);
					free(ipPort);
					break;
		default:
					break;
	}

}

//---------------------------------------------------------------------------

void seniales(int senial,void (*funcion) (int nroSenial))
{
	if(signal(senial,funcion)==SIG_ERR)
	{
		perror("Signal");
		exit(EXIT_FAILURE);
	}
}

//------------------------------------------------------------------------------

void select_controlar(fd_set * readfds,int listenerFile,fd_set *master)
{
	if(errno==EINTR)
	{
		FD_ZERO(readfds);
		FD_SET(listenerFile,master);
	}
	else
	{
		perror("select");
		exit(EXIT_FAILURE);
	}
}

//------------------------------------------------------------------------------
int robot( int *vsockets, int CONFERENCIA )
{
	if( cMensj == MENSAJES)
		cMensj = 0;
	enviarCame(SMS,vMensajes[cMensj],vsockets,CONFERENCIA,1,0,ID);
	cMensj++;
	return 1;
}
