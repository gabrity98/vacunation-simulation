#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>

// Jiajie Ni
// Gabriel Sánchez Losa

#define FABRICAS 3
#define CENTROS 5
#define TANDAS 10
#define POBLACION_MAX 4800
#define TANDA_MAX 480

void parametrosPorDefecto(int (*));
void estadisticasFinales(int [FABRICAS][1+CENTROS], int [CENTROS][3]);

pthread_mutex_t mutexF[FABRICAS];
pthread_mutex_t mutexC[CENTROS];
pthread_mutex_t mutexP[CENTROS];
pthread_mutex_t mutexEstF[FABRICAS][1+CENTROS], mutexEstC[CENTROS][3];

int vacunasFabricas[FABRICAS];
int vacunasCentros[CENTROS];
int genteEnEspera[CENTROS];
int parametros[9];	//nº de habitantes, tiempo de fabricación de vacunas, etc.
int estadisticasF[FABRICAS][1+CENTROS];
int estadisticasC[CENTROS][3];

FILE * writeFile;

void parametrosPorDefecto(int *para) {

	para[0] = 1200;
	para[1] = 15;
	para[2] = 25;
	para[3] = 50;
        para[4] = 20;
	para[5] = 40;
        para[6] = 3;
	para[7] = 4;
        para[8] = 2;
}

void estadisticasFinales(int estF[FABRICAS][1+CENTROS], int estC[CENTROS][3]) {

	int i,j;

	printf("\nESTADÍSTICAS FINALES:\n");
	fprintf(writeFile,"\nESTADÍSTICAS FINALES:\n");
	for (i=0; i<FABRICAS; i++) {
		printf("\nFÁBRICA %i:\n",i+1);
		fprintf(writeFile,"\nFÁBRICA %i:\n",i+1);
		printf("%i vacunas fabricadas en total.\n",estF[i][0]);
		fprintf(writeFile,"%i vacunas fabricadas en total.\n",estF[i][0]);
		for (j=1; j<=CENTROS; j++) {
			printf("%i vacunas entregadas en el centro %i\n",estF[i][j],j);
			fprintf(writeFile,"%i vacunas entregadas en el centro %i\n",estF[i][j],j);
		}
	}
	for (i=0; i<CENTROS; i++) {
		printf("\nCENTRO %i:\n",i+1);
		fprintf(writeFile,"\nCENTRO %i:\n",i+1);
		printf("%i vacunas recibidas en total\n",estC[i][0]);
		fprintf(writeFile,"%i vacunas recibidas en total\n",estC[i][0]);
		printf("%i personas vacunadas\n",estC[i][1]);
                fprintf(writeFile,"%i personas vacunadas\n",estC[i][1]);
		printf("%i vacunas restantes\n",estC[i][2]);
                fprintf(writeFile,"%i vacunas restantes\n",estC[i][2]);
	}
}

