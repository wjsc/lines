#include "soporteGeneral.h"
#include "soporteCargador.h"

//VARIABLES GLOBALES
extern int algoritmo;
extern int estado;
extern int numCelCarg;
extern int timeRafaga;
extern int tasa;
extern char centConexion;
extern const char* vcodigos[MAXCODIGOS];
extern int contadorID;
extern char centralID[10];  //Este seria el celular ID, pero lo dejo asi para que sea igual a la central
extern char ID[15];  //Id de los mensajes...concatenacion de centralID + contador

//FUNCIONES AUXILIARES/////////////////////////////////////////////////////////////////////////

int cargarCargConfig( 	char* ruta,
			            char** ip,
						char** port,
						int* capacidad,
						int* tasa,
						char** ipCent,
						char** portCent,
						int *timeRafaga	)
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
					if((*ip = malloc( strlen(palabra) + 1 )) == NULL)
					{
						printf("Memoria insuficiente\n");
						return -1;
					}
					strcpy( *ip, palabra );
		
					}break;
			case 1:	
					if((*port = malloc( strlen(palabra) + 1 )) == NULL)
					{
						printf("Memoria insuficiente\n");
						return -1;
					}
					strcpy( *port, palabra);
					break;
			case 2:	
					*capacidad = atoi(palabra);
					break;
			case 3: 
					*tasa = atoi(palabra);
					break;
			case 4:	
					if((*ipCent = malloc( strlen(palabra) + 1 )) == NULL)
					{
						printf("Memoria insuficiente\n");
						return -1;
					}
					strcpy( *ipCent, palabra);
					break;
			case 5: 
					if((*portCent = malloc( strlen(palabra) + 1 )) == NULL)
					{
						printf("Memoria insuficiente\n");
						return -1;
					}
					strcpy( *portCent, palabra);
					break;
			case 6: 
					*timeRafaga = atoi(palabra);
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

///////////////////////***************************************////////////////////////

int conectarConCentral(int sok, char *port, char *ip)
{
	char *buffer;

	if ( (buffer = calloc( 1, strlen(port) + 1 + strlen(ip) + 1 + 1 ) ) == NULL )
	{
		logger("Error","ConectarConCentral", "Memoria Insuficiente", 0);
		return -1;
	}
	strcpy(buffer, ip);
	buffer[strlen(ip)] = '*';
	buffer[strlen(ip) + 1] = '\0';
	strcat(buffer, port);
	buffer[strlen(ip) + strlen(port) + 1] = '*';
	buffer[strlen(ip) + strlen(port) + 1 + 1] = '\0';
	if( enviarCame(GCOA,buffer,&sok,0,1,0,"\0") == -1 )
	{
		logger("Error","Cargador", "No se pudo enviarCame GCOA.", 1);
		return -1;
	}
	free( buffer);
	return 1;
}
//------------------------------
	 
