#include "soporteGeneral.h"
#include "soporteCentralLinea.h"
//FUNCIONES AUXILIARES/////////////////////////////////////////////////////////////////////////

extern int contadorID;
extern int contadorVids;
extern char centralID[6];
extern char ID[15];
extern int estoyConectado;    //Determina si ya estoy conectado a la red de centrales(1) o no(0)! 
extern nodoVecQuery vecQuery[TOPEVEC];
extern int esperoQueryHit;

//-----------------------------------------------------
int atenderConexion(	int listener,
						int *fdmax,
						fd_set *master,
						nodo*vcelulares,
						int tope, 
						ptrNodoCola *colaFte,
						ptrNodoCola *colaFin) 
{
	struct sockaddr_in cliente; //estructura con la info de un Celular que me llama	
	int addrlen;
	int new_fd;
	char*buffer2;
	cabecera *cabe;
	int lenmsj;

	addrlen = sizeof(cliente);
	if( (new_fd = accept(listener,(struct sockaddr *)&cliente,&addrlen) ) ==-1)
	{
		logger("Error","CentralLinea", "Fallo aceptacion de la nueva conexion", 0);
		return -1;
	}
	logger("Informacion","CentralLinea", "Atendiendo Nueva Conexion", 1);
	//printf("Nueva conexion desde: %s\n",inet_ntoa(cliente.sin_addr));
	FD_SET(new_fd,master);
	if(new_fd > *fdmax) 
		*fdmax=new_fd;
	return 1;					
}
//--------------------------------------------------------------------

int agregarVector( nodo *newCel, nodo *pVector, int tope)
{
	
	int bol = -1; /* BOOLEANA, si esta en -1  no lo encontro, si tiene otro valor lo encontro  en la posicion que indica dicho valor */
	int i = 0;
	
	while( (bol == -1) && ( i < tope) ) /* Revisar si va  <= o solamente < */
	{
		if( pVector[i].socket == 0 )
		{
			strcpy(pVector[i].numero, newCel->numero);
			strcpy(pVector[i].ip, newCel->ip);
			strcpy(pVector[i].port, newCel->port);
			pVector[i].socket = newCel->socket;
			bol = i;
		}
		i++;
	}
	return bol;
}

//------------------------------------------------------------------------

int buscarVector(char* nroBuscado, nodo *pVector, int tope)
{
	int bol = -1;
	int i = 0;
	
	while( bol == -1 && i < tope)
	{
		if( strcmp( nroBuscado, pVector[ i ].numero ) == 0 )
			bol = i;
	    i++;
	}
	return bol;
}


//------------------------------------------------------------------------
int buscarPorSocket(int socket, nodo *pVector, int tope)
{
	int bol = -1;
	int i = 0;
	
	while( bol == -1 && i < tope)
	{
		if( socket== pVector[ i ].socket ) 
			bol = i;
	    i++;
	}
	return bol;
}
//------------------------------------------------------------------------
int tengoCapacidad(nodo *pVector, int tope)
{
	int i = 0;

	mostrarVector(pVector,tope);
	while( (pVector[i].socket !=0) && (i<tope) )
		i++;
	if(i == tope) 
		return -1;
	return 1;	
}
//-------------------------------------------------------------------------
int quitarVector(int socket, nodo *pVector, int tope)
{
	int bol = -1;
	int i = 0;
	
	while( bol == -1 && i < tope ) /* Revisar si va  <= o solamente < */
	{
		if(  socket == pVector[ i ].socket )
		{
			pVector[ i ].numero[0] = '\0';
			pVector[ i ].ip[0] = '\0';
			pVector[ i ].port[0] = '\0';
			pVector[ i ].socket = 0;
			bol = i;
		}
		i++;
	}
	return bol;
}
//-------------------------------------------------------------------------

