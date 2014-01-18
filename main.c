/*
 *	Detección de gestos con WiiMote
 *  Práctica de NPI realizada por:
 *      -  Daniel Pascual Pantoja
 *      -  Fernando Palacios López
 *
 *  Haciendo uso de la biblioteca wiiuse escrita por:
 *      -  Michael Laforest  -  thepara@gmail.com  -
 *		-  Página web:	http://public.vrac.iastate.edu/~rpavlik/doxygen/wiiuse-fork/
 *		-  Biblioteca: http://sourceforge.net/projects/wiiuse/
 */

#include <stdio.h>
#include <stdlib.h>
#include "wiiuse.h"


#ifndef WIN32
	#include <unistd.h>
	#define clear() system("clear");
#else
     #define clear() system("cls");
#endif

//Macro para definir el numero maximo de mandos
#define MAX_WIIMOTES    1
//Macro para definir el umbral de sensibilidad
#define SENSITIVITY     2
//Capacidad del vector de fuerzas
#define capacidad		30
//Vector de fuerzas
gforce_t lista_gforce[capacidad];
//Vector de orientaciones
orient_t lista_orientacion[capacidad];
//Vector de dos componentes para los movimientos compuestos
int compuesto[2];
int pos=0;
int posCompuesto=0;
// Función que detiene la ejecución del proceso por un tiempo t.
void sleep_ex(int t)
{
    #ifndef WIN32
		usleep(t*1000);
	#else
		Sleep(t);
	#endif
}
//Devuelve el valor absoluto de un flotante
float absFloat(float num){
	if (num<0)
		return -num;
	return num;
}

//Funcion que se encarga de obtener el indice del valor mas extremo de una lista de fuerzas
int gForceMaxAbs(gforce_t *lista, orient_t *orientacion){
	float maximo_x = absFloat(lista[0].x);
	float maximo_y = absFloat(lista[0].y);
	float maximo_z = absFloat(lista[0].z);
	int i, indice_x, indice_z;
	int indiceMax=-1;
	indice_x=indice_z=0;
	for(i=1; i<capacidad; ++i){
		if (maximo_x<absFloat(lista[i].x)){
			maximo_x=absFloat(lista[i].x);
			indice_x=i;
		}
	}
	for(i=1; i<capacidad; ++i){
		if (maximo_z<absFloat(lista[i].z)){
			maximo_z=absFloat(lista[i].z);
			indice_z=i;
		}
	}
	if (maximo_x>maximo_y && maximo_x>maximo_z)
		indiceMax = indice_x;
	else if (maximo_z>maximo_y && maximo_z>maximo_x)
		indiceMax = indice_z;
	return indiceMax;
}
// Manejador de movimientos
/*Devuelve:
- 0: si el movimiento es hacia la izquierda.
- 1: si el movimiento es hacia la derecha.
- 2: si el movimiento es hacia arriba.
- 3: si el movimiento es hacia abajo.
Para realizar movimientos de derecha e izquierda es necesario rotar ligeramente el mando sobre
el eje y (roll) hacia el sentido positivo del eje x y realizar un movimiento similar al que se realiza para "arriba" y para "abajo".
Se ha optado por esta metodologia debido a que se ha realizado sin Sensor Bar y el yaw, o rotación sobre el eje z, 
no funciona sin ella
Para diferenciar entre los movimientos "arriba" y "abajo" se tiene en cuenta la rotación del mando cuando se realiza el movimiento sobre
el eje x (pitch).
http://www.osculator.net/doc/_media/faq:pry-wiimote.gif
*/
int handle_movement(int unid, gforce_t gforce, orient_t orient)
{
	//Solo aceptamos el movimiento si los valores de los acelerometros superan el umbral de sensibilidad
	if (absFloat(gforce.z)>=SENSITIVITY && orient.roll<-60 && orient.pitch<100)
		return 0;
	else if (absFloat(gforce.z)>=SENSITIVITY && orient.roll<-60 && orient.pitch>=100)
		return 1;
	else if (absFloat(gforce.z)>=SENSITIVITY && orient.pitch<100)
		return 2;
	else if (absFloat(gforce.z)>=SENSITIVITY && orient.pitch>=100)
		return 3;
}

