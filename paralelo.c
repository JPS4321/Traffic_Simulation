#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <omp.h>

#define TOTAL_INTERSECCIONES 4
#define DURACION_VERDE 2
#define CARROS_POR_CALLE 1000
#define VEHICULOS_POR_CICLO 3
#define BUFFER_SIZE 65536  

typedef struct {
    int id;
} Vehiculo;

typedef struct {
    Vehiculo** cola;
    int cantidad_carros;
    int capacidad;
} Calle;

typedef struct {
    int estado; 
} Semaforo;

typedef struct {
    Calle calles[4];
    Semaforo semaforos[4];
    int id;
} Interseccion;

void inicializarInterseccion(Interseccion* interseccion, int id_interseccion) {
    interseccion->id = id_interseccion;
    for (int i = 0; i < 4; i++) {
        interseccion->calles[i].cantidad_carros = 0;
        interseccion->calles[i].capacidad = CARROS_POR_CALLE;
        interseccion->calles[i].cola = malloc(CARROS_POR_CALLE * sizeof(Vehiculo*));
        for (int j = 0; j < CARROS_POR_CALLE; j++) {
            Vehiculo* v = malloc(sizeof(Vehiculo));
            v->id = i * 100 + j;
            interseccion->calles[i].cola[j] = v;
            interseccion->calles[i].cantidad_carros++;
        }
    }
    for (int i = 0; i < 4; i++) {
        interseccion->semaforos[i].estado = 0;
    }
    interseccion->semaforos[0].estado = 1;
    interseccion->semaforos[2].estado = 1;
}

void imprimirSemaforosBuffer(Interseccion* interseccion, char* buffer, size_t* offset) {
    const char* nombres[4] = {"Norte", "Este", "Sur", "Oeste"};
    const char* estados[2] = {"ROJO", "VERDE"};
    *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset,
                        "\n[Interseccion %d] ESTADO DE SEMAFOROS:\n", interseccion->id);
    for (int i = 0; i < 4; i++) {
        *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset,
                            " Calle %s: %s\n", nombres[i], estados[interseccion->semaforos[i].estado]);
    }
}

void imprimirCallesBuffer(Interseccion* interseccion, char* buffer, size_t* offset) {
    const char* nombres[4] = {"Norte", "Este", "Sur", "Oeste"};
    *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset,
                        "\n[Interseccion %d] VEHICULOS EN CADA CALLE:\n", interseccion->id);
    for (int i = 0; i < 4; i++) {
        *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset,
                            " Calle %s (%d):", nombres[i], interseccion->calles[i].cantidad_carros);
        for (int j = 0; j < interseccion->calles[i].cantidad_carros; j++) {
            Vehiculo* v = interseccion->calles[i].cola[j];
            *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset, " [%d]", v->id);
        }
        *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset, "\n");
    }
}

void avanzarSemaforos(Interseccion* interseccion, int* par_actual) {
    for (int i = 0; i < 4; i++) {
        interseccion->semaforos[i].estado = 0;
    }
    if (*par_actual == 0) {
        interseccion->semaforos[1].estado = 1;
        interseccion->semaforos[3].estado = 1;
        *par_actual = 1;
    } else {
        interseccion->semaforos[0].estado = 1;
        interseccion->semaforos[2].estado = 1;
        *par_actual = 0;
    }
}

void moverVehiculosBuffer(Interseccion* interseccion, int calle, char* buffer, size_t* offset) {
    Calle* c = &interseccion->calles[calle];
    int mover = (c->cantidad_carros < VEHICULOS_POR_CICLO) ? c->cantidad_carros : VEHICULOS_POR_CICLO;

    for (int k = 0; k < mover; k++) {
        Vehiculo* v = c->cola[0];
        *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset,
                            " [Inter %d] Vehiculo %d cruzo desde calle %d\n", interseccion->id, v->id, calle);
        free(v);
        for (int i = 1; i < c->cantidad_carros; i++) {
            c->cola[i - 1] = c->cola[i];
        }
        c->cantidad_carros--;
    }
}

int quedanVehiculos(Interseccion* interseccion) {
    for (int i = 0; i < 4; i++) {
        if (interseccion->calles[i].cantidad_carros > 0) return 1;
    }
    return 0;
}

void simularInterseccion(Interseccion* inter) {
    char buffer[BUFFER_SIZE];
    size_t offset = 0;

    int par_actual = 0;
    int tiempo_restante = DURACION_VERDE;
    int ciclo = 1;

    while (quedanVehiculos(inter)) {
        offset = 0; 
        offset += snprintf(buffer + offset, BUFFER_SIZE - offset,
                           "\nINTERSECCION %d - CICLO %d\n", inter->id, ciclo++);

        imprimirSemaforosBuffer(inter, buffer, &offset);
        imprimirCallesBuffer(inter, buffer, &offset);

        for (int i = 0; i < 4; i++) {
            if (inter->semaforos[i].estado == 1) {
                moverVehiculosBuffer(inter, i, buffer, &offset);
            }
        }

        
        printf("%s", buffer);

        tiempo_restante--;
        if (tiempo_restante == 0) {
            avanzarSemaforos(inter, &par_actual);
            tiempo_restante = DURACION_VERDE;
        }
    }
    printf("[Interseccion %d] Todos los vehiculos han cruzado.\n", inter->id);

    for (int i = 0; i < 4; i++) {
        free(inter->calles[i].cola);
    }
}

int main() {
    printf("Inicio de simulacion\n");
    srand(time(NULL));
    Interseccion intersecciones[TOTAL_INTERSECCIONES];

    for (int i = 0; i < TOTAL_INTERSECCIONES; i++) {
        inicializarInterseccion(&intersecciones[i], i);
    }

    double start_time = omp_get_wtime();

    #pragma omp parallel for
    for (int i = 0; i < TOTAL_INTERSECCIONES; i++) {
        simularInterseccion(&intersecciones[i]);
    }

    double end_time = omp_get_wtime();
    printf("\nTiempo total de simulacion: %.3f segundos\n", end_time - start_time);
    return 0;
} 