int atenderCliente	( 	char *buffer,
						int sok, 
						nodo *pVector, 
						int tope, 
						ptrNodoCola *colaFte,
						ptrNodoCola *colaFin,
						int*vcentrales,
						int centmax,
						int maxID,
						nodoID*vids,
						char*port,
						char*ip,
						fd_set * master,
						int*fdmax
					)
{
		
	cabecera *cabe;
	mensajePONG *menPONG;
	mensajeQueryHit *menQueryHit; 
	int lenmsj, numbytes;
	char *buffmsj;
	int pos;
	int sokBuscado;
	char *ipPort;
	nodo aux;
	char*nombreCentral;
	int new_fd;
	char puerto[6];
	char*IDespecial;
	char*aux2;
	char letraAuxiliar;
	int tipo;
	
	//Desarrollo de la funcion atenderCliente
	logger("Informacion","Central", "Atendiendo Cliente", 0);
	cabe = (cabecera*)buffer;	//Casteo lo recibido en el buffer y lo guardo en la cabe	
	lenmsj = cabe->largo;		
	IDMensaje();//Creo un ID con el que voy a responder
	switch( cabe->tipo )
	{
		case CONA: 	contadorID--; //Para emparejar el ID, no voy a mandar un CAme
								if( atenderCONA(sok,pVector,tope,lenmsj, colaFte, colaFin) == -1)
								{
									logger("Error","CentralLinea", "Fallo AtenderCONA", 0);
									return -1;
								}
								break;
		
		case TCOA:	logger("Informacion","Central", "Recibi TCOA. Pedido de una central por conexion", 0);		
					if((pos = buscarLibreVsockets(vcentrales,centmax))==-1)
					{
						logger("Informacion","Central", "No hay espacion para la nueva central", 0);
						if( enviarCame(TCOX,"\0",&sok,0,1,0,ID) == -1 )
						{
							logger("Error","Central", "Fallo enviarCame de TCOX", 0);
							return -1;
						}
					}
					else 
					{	
						if( agregarAVsockets(vcentrales,sok,pos) == -1)
						{
							logger("Error","Central", "No se pudo agregar a Vcentrales", 0);
						}
						imprimirVsockets(vcentrales,centmax);
						if( enviarCame(TCOZ,"\0",&sok,0,1,0,ID) == -1)
						{
							logger("Error","Central", "Fallo enviarCame de TCOZ", 0);
							return -1;
						}
						logger("Informacion","Central", "Nueva central concida y agregada a Vcentrales", 0);
					}
					break;
		
		case TCOZ:	estoyConectado = 1;
					pos = buscarLibreVsockets(vcentrales,centmax);
					if( agregarAVsockets(vcentrales,sok,pos) == -1)
					{
						logger("Error","Central", "No se pudo agregar a Vcentrales", 0);
						return -1;
					}
					imprimirVsockets(vcentrales,centmax);
					logger("Informacion","Central", "Conexion Aceptada. Central en Red", 1);
					break; 
				
		case TCOX:	printf("Recibi TCOX\n");
					break;
		case PING:	logger("Informacion","Central", "Se recibio un PING", 1);
					contadorID--; //Emparejo pq mando el ID original, no creo
					if( agregarAVids(maxID, cabe->ID,sok,vids) == -1)
					{
						logger("Error","Central", "No se pudo agregar ID al historial de IDs", 0);
						return 1;
					}
					//imprimirVids(maxID,vids);
					printf("\tTTL= %d\n",cabe->TTL);
					//NOTA: si me lo mando alguien que esta en vcentrales, lo saco, envio y lo agrego,
					//sino le envio PING a todo vcentrales
					if( (quitarDeVsockets(vcentrales,sok,centmax) ) ==-1 )
					{
						if( enviarCame(PING,"\0",vcentrales,centmax,cabe->TTL,cabe->HOPS,cabe->ID) == -1)
						{
							logger("Error","Central", "Fallo enviarCame de PING", 0);
							return -1;
						}
						logger("Informacion","Central", "PING propagado", 1);
					}
					else
					{
						if( enviarCame(PING,"\0",vcentrales,centmax,cabe->TTL,cabe->HOPS,cabe->ID) == -1)
						{
							logger("Error","Central", "Fallo enviarCame de PING", 0);
							return -1;
						}
						pos = buscarLibreVsockets(vcentrales,centmax);
						if( agregarAVsockets(vcentrales,sok,pos) == -1 )
						{
							logger("Error","Central", "No se pudo agregar a Vcentrales", 0);
							return -1;
						}
						logger("Informacion","Central", "PING propagado", 1);
					}
					if( ( menPONG = calloc(1,sizeof(mensajePONG)) ) == NULL)
					{
						logger("Error","Central", "Memoria Insuficiente", 0);
						return -1;
					}
					menPONG->port=(unsigned int)atoi(port);
					menPONG->ip.s_addr=inet_addr(ip);
					menPONG->ocupados = cantidadCentralesOcupadas(vcentrales,centmax);
					menPONG->libres = centmax-menPONG->ocupados;
					if( enviarCame(PONG,(char*)menPONG,&sok,0,TTLdef,0,cabe->ID) == -1)
					{
						logger("Error","Central", "Fallo el enviarCame de PONG", 0);
						return -1;
					}
					logger("Informacion","Central", "PONG enviado", 1);
					free (menPONG);
					break;
				
		case PONG:	//if( ( menPONG=calloc(1,sizeof(mensajePONG)) ) == NULL )
					//{
					//	logger("Error","Central", "Memoria Insuficiente", 0);
					//	return -1;
					//}
					if( ( buffmsj = calloc(1, lenmsj) ) == NULL )
					{
						logger("Error","Central", "Memoria Insuficiente", 0);
						return -1;
					}
					if( ( numbytes = recv(sok,buffmsj,lenmsj,0) ) < 0 )
					{
						logger("Error","Central", "Fallo recepcion de datos de AtenderEntrada", 0);
						close(sok);
						return -1;
					}
					if( ( nombreCentral=calloc(1,strlen(cabe->ID)) ) == NULL )
					{
						logger("Error","Central", "Memoria Insuficiente", 0);
						return -1;
					}
					strcpy(nombreCentral,cabe->ID);
					nombreCentral[strlen(cabe->ID)-4]='\0';
					if(strcmp(nombreCentral,centralID)==0)
					{
						logger("Informacion","Central", "Recibi PONG y es para mi. No lo propago", 1);
						menPONG = (mensajePONG*)buffmsj;
						/*printf ( "Ip %s Puerto %d\nOcupados %d Libres %d \n\n",
									inet_ntoa(menPONG->ip),
									menPONG->port,							 
									menPONG->ocupados,
									menPONG->libres
								); */
						if((menPONG->libres)>0) 
						{
							sprintf(puerto,"%d",menPONG->port);
							estoyConectado=1;
							if( ( new_fd = crearSocket(puerto,inet_ntoa(menPONG->ip)) ) == -1)
							{
								logger("Error","Central", "No se pudo crear socket", 0);
								return -1;
							}
							if( enviarCame(TCOA,"\0",&new_fd,0,1,0,ID) == -1 )
							{
								logger("Error","Central", "Fallo enviarCame de TCOA", 0);
								return -1;
							}
							FD_SET(new_fd,master);
							if(new_fd>*fdmax) 
								*fdmax=new_fd;
						}
					// saco la ip y puerto del mensaje y me conecto a esa central
					}
					else 	
					{
						logger("Informacion","Central", "Recibi PONG y no es para mi. Lo propago", 1);
						sokBuscado=buscarEnVids(maxID,cabe->ID,vids);
						if( enviarCame(PONG,buffmsj,&sokBuscado,0,TTLdef,cabe->HOPS,cabe->ID) == -1)
						{
							logger("Informacion","Central", "Fallo enviarCame de PONG", 1);
							return -1;
						}
					}
					free(buffmsj);
					//free (menPONG);
					free(nombreCentral);
					break;
		
		case CALA:		if( ( buffmsj = calloc(1, lenmsj) ) == NULL )
								{
									logger("Error","CentralLinea", "Memoria Insuficiente", 0);
									return -1;
								}
								if( ( numbytes = recv(sok,buffmsj,lenmsj,0) ) < 0 )
								{
									logger("Error","Central", "Fallo recepcion de datos en AtenderEntarada", 0);
									close(sok);
									return -1;
								}	
								printf("\tQuieren Llamar a: %s\n",buffmsj);
								printf("\tBuscando en nuestros clientes\n");
								pos = buscarVector( buffmsj, pVector, tope); // Buscar el ipport 
								if( pos == -1)
								{
									contadorID--;
									if( ( IDespecial = calloc(1,15) ) == NULL )
									{
										logger("Error","CentralLinea", "Memoria Insuficiente", 0);
										return -1;
									}
									if( ( aux2 = calloc(1,6) ) == NULL )
									{
										logger("Error","CentralLinea", "Memoria Insuficiente", 0);
										return -1;
									}
									pos = buscarPorSocket(sok, pVector,tope);
									sprintf(aux2,"%d",10000+contadorID);
									strcpy(IDespecial,pVector[pos].numero);
									strcat(IDespecial,&aux2[1]);
									if( enviarCame(Query,buffmsj,vcentrales,centmax,TTLdef,0,IDespecial) == -1 )
									{
										logger("Error","CentralLinea", "Fallo enviarCame de Query", 0);
										return -1;
									}
									agregarVecQuery( sok,0 );
									contadorID++;
									logger("Informacion","CentralLinea", "No es un cliente nuestro", 1);
									logger("Informacion","Central", "Enviando Query a las centrales vecinas.", 1);
									free(aux2);
									free(IDespecial);
									return 0;
								}
								if( (ipPort = calloc ( 1, strlen(pVector[pos].ip) + strlen( pVector[pos].port ) + 1 + 1 + 1 ) ) == NULL )
								{
									logger("Error","CentralLinea", "Memoria Insuficiente", 0);
									return -1;
								}
								strcpy( ipPort, pVector[pos].ip);
								ipPort[ strlen( pVector[pos].ip ) ] = '*';
								ipPort[ strlen( pVector[pos].ip )+ 1] = '\0';		
								strcat( ipPort, pVector[pos].port);
								ipPort[strlen(pVector[pos].ip) + 1 + strlen(pVector[pos].port)] = '*';
								ipPort[strlen(pVector[pos].ip)+1+1+ strlen(pVector[pos].port)+1] = '\0';
								if( enviarCame(CALZ, ipPort, &sok, 0,1,0,ID) == -1 )
								{
									logger("Error","CentralLinea", "Fallo enviarCame de CALZ", 0);
									return -1;
								}
								logger("Informacion","CentralLinea", "Cliente encontrado", 1);
								free( buffmsj );
								free( ipPort);
								break;
		case DSPA:	{
								printf("Recibi DSPA, pero soy una central de linea\n");
								if( ( buffmsj = calloc(1, lenmsj) ) == NULL )
								{
									logger("Error","Central", "Memoria Insuficiente", 0);
									return -1;
								}
								if( ( numbytes = recv(sok,buffmsj,lenmsj,0) ) < 0 )
								{
									logger("Error","Central", "Fallo la recepcion de datos en AtenderEntrada", 0);
									close(sok);
									return -1;
								}
								if( enviarCame(DSPX,buffmsj,&sok,0,1,0,cabe->ID) == -1)
								{
									logger("Error","Central", "Fallo el enviarCame de DSPX", 0);
									return -1;
								}
							}break;
				
		case APGA:		logger("Informacion","CentralLinea", "Un telefono se ha apagado.", 1);
								quitarVector( sok, pVector, tope);
								logger("Informacion","CentralLinea", "Telefono quitado de nuestra red de clientes", 1);
								//Si alguien en la cola, agrego	
								if( *colaFte != NULL )
								{		
									logger("Informacion","CentralLinea", "Se agregara un telefono que esperaba lugar en la red", 1);
									imprimeCola( *colaFte );		
					 				if( retirar( colaFte, colaFin, &aux) == -1 )
					   			{
					 					logger("Error","CentralLinea", "No se pudo retirar de la Cola de espera", 0);
					 					return -1;
									} 
									if(agregarVector(&aux,pVector,tope) == -1)
									{
										logger("Error","CentralLinea", "No se pudo agregar a la red un nuevo cliente", 0);
										return -1;
									}
									logger("Informacion","CentralLinea", "Nuevo Telfono conectado a nuestra red de clientes", 1);
									mostrarVector( pVector, tope);
									if( send(aux.socket,"CONZ",5,0) < 0)
									{
										logger("Error","Central", "Fallo el send de CONZ", 0);
										return -1;
									}
									if(send(aux.socket,centralID,6,0)<0)
									{
										printf("Error en la funcion send\n");
										return -1;
									}
								}		
								break;
					
		case Query:	{
					logger("Informacion","CentralLinea", "Recibi un Query", 1);	
					if( ( buffmsj = calloc(1, lenmsj) ) == NULL )
					{
						logger("Error","CentralLinea", "Memoria Insuficiente", 0);
						return -1;
					}
					if( ( numbytes = recv(sok,buffmsj,lenmsj,0) ) < 0 )
					{
						logger("Error","CentralLinea", "Fallo la recepcion de datos en atenderEntrada", 0);
						close(sok);
						return -1;
					}
					if( (agregarAVids(maxID, cabe->ID,sok,vids))==-1)
					{
						logger("Informacion","CentralLinea", "Query Repetido. No debo hacer nada", 1);
						return 0;
					} 	
					//imprimirVids(maxID,vids);
					if((pos=buscarVector(buffmsj,pVector,tope))==-1)
					{
						quitarDeVsockets(vcentrales,sok,centmax);
						if( enviarCame(Query,buffmsj,vcentrales,centmax,cabe->TTL,cabe->HOPS,cabe->ID) == -1)
						{
							logger("Error","CentralLinea", "Fallo el enviarCame de Query", 0);
							return -1;
						}
						pos = buscarLibreVsockets(vcentrales,centmax);
						agregarAVsockets(vcentrales,sok,pos);
						logger("Informacion","CentralLinea", "Numero No encontrado en mi red. Query Propagado", 1);
					}
					else 
					{
						if( ( menQueryHit = calloc(1,sizeof(mensajeQueryHit)) ) == NULL )
						{
							logger("Error","CentralLinea", "Memoria Insuficiente", 0);
							return -1;
						}
						menQueryHit->port=(unsigned int)atoi(pVector[pos].port);
						menQueryHit->ip.s_addr=inet_addr(pVector[pos].ip);
						if( enviarCame(QueryHit,(char*) menQueryHit,&sok,0,TTLdef,0,cabe->ID) == -1)
						{
							logger("Error","CentralLinea", "Fallo el enviarCame de QueryHit", 0);
							return -1;
						}	
						logger("Informacion","CentralLinea", "Numero Encontrado. Enviando QueryHit", 1);
						free(menQueryHit);
					}
					free(buffmsj);
					}break; 
					
		case QueryHit:	{
					if( ( buffmsj = calloc(1, lenmsj) ) == NULL )
					{
						logger("Error","CentralLinea", "Memoria Insuficiente", 0);
						return -1;
					}
					if( ( numbytes = recv(sok,buffmsj,lenmsj,0) ) < 0 )
					{
						logger("Error","CentralLinea", "Fallo la recepcion de datos en AtenderEntrada", 0);
						close(sok);
						return -1;
					}
					if( ( menQueryHit = calloc(1,sizeof(mensajeQueryHit)) ) == NULL )
					{
						logger("Error","CentralLinea", "Memoria Insuficiente", 0);
						return -1;
					}
					menQueryHit=(mensajeQueryHit*)buffmsj;
					letraAuxiliar=cabe->ID[10];
					cabe->ID[10]='\0';
					if( ( pos = buscarVector(cabe->ID,pVector,tope) ) ==-1 )
					{
						logger("Informacion","CentralLinea", "Recibi QueryHit. No es para mi. Lo propago", 1);
						cabe->ID[10]=letraAuxiliar;
						sokBuscado=buscarEnVids(maxID,cabe->ID,vids);
						//quitarDeVsockets(vcentrales,sok,centmax);
						//printf("sokBuscado: %d\n", sokBuscado);
						if( enviarCame(QueryHit,(char*)menQueryHit,&sokBuscado,0,cabe->TTL,cabe->HOPS,cabe->ID) == -1)
						{
							logger("Error","CentralLinea", "Fallo el enviarCame de QueryHit", 0);
							return -1;
						}
						//pos=buscarLibreVsockets(vcentrales,centmax);
						//agregarAVsockets(vcentrales,sok,pos);
					}
					else 
					{
						logger("Informacion","CentralLinea", "Recibi QueryHit. Es para mi.", 1);
						if(quitarVecQuery( pVector[pos].socket )==-1) 
						{
							logger("Informacion","CentralLinea", "QueryHit recibido fuera de tiempo. Descartado", 1);
							return 1;
						}
						//En esta central esta el celular que origino todo
						cabe->ID[10]=letraAuxiliar;
						if( ( ipPort=calloc(1,30) ) == NULL ) //OJO CON ESTE NRO MAGICO
						{
							logger("Error","CentralLinea", "Memoria Insuficiente", 0);
							return -1;
						}
						strcpy(ipPort,inet_ntoa(menQueryHit->ip));
						//printf("el ip es: %s", inet_ntoa(menQueryHit->ip));
						lenmsj = strlen(ipPort);
						ipPort[lenmsj]='*'; 
						sprintf(&ipPort[lenmsj+1] ,"%d",menQueryHit->port);
						lenmsj=strlen(ipPort);
						ipPort[lenmsj]='*';
						ipPort[lenmsj+1]='\0';
						//printf("Recibi un QueryHit y dice: %s \n",ipPort);
						sokBuscado=buscarEnVids(maxID,cabe->ID,vids);
						tipo=buscarTipoVecQuery(sokBuscado);
						if( enviarCame(CALZ,ipPort,&pVector[pos].socket,0,1,0,cabe->ID) == -1)
						{
							logger("Error","CentralLinea", "Fallo el enviarCame de CALZ", 0);
							return -1;
						}
						logger("Informacion","CentralLinea", "QueryHit para llamada. Avisando al celular", 1);
						}
					free(buffmsj);
					}break;
	}

return 1;
}
//----------------------------------------------------------------