// Manejador de eventos
void handle_event(struct wiimote_t* wm)
{
    // Imprime un botón si ha sido pulsado
    if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_A))		printf("A pressed\n");
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_B))		printf("B pressed\n");
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_UP))		printf("UP pressed\n");
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_DOWN))	printf("DOWN pressed\n");
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_LEFT))	printf("LEFT pressed\n");
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_RIGHT))	printf("RIGHT pressed\n");
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_MINUS))	printf("MINUS pressed\n");
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_PLUS))	printf("PLUS pressed\n");
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_ONE))		printf("ONE pressed\n");
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_TWO))		printf("TWO pressed\n");
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_HOME))	printf("HOME pressed\n");

    // Botón MINUS: Desactiva el acelerómetro
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_MINUS))
		wiiuse_motion_sensing(wm, 0);

	// Botón PLUS: Activa el acelerómetro
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_PLUS)){
		wiiuse_motion_sensing(wm, 1);
		wiiuse_set_ir(wm, 1);
	}
	// Acelerómetro usado
	if (WIIUSE_USING_ACC(wm)) {
		/*El acelerómetro toma siempre demasiados valores en el transcurso del movimiento
		por ese motivo se toman un número determinado de muetras y se usa el valor más extremo*/
		if (pos<capacidad){
			lista_gforce[pos] = wm->gforce;
			lista_orientacion[pos] = wm->orient;
			pos++;
		}
		else{
			int indiceF=gForceMaxAbs(lista_gforce, lista_orientacion);
			//Obtenemos la fuerza de valor mas extremo y su respectiva orientacion
			gforce_t fuerza = lista_gforce[indiceF];
			orient_t orientacion = lista_orientacion[indiceF];
			if (indiceF!=-1){
					int mov=handle_movement(wm->unid,fuerza, orientacion);
					if (mov==0)
						printf("Izquierda\n");
					else if (mov==1)
						printf("Derecha\n");
					else if (mov==2)
						printf("Arriba\n");
					else if (mov==3)
						printf("Abajo\n");
					//Si está presionado el botón B, se activa el modo gesto compuesto
					/*Permite unir dos gestos para detectar los gestos que se muestran en el 
					/pdf del guion de prácticas*/
					if (IS_PRESSED(wm, WIIMOTE_BUTTON_B)){
						compuesto[posCompuesto] = mov;
						posCompuesto++;
						if (posCompuesto==2){
							if (compuesto[0]==3 && compuesto[1]==1)
								printf("Gesto compuesto 1: Abajo-Derecha\n");
							else if (compuesto[0]==3 && compuesto[1]==0)
								printf("Gesto compuesto: Abajo-Izquierda\n");
							else if (compuesto[0]==2 && compuesto[1]==0)
								printf("Gesto compuesto: Arriba-Izquierda\n");
							else if (compuesto[0]==2 && compuesto[1]==1)
								printf("Gesto compuesto: Arriba-Derecha\n");
							else if (compuesto[0]==0 && compuesto[1]==2)
								printf("Gesto compuesto: Izquierda-Arriba\n");
							else if (compuesto[0]==1 && compuesto[1]==2)
								printf("Gesto compuesto: Derecha-Arriba\n");
							else if (compuesto[0]==0 && compuesto[1]==3)
								printf("Gesto compuesto: Izquierda-Abajo\n");
							else if (compuesto[0]==1 && compuesto[1]==3)
								printf("Gesto compuesto: Derecha-Abajo\n");
							posCompuesto=0;
						}
					}
					else posCompuesto = 0;
			}
			pos=0;
		}
	}

}