void *fabricacionVacunas(void *arg) {

	int idF;
	int vacunas_min, vacunas_max, tiempo_min, tiempo_max;
	int v_random, t_random;
	int i, aux, suma, resto, entrega;
	int entregas[CENTROS];
	int demanda;

	idF = *((int*)arg);
	demanda = parametros[0]/FABRICAS;

	while (demanda > 0) {

		vacunas_min = parametros[2];
		vacunas_max = parametros[3];
		tiempo_min = parametros[4];
		tiempo_max = parametros[5];

		v_random = rand() % (vacunas_max - vacunas_min + 1) + vacunas_min;
		t_random = rand() % (tiempo_max - tiempo_min + 1) + tiempo_min;

		sleep(t_random);

		pthread_mutex_lock(&mutexF[idF]);
		vacunasFabricas[idF] = vacunasFabricas[idF] + v_random;
		pthread_mutex_unlock(&mutexF[idF]);

		demanda = demanda - v_random;
		pthread_mutex_lock(&mutexEstF[idF][0]);
		estadisticasF[idF][0] = estadisticasF[idF][0] + v_random;
		pthread_mutex_unlock(&mutexEstF[idF][0]);

		printf("Fábrica %i prepara %i vacunas\n",idF+1,vacunasFabricas[idF]);
		fprintf(writeFile, "Fábrica %i prepara %i vacunas\n",idF+1,vacunasFabricas[idF]);

		for (i=0; i<CENTROS; i++) {
			entregas[i] = 0;
		}

		tiempo_min = 1;
		tiempo_max = parametros[6];

		t_random = rand() % (tiempo_max - tiempo_min + 1) + tiempo_min;

		sleep(t_random);

		for (i=0; i<CENTROS; i++) {	//Manda vacunas a los centros que tengan gente en espera
			if (genteEnEspera[i] > vacunasCentros[i]) {
				suma = genteEnEspera[i] - vacunasCentros[i];

				if (suma > vacunasFabricas[idF])
					suma = vacunasFabricas[idF];

				pthread_mutex_lock(&mutexC[i]);
				vacunasCentros[i] = vacunasCentros[i] + suma;
				pthread_mutex_unlock(&mutexC[i]);

				pthread_mutex_lock(&mutexF[idF]);
				vacunasFabricas[idF] = vacunasFabricas[idF] - suma;
				pthread_mutex_unlock(&mutexF[idF]);

				pthread_mutex_lock(&mutexEstF[idF][i+1]);
				estadisticasF[idF][i+1] = estadisticasF[idF][i+1] + suma;
				pthread_mutex_unlock(&mutexEstF[idF][i+1]);

				entregas[i] = entregas[i] + suma;

				if (vacunasFabricas[idF] <= 0)
					break;
			}
		}

		aux = vacunasCentros[0];	//Comprueba que centro tiene más vacunas para igualar el resto
		for (i=1; i<CENTROS; i++) {
			if (aux < vacunasCentros[i])
				aux = vacunasCentros[i];
		}
		if (vacunasFabricas[idF] > 0) {
			for (i=0; i<CENTROS; i++) {
				suma = aux - vacunasCentros[i];

				if (suma > vacunasFabricas[idF])
					suma = vacunasFabricas[idF];

				pthread_mutex_lock(&mutexC[i]);
				vacunasCentros[i] = vacunasCentros[i] + suma;
				pthread_mutex_unlock(&mutexC[i]);

				pthread_mutex_lock(&mutexF[i]);
				vacunasFabricas[idF] = vacunasFabricas[idF] - suma;
				pthread_mutex_unlock(&mutexF[i]);

				pthread_mutex_lock(&mutexEstF[idF][i+1]);
                                estadisticasF[idF][i+1] = estadisticasF[idF][i+1] + suma;
                                pthread_mutex_unlock(&mutexEstF[idF][i+1]);

				entregas[i] = entregas[i] + suma;

				if (vacunasFabricas[idF] <= 0)
					break;
			}
		}

		suma = vacunasFabricas[idF]/CENTROS;
		resto = vacunasFabricas[idF]%CENTROS;
		entrega = suma;

		for (i=0; i<CENTROS; i++) {	//Reparte el resto de vacunas equitativamente
			pthread_mutex_lock(&mutexC[i]);
			pthread_mutex_lock(&mutexF[idF]);
			pthread_mutex_lock(&mutexEstF[idF][i+1]);
			vacunasCentros[i] = vacunasCentros[i] + suma;
			vacunasFabricas[idF] = vacunasFabricas[idF] - suma;
			estadisticasF[idF][i+1] = estadisticasF[idF][i+1] + suma;
			if (resto > 0) {
				vacunasCentros[i]++;
				vacunasFabricas[idF]--;
				resto--;
				estadisticasF[idF][i+1]++;
				entrega++;
			}
			pthread_mutex_unlock(&mutexC[i]);
			pthread_mutex_unlock(&mutexF[idF]);
			pthread_mutex_unlock(&mutexEstF[idF][i+1]);

			entregas[i] = entregas[i] + entrega;
			entrega = suma;
		}

		for (i=0; i<CENTROS; i++) {
			printf("Fábrica %i entrega %i vacunas en el centro %i\n",idF+1,entregas[i],i+1);
			fprintf(writeFile, "Fábrica %i entrega %i vacunas en el centro %i\n",idF+1,entregas[i],i+1);
		}
	}
	pthread_exit(NULL);
}

