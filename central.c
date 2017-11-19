//                                       CENTRAL TELEFONICA CELULAR
#include "soporteGeneral.h"
#include "soporte.h"

//Variables Globales
int contadorID = 0;
int contadorVids = 0;
char centralID[6];
char ID[15];
char *cargador;  // ip*puerto del cargador asociado a  dicha central
int sokCargador;
int estoyConectado;    //Determina si ya estoy conectado a la red de centrales(1) o no(0)! 
int esperoQueryHit;
nodoVecQuery vecQuery[TOPEVEC];
int contadorMGR=0;

int main(int argc, char* argv[])
{

	int new_fd; 						//El socket de una nueva conexion aceptada
	struct sockaddr_in my_addr; 		//estructura con mi informacion
	nodo * vcelulares;          		//Vector de los Celulares que tengo conectados!!!!!!!
	nodoID * vids;						//Estructura de maxID IDs de los PING o PONG recibidos
	int maxID;               			//Cantidad de IDs que se guardan en memoria, sacado de AdeC
	int * vcentrales;
	int yes=1;
	int i,j, listener,socketVecino = 0;	//SOCKETS dejar el =0
	int sin_size,numbytes;
	char buffer[MAXDATASIZE];
	int fdmax; 							//numero maximo de descriptores de fichero
	int puertoAjeno;
	char port[6];
	char portVecino[6];
	char ip[16];
	char ipVecino[16];
	int topeVector,centmax;
	struct timeval tiempo;
	ptrNodoCola colaFte = NULL; 		/* incializa ptrCabeza */
	ptrNodoCola colaFin = NULL;
	nodo aux;							//VER SI VA ACA O DENTRO DE LA FUNCION 
	fd_set master; 						//Conjunto Master de ficheros
	fd_set readfds; 					//Conjuntos de ficheros de los cuales leer
	int k,w;

	if(argc!=2) 
	{
		logger("Error","Central", "Debo usar:./cen < archivo de configuracion >", 1);
		return -1;
	}
	
	cargador = NULL;
	//system("clear");//Limpia la Pantalla

	if( cargarConfigCentral( 	argv[1],
								ip, 
								port, 
								&topeVector, 
								ipVecino,
								portVecino,
								&centmax,
								&maxID,
								centralID ) == -1 )
	{
		logger("Error","Central", "No se pudo cargar el archivo de Configuracion ", 1);
		return -1;
	}
	logger("Informacion","Central", "Configuracion Cargada Correctamente", 1);
	//printf(" bienvenidos\n-= Central: %s =- \n", centralID);
	
		
	seniales(SIGCHLD,sigHandler);

	tiempo.tv_sec = TIMEVUELTA;
	tiempo.tv_usec = 0;	
	
	inicializarVecQuery();
	actualizarEsperoQueryHit();
	FD_ZERO(&master); 	//Inicializo
	FD_ZERO(&readfds);	//Inicializo
	
	//printf("ip: %s\npuerto: %s\n",ip, port);
	//printf("topeVector: %d\ncentMax: %d\n", topeVector, centMax);
	fdmax=0;
	estoyConectado=0;
	if(strcmp(ipVecino,"*")==0) 
	{
	    logger("Informacion","Central", "Primer Central. No se conectara a ninguna otra", 0);
	    estoyConectado=1;
	}	
	else 
	{
		IDMensaje();
		if( (socketVecino = crearSocket(portVecino,ipVecino) ) == -1 )
		{
			logger("Error","Central", "No se pudo crear socket con el vecino", 0);
			return -1;
		}
		if( enviarCame(PING,"\0", &socketVecino,0,TTLdef,0,ID) == -1)
		{
			logger("Informacion","Central", "Fallo el enviarCame de TCOA", 0);
			return -1;
		}
		FD_SET(socketVecino,&master);
	}
	if( (vcelulares = calloc(topeVector,sizeof(nodo) ) )==NULL) 
	{
		logger("Error","Central", "Memoria Insuficiente", 0);
		return -1;
	}
	if( (vcentrales = calloc(centmax,sizeof(int) ) ) ==NULL ) 
	{
		logger("Error","Central", "Memoria Insuficiente", 0);
		return -1;
	}
	if( ( vids = calloc(maxID,sizeof(nodoID) ) ) ==NULL ) 
	{
		logger("Error","Central", "Memoria Insuficiente", 0);
		return -1;
	}
	inicializarVsockets(vcentrales,centmax);
	//imprimirVsockets(vcentrales,centmax);
	if((listener = crearListener( port, ip ))==-1) 
	{
		logger("Error","Central", "No se pudo crear el listener de celulares", 0);
		return -1;
	}
	FD_SET(listener,&master);
	fdmax = listener;  //Por ahora este es el maximo, el unico
	if(socketVecino > listener) 
		fdmax = socketVecino;
	logger("Informacion","Central", "Estructuras Inicializadas Correctamente", 0);
	printf("***********************************************************************\n");
	printf("* NOMBRE DE LA CENTRAL * %s                                        *\n", centralID);
	printf("***********************************************************************\n");
	printf("* DIRECCION DE IP      * %s                                    *\n", ip);
	printf("***********************************************************************\n");
	printf("* TELEFONOS CONOCIDOS  *      NO HAY TELEFONOS CONOCIDOS              *\n");
	printf("***********************************************************************\n");
	
	//BUCLE PRINCIPAL
	for(;;)
	{
		readfds = master;
		if(select (fdmax+1,&readfds,NULL,NULL,&tiempo) == -1)
		{
			select_controlar(&readfds,&master);
			return -1;
		}
		//exploramos las conexiones de readfds
		for(i=0;i<=fdmax;i++)
		{
			if(FD_ISSET(i,&readfds))
			{ 
				//Hay datos en el socket i
				if(i==listener)	
					if( atenderConexion(i,&fdmax,&master,vcelulares,topeVector, &colaFte,&colaFin ) == -1)
					{
						logger("Error","Central", "Fallo AtenderConexion", 0);
						return -1;
					}
				if(i!=listener)
				{
					//manejar los datos de un cliente
					if(( numbytes = recv(i,buffer,sizeof(cabecera),0) ) <=0)
					{
						// es un error o conexion cerrada
						//Verificar si es una central o un celular o el cargador
						quitarDeVsockets(vcentrales,i,centmax);
						if( quitarVector(i,vcelulares,topeVector) != -1 )
						{
							printf("\tCelular quitado de nuestra red de clientes\n");
							if( colaFte != NULL )
							{		
								logger("Informacion","Central", "Se agregara un celular que esperaba lugar en la red", 1);
								imprimeCola( colaFte );		
								if( retirar( &colaFte, &colaFin, &aux) == -1 )
								{
									logger("Error","Central", "No se pudo retirar de la Cola de espera", 0);
									return -1;
								} 
								if(agregarVector(&aux,vcelulares,topeVector) == -1)
								{
									logger("Error","Central", "No se pudo agregar a la red un nuevo cliente", 0);
									return -1;
								}
								logger("Informacion","Central", "Nuevo celular conectado a nuestra red de clientes", 1);
								mostrarVector( vcelulares, topeVector);
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
						}
						buscaYsaca( &colaFte, &colaFin, i);
						if( i == sokCargador )
						{
							cargador = NULL;
							logger("Informacion","Central", "El cargador se ha ido", 1);
						}
						if(numbytes==0) 
						{	
							//conexion cerrada
							logger("Informacion","Central", "Conexion Cerrada", 1);
							close(i);
							FD_CLR(i,&master);
						}
						else	
						{ 	//Error!
							logger("Error","Central", "Fallo recepcion de datos", 0);
							close(i);
							FD_CLR(i,&master);
						}
					}
					else
					{ 	//No hay errores y hay datos
						if(atenderCliente(	buffer,
											i,
											vcelulares,
											topeVector,
											&colaFte,
											&colaFin,
											vcentrales,
											centmax,
											maxID,
											vids,
											port,
											ip,
											&master,
											&fdmax	)==-1)
						{			
							logger("Error","Central", "Fallo atenderCliente", 0);
							return -1;
						}
					}	
				} // Cierra el If( i != listener)	
			} // Cierra if que veridica el socket que se esta moviendo		
		} // Cierra!el for que recorre el master en busca del socket que se esta moviendo 
		if( tiempo.tv_sec == 0 )
		{	
			tiempo.tv_sec = TIMEVUELTA;
			tiempo.tv_usec = 0;
			actualizarEsperoQueryHit();
			if( esperoQueryHit == SI )
			{
				procesarVueltas();
			}
		}
	} // Cierra el Bucle Principal					
	free(vcelulares);
	free(vids);
	return 1;
} //FIN DEL MAIN.



