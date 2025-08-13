#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <omp.h>
#include <unistd.h>

#define NUM_SEMAFOROS 14
#define DURACION_CAMBIO 2
#define CARROS_POR_CALLE 3000
#define VEHICULOS_POR_CICLO 50
#define BUFFER_SIZE 2000000

typedef struct {
    int id;
} Vehiculo;

typedef struct {
    Vehiculo** cola;
    int cantidad_carros;
} Calle;

typedef struct {
    int estado; 
} Semaforo;

typedef struct {
    Calle calles[NUM_SEMAFOROS];
    Semaforo semaforos[NUM_SEMAFOROS];
} Interseccion;

void inicializarInterseccion(Interseccion* inter) {
    for (int i = 0; i < NUM_SEMAFOROS; i++) {
        inter->calles[i].cantidad_carros = CARROS_POR_CALLE;
        inter->calles[i].cola = malloc(CARROS_POR_CALLE * sizeof(Vehiculo*));
        for (int j = 0; j < CARROS_POR_CALLE; j++) {
            Vehiculo* v = malloc(sizeof(Vehiculo));
            v->id = i * 100 + j;
            inter->calles[i].cola[j] = v;
        }
        inter->semaforos[i].estado = rand() % 3;
    }
}

void imprimirSemaforosBuffer(Interseccion* inter, char* buffer, size_t* offset) {
    const char* estados[] = {"ROJO", "AMARILLO", "VERDE"};
    for (int i = 0; i < NUM_SEMAFOROS; i++) {
        *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset,
                            "Semaforo %d: %s\n", i + 1, estados[inter->semaforos[i].estado]);
    }
}

void moverVehiculosBuffer(Interseccion* inter, char* buffer, size_t* offset) {
    for (int i = 0; i < NUM_SEMAFOROS; i++) {
        int mover = 0;
        if (inter->semaforos[i].estado == 2) { 
            mover = (inter->calles[i].cantidad_carros < VEHICULOS_POR_CICLO) ?
                    inter->calles[i].cantidad_carros : VEHICULOS_POR_CICLO;
        }
        for (int k = 0; k < mover; k++) {
            Vehiculo* v = inter->calles[i].cola[0];
            *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset,
                                "Vehiculo %d cruzo desde calle %d\n", v->id, i + 1);
            free(v);
            for (int m = 1; m < inter->calles[i].cantidad_carros; m++) {
                inter->calles[i].cola[m - 1] = inter->calles[i].cola[m];
            }
            inter->calles[i].cantidad_carros--;
        }
    }
}

void imprimirCallesBuffer(Interseccion* inter, char* buffer, size_t* offset) {
    for (int i = 0; i < NUM_SEMAFOROS; i++) {
        *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset, "Calle %d:", i + 1);
        for (int j = 0; j < inter->calles[i].cantidad_carros; j++) {
            *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset, " %d", inter->calles[i].cola[j]->id);
        }
        *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset, "\n");
    }
}

void actualizarSemaforos(Interseccion* inter, int ciclo) {
    if (ciclo % DURACION_CAMBIO == 0) {
        for (int i = 0; i < NUM_SEMAFOROS; i++) {
            inter->semaforos[i].estado = (inter->semaforos[i].estado + 1) % 3;
        }
    }
}

int contarVehiculos(Interseccion* inter) {
    int total = 0;
    for (int i = 0; i < NUM_SEMAFOROS; i++) {
        total += inter->calles[i].cantidad_carros;
    }
    return total;
}

void simularInterseccionDinamica(Interseccion* inter, int num_ciclos) {
    omp_set_dynamic(1);

    for (int ciclo = 1; ciclo <= num_ciclos && contarVehiculos(inter) > 0; ciclo++) {
        printf("\n=== CICLO %d ===\n", ciclo);

        char* bufferSemaforos = malloc(BUFFER_SIZE);
        char* bufferMovimientos = malloc(BUFFER_SIZE);
        char* bufferCalles = malloc(BUFFER_SIZE);
        size_t offsetSemaforos = 0, offsetMovimientos = 0, offsetCalles = 0;

        int num_vehiculos = contarVehiculos(inter);
        int num_threads = num_vehiculos / 10 + 1;
        omp_set_num_threads(num_threads);

        #pragma omp parallel sections
        {
            #pragma omp section
            {
                actualizarSemaforos(inter, ciclo);
                imprimirSemaforosBuffer(inter, bufferSemaforos, &offsetSemaforos);
            }

            #pragma omp section
            {
                moverVehiculosBuffer(inter, bufferMovimientos, &offsetMovimientos);
            }

            #pragma omp section
            {
                imprimirCallesBuffer(inter, bufferCalles, &offsetCalles);
            }
        }

        
        printf("%s", bufferSemaforos);
        printf("%s", bufferMovimientos);
        printf("%s", bufferCalles);

        free(bufferSemaforos);
        free(bufferMovimientos);
        free(bufferCalles);
    }
}

int main() {
    srand(time(NULL));
    Interseccion inter;
    inicializarInterseccion(&inter);

    double start_time = omp_get_wtime();
    simularInterseccionDinamica(&inter, 200);
    double end_time = omp_get_wtime();

    printf("\nTiempo total de simulacion: %.3f segundos\n", end_time - start_time);
    return 0;
}