int atenderConexionCelular(	int listener,int *fdmax,fd_set *master,int capacidad,struct timeval *tiempo,
							ptrNodoCola *CC_Carga, ptrNodoCola *CF_Carga, ptrNodoCola *CC_Espera,
							ptrNodoCola *CF_Espera)
{
	struct sockaddr_in cliente; //esctructura con la info de un Celular que me llama
	int addrlen;
	int new_fd;
	char *buffer;
	cabecera *cabe;
	int numbytes;
	char *restoCarga;
	nodo valor;
	char *NRO;
	
	//Hay un celular entrante que va a ser cargado!
	addrlen = sizeof( cliente );
	//Acepto el nuevo socket... es decir el nuevo celular a cargar.
	if( ( new_fd = accept(listener,(struct sockaddr *)&cliente,&addrlen) ) == -1 )	
	{	
		logger("Error","Cargador", "Fallo aceptacion de un nuevo celular", 0);
		return -1;	
	}
	//printf("Nueva solicitud de carga desde: %s\n",inet_ntoa(cliente.sin_addr));
	if( (buffer = calloc( 1, sizeof(cabecera) ) ) == NULL)
	{
		logger("Error","Cargador", "Memoria Insuficiente", 0);
		return -1;
	}
	if( recv( new_fd, buffer, sizeof(cabecera), 0 ) == -1 )
	{
		logger("Error","Cargador", "Fallo al recibir datos en AtenderConexionCelular", 0);
		close(new_fd);
		return -1;
	}
	cabe = (cabecera*)buffer;
	if( cabe->tipo == GBBA )
	{	
		if( (buffer = realloc( buffer, cabe->largo ) ) == NULL )
		{
			logger("Error","Cargador", "Memoria Insuficiente.", 0);
			return -1;
		}
		if( ( numbytes = recv(new_fd,buffer,cabe->largo,0) ) <0)
		{
			//Log Error en el Recv
			logger("Error","Cargador", "Fallo al recibir datos en AtenderConexionCelular", 0);
			close(new_fd);
			return -1;
		}	
		restoCarga = strtok(buffer,"*");
		NRO = strtok(NULL,"*");
		valor.restoCarga = atoi(restoCarga);
		valor.socket = new_fd;
		strcpy( valor.NRO, NRO );
		valor.NRO[10] = '\0'; 
		if( estado == APAGADO)
		{
			if( enviarCame(GBBX,"\0",&new_fd,0,1,0,"\0") == -1 )
			{
				logger("Error","Cargador", "Fallo en AtenderConexionCelular el enviarCame de GBBX ", 0);
				return -1;
			}
			free(buffer);
			return 1;
		}
	
		if( numCelCarg < capacidad)
		{
			switch( algoritmo )
			{
				case FIFO: 	if( agregar(CC_Carga, CF_Carga, &valor) == -1 )
							{
								logger("Error","Cargador", "No se pudo agregar el Celular a la Cola de Carga", 0);
								return -1;
							}
							break;
				case RR:	if( agregarComoRR( CC_Carga, CF_Carga, &valor) == -1)
							{
								logger("Error","Cargador", "No se pudo agregar el Celular a la Cola de Carga", 0);
								return -1;
							}
							break;
				case SRT: 	if( agregarComoSRT(CC_Carga, CF_Carga, &valor) == -1 )
							{
								logger("Error","Cargador", "No se pudo agregar el Celular a la Cola de Carga", 0);
								return -1;
							}
							break;
			}
			numCelCarg++;
		}
		else 
			if( agregar(CC_Espera, CF_Espera, &valor) == -1 )
			{
				logger("Error","Cargador", "No se pudo agregar el Celular a la Cola de Espera", 0);
				return -1;
			}
		printf("\n*************************\n");	
		printf(  "*     Cola de Carga     *\n" );
		printf(  "*************************\n");	
		imprimeCola( *CC_Carga );
		printf("\n*************************\n");	
		printf(  "*     Cola de Espera    *\n" );
		printf(  "*************************\n");
		imprimeCola( *CC_Espera );
		FD_SET(new_fd, master);
		if( new_fd > *fdmax ) 
			*fdmax = new_fd;
		if( asignarTiempo( CC_Carga, tiempo ) == -1 )
		{	
			logger("Error","Cargador", "No se pudo recalcular el tiempo de Carga", 0);
			return -1;
		}
		if( enviarCame(GBBZ,"\0",&new_fd,0,1,0,"\0") == -1 )
		{
			logger("Error","Cargador", "Fallo en AtenderConexionCelular el enviarCame de GBBZ", 0);
			return -1;
		}
	}
	else
	{
		logger("Error","Cargador", "Fallo del Protocolo", 0);
		return -1;
	}
	
	free(buffer);
	return 1;
}