void mostrarVector( nodo* vector, int tope)
{
	int i;
	
	//printf( "Ejecutando mostarVector\n");
	printf("\n");
	printf("***********************************************************************\n");
  printf("* TELEFONOS CONOCIDOS *                                               *\n");
	printf("***********************************************************************\n");
	printf("*    NUMERO     *     Nº de IP      *    Nº PUERTO     *    SOCKET    *\n");  
	printf("***********************************************************************\n");
	for( i=0; i< tope; i++)
	{
		if(vector[i].socket!=0)
		{
			printf("*  %s   *     %s     *       %s      *       %d      *\n",vector[i].numero
																																			 ,vector[i].ip
																																			 ,vector[i].port
																																			 , vector[i].socket);
			printf("***********************************************************************\n");
		}
	}
	return;
}
//------------------------------------------------------------------

int atenderCONA(int new_fd,nodo *vcelulares,int tope,int lenmsj, ptrNodoCola *colaFte, ptrNodoCola *colaFin)
{
	nodo auxiliar;
	char buffer[30];
				
	logger("Informacion","CentralLinea", "Atendiendo CONA", 0);
	if(recv(new_fd,buffer,lenmsj,0)<0)
	{
		logger("Error","CentralLinea", "Fallo recepcion de datos en AtenderCona", 0);
		return  -1;
	}	
	strcpy(auxiliar.numero,strtok( buffer, "*"));
	if( strlen(auxiliar.numero) != 8 )
	{
		logger("Informacion","Central", "El telefono que intenta conectarse no es telefono de linea. Rechazado", 1);
		if(send(new_fd,"CLIN",5,0)<0)
		{
			logger("Error","CentralLinea", "Fallo el send AtenderCONA", 0);
			return -1;
		}
		return 1;
	}
	strcpy(auxiliar.ip,strtok( NULL, "*" ));
	strcpy(auxiliar.port, strtok( NULL, "*" ));
	auxiliar.socket = new_fd;
	if(agregarVector(&auxiliar,vcelulares,tope) == -1)
	{
		//Aca habria que mandarlo a la cola...
		logger("Informacion","CentralLinea", "Red Saturada. Enviando al telefono solicitante a la cola de espera", 1);
		agregar( colaFte, colaFin, &auxiliar);
		imprimeCola( *colaFte );
		if(send(new_fd,"CONX",5,0)<0)
		{
			logger("Error","CentralLinea", "Fallo el send de AtenderCONA", 0);
			return -1;
		}
		//printf("El celular esta esperando senial\n");
		return 1;
	}
	logger("Informacion","Central", "Nuevo telefono conectado satisfactoriamente a nuestra red", 1);
	mostrarVector( vcelulares, tope);
	if(send(new_fd,"CONZ",5,0)<0)
	{
		printf("Error en la funcion send\n");
		return -1;
	}
	if(send(new_fd,centralID,6,0)<0)
	{
		printf("Error en la funcion send\n");
		return -1;
	}
	return 1;
}

