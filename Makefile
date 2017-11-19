C=gcc -o
SG= soporteGeneral.c
central:
	$(C) cen central.c $(SG) soporte.c 
celular:
	$(C) cel celular.c soporteCelular.c $(SG)
cargador:
	$(C) car cargador.c soporteCargador.c $(SG)
telefono:
	$(C) tel $(SG) telefono.c soporteTelefono.c
centralLinea:
	$(C) cenL $(SG) soporteCentralLinea.c centralLinea.c	
limpiar:
	clear
log:
	rm *.txt

all: central celular cargador telefono centralLinea
	
clean:
	rm cen
	rm cel
	rm car
	rm tel
	rm cenL
	rm *.txt
