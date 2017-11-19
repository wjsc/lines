//////////////////////////*CARGADOR LOCO LOCO*////////////////////////////////
#include "soporteGeneral.h"
#include "soporteCargador.h"

//VARIABLES GLOBALES

int algoritmo;
int estado;
int numCelCarg;
int timeRafaga;
int tasa;
char centConexion;
const char* vcodigos[MAXCODIGOS]={"FIFO", "RR","SRT","ENCENDER","APAGAR","ESTADO"};
int contadorID=0;
char centralID[10];	//Este seria el celular ID, pero lo dejo asi para que sea igual a la central
char ID[15];			//Id de los mensajes...concatenacion de centralID + contador

//Programa principal...
int main(int argc,char* argv[])
{
	//VARIABLES DEL PROCESO
	char *buffer;
	char *ip;
	char *ipCent;
	char *port;
	char *portCent;
	int capacidad;
	int sokCent, listener;
	int fdmax;
	ptrNodoCola CC_Carga = NULL;
	ptrNodoCola CF_Carga = NULL;
	ptrNodoCola CC_Espera = NULL;
	ptrNodoCola CF_Espera = NULL; 
	fd_set master; 		
	fd_set readfds; 	
	struct timeval tiempo;
	int time0, cantRafagas;
	int i, numbytes,opcion;
	nodo valor;
	
	//Inicializo los ficheros de descriptores
	FD_ZERO(&master); 
	FD_ZERO(&readfds);	
	
	//Prendo el cargador
	estado = ENCENDIDO;
	numCelCarg = 0;
	
	//Verificamos si la cantidad de parametros esta bien
	if(argc != 2) 
	{
		logger("Error","Cargador", "Debo usar:./cargador <Archivo de configuracion> ", 1);
		return -1;
	}
	
	//Cargamos el archivo de configuracion
	if( cargarCargConfig( argv[1], &ip, &port, &capacidad, &tasa, &ipCent, &portCent, &timeRafaga ) == -1 )
	{
		logger("Error","Cargador", "No se pudo cargar el archivo de Configuracion ", 1);
		return -1;
	}
	logger("Informacion","Cargador", "Configuracion Cargada", 0);
	//Establecemos el tiempo por el cual va a cortar el select para cargar una rafaga.
	tiempo.tv_sec = STANDBY_TIME;
	tiempo.tv_usec = 0;
	//Creamos el socket y nos conectamos a la direccion pasada por parametro
	if( ( sokCent = crearSocket(portCent,ipCent) ) == -1 )
	{	
		logger("Error","Cargador", "No se pudo crear el socket con la Central", 1);
		return -1;
	}	
	logger("Informacion","Cargador", "Socket con la Central creado.", 0);
	//Le paso a la central el puerto donde voy a escuchar celulares
	if( conectarConCentral( sokCent, port, ip) == -1 )
	{
		logger("Error","Cargador", "No se pudo establecer la conexion con la Central.", 1);
	}
	logger("Informacion","Cargador", "Conexion con la Central establecida.", 1);
	//Con este socket vamos a escuchar celulares que quieran entrar a cargarse
	if( ( listener = crearListener( port, ip ) ) == -1)
	{
		logger("Error","Cargador", "No se pudo crear el listener de telefonos a cargar.", 0);
		return -1;
	}
	logger("Informacion","Cargador", "Listener escucha de celulares a cargar, Creado", 0);
	//En este buffer recibo las cabeceras...
	if( ( buffer = calloc(1,sizeof(cabecera)) ) == NULL )
	{
		logger("Error","Cargador", "Memoria Insuficiente", 0);
		return -1;
	}
	//Determino que algoritmo va a usar el cargador
	printf("********************************************************************************\n"); 
	printf("*                                                                              *\n");     
	printf("*     Digite alguna de las 3 primeras opciones y pulse ENTER:                  *\n");
	printf("*                                                                              *\n"); 
	MensajeAyuda( );
	printf("*   Su opcion: ");
	scanf( "%d" , &opcion );
	printf("*   Su opcion fue: %d.                                                        *\n", opcion);
    printf("********************************************************************************\n"); 
	switch(opcion)
	{
		case 1:	
			algoritmo = FIFO;
			logger("Informacion","Cargador", "Se ha establecido una planificacion FIFO.", 1);
			break;
		case 2: 
			algoritmo = RR;
			logger("Informacion","Cargador", "Se ha establecido una planificacion ROUND ROBIN.", 1);
			break;
		case 3: 
			algoritmo = SRT;
			logger("Informacion","Cargador", "Se ha establecido una planificacion SRT.", 1);
			break;
		default: 
			algoritmo = FIFO;
			logger("Informacion","Cargador", "Opcion Invalida.Por defecto se usara una planificacion FIFO.", 1);
			printf("\tSi desea cambiarla utilice alguna de las opciones indicadas anteriormente\n");
			break;
	}
	 printf("********************************************************************************\n");
	//Agrego los sockets al master. Y actualizo el fdmax...
	FD_SET(listener,&master);
	FD_SET(STDIN,&master);
	FD_SET(sokCent,&master);
	fdmax = listener;
	if( listener > fdmax) 
		fdmax = listener;
	if( sokCent > fdmax) 
		fdmax = sokCent;
	//Bucle Principal
	printf("\n\t*************************\n");	
	printf(  "\t*     Cola de Carga     *\n" );
	printf(  "\t*************************\n");	
	imprimeCola( CC_Carga );
	printf("\n\t*************************\n");	
	printf(  "\t*     Cola de Espera    *\n" );
	printf(  "\t*************************\n");
	imprimeCola( CC_Espera );
	for( ; ; )
	{
		readfds = master;
		time0 = tiempo.tv_sec; //tiempo De Entrada Al Cargador ( que simula al Procesador )
		//printf( "Se le asignaran %d segundos como maximo de uso del cargador\n", time0 );
		if( select( fdmax+1, &readfds, NULL, NULL, &tiempo ) == -1)
		{
			logger("Error","Cargador", "Fallo el select.", 0);
			return -1;
		}
		//printf( "Se interrumpio el select, faltaban cargar %d segundos\n", tiempo.tv_sec);
		//Descontamos lo que se cargo hasta que se interrumpio el select
		if(estado == ENCENDIDO && CC_Carga != NULL)
		{
			cantRafagas = (time0 - tiempo.tv_sec) / timeRafaga;
			CC_Carga->info.restoCarga -= cantRafagas * tasa;
		}
		
		//exploramos las conexiones de readfds
		for(i=0; i<=fdmax; i++)
		{
			if( FD_ISSET(i,&readfds) )
			{	
				//Hay datos en el socket 			
				if( i == listener )
				{
					//Es un nuevo celular que necesita ser cargado
					logger("Informacion","Cargador", "Procesando Nuevo Celular a Cargar.", 1);
					if( atenderConexionCelular(i,&fdmax,&master,capacidad,&tiempo,&CC_Carga,&CF_Carga,&CC_Espera,&CF_Espera ) == -1 )
					{
						logger("Error","Cargador", "Fallo atenderConexionCelular.", 0);
						return -1;
					}
 
				}
				else
				{
					if(i == STDIN) 
					{
						//Es una instruccion que se le dara al cargador...
						if( atenderTecladoCargador( i, sokCent, &CC_Carga, &CF_Carga, &tiempo, &master, &CC_Espera, &CF_Espera ) == -1 )
						{
							logger("Error","Cargador", "Fallo atenderTecladoCargador", 0);
							return -1;
						}
					}
					else
					{
						if( i == sokCent )
						{
							if( atenderCentral( i,&fdmax,&master ) == -1 )
							{
								logger("Error","Cargador", "Fallo AtenderCentral.", 0);
								return -1;
							}
						}
			 			else
						{
							if( (numbytes = recv(i,buffer,sizeof(cabecera),0)) <= 0)
							{
								// ES UN ERROR O CONEXOIN CERRADA.
								if(numbytes == 0) 
								{	//conexion cerrada
									printf("\n");
									logger("Informacion","Cargador", "Un celular se ha caido.", 1);
									printf("\tProcesando Caida,\n");
								}
								else	
								{
									logger("Error","Cargador", "Fallo al recibir datos", 0);
								}
								if( buscarSacar(&CC_Carga, &CF_Carga, i) == 0 )
								{
									printf( "\tNo se encontro el celular en la cola de Carga\n" );
									if( buscarSacar(&CC_Espera, &CF_Espera, i) == 0)
										printf( "No se encontro el celular en la cola de Espera\n");
									else 
										logger("Informacion","Cargador", "El celular fue quitado de la Cola de Espera.", 1);
								}
								else
								{
									logger("Informacion","Cargador", "El celular fue quitado de la Cola de Carga.", 1);
									numCelCarg--;
									if( (numCelCarg < capacidad) && (CC_Espera != NULL) )
									{
										retirar( &CC_Espera, &CF_Espera, &valor);
										switch( algoritmo )
										{
											case FIFO: 	if( agregar(&CC_Carga, &CF_Carga, &valor) == -1 )
														{
															logger("Error","Cargador", "No se pudo agregar un celular a la Cola de Carga.", 0);
															return -1;
														}
														break;
											case RR:	if( agregarComoRR( &CC_Carga, &CF_Carga, &valor) == -1)
														{	
															logger("Error","Cargador", "No se pudo agregar un celular a la Cola de Carga.", 0);
															return -1;
														}
														break;
											case SRT: 	if( agregarComoSRT(&CC_Carga, &CF_Carga, &valor) == -1 )
														{
															logger("Error","Cargador", "No se pudo agregar un celular a la Cola de Carga.", 0);
															return -1;
														}
														break;
										}
										numCelCarg++;
									}
								}
								logger("Informacion","Cargador", "Cola de Espera y Cola de Carga Actualizadas.", 1);
			
								printf("\n\t*************************\n");	
								printf(  "\t*     Cola de Carga     *\n" );
								printf(  "\t*************************\n");	
								imprimeCola( CC_Carga );
								printf("\n\t*************************\n");	
								printf(  "\t*     Cola de Espera    *\n" );
								printf(  "\t*************************\n");
								imprimeCola( CC_Espera );
								
								FD_CLR(i,&master); //Lo saco del master ya que no  lo voy a antender mas.
								close(i);
							}
							else 
							{ 	//NO HAY ERRORES Y HAY DATOS.
								//No se que podria hacer aca!!!!!!				       
								//atenderEntrada(i,buffer);
						  	}
						}
					}
				}
			}			
		}//Cierra el for que recorre readfs
		//printf( "El tiempo es: %d\n", tiempo.tv_sec);
		//Si salio porque se acabo el tiempo del procesador, debemos cambiar el celular que carga...
		if( estado == ENCENDIDO && tiempo.tv_sec == 0 )
		{
			if( CC_Carga != NULL )
				CambiarCelularEnCarga( &CC_Carga, &CF_Carga, &CC_Espera, &CF_Espera, &tiempo, capacidad, &master);
			else
				{
					tiempo.tv_sec = STANDBY_TIME;
					tiempo.tv_usec = 0;
				}
		}
		//printf( "Cola de Carga:\n" );
		//imprimeCola( CC_Carga );
		//printf( "Cola de Espera:\n" );
		//imprimeCola( CC_Espera );
		//Verificamos si la central no se cayo!!
		if( estado == ENCENDIDO && centConexion == DESCONECTADO )
		{
			if( ( sokCent = crearSocket(portCent,ipCent) ) == -1 )
			{	
				printf("La central no se encuentra. Se intentará conectarse mas adelante.\n");
			}	
			else
			{
				//printf("La central es el socket : %d\n",sokCent);
				//Le paso a la central el puerto donde voy a escuchar celulares
				conectarConCentral( sokCent, port, ip);
			}
		}
	}//Cierra el BuclePrincipal
	close( sokCent );
	close( listener );
	free( buffer );
	return 1;
}
