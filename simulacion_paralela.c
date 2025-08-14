#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <omp.h>
#include <unistd.h>

#define NUM_SEMAFOROS 4
#define DURACION_CAMBIO 3
#define CARROS_POR_CALLE 2000
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

/**
 * Inicializa la intersección con calles llenas de vehículos
 * y semáforos en estados aleatorios.
 */
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

/**
 * Imprime en el buffer el estado actual de todos los semáforos.
 */
void imprimirSemaforosBuffer(Interseccion* inter, char* buffer, size_t* offset) {
    const char* estados[] = {"ROJO", "AMARILLO", "VERDE"};
    for (int i = 0; i < NUM_SEMAFOROS; i++) {
        *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset,
                            "Semaforo %d: %s\n", i + 1, estados[inter->semaforos[i].estado]);
    }
}

/**
 * Mueve los vehículos que pueden cruzar en este ciclo.
 * Paralelizado por calles para que varios hilos procesen diferentes calles a la vez.
 */
void moverVehiculosBuffer(Interseccion* inter, char* buffer, size_t* offset) {
    #pragma omp for schedule(static)
    for (int i = 0; i < NUM_SEMAFOROS; i++) {
        int mover = 0;
        if (inter->semaforos[i].estado == 2) { // Solo verde
            mover = (inter->calles[i].cantidad_carros < VEHICULOS_POR_CICLO) ?
                    inter->calles[i].cantidad_carros : VEHICULOS_POR_CICLO;
        }
        for (int k = 0; k < mover; k++) {
            Vehiculo* v = inter->calles[i].cola[0];

            #pragma omp critical(buffer_write)
            {
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

/**
 * Imprime en el buffer el listado de vehículos que quedan en cada calle.
 * Paralelizado por calles para evitar cuellos de botella.
 */
void imprimirCallesBuffer(Interseccion* inter, char* buffer, size_t* offset) {
    #pragma omp for schedule(static)
    for (int i = 0; i < NUM_SEMAFOROS; i++) {
        size_t local_offset = 0;
        char temp[BUFFER_SIZE];

        local_offset += snprintf(temp + local_offset, BUFFER_SIZE - local_offset, "Calle %d:", i + 1);
        for (int j = 0; j < inter->calles[i].cantidad_carros; j++) {
            local_offset += snprintf(temp + local_offset, BUFFER_SIZE - local_offset, " %d", inter->calles[i].cola[j]->id);
        }
        local_offset += snprintf(temp + local_offset, BUFFER_SIZE - local_offset, "\n");

        #pragma omp critical(buffer_write)
        {
            *offset += snprintf(buffer + *offset, BUFFER_SIZE - *offset, "%s", temp);
        }
    }
}

/**
 * Cambia el estado de todos los semáforos cada cierto número de ciclos.
 */
void actualizarSemaforos(Interseccion* inter, int ciclo) {
    if (ciclo % DURACION_CAMBIO == 0) {
        for (int i = 0; i < NUM_SEMAFOROS; i++) {
            inter->semaforos[i].estado = (inter->semaforos[i].estado + 1) % 3;
        }
    }
}

/**
 * Cuenta el total de vehículos en toda la intersección.
 */
int contarVehiculos(Interseccion* inter) {
    int total = 0;
    for (int i = 0; i < NUM_SEMAFOROS; i++) {
        total += inter->calles[i].cantidad_carros;
    }
    return total;
}

/**
 * Simula el funcionamiento de la intersección por un número de ciclos.
 * Usa 3 hilos fijos y divide las tareas en tres fases:
 *   Actualización e impresión de semáforos.
 *   Movimiento de vehículos.
 *   Impresión del estado de las calles.
 * 
 * Los buffers se limpian en cada ciclo para evitar overflow.
 */
void simularInterseccion(Interseccion* inter, int num_ciclos) {
    omp_set_dynamic(0); // No dinámico
    omp_set_num_threads(3); // Fijo a 3 hilos

    for (int ciclo = 1; ciclo <= num_ciclos && contarVehiculos(inter) > 0; ciclo++) {
        printf("\nCICLO %d\n", ciclo);

        char* bufferSemaforos = malloc(BUFFER_SIZE);
        char* bufferMovimientos = malloc(BUFFER_SIZE);
        char* bufferCalles = malloc(BUFFER_SIZE);

        // Limpiar buffers
        bufferSemaforos[0] = '\0';
        bufferMovimientos[0] = '\0';
        bufferCalles[0] = '\0';

        size_t offsetSemaforos = 0, offsetMovimientos = 0, offsetCalles = 0;

        #pragma omp parallel
        {
            #pragma omp single
            {
                actualizarSemaforos(inter, ciclo);
                imprimirSemaforosBuffer(inter, bufferSemaforos, &offsetSemaforos);
            }
        }

        #pragma omp parallel
        {
            moverVehiculosBuffer(inter, bufferMovimientos, &offsetMovimientos);
        }

        #pragma omp parallel
        {
            imprimirCallesBuffer(inter, bufferCalles, &offsetCalles);
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
    simularInterseccion(&inter, 2000);
    double end_time = omp_get_wtime();

    printf("\nTiempo total de simulacion: %.3f segundos\n", end_time - start_time);
    return 0;
}