//----------------------------------------------------------------------------

int cargarConfigCentral(char* ruta,
						char *ip,
						char *port,
						int *celumax,
						char*ipVecino,
						char*portVecino,
						int*centmax,
						int*maxID,
						char*centralID
						)
{
	char* linea;
	char* palabra;
	FILE* arch;
	int cont = 0;

	//printf("Ejecutando CargarConfigCentral\n");
	//printf("%s\n", ruta);

	if( (arch = fopen( ruta, "r" ) ) == NULL )	
	{
		logger("Error","Central", "No se pudo abrir el archivo", 0);
		return -1;
	}

	if( (linea = malloc( MAXLINEA )) == NULL)
	{
		logger("Error","Central", "Memoria Insuficiente", 0);
		return -1;
	}
 
	while( linea = fgets( linea, MAXLINEA+1+1, arch ) )
	{
		palabra = strtok(linea,"//" );
		switch(cont)
		{
			case 0:	strcpy( ip, palabra);
					break;	
			case 1:	
					strcpy( port, palabra );
					break;
			case 2:	*celumax = atoi(palabra);
					break;
			case 3:	strcpy( ipVecino, palabra );
					break;
			case 4:	strcpy( portVecino, palabra );
					break;
			case 5: *centmax = atoi(palabra);
					break;
			case 6: *maxID=atoi(palabra);
					break;
			case 7: strcpy(centralID,palabra);
					break;
		}
		palabra = strtok( NULL , "\0\n" );
		cont++;
	}
	if( ferror(arch) != 0 )
	{
		logger("Error","Central", "Fallo lectura del archivo de configuracion", 0);
		return -1;
	}
	fclose(arch);
	return 1;
}
//----------------------------------------------------------------------

