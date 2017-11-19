#include "soporteGeneral.h"

extern int contadorID;
extern char centralID[6];
extern char ID[15];

//////////////////////////////////////////////////////
int crearListener(char* port, char *ip ) 
{
	int flag = 0;	
	struct sockaddr_in myaddr;     // server address
	int yes=1;       // for setsockopt() SO_REUSEADDR, below
	int listener;
	int puerto;
	
	puerto=atoi(port);
	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{
		logger("Error","Socket", "No se pudo crear el listener", 0);
		return -1;
	}

	// lose the pesky "address already in use" error message
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
	{
		logger("Error","Listener", "No se pudo crear el listener", 0);
		return -1;
	}

	// bind
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = inet_addr(ip);
	memset(&(myaddr.sin_zero), '\0', 8);	
	myaddr.sin_port = htons(puerto);

	if (puerto == 0) flag = 1;

	if (bind(listener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) 
	{
		logger("Error","Bind", "No se pudo crear el listener", 0);
		return -1;
	}

	// listen
	if (listen(listener, 20) == -1) 
	{
		logger("Error","Listen", "No se pudo crear el listener", 0);
		return -1;
	}

	//if (flag) printf("El puerto es %d\n", ntohs(myaddr.sin_port));

	return listener;
}

//////////////////////////////////////////////////////
int enviarCame(	int tipo,
				char* mensaje,
				int*vsockets,
				int tope,
				unsigned short int TTL,
				unsigned short int HOPS, 
				char*ID )
{
	cabecera* cabe;
	int i=0;
	
	//printf("Ejecutando enviarCame %d\n",tipo);
	if( ( cabe = calloc(1,sizeof(cabecera) ) ) == NULL)
	{
		logger("Error","EnviarCame", "Memoria Insuficiente", 0);
		return -1;
	}	
	cabe->tipo=(unsigned short int)tipo;
	if(TTL==0) 
	{
		//printf("Came No Enviado! (TTL=0)\n");
		free(cabe);
		return 1;
	}
	TTL--;
	HOPS++;
	cabe->HOPS=HOPS; 
	cabe->TTL=TTL;
	strcpy(cabe->ID,ID);
	if(strcmp(mensaje,"\0")==0) 
		cabe->largo=0;
	else 	
	{
		if(tipo==PONG) 
		{
			cabe->largo=sizeof(mensajePONG);	
			//printf("Es un PONG, el largo es: %d\n", cabe->largo);
		}
		else 
		{
			if (tipo==QueryHit) 
				cabe->largo=sizeof(mensajeQueryHit);	
			else 	
			{
				if( tipo == PARZ)
					cabe->largo = sizeof( migracion); 
				else 
					cabe->largo=strlen(mensaje);
			}	
		}
	}
	if(tope==0)
	{
		if( send( *vsockets, (char*)cabe, sizeof(cabecera), 0 ) == -1 )
		{
			logger("Error","EnviarCame", "Fallo el envio cabecera", 1);
			close(*vsockets);
			free(cabe);
			return -1;
		}
		if((cabe->largo)>0)
		{
			if( send( *vsockets, mensaje, cabe->largo, 0 ) == -1 )
			{
				logger("Error","EnviarCame", "Fallo el envio del mensaje", 1);
				close(*vsockets);
				free(cabe);
				return -1;
			}
		}
		free(cabe);
		return 1;
	}
	if(!hayAlguien (vsockets,tope)) 
	{
		//printf("Sin Enviar, No hay nadie conectado a mi\n");
		free(cabe);
		return 1;
	}
	//printf("Enviando a Quienes esten en el vector\n");
	for(i=0;i<tope;i++)
	{
		if(vsockets[i]!=0)
		{
			//printf("Le estoy enviando a %d\n",vsockets[i]);
			if( send( vsockets[i], (char*)cabe, sizeof(cabecera), 0 ) == -1) 
			{
				logger("Error","EnviarCame", "Fallo el envio", 0);
				close(vsockets[i]);
				free(cabe);
				return -1;
			}
			if(cabe->largo>0)
			{
				if( send( vsockets[i], mensaje, strlen(mensaje), 0 ) == -1 )
				{
					logger("Error","EnviarCame", "Fallo el envio", 0);
					close(vsockets[i]);
					free(cabe);
					return -1;
				}
			}
		} 
	}
	free(cabe);
	return 1;
}
//////////////////////////////////////////////////////
int crearSocket(char*puerto,char*ip)//Compartida con celular
{
	int sok;
	struct sockaddr_in servidor; //  direccion servidor

	//Con este socket nos vamos a conectar a la central o a otro servidor(Celular)
	if((sok = socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		//Log
		logger("Error","CrearSocket", "No se pudo crear", 0);
		close(sok);
		return -1; 
	}
	//printf("Conectando a ->  %s : %s\n", ip,puerto);			
	logger("Informacion","CrearSocket", "Conectado con el IP,PUERTO indicado", 0);
	//LLENO LA ESTRUCTURA DEL SERVIDOR
	servidor.sin_family=AF_INET;
	servidor.sin_port=htons(atoi(puerto));
	servidor.sin_addr.s_addr=inet_addr(ip);
	memset(&(servidor.sin_zero),'\0',8);
	//Log
	if ((connect(sok,(struct sockaddr*)&servidor,sizeof(struct sockaddr)))==-1)
	{
		logger("Error","CrearSocket", "Fallo el connect", 0);
		close(sok);		
		return -1;
	}
	//printf("Conectado!\n");
	return sok;
}
//////////////////////////////////////////////////////

int IDMensaje(void)  //COMPARTIDA!!!!!!!!
{
	char aux[6];

	contadorID++;
	sprintf(aux,"%d",10000+contadorID);
	strcpy(ID,centralID);
	strcat(ID,&aux[1]);
	return 1;
}
//////////////////////////////////////////////////////

int hayAlguien ( int *vsockets, int tope ) //Compartida con celular
{
	int i;	

	for ( i = 0; i< tope; i++)
		if( vsockets[i] != 0)
			return 1;
	return 0;
}
//////////////////////////////////////////////////////

void logger(char* tipo, char *proceso, char *evento, int out) 
{
	FILE* file;
	time_t actualTime;
	struct tm *pTiempo;
	char fecha[50];
	char pid[15];
	char *nombre;
	
	sprintf( pid, "%d", getpid() );
	nombre = calloc(1, strlen(pid) + strlen("log.txt") + 1);
	strcpy( nombre, pid );
	strcat( nombre, "Log.txt");
	if ( (file = fopen(nombre, "a+")) == NULL)
	{
		perror("fopen");
	}

	//Se obtine el tiempo actual
	actualTime = time(NULL);
	pTiempo = localtime(&actualTime);
	
	//Transforma los datos de fecha y hora a un formato de cadena
	strftime(fecha, 50, "%d-%m %T", pTiempo);
	
	fprintf(file, "%s %s [%d]: %s\n", fecha, proceso, getpid(), tipo);
	fprintf(file, "\t%s\n", evento);
	
	if (out) 
	{
		printf("\t%s\n", evento);
	}
	fclose(file);
	free(nombre);
}
