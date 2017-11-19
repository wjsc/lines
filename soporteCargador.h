// SOPORTE CARGADOR

//Define -------------------------------------------------------
#define STANDBY_TIME 100
#define MAXDATASIZE 60
#define MAXLINEA 60
#define FIFO 0
#define RR 1
#define SRT 2
#define ENCENDIDO 1
#define APAGADO 0
#define CONECTADO 1
#define DESCONECTADO 0
#define MAXCODIGOS 6

// Estructuras -------------------------------------------------

typedef struct {
		int restoCarga;
		int socket;
		char NRO[11];
	      } nodo;

struct nodoCola {   
   		nodo info;                
   		struct nodoCola *sgte; /* apuntador nodoCola */
		};                    /* fin de la estructura nodoCola */

typedef struct nodoCola NodoCola;
typedef NodoCola *ptrNodoCola;

//Prototipos-----------------------------------------------------

int conectarConCentral( int sok, char *port, char *ip );
int cargarCargConfig( 	char* ruta,
			            char** ip,
						char** port,
						int* capacidad,
						int* tasa,
						char** ipCent,
						char** portCent,
						int *timeRafaga	);
int atenderConexionCelular(int listener,int *fdmax,fd_set *master,int capacidad,struct timeval *tiempo,
							ptrNodoCola *CC_Carga, ptrNodoCola *CF_Carga, ptrNodoCola *CC_Espera,
							ptrNodoCola *CF_Espera);
int atenderTecladoCargador( int teclado, int sokCent, ptrNodoCola *CC_Carga, ptrNodoCola *CF_Carga, struct timeval *tiempo, fd_set *master, ptrNodoCola *CC_Espera, ptrNodoCola *CF_Espera );
int atenderCentral( int sok, int *fdmax,fd_set *master );
int agregar( ptrNodoCola *colaFte, ptrNodoCola *colaFin, nodo* valor );
int agregarComoRR( ptrNodoCola *colaFte, ptrNodoCola *colaFin, nodo* valor );
int agregarComoSRT( ptrNodoCola *colaFte, ptrNodoCola *colaFin, nodo* valor );
int retirar( ptrNodoCola *colaFte, ptrNodoCola *colaFin, nodo* valor );
void MensajeAyuda( );
int estaVacia( ptrNodoCola colaFte );
int CambiarCelularEnCarga( 	ptrNodoCola *CC_Carga,ptrNodoCola *CF_Carga, ptrNodoCola *CC_Espera,
							ptrNodoCola *CF_Espera, struct timeval *tiempo, int capacidad, fd_set *master);
int rotar(ptrNodoCola *CC_Carga, ptrNodoCola *CF_Carga);
int asignarTiempo( ptrNodoCola *CC_Carga, struct timeval *tiempo );
int buscarSacar( ptrNodoCola *colaFte, ptrNodoCola *colaFin, int socket );
void imprimeCola( ptrNodoCola ptrActual );
int buscarEnVector(char*buffer);
