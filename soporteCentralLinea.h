//Librerias...
//#include "soporteGeneral.h"

#define BACKLOG 10 // Cuantas conexiones se mantienen en la cola
#define MAXDATASIZE 100
#define MAXLINEA 100
#define STDIN 0
#define CAPACIDAD 7
#define TTLdef 5
#define TIMEVUELTA 10
#define NUMERODEVUELTAS 2
#define SI 1
#define NO 0
#define TOPEVEC 10

// Estructuras -------------------------------------------------

typedef struct {
		char numero[11];
		char ip[16]; //averiguar si el max es 16
		char port[6];
		int socket;
	      } nodo;

struct nodoCola {   
   		nodo info;                
   		struct nodoCola *sgte; /* apuntador nodoCola */
		};                    /* fin de la estructura nodoCola */

typedef struct nodoCola NodoCola;
typedef NodoCola *ptrNodoCola;
	
//Prototipos-----------------------------------------------------
int quitarVector(int socket, nodo *pVector, int tope);
int agregarVector( nodo *newCel, nodo *pVector, int tope);
int buscarVector(char* nroBuscado, nodo *pVector, int tope);
int buscarPorSocket(int socket, nodo *pVector, int tope);
int tengoCapacidad(nodo *pVector, int tope);
int atenderConexion(int listener,int *fdmax,fd_set *master,nodo*vcululares,int tope, ptrNodoCola *colaFte, ptrNodoCola *colaFin);
int atenderCliente( 	char *buffer,
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
			);
void mostrarVector( nodo* vector, int tope);
int atenderCONA(int new_fd,nodo *vcelulares,int tope,int lenmsj, ptrNodoCola *colaFte, ptrNodoCola *colaFin);
int cargarConfigCentral(char* ruta,
			char* ip,
			char* port,
			int* celumax,
			char*ipVecino,
			char*portVecino,	
			int*centmax,
			int*maxID,
			char*centralID
			);
void imprimeCola( ptrNodoCola ptrActual );
int estaVacia( ptrNodoCola ptrCabeza );
int retirar( ptrNodoCola *colaFte, ptrNodoCola *colaFin, nodo* valor);
int agregar( ptrNodoCola *colaFte, ptrNodoCola *colaFin, nodo* valor );
int buscaYsaca( ptrNodoCola *colaFte, ptrNodoCola *colaFin, int socket );
int agregarAVsockets(int*vsockets,int socket,int pos);
int buscarLibreVsockets(int*vsockets,int CONFERENCIA);
int quitarDeVsockets(int*vsockets,int socket,int CONFERENCIA);
int inicializarVsockets(int*vsockets,int CONFERENCIA);
void imprimirVsockets(int*vsockets,int CONFERENCIA);
int cantidadCentralesOcupadas(int*vsockets,int centmax);
int agregarAVids(int maxID, char ID[15],int sok,nodoID * vids);
int imprimirVids(int maxID,nodoID*vids);
int buscarEnVids(int maxID,char*ID,nodoID*vids);
//Funciones de VecQuery
int inicializarVecQuery(void);
int procesarVuelta(void);
int quitarVecQuery(int sok);
int agregarVecQuery(int sok,int tipo);
int imprimirVecQuery(void);
int buscarTipoVecQuery(int sokBuscado);
int actualizarEsperoQueryHit(void);
//----------------------------------------------------------------
 
