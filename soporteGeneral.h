//Librerias
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include<sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <ctype.h>
#include <time.h>

//Defines
#define STDIN 0
#define STDOUT 1

//Protocolo
#define CONA 0x10
#define CALA 0x12
#define CALX 0x13
#define CALZ 0x14
#define YAMA 0x15
#define YAMZ 0x16
#define YAMX 0x17
#define FINA 0x18
#define FINZ 0x19
#define SMS  0x20
#define APGA 0x30
#define TCOA 0x40
#define TCOX 0x41
#define TCOZ 0x42
#define GCOA 0x45
#define GCOZ 0x46
#define GBBA 0x47
#define GBBZ 0x48
#define GCCA 0x49
#define CRGA 0x50
#define CRGZ 0x51
#define CRGX 0x52
#define GBBX 0x53
#define GCCX 0x54
#define MGRA 0x55
#define MGRX 0x56
#define MGRZ 0x57
#define ENVA 0x60
#define ENVZ 0x61
#define ENVX 0x62
#define DSPA 0x63
#define DSPX 0x64
#define DSPZ 0x65
#define PING 0x70
#define PONG 0x71
#define Query 0x80
#define QueryHit 0x81
#define PARA 0x90
#define PARZ 0x91
#define KILL 0x92
#define MGGO 0x93	

//Estructuras
typedef struct {
		int sok;
		int vueltas;
		int tipo;
	      } nodoVecQuery;

typedef struct {
		char ID[16];
		unsigned short int tipo; 
		unsigned short int TTL;
		unsigned short int HOPS;	
		int largo;
	      } cabecera;

typedef struct {
		char ID[15];
		int sok;
		} nodoID;
		
		
typedef struct {
		unsigned int port;
		struct in_addr ip; //son 4 bytes???
		int ocupados;
		int libres;
		} mensajePONG;	      
		
typedef struct {
		unsigned int port;
		struct in_addr ip; //son 4 bytes???
		} mensajeQueryHit;

// Estructuras 
typedef struct{
	char NROCELULAR[11];
	int DUALMODE;
	int CONFERENCIA;
	int TRANSFERENCIA;
	int CARGAACT;
	int CARGAMAX;
	int CARGAMIN;
	int contadorID;
} migracion;


//Prototipos
int crearListener(char* port, char *ip);	
int crearSocket(char*puerto,char*ip);
int enviarCame(	int tipo,
				char* mensaje,
				int*vsockets,
				int tope,
				unsigned short int TTL,
				unsigned short int HOPS, 
				char*ID );
int hayAlguien ( int *vsockets, int tope );
int IDMensaje( void );
void logger(char* tipo, char *proceso, char *evento, int out);