int agregar( ptrNodoCola *colaFte, ptrNodoCola *colaFin, nodo* valor )
{ 
   ptrNodoCola ptrNuevo; /* apuntador a un nuevo nodo */

   ptrNuevo = malloc( sizeof( NodoCola ) );

   if ( ptrNuevo != NULL ) { /* es espacio disponible */ 
      ptrNuevo->info = *valor;
      ptrNuevo->sgte = NULL;

      /* si esta vacía inserta un nodo en la cabeza */
      if ( estaVacia( *colaFte ) ) {
         *colaFte = ptrNuevo;
      } /* fin de if */
      else {
         ( *colaFin )->sgte = ptrNuevo;
      } /* fin de else */

      *colaFin = ptrNuevo;
   } /* fin de if */
   else {
      logger("Error","Central", "Memoria Insuficiente", 0);
      return -1;
   } /* fin de else */
return 1;
} /* fin de la función agregar */

/* elimina el nodo de la cabeza de la cola */

int retirar( ptrNodoCola *colaFte, ptrNodoCola *colaFin, nodo* valor )
{ 
   ptrNodoCola tempPtr; /* apuntador a un nodo temporal */

   *valor = ( *colaFte )->info;
   tempPtr = *colaFte;
   *colaFte = ( *colaFte )->sgte;

   /* si la cola está vacía */
   if ( *colaFte == NULL ) {
      *colaFin = NULL;
   } /* fin de if */

   free( tempPtr );

   return 1;

} /* fin de la función retirar */