//------------------------------------------------------------------------------------------		
int agregar( ptrNodoCola *colaFte, ptrNodoCola *colaFin, nodo* valor )
{ 
   ptrNodoCola ptrNuevo; /* apuntador a un nuevo nodo */

   ptrNuevo = malloc( sizeof( NodoCola ) );

   if ( ptrNuevo != NULL ) 
	{ 	/* es espacio disponible */ 
		ptrNuevo->info = *valor;
		ptrNuevo->sgte = NULL;

		/* si esta vacía inserta un nodo en la cabeza */
		if ( estaVacia( *colaFte ) ) 
		{
			*colaFte = ptrNuevo;
		}
		else 
		{
			( *colaFin )->sgte = ptrNuevo;
		}
		*colaFin = ptrNuevo;
   }
   else 
	{
		printf( "no se inserto. No hay memoria disponible.\n" );
		return -1;
	}
	return 1;
}
//------------------------------------------------------------------------------------
int agregarComoRR( ptrNodoCola *colaFte, ptrNodoCola *colaFin, nodo* valor )
{ 
   ptrNodoCola ptrNuevo; /* apuntador a un nuevo nodo */

   ptrNuevo = malloc( sizeof( NodoCola ) );
   if ( ptrNuevo != NULL ) 
   { /* es espacio disponible */ 
      ptrNuevo->info = *valor;
      ptrNuevo->sgte = NULL;

      /* si esta vacía inserta un nodo en la cabeza */
      if ( estaVacia( *colaFte ) ) 
	  {
         *colaFte = ptrNuevo;
      }
      else 
	  {
         ( *colaFin )->sgte = ptrNuevo;
      }
	  ptrNuevo->sgte = *colaFte;
      *colaFin = ptrNuevo;
   }
   else 
   {
      printf( "no se inserto. No hay memoria disponible.\n");
      return -1;
   }
	return 1;
}
//--------------------------------------------------------------------------------------
int estaVacia( ptrNodoCola colaFte )
{ 
   return colaFte == NULL;
}
//---------------------------------------------------------------------------------------
int agregarComoSRT( ptrNodoCola *colaFte, ptrNodoCola *colaFin, nodo* valor )
{
   ptrNodoCola ptrNuevo;    /* apuntador a un nuevo nodo */
   ptrNodoCola ptrAnterior; /* apuntador a un nodo previo de la lista */
   ptrNodoCola ptrActual;   /* apuntador al nodo actual de la lista */

   ptrNuevo = malloc( sizeof( NodoCola ) ); /* crea un nodo */

   if ( ptrNuevo != NULL ) 
   {    /* es espacio disponible */
      ptrNuevo->info = *valor;   /* coloca el valor en el nodo */
      ptrNuevo->sgte = NULL; /* el nodo no se liga a otro nodo */

      ptrAnterior = NULL;
      ptrActual = *colaFte;

      /* ciclo para localizar la ubicación correcta en la lista */
      while ( ptrActual != NULL && valor->restoCarga > ptrActual->info.restoCarga ) 
	  {	  
         ptrAnterior = ptrActual;          /* entra al ...   */
         ptrActual = ptrActual->sgte;  /* ... siguiente nodo */
      }

      /* inserta un nuevo nodo al principio de la lista */
      if ( ptrAnterior == NULL ) 
	  {	  
         ptrNuevo->sgte = *colaFte;
         *colaFte = ptrNuevo;
      }
      else 
	  { /* inserta un nuevo nodo entre ptrAnterior y ptrActual */
         ptrAnterior->sgte = ptrNuevo;
         ptrNuevo->sgte = ptrActual;
      }
	  if( ptrActual == NULL )
	  {
		*colaFin = ptrNuevo;
      }
   }
   else 
   {
      printf( "No se inserto. No hay memoria disponible.\n");
	  return -1;
   }
	return 1;
}
//-------------------------------------------------------------------------------
int atenderTecladoCargador( int teclado, int sokCent,ptrNodoCola *CC_Carga, ptrNodoCola *CF_Carga, struct timeval *tiempo, fd_set *master, ptrNodoCola *CC_Espera, ptrNodoCola *CF_Espera  )
{
	char *buffer;
	int numbytes;
	int pos;
	int i = 1;
	nodo valor;

	if( ( buffer = calloc(1,MAXDATASIZE) ) == NULL )
	{
		logger("Error","Cargador", "Memoria Insuficiente", 0);
		return -1;
	}
	if( (numbytes = read(teclado,buffer,MAXDATASIZE) ) <= 0 )
	{
		logger("Error","Cargador", "Fallo la recepcion de datos de AtenderTeclado", 0);
		return -1;
	}
	buffer[numbytes-1]='\0';
	//printf("En el buffer hay: %s\n\n", buffer);
	if( (pos = buscarEnVector( buffer ) ) == -1 )
	{
		logger("Error","Cargador", "Comando Invalido", 1);
		pos = -1;
	}
	switch(pos) 
	{	
		case -1:{ 	//Comando no valido...	
					printf("\tComandos del Cargador:\n\n");
					MensajeAyuda();
				}break;
		case 0: { 	//RECIBI "FIFO" POR TECLADO
					algoritmo = FIFO;
					logger("Informacion","Cargador", "Cambio de Algoritmo. En adelante dera FIFO", 1);
					if( *CC_Carga != NULL )
					{
						if( cambiarAFIFO( CC_Carga, CF_Carga ) == -1 )
						{
							logger("Error","Cargador", "Fallo cambiar a FIFO", 0);
							return -1;
						}
					}
				}break;
		case 1: {	//RECIBI "RR" POR TECLADO
					algoritmo = RR;
					logger("Informacion","Cargador", "Cambio de Algoritmo. En adelante dera ROUND ROBIN", 1);
					if( *CC_Carga != NULL )
					{
						if( cambiarARR( CC_Carga, CF_Carga) == -1)
						{
							logger("Error","Cargador", "Fallo cambiar a ROUND ROBIN", 0);
							return -1;
						}
					}
				}break;
		case 2: {	//RECIBI "SRT" POR TECLADO 
					algoritmo = SRT;
					logger("Informacion","Cargador", "Cambio de Algoritmo. En adelante dera SRT", 1);
					if( *CC_Carga != NULL )
					{
						if( cambiarASRT( CC_Carga, CF_Carga) == -1)
						{
							logger("Error","Cargador", "Fallo cambiar a SRT", 0);
							return -1;
						}
					}
				}break;
		case 3: {	//RECIBI "ENCENDER" POR TECLADO
					estado = ENCENDIDO;
					numCelCarg = 0;
					logger("Informacion","Cargador", "El cargador se ha encendido", 1);
					//VER QUE CARAJO HACER CON LOS CELULARES QUE SE ESTABAN CARGANDO O QUE ESTABAN ESPERANDO
				}break;	
		case 4: { 	//RECIBI "APAGAR" POR TECLADO
					logger("Informacion","Cargador", "El cargador se ha Apagado", 1);
					estado = APAGADO;
					logger("Informacion","Cargador", "Apagando el celular y Vaciando Colas De Carga y Espera", 0);
					if( (buffer = realloc( buffer, 5 ) ) == NULL )
					{
						logger("Error","Cargador", "Memoria Insuficiente", 0);
						return -1;
					}
					while( *CC_Carga != NULL && i <= numCelCarg )
					{
						if( retirar( CC_Carga, CF_Carga, &valor) == -1 )
						{
							logger("Error","Cargador", "No se pudo retirar de la Cola de Carga", 0);
							return -1;
						}
						sprintf(buffer, "%d", valor.restoCarga);
						if( enviarCame( GCCX, buffer, &valor.socket,0,1,0,"\0") == -1 )
						{
							logger("Error","Cargador", "Fallo en atenderTeclado el enviarCame de GCCX", 0);
							return -1;
						}
						FD_CLR(valor.socket,master); //Lo saco del master ya que no  lo voy a antender mas.
						close(valor.socket);
						i++;
					}
					i = 1;
					while( *CC_Espera != NULL )
					{
						if( retirar( CC_Espera, CF_Espera, &valor) == -1 )
						{
							logger("Error","Cargador", "No se pudo retirar de la Cola de Espera", 0);
							return -1;
						}
						sprintf(buffer, "%d", valor.restoCarga);
						if( enviarCame( GCCX, buffer, &valor.socket,0,1,0,"\0") == -1 )
						{
							logger("Error","Cargador", "Fallo en atenderTeclado el enviarCame de GCCX", 0);
							return -1;
						}
						FD_CLR(valor.socket,master); //Lo saco del master ya que no  lo voy a antender mas.
						close(valor.socket);
						i++;
					}
					*CC_Espera = NULL;
					*CC_Carga = NULL;
					
				}break;
		case 5: {
					printf("\n\t*************************\n");	
					printf(  "\t*     Cola de Carga     *\n" );
					printf(  "\t*************************\n");	
					imprimeCola( *CC_Carga );
					printf("\n\t*************************\n");	
					printf(  "\t*     Cola de Espera    *\n" );
					printf(  "\t*************************\n");
					imprimeCola( *CC_Espera );
				}break;
	}	
	if( asignarTiempo( CC_Carga, tiempo ) == -1 )
	{
		logger("Error","Cargador", "No se pudo a recalular el tiempo de carga", 0);
		return -1;
	}
	return 1;
}