void *Vacunacion(void *arg){

	int min, max;
	int t_random, centro_random;
	int idP;

	idP = *((int*)arg);

	min = 1;
        max = parametros[7];
	t_random = rand() % (max - min +1) + min;	//para sleep

	sleep(t_random);

	min = 0;
	max = 4;
	centro_random = rand() % (max - min +1) + min;	//para elegir un centro

	printf("Habitante %i elige el centro %i para vacunarse\n", idP+1, centro_random+1);
	fprintf(writeFile,"Habitante %i elige el centro %i para vacunarse\n", idP+1, centro_random+1);

	min = 1;
	max = parametros[8];
	t_random= rand() % (max - min +1) + min;	//lo que tarda en llegar al centro

	sleep(t_random);

	if (vacunasCentros[centro_random]<=0) {
		pthread_mutex_lock(&mutexP[centro_random]);
		genteEnEspera[centro_random]++;
		pthread_mutex_unlock(&mutexP[centro_random]);
		while(1) {
			if(vacunasCentros[centro_random]>0){
				pthread_mutex_lock(&mutexP[centro_random]);
				genteEnEspera[centro_random]--;
				pthread_mutex_unlock(&mutexP[centro_random]);
				break;
			}
		}
	}
	pthread_mutex_lock(&mutexC[centro_random]);
	vacunasCentros[centro_random]--;
	pthread_mutex_unlock(&mutexC[centro_random]);

	pthread_mutex_lock(&mutexEstC[centro_random][1]);
	estadisticasC[centro_random][1]++;
	pthread_mutex_unlock(&mutexEstC[centro_random][1]);

	printf("Habitante %i vacunado en el centro %i\n", idP+1, centro_random+1);
	fprintf(writeFile,"Habitante %i vacunado en el centro %i\n", idP+1, centro_random+1);

	pthread_exit(NULL);

}