/* Devuelve 1 si la cola está vacía, de lo contrario devuelve 0 */
int estaVacia( ptrNodoCola colaFte )
{
   return colaFte == NULL;

} /* fin de la función estaVacia */

/* Imprime la cola */
void imprimeCola( ptrNodoCola ptrActual )
{ 

   /* si la cola esta vacía */
   if ( ptrActual == NULL ) {
      printf( "La cola esta vacia.\n\n" );
   } /* fin de if */
   else { 
      printf( "La cola es:\n" );

      /* mientras no sea el final de la cola */
      while ( ptrActual != NULL ) { 
         printf( "El numero es: %s\n", ptrActual->info.numero);
	 printf( "El ip es: %s\n", ptrActual->info.ip);
	 printf( "El puerto es: %s\n", ptrActual->info.port);
	 printf( "El socket es: %d\n", ptrActual->info.socket);
         ptrActual = ptrActual->sgte;
      } /* fin de while */

      printf( "NULL\n\n" );
   } /* fin de else */

} /* fin de la función imprimeCola */
//////////////////////////////////////////////////////
int buscarLibreVsockets(int*vsockets,int CONFERENCIA)//Compartida con celular
{
	//Esta funcion detecta la primer posicion que contenga -1 en el vsockets 
	int i=0;	
	
	//printf("Ejecutando buscarLibreVsockets\n");
	while((vsockets[i]!=0)&&(i<CONFERENCIA))
		i++;
	if(i==CONFERENCIA) 
	{
		printf("\tNo hay espacio en el vector\n\tLlegaste al tope de CONFERENCIA\n");
		return -1;
	}
	return i;
} 