//------------------------------
void MensajeAyuda( )
{
	printf("********************************************************************************\n"); 
	printf("*   1.FIFO: cambia la planificacion a modo FIFO de la cola de carga            *\n");
	printf("*   2.RR: cambia la planificacion a modo RoundRobin de la cola de carga        *\n");
	printf("*   3.SRT: cambia la planificacion a modo SRT de la cola de carga              *\n");
	printf("*   4.ENCENDER: enciende el cargador                                           *\n");
	printf("*   5.APAGAR: apaga el cargador                                                *\n");
	printf("*                                                                              *\n");
	printf("********************************************************************************\n"); 
	return;
}
//-------------------------------
int CambiarCelularEnCarga( 	ptrNodoCola *CC_Carga,ptrNodoCola *CF_Carga, ptrNodoCola *CC_Espera,
							ptrNodoCola *CF_Espera, struct timeval *tiempo, int capacidad, fd_set *master)
{
	//VARIABLES DE LA FUNCION
	nodo aux;

	logger("Error","Cargador", "Procesando Cambio de Celular en Carga", 0);
	switch( algoritmo )
	{
		case FIFO:	if( retirar( CC_Carga, CF_Carga, &aux) == -1 )
					{
						logger("Error","Cargador", "No se pudo retirar de la Cola de Carga", 0);
						return -1;
					}
					numCelCarg--;
					printf("\n\tCelular Nro %s cargado exitosamente\n", aux.NRO);
					printf("\tAvisandole al celular que su carga ha sido completada\n");
					//printf("RESTO CARGA: %d\n", aux.restoCarga);
					if( enviarCame(GCCA,"\0",&(aux.socket),0,1,0,"\0") == -1 )
					{
						logger("Error","Cargador", "Fallo el enviarCame de GCCA", 0);;
						return -1;
					}
					FD_CLR(aux.socket,master);
					close(aux.socket);
					if( (numCelCarg < capacidad) && ((*CC_Espera) != NULL) )  //VER QUE DEVUELVE ESTA VACIA...	
					{
						if( retirar( CC_Espera, CF_Espera, &aux ) == -1)
						{
							logger("Error","Cargador", "No se pudo retirar de la Cola de Espera", 0);
							return -1;
						}
						if( agregar( CC_Carga, CF_Carga, &aux ) == -1 )
						{
							logger("Error","Cargador", "No se pudo agregar a la Cola de Carga", 0);
							return -1;
						}
						numCelCarg++;
					}
					break;
		
		case SRT:  	if( retirar( CC_Carga, CF_Carga, &aux) == -1 )
					{
						logger("Error","Cargador", "No se pudo retirar de la Cola de Carga", 0);
						return -1;
					}
					numCelCarg--;
					printf("\n\tCelular Nro %s cargado exitosamente\n", aux.NRO);
					printf("\tAvisandole al celular que su carga ha sido completada\n");
					if( enviarCame(GCCA,"\0",&(aux.socket),0,1,0,"\0") == -1 )
					{
						logger("Error","Cargador", "Fallo el enviarCame de GCCA", 0);
						return -1;
					}
					FD_CLR(aux.socket,master);
					close(aux.socket);
					if( (numCelCarg < capacidad) && ((*CC_Espera) != NULL) )  //VER QUE DEVUELVE ESTA VACIA...	
					{
						if( retirar( CC_Espera, CF_Espera, &aux ) == -1 )
						{
							logger("Error","Cargador", "No se pudo retirar de la Cola de Espera", 0);
							return -1;
						}
						if( agregarComoSRT( CC_Carga, CF_Carga, &aux ) == -1 )
						{
							logger("Error","Cargador", "No se pudo agregar a la Cola de Carga", 0);
							return -1;
						}
					}	 
					break;
		
		case RR:	if( (*CC_Carga)->info.restoCarga <= 0 )
					{
						if( retirarComoRR( CC_Carga, CF_Carga, &aux ) == -1 )
						{
							logger("Error","Cargador", "No se pudo retirar de la Cola de Carga", 0);
							return -1;
						}
						printf("\n\tCelular Nro %s cargado exitosamente\n", aux.NRO);
						printf("\tAvisandole al celular que su carga ha sido completada\n");
						if( enviarCame(GCCA,"\0",&(aux.socket),0,1,0,"\0") == -1 )
						{
							logger("Error","Cargador", "Fallo el enviarCame de GCCA", 0);
							return -1;
						}
						FD_CLR(aux.socket,master);
						close(aux.socket);
						numCelCarg--;	
						if( (numCelCarg < capacidad) && ((*CC_Espera) != NULL) )  //VER QUE DEVUELVE ESTA VACIA...	
						{
							if( retirar( CC_Espera, CF_Espera, &aux ) == -1 )
							{
								logger("Error","Cargador", "No se pudo retirar de la Cola de Espera", 0);
								return -1;
							}
							if( agregarComoRR( CC_Carga, CF_Carga, &aux ) == -1 )
							{
								logger("Error","Cargador", "No se pudo agregar a la Cola de Carga", 0);
								return -1;
							}
						}
					}
					else
					{
						rotar( CC_Carga, CF_Carga );	
					}
					break; 
	}
	if( asignarTiempo( CC_Carga, tiempo ) == -1 )
	{
		logger("Error","Cargador", "No se pudo recalcular el tiempo de carga", 0);
		return -1;
	}
	return 1;
}
//------------------------------
int cambiarAFIFO(ptrNodoCola *CC_Carga, ptrNodoCola *CF_Carga)
{
	(*CF_Carga)->sgte = NULL;
	return 1;
}
//------------------------------
int cambiarARR(ptrNodoCola *CC_Carga, ptrNodoCola *CF_Carga)
{
	(*CF_Carga)->sgte = *CC_Carga;
	return 1;
}
//------------------------------
int cambiarASRT(ptrNodoCola *CC_Carga, ptrNodoCola *CF_Carga)
{
	ptrNodoCola CC_Aux = NULL; 
  ptrNodoCola CF_Aux = NULL;
	nodo nodoAux;

	cambiarAFIFO( CC_Carga, CF_Carga);
	/*if( ( CC_Aux = malloc( sizeof( NodoCola ) ) ) == NULL )
	{
		logger("Error","Cargador", "Memoria Insuficiente", 0);
		return -1;
	}*/
	for( ; (*CC_Carga) != NULL ; )  //No lo podria reeplazar por...  !estaVacia( *CC_Carga)
	{
		retirar(CC_Carga, CF_Carga, &nodoAux);
		agregarComoSRT(&CC_Aux, &CF_Aux, &nodoAux);
	}
	(*CC_Carga)  = CC_Aux;
	(*CF_Carga) = CF_Aux;
	
	return 1;
}
//------------------------------
int retirar( ptrNodoCola *colaFte, ptrNodoCola *colaFin, nodo* valor )
{ 
   ptrNodoCola tempPtr;

   *valor = ( *colaFte )->info;
   tempPtr = *colaFte;   *colaFte = ( *colaFte )->sgte;

   /* si la cola está vacía */
   if ( *colaFte == NULL ) {
      *colaFin = NULL;
   }
   free( tempPtr );

   return 1;

}
//------------------------------
int retirarComoRR( ptrNodoCola *colaFte, ptrNodoCola *colaFin, nodo* valor )
{ 
    ptrNodoCola tempPtr;

	*valor = ( *colaFte )->info;
	tempPtr = *colaFte;
	if( *colaFte == *colaFin )
	{   
		*colaFte = NULL;
		*colaFin = NULL;		
	}
   	else
	{    		*colaFte = ( *colaFte )->sgte;
		(*colaFin)->sgte = *colaFte;
	}   	free( tempPtr );

   	return 1;

}
//------------------------------------------------------------------
int rotar(ptrNodoCola *CC_Carga, ptrNodoCola *CF_Carga)
{
	*CC_Carga = (*CC_Carga)->sgte;
	*CF_Carga = (*CF_Carga)->sgte;

	return 1;
}
//---------------------------------------------------------------
int atenderCentral( int sokCent, int *fdmax,fd_set *master )
{
	char *buffer;
	int numbytes;
	cabecera *cabe;
	
	logger("Informacion","Cargador", "Atendiendo a la Central", 0);
	if( ( buffer = calloc( 1, sizeof(cabecera) ) ) == NULL )
	{
		logger("Error","Cargador", "Memoria Insufiente", 0);
		return -1;
	}
	if( (numbytes = recv( sokCent, buffer, sizeof(cabecera), 0 ) ) == 0 )
	{
		//SE CERRO EL SOCKET QUE NOS MANTENIA CONECTADOS CON LA CENTRAL
		//atenderCaida(); // VERRRRRRRRRRRRRRRRRR
		logger("Informacion","Cargador", "Se ha perdido la conexion con la Central", 1);
		centConexion = DESCONECTADO;
		FD_CLR(sokCent,master); //Lo saco del master ya que no  lo voy a antender mas.
		close(sokCent);
		return 1;
	}
	cabe = (cabecera*)buffer;
	if( cabe->tipo == GCOZ )
	{
		logger("Informacion","Cargador", "El cargador se ha conectado correctamente con la Central", 1);
		printf("\n\tCargador Inciado. Listo para cargar telefonos celulares.\n");
		centConexion = CONECTADO;
		return 1;
	}
	free( buffer );	
	return 1;
}

