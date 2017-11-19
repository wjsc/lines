// SOPORTE CELULAR

#define TOPE 5
#define MAXDATASIZE 100 //Por ejemplo cuantos caracteres puedo leer del teclado
#define TAMMAXNOMBRE 35
#define MAXLINEA 50
#define MAXCODIGOS 7
#define MENSAJES 5
#define BUFFENVIO 4096
#define VUELTAS 10
#define OFFLINE 0
#define ONLINE 1
#define SMSTTL 2
#define TIMEDESC 10
typedef struct{
		char NRO[11];
	} nodoNRO;

//Prototipos
int atenderConexion(int listener,int *fdmax,fd_set *master);
int atenderEntrada(	int sokMov,
					char**buffer1,
					int**vsockets,
					int *fdmax,
					fd_set *master,
					int central) ;
int atenderTeclado(int,int,int*vsockets);
int entrarEnRed( char* nroCel, int socket,char*puerto );
int buscarEnVector(char*);
int agregarAVsockets(int*vsockets,int socket,int pos, char* AUX);
int buscarLibreVsockets(int*vsockets,int CONFERENCIA);
int quitarDeVsockets(int*vsockets,int socket,int CONFERENCIA);
int inicializarVsockets(int*vsockets,int CONFERENCIA);
void imprimirVsockets(int*vsockets,int CONFERENCIA);
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
						char** PATH );
int receptor(int listenerFile,char*SENDPORT,fd_set *master,int fdmax,int estoyHablando);
int enviador(char*ip,char*puerto,fd_set *master,int fdmax);
void sigHandler(int senial);
void seniales(int senial,void (*funcion) (int nroSenial));
void select_controlar(fd_set * readfds,int listenerFile,fd_set *master);
int robot( int *vsockets, int CONFERENCIA );
