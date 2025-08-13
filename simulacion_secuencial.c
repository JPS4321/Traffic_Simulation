#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
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

void imprimirSemaforos(Interseccion* inter, char* buffer, size_t* offset) {
    const char* estados[] = {"ROJO", "AMARILLO", "VERDE"};
    for (int i = 0; i < NUM_SEMAFOROS; i++) {
        if (*offset < BUFFER_SIZE - 1) {
            *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset,
                                "Semaforo %d: %s\n", i + 1, estados[inter->semaforos[i].estado]);
        }
    }
}

void moverVehiculos(Interseccion* inter, char* buffer, size_t* offset) {
    for (int i = 0; i < NUM_SEMAFOROS; i++) {
        if (inter->semaforos[i].estado == 2) { 
            int mover = (inter->calles[i].cantidad_carros < VEHICULOS_POR_CICLO)
                        ? inter->calles[i].cantidad_carros
                        : VEHICULOS_POR_CICLO;
            for (int k = 0; k < mover; k++) {
                Vehiculo* v = inter->calles[i].cola[0];
                if (*offset < BUFFER_SIZE - 1) {
                    *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset,
                                        "Vehiculo %d cruzo desde calle %d\n", v->id, i + 1);
                }
                free(v);
                for (int m = 1; m < inter->calles[i].cantidad_carros; m++) {
                    inter->calles[i].cola[m - 1] = inter->calles[i].cola[m];
                }
                inter->calles[i].cantidad_carros--;
            }
        }
    }
}

void imprimirCalles(Interseccion* inter, char* buffer, size_t* offset) {
    for (int i = 0; i < NUM_SEMAFOROS; i++) {
        if (*offset < BUFFER_SIZE - 1) {
            *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset, "Calle %d:", i + 1);
        }
        for (int j = 0; j < inter->calles[i].cantidad_carros; j++) {
            if (*offset < BUFFER_SIZE - 1) {
                *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset, " %d", inter->calles[i].cola[j]->id);
            }
        }
        if (*offset < BUFFER_SIZE - 1) {
            *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset, "\n");
        }
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

void simularInterseccionSecuencial(Interseccion* inter, int num_ciclos) {
    char* buffer = malloc(BUFFER_SIZE);
    for (int ciclo = 1; ciclo <= num_ciclos && contarVehiculos(inter) > 0; ciclo++) {
        size_t offset = 0;
        offset += snprintf(buffer + offset, BUFFER_SIZE - offset, "\n--- Ciclo %d ---\n", ciclo);

        actualizarSemaforos(inter, ciclo);
        imprimirSemaforos(inter, buffer, &offset);
        moverVehiculos(inter, buffer, &offset);
        imprimirCalles(inter, buffer, &offset);

        printf("%s", buffer);
    }
    free(buffer);
}

int main() {
    srand(time(NULL));
    Interseccion inter;
    inicializarInterseccion(&inter);

    double start_time = (double)clock() / CLOCKS_PER_SEC;
    simularInterseccionSecuencial(&inter, 200);
    double end_time = (double)clock() / CLOCKS_PER_SEC;

    printf("\nTiempo total de simulacion: %.3f segundos\n", end_time - start_time);
    return 0;
}