//---------------------------------
int asignarTiempo( ptrNodoCola *CC_Carga, struct timeval *tiempo )
{
	int resto, rafagasNec;
	
	//printf( "Ejecutando asignar tiempo\n" );
	if( estado == APAGADO || *CC_Carga == NULL )
	{
		tiempo->tv_sec = STANDBY_TIME;
		tiempo->tv_usec = 0;
		return 1;
	}
	if( algoritmo != RR)
		{
			resto = (*CC_Carga)->info.restoCarga % tasa;
			rafagasNec = (*CC_Carga)->info.restoCarga / tasa;
			if( resto == 0 )  
				tiempo->tv_sec = rafagasNec * timeRafaga;
			else
				tiempo->tv_sec = ( rafagasNec + 1 ) * timeRafaga;
			tiempo->tv_usec = 0;
		}
		else
		{
			tiempo->tv_sec = timeRafaga;
			tiempo->tv_usec = 0;
		}
	printf("\n");
	printf("\t-----> Cargando el celular: %s\n", (*CC_Carga)->info.NRO );
	printf("\t-----> El tiempo que se asignara es: %d seg.\n", tiempo->tv_sec);
	printf("\t-----> Resto de Carga: %d\n", (*CC_Carga)->info.restoCarga );
	return 1;
}
//-------------------
int buscarSacar( ptrNodoCola *colaFte, ptrNodoCola *colaFin, int socket )
{
   ptrNodoCola ptrAnterior; /* apuntador a un nodo previo de la lista */
   ptrNodoCola ptrActual;   /* apuntador al nodo actual de la lista */
	 int cont = 0;
   
   ptrAnterior = NULL;
   ptrActual = *colaFte;

      /* ciclo para localizar la ubicación correcta en la lista */
   while ( ptrActual != NULL && socket != ptrActual->info.socket && cont < numCelCarg ) 
	 {	  
        ptrAnterior = ptrActual;          /* entra al ...   */
        ptrActual = ptrActual->sgte;  /* ... siguiente nodo */
				cont++;
   }
   /* Saco el que esta  al principio de la lista */
	 if( algoritmo == RR && cont == numCelCarg )
	 {
		logger("Informacion","Cargador", "No se encontro el celular en la cola", 0);
		return 0;
	 }
	 if( ptrActual == NULL )
	 {
	      logger("Informacion","Cargador", "No se encontro el celular en la cola", 0);
		   return 0;
   }
   if ( ptrAnterior == NULL ) 
	 {	  
   		
			if( algoritmo == RR )
				*colaFte = NULL;
	    else
				*colaFte = ptrActual->sgte;
		  free( ptrActual );
		  return 1;
   }
   else 
	 { /* inserta un nuevo nodo entre ptrAnterior y ptrActual */
      ptrAnterior->sgte = ptrActual->sgte;
      free( ptrActual );
		  return 1;
   }
}

void imprimeCola( ptrNodoCola ptrActual )
{ 
	int cont;
   if ( ptrActual == NULL ) 
   {
      printf( "\tLa cola esta vacia.\n\n" );
   } /* fin de if */
   else 
   { 
		cont = 0;
		//printf( "La cola es:\n" );
		while ( ptrActual != NULL && cont < numCelCarg) 
		{ 
			printf( "--------------------------------------\n");
			printf( "    Nro. Celular: %s\n", ptrActual->info.NRO);
			//printf( "El socket es: %d\n", ptrActual->info.socket);
			printf( "    Unidades a cargar: %d\n", ptrActual->info.restoCarga);
			printf( "--------------------------------------\n");
			ptrActual = ptrActual->sgte;
			cont++;
		} 
		printf( "NULL\n\n" );
   } 

} 

int buscarEnVector(char*buffer)
{
	int i=0;

	while((i<MAXCODIGOS)&&(strcmp(buffer,vcodigos[i])!=0)) i++;
	if(i== MAXCODIGOS) 
	{
		//printf("Comando Desconocido\n");
		return -1;
	}
	return i;
}