//--------------------------------------------------------------------------
 
int agregarAVsockets(int*vsockets,int socket,int pos)//Compartida con celular
{
	//Esta0Funcion agrega un Socket a la posicion de pos
	//printf("Ejecutando agregarAVsockets\n");
	vsockets[pos]=socket;
}

//--------------------------------------------------------------------------

int quitarDeVsockets(int*vsockets,int socket,int tope)//Compartida con celular
{
	//Esta funcion quita del vector vsockets el socket y pone -1 en su psicion
	int i=0;

	//printf("Ejecutando quitarDeVsockets\n");
	while((vsockets[i]!=socket)&&(i<tope)) 
		i++;
	if(i==tope) 
	{
		//printf("No se encuentra el socket en el vector\n");
		return -1;
	}
	vsockets[i]=0;	
	return 1;
}

//-----------------------------------------------------------------------------

int inicializarVsockets(int*vsockets,int CONFERENCIA)//Compartida con celular
{
	//Esta Funcion inicializa vsockets con todo -1
	int i;

	//printf("Ejecutando inicializarVsockets\n");
	for(i=0;i<CONFERENCIA;i++) 
		vsockets[i]=0;
	return 1;
}

void imprimirVsockets(int*vsockets,int CONFERENCIA)//Compartida con celular
{
	//Esta funcion muestra en un momento dado el contenido de todo vsockets
	int aux;

	//printf("Ejecutando imprimirVsockets\n");
	//printf("Este es mi vsockets actual: \n");
	printf("******************************\n");
	printf("* SOK DE CENTRALES CONOCIDAS *\n");
	printf("******************************\n");
	for(aux=0;aux<CONFERENCIA;aux++) 
	printf("*  POS  *  %d  *  SOK  *  %d   * \n",aux,vsockets[aux]);
	printf("******************************\n");
	return;
}
//Funciones de vids...Vector de Ids


int agregarAVids(int maxID, char ID[15],int sok,nodoID * vids)
{
	//Devuelve -1 si ya me llego ese ID o 1 si lo pudo agregar bien
	int i;

	//printf("Ejecutando agregarAVids\n");
	for(i=0;i<maxID;i++)
	{
		//Verificamos que el ID no me halla llegado ya
		if(strcmp(vids[i].ID,ID)==0) 
		{
			printf("Este Id ya me ha llegado!\n");
			return -1;
		}
	}
	if(contadorVids==maxID) 
		contadorVids=0;
	strcpy(vids[contadorVids].ID,ID); //Copio el ID pasado por parametro en el vector
	vids[contadorVids].sok=sok;
	contadorVids++;
	return 1;
}

int imprimirVids(int maxID,nodoID*vids)
{
	int i;

	//printf("Ejecutando imprimirVids\n");
	printf("********************\n");
	for(i=0;i<maxID;i++)
		printf("%s - %d\n",vids[i].ID,vids[i].sok);
	printf("********************\n");
	return 1;
}

int buscarEnVids(int maxID,char*ID,nodoID*vids)
{
	int i;

	//printf("Ejecutando buscarEnVids\n");
	for(i=0;i<maxID;i++)
	{
		//Verificamos que el ID no me halla llegado ya
		if(strcmp(vids[i].ID,ID)==0) 
		{
			return vids[i].sok;
		}
	}
	//printf("No esta el ID!  Probar con un Vector Mayor\n");
	return -1;	
}