int main(int argc, char **argv) {

	FILE * readFile;
	int tandaP;
	int tandaListaP[TANDAS];
	int i, j, resto, contadorP;
	int *idP;
	int idF[FABRICAS];
	pthread_t threadF[FABRICAS], *threadP;


//ENTRADA/SALIDA

	if (argc == 1) {
		writeFile = fopen("salida_vacunacion.txt", "w");
		if (writeFile == NULL) {
			fprintf(stderr, "Error en la creación de fichero de redirección de salida: %s\n", strerror(errno));
			return 1;
		}
		readFile = fopen("entrada_vacunacion.txt", "r");
		if (readFile == NULL) {
                        fprintf(stderr, "Error en la apertura del fichero de redirección de entrada: %s\n", strerror(errno));
                        printf("Cargando los parámetros de entrada por defecto...\n");
			parametrosPorDefecto(parametros);
                }
		else {
                	for (i=0; i<9; i++) {
                        	fscanf(readFile,"%d",&parametros[i]);
                	}
                	fclose(readFile);
		}
	}
	else if (argc == 2) {
		writeFile = fopen("salida_vacunacion.txt", "w");
                if (writeFile == NULL) {
                        fprintf(stderr, "Error en la creación de fichero de redirección de salida: %s\n", strerror(errno));
                        return 1;
                }
		readFile = fopen(argv[1], "r");
		if (readFile == NULL) {
                        fprintf(stderr, "Error en la apertura del fichero de redirección de entrada: %s\n", strerror(errno));
			printf("Cargando los parámetros de entrada por defecto...\n");
                        parametrosPorDefecto(parametros);
                }
		else {
			for (i=0; i<9; i++) {
				fscanf(readFile,"%d",&parametros[i]);
			}
			fclose(readFile);
		}
	}
	else if (argc == 3) {
		writeFile = fopen(argv[2], "w");
                if (writeFile == NULL) {
                        fprintf(stderr, "Error en la creación de fichero de redirección de salida: %s\n", strerror(errno));
                        return 1;
                }
                readFile = fopen(argv[1], "r");
                if (readFile == NULL) {
                        fprintf(stderr, "Error en la creación de fichero de redirección de entrada: %s\n", strerror(errno));
                        printf("Cargando los parámetros de entrada por defecto...\n");
                        parametrosPorDefecto(parametros);
                }
		else {
			for (i=0; i<9; i++) {
                        	fscanf(readFile,"%d",&parametros[i]);
                	}
                	fclose(readFile);
		}
	}
	else {
		fprintf(stderr, "Error. Demasiados argumentos\n");
		return 1;
	}


	srand(time(NULL));	//Cambia la semilla de rand() según la hora actual (si no sale siempre lo mismo)

	threadP = (pthread_t *) malloc (TANDA_MAX*sizeof(pthread_t));		//Reservamos memoria para el array de threads de la población
	idP = (int *) malloc (POBLACION_MAX*sizeof(int));			//Reservamos memoria para los identificadores

	for (i=0; i<parametros[0]; i++) {	//Inicializamos el array de personas
		idP[i] = i;
	}

	for (i=0; i<CENTROS; i++) {
		genteEnEspera[i] = 0;
	}

	for (i=0; i<FABRICAS; i++) {	//Inicializamos el array vacunasFabricas y creamos los mutex de las fábricas
		vacunasFabricas[i] = 0;
		pthread_mutex_init(&mutexF[i], NULL);
	}

	for (i=0; i<CENTROS; i++) {	//Inicializamos el array vacunasCentros y creamos los mutex de los centros
		vacunasCentros[i] = parametros[1];
		pthread_mutex_init(&mutexC[i], NULL);
		pthread_mutex_init(&mutexP[i], NULL);
	}

	for (i=0; i<FABRICAS; i++) {	//Inicializamos el array de estadísticas de las fábricas y creamos sus mutex
		for (j=0; j<1+CENTROS; j++) {
			estadisticasF[i][j] = 0;
			pthread_mutex_init(&mutexEstF[i][j], NULL);
		}
	}

	for (i=0; i<CENTROS; i++) {	//Inicializamos el array de estadísticas de los centros y creamos sus mutex
		for (j=0; j<3; j++) {
			estadisticasC[i][j] = 0;
			pthread_mutex_init(&mutexEstC[i][j], NULL);
		}
	}

	tandaP = parametros[0]/TANDAS;

	printf("\n");
	printf("VACUNACIÓN EN PANDEMIA: CONFIGURACIÓN INICIAL:\n");
	fprintf(writeFile,"VACUNACIÓN EN PANDEMIA: CONFIGURACIÓN INICIAL:\n");
	printf("Habitantes: %i\n", parametros[0]);
	fprintf(writeFile,"Habitantes: %i\n", parametros[0]);
	printf("Centros de vacunación: %i\n", CENTROS);
        fprintf(writeFile,"Centros de vacunación: %i\n", CENTROS);
	printf("Fábricas: %i\n", FABRICAS);
        fprintf(writeFile,"Fábricas: %i\n", FABRICAS);
	printf("Vacunados por tanda: %i\n", tandaP);
        fprintf(writeFile,"Vacunados por tanda: %i\n", tandaP);
	printf("Vacunas iniciales en cada centro: %i\n", parametros[1]);
        fprintf(writeFile,"Vacunas iniciales en cada centro: %i\n", parametros[1]);
	printf("Vacunas totales por fábrica: %i\n", parametros[0]/FABRICAS);
        fprintf(writeFile,"Vacunas totales por fábrica: %i\n", parametros[0]/FABRICAS);
	printf("Mínimo número de vacunas fabricadas en cada tanda %i\n",parametros[2])	;
	fprintf(writeFile,"Mínimo número de vacunas fabricadas en cada tanda: %i\n",parametros[2]);
	printf("Máximo número de vacunas fabricadas en cada tanda: %i\n",parametros[3]);
	fprintf(writeFile,"Máximo número de vacunas fabricadas en cada tanda: %i\n", parametros[3]);
	printf("Tiempo mínimo de fabricación  de una tanda de vacunas: %i\n", parametros[4]);
	fprintf(writeFile,"Tiempo mínimo de fabricación de una tanda de vacunas: %i\n",parametros[4]);
	printf("Tiempo máximo de fabricación de una tanda de vacunas: %i\n", parametros[5]);
	fprintf(writeFile,"Tiempo máximo de fabricación de una tanda de vacunas: %i\n", parametros[5]);
	printf("Tiempo máximo de reparto de vacunas a los centros: %i\n",parametros[6]);
	fprintf(writeFile,"Tiempo máximo de reparto de vacunas a los centros: %i\n",parametros[6]);
	printf("Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %i\n",parametros[7]);
	fprintf(writeFile,"Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %i\n", parametros[7]);
	printf("Tiempo máximo de desplazamiento del habitante al centro de vacunación: %i\n", parametros[8]);
	fprintf(writeFile,"Tiempo máximo de desplazamiento del habitante al centro de vacunación: %i\n", parametros[8]);
	printf("\nPROCESO DE VACUNACIÓN\n");
	fprintf(writeFile,"\nPROCESO DE VACUNACIÓN\n");
	printf("\n");
	fprintf(writeFile,"\n");

	for(i=0; i<TANDAS; i++){
		tandaListaP[i] = tandaP;
	}

	resto = parametros[0]%TANDAS;

	if(resto>0) {
		for(i=0; i<TANDAS; i++){
			tandaListaP[i]++;
			resto--;
			if(resto<=0)
				break;
		}
	}

	for (i=0; i<FABRICAS; i++) {    //Creamos los threads de las fábricas
        	idF[i] = i;
                if (pthread_create(&threadF[i], NULL, fabricacionVacunas, (void*)&idF[i]) != 0) {
			fprintf(stderr, "Error en la creación de threads: %s\n", strerror(errno));
			return 1;
		}
        }

	contadorP = 0;
	for(i=0; i<TANDAS; i++) {

		for(j=0; j<tandaListaP[i]; j++){	//Creamos los threads de los habitantes
			if (pthread_create(&threadP[j], NULL, Vacunacion, (void*)&idP[contadorP]) != 0) {
				fprintf(stderr, "Error en la creación de threads: %s\n", strerror(errno));
                        	return 1;
			}
			contadorP++;
		}

		for (j=0; j<tandaListaP[i]; j++) {
                	pthread_join(threadP[j], NULL);
        	}
	}

	for (i=0; i<FABRICAS; i++) {	//Esperamos que terminen los threads de fábricas
                pthread_join(threadF[i], NULL);
        }
	printf("Vacunación finalizada.\n");
	fprintf(writeFile,"Vacunación finalizada.\n");

	for (i=0; i<CENTROS; i++) {
		estadisticasC[i][0] = estadisticasC[i][1] + vacunasCentros[i] - 15;
		estadisticasC[i][2] = vacunasCentros[i];
	}

	estadisticasFinales(estadisticasF,estadisticasC);

	free(idP);	//Liberamos memoria
	free(threadP);

	for (i=0; i<FABRICAS; i++) {	//Destruimos los mutex
		pthread_mutex_destroy(&mutexF[i]);
		for (j=0; j<1+CENTROS; j++) {
			pthread_mutex_destroy(&mutexEstF[i][j]);
		}
	}
	for (i=0; i<CENTROS; i++) {
		pthread_mutex_destroy(&mutexC[i]);
		for (j=0; j<3; j++) {
			pthread_mutex_destroy(&mutexEstC[i][j]);
		}
	}
	fclose(writeFile);	//Cerramos el archivo de escritura
	return 0;
}