// Función que maneja una desconexión. Imprime el mando que se ha desconectado
void handle_disconnect(wiimote* wm)
{
	printf("\n\n--- DISCONNECTED [wiimote id %i] ---\n", wm->unid);
	wiiuse_disconnect(wm);
}


int main()
{
    wiimote** wiimotes;
    int found, connected;
    int end = 0;
    int i;
    // Inicializa array de objetos wiimote
    wiimotes =  wiiuse_init(MAX_WIIMOTES);

    // Busca wiimotes conectados al equipo
    found = wiiuse_find(wiimotes, MAX_WIIMOTES, 5);
	if (!found) {
		printf ("No wiimotes found.");
		return 0;
	}
    // Establece la conexión con los wiimotes encontrados
 	connected = wiiuse_connect(wiimotes, MAX_WIIMOTES);
	if (connected)
		printf("Connected to %i wiimotes (of %i found).\n", connected, found);
	else {
		printf("Failed to connect to any wiimote.\n");
		return 0;
	}

	// Asigna un led encendido a cada mando
    wiiuse_set_leds(wiimotes[0], WIIMOTE_LED_1);
	/*wiiuse_set_leds(wiimotes[1], WIIMOTE_LED_2);
	wiiuse_set_leds(wiimotes[2], WIIMOTE_LED_3);
	wiiuse_set_leds(wiimotes[3], WIIMOTE_LED_4);*/
	wiiuse_rumble(wiimotes[0], 1);

    sleep_ex(200);
	//El mando conectado emite una vibracion
	wiiuse_rumble(wiimotes[0], 0);


	while(!end)
	{
        // wiiuse_poll() busca eventos que se hayan producido en los mandos.
        // Devuelve un flag si ha ocurrido algún evento en alguno de los mandos.
        if (wiiuse_poll(wiimotes, MAX_WIIMOTES)) {

            // Comprueba eventos para cada mando
			for (i=0; i < MAX_WIIMOTES; ++i) {

                // Switch con los eventos
                switch (wiimotes[i]->event) {
                    // Evento genérico
                    case WIIUSE_EVENT:
						handle_event(wiimotes[i]);

						// Si pulsa botón HOME, sale del bucle para terminar.
                        if (IS_PRESSED(wiimotes[i], WIIMOTE_BUTTON_HOME)) end = 1;
						break;

                    // Se desconecta algún mando
                    case WIIUSE_DISCONNECT:
					case WIIUSE_UNEXPECTED_DISCONNECT:
					    handle_disconnect(wiimotes[i]);

						// Si no hay ningún mando conectado, sale del bucle principal
						if(wiiuse_connect(wiimotes, MAX_WIIMOTES) == 0) end = 1;
						break;

                    // Cambia el estado del mando
                    case WIIUSE_STATUS:
                         //Muestra el estado de la bateria del mando
                            printf("Battery in WiiMote n# %i: %f\n",i,wiimotes[i]->battery_level);
                         break;

					// El resto de eventos se han puesto para evitar warnings en la compilación
					// de esta versión. Pueden ser usado igual que los eventos anteriores.
					case WIIUSE_NONE:
					case WIIUSE_CONNECT:
                    case WIIUSE_READ_DATA:
					case WIIUSE_NUNCHUK_INSERTED:
					case WIIUSE_CLASSIC_CTRL_INSERTED:
					case WIIUSE_GUITAR_HERO_3_CTRL_INSERTED:
					case WIIUSE_NUNCHUK_REMOVED:
					case WIIUSE_CLASSIC_CTRL_REMOVED:
					case WIIUSE_GUITAR_HERO_3_CTRL_REMOVED:
                        break;
                }
			}
        }

        // Duerme el sistema para que no tome demasiadas muestras
        sleep_ex(20);
	}

	// Desconecta los mandos
	wiiuse_cleanup(wiimotes, MAX_WIIMOTES);

	return 0;
}