int cantidadCentralesOcupadas(int*vsockets,int centmax)
{
	int i;
	int cantidad=0;
	
	for(i=0;i<centmax;i++)
	{
		if(vsockets[i]!=0) 
			cantidad++;
	}	
	return cantidad;
}

///////////////////////////////////////////////////////////////////////////////////////
int procesarVueltas(void)
{
	int i,tipo;

	//printf("Ejecutando procesar vueltas\n");
	for(i=0;i<TOPEVEC;i++)
	{
		if(vecQuery[i].sok!=-1)
		{
			vecQuery[i].vueltas--;
			if(vecQuery[i].vueltas==0)
			{
				tipo=buscarTipoVecQuery(vecQuery[i].sok);
				if(tipo)
					enviarCame(ENVX,"\0",&vecQuery[i].sok,0,1,0,"\0");			
				else 
					enviarCame(CALX,"\0",&vecQuery[i].sok,0,1,0,"\0");
				quitarVecQuery(vecQuery[i].sok);
				printf("\tNo se encontro en las centrales vecinas el celular buscado\n");
			}	
		}
	}
	return 1;
}

int quitarVecQuery(int sok)
{
	int i,j,bol=0;

	//printf("Ejecutando quitarVecQuery\n");
	for(i=0;i<TOPEVEC;i++)
	{
		if(vecQuery[i].sok==sok)
		{
			vecQuery[i].sok=-1;
			vecQuery[i].vueltas=-1;	
			//imprimirVecQuery();
			return 1;
		} 
	}
	if(TOPEVEC==i) 
	{
		//printf("no estaba en el vector la concha tuyaaaa!!!!!!!\n");
		//imprimirVecQuery();
		return -1;
	}
}


int agregarVecQuery(int sok,int tipo)
{
	int i;

	//printf("Ejecutando agregarVecQuery\n");
	for(i=0;i<TOPEVEC;i++)
	{
		if(vecQuery[i].sok==-1)
		{
			vecQuery[i].sok=sok;
			vecQuery[i].vueltas=NUMERODEVUELTAS;
			vecQuery[i].tipo=tipo;
			//imprimirVecQuery();
			return 1;
		}
	}
	if(i==TOPEVEC) 
		return -1;
	return 1;
}


int inicializarVecQuery(void)
{
	int i;

	//printf("Ejecutando inicializarVecQuery\n");
	for(i=0;i<TOPEVEC;i++)
	{
		vecQuery[i].sok=-1;
		vecQuery[i].vueltas=-1;
	}
	//imprimirVecQuery();
	return 1;
}

int imprimirVecQuery(void)
{
	int i;

	printf("VecQuery\n");
	for(i=0;i<TOPEVEC;i++)
	{
		printf ("%d - sok: %d vueltas: %d\n",i,vecQuery[i].sok,vecQuery[i].vueltas);
	}
	return 1;
}

int buscarTipoVecQuery(int sokBuscado)
{
	int i;

	for(i=0;i<TOPEVEC;i++)
	{
		if(vecQuery[i].sok==sokBuscado) 
			return vecQuery[i].tipo;
	}
}


int actualizarEsperoQueryHit(void)
{
	int j,bol=0;
	
	//printf("Ejecutando actualizarEsperoQueryHit\n");
	for(j=0;j<TOPEVEC;j++)
	{
		if(vecQuery[j].sok!=-1) 
			bol=1;		
	}
	if(bol==0) 
		esperoQueryHit=NO;
	else 
		esperoQueryHit=SI;	
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////
int buscaYsaca( ptrNodoCola *colaFte, ptrNodoCola *colaFin, int socket )
{
   ptrNodoCola ptrAnterior; /* apuntador a un nodo previo de la lista */
   ptrNodoCola ptrActual;   /* apuntador al nodo actual de la lista */

   
   ptrAnterior = NULL;
   ptrActual = *colaFte;

	/* ciclo para localizar la ubicación correcta en la lista */
	while ( ptrActual != NULL && socket != ptrActual->info.socket ) 
	{	  
		ptrAnterior = ptrActual;          /* entra al ...   */
		ptrActual = ptrActual->sgte;  /* ... siguiente nodo */
	}

	/* Saco el que esta  al principio de la lista */
	if ( ptrAnterior == NULL ) 
	{	  
		if( *colaFte == NULL )
		{
			//printf("No esta el socket en la cola, ya que esta vacia\n");
			return 1;
		}
		*colaFte = ptrActual->sgte;
		free( ptrActual );
		return 1;
	}
    else 
	{  /* Saco del medio */
		if( ptrActual == NULL )
		{
			//printf( "No se encontro el socket en esta cola\n");
			return 1;
   		} 
		ptrAnterior->sgte = ptrActual->sgte;
		if( ptrActual->sgte == NULL)
			*colaFin = ptrAnterior;
		free( ptrActual );
		 return 1;
    }	  
	
}
