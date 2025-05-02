#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define NB_BUS_X 5
#define NB_BUS_Y 4
#define NB_TRAJETS 10
#define XtoY 0
#define YtoX 1

typedef struct {
    int id;
    int ville; // 0 pour X, 1 pour Y
} Bus;

// Sémaphores et variables de synchronisation
sem_t mutex;
sem_t tunnel;
int nbBusDansTunnel = 0;
int direction = -1; // -1 = vide, 0 = X->Y, 1 = Y->X
int attenteX = 0, attenteY = 0;

// Entrée dans le tunnel
void entrerTunnel(int ville) {
    sem_wait(&mutex);
    if (direction == -1 || direction == ville) {
        nbBusDansTunnel++;
        direction = ville;
        sem_post(&mutex);
    } else {
        if (ville == XtoY) attenteX++;
        else attenteY++;
        sem_post(&mutex);
        sem_wait(&tunnel); // attente jusqu’à son tour
    }
}

// Sortie du tunnel
void sortirTunnel(int ville) {
    sem_wait(&mutex);
    nbBusDansTunnel--;
    if (nbBusDansTunnel == 0) {
        int autre = 1 - ville;
        if ((ville == XtoY && attenteY > 0) || (ville == YtoX && attenteX > 0)) {
            direction = autre;
            int* attente = (ville == XtoY) ? &attenteY : &attenteX;
            for (int i = 0; i < *attente; i++) sem_post(&tunnel);
            *attente = 0;
        } else {
            direction = -1;
        }
    }
    sem_post(&mutex);
}

// Fonction exécutée par chaque thread de bus
void* routineBus(void* arg) {
    Bus* bus = (Bus*) arg;
    int villeDepart = bus->ville;
    int villeArrivee = 1 - villeDepart;
    char* nomVille = villeDepart == XtoY ? "X" : "Y";
    char* nomArrivee = villeArrivee == XtoY ? "X" : "Y";

    for (int i = 1; i <= NB_TRAJETS; i++) {
        // TRAJET ALLER
        entrerTunnel(villeDepart);
        printf("[ENTRÉE  ] Bus %d de %s entre dans le tunnel pour aller vers %s (Trajet %d)\n", bus->id, nomVille, nomArrivee, i);
        printf("[TRAJET  ] Bus %d : %s -> %s (Trajet %d)\n", bus->id, nomVille, nomArrivee, i);
        usleep((rand() % 501 + 1000) * 1000);
        sortirTunnel(villeDepart);
        printf("[SORTIE  ] Bus %d de %s sort du tunnel vers %s (Trajet %d)\n\n", bus->id, nomVille, nomArrivee, i);

        // TRAJET RETOUR
        entrerTunnel(villeArrivee);
        printf("[ENTRÉE  ] Bus %d de %s entre dans le tunnel pour aller vers %s (Trajet %d)\n", bus->id, nomVille, nomVille, i);
        printf("[TRAJET  ] Bus %d : %s -> %s (Trajet %d)\n", bus->id, nomArrivee, nomVille, i);
        usleep((rand() % 501 + 1000) * 1000);
        sortirTunnel(villeArrivee);
        printf("[SORTIE  ] Bus %d de %s sort du tunnel vers %s (Trajet %d)\n\n", bus->id, nomVille, nomVille, i);
    }

    free(bus);
    pthread_exit(NULL);
}

// Fonction principale
int main() {
    srand(time(NULL));
    pthread_t threads[NB_BUS_X + NB_BUS_Y];
    sem_init(&mutex, 0, 1);
    sem_init(&tunnel, 0, 0);
    int idx = 0;

    // Créer les threads pour les bus de la ville X
    for (int i = 0; i < NB_BUS_X; i++) {
        Bus* b = malloc(sizeof(Bus));
        b->id = i + 1;
        b->ville = XtoY;
        pthread_create(&threads[idx++], NULL, routineBus, b);
    }

    // Créer les threads pour les bus de la ville Y
    for (int i = 0; i < NB_BUS_Y; i++) {
        Bus* b = malloc(sizeof(Bus));
        b->id = i + 1;
        b->ville = YtoX;
        pthread_create(&threads[idx++], NULL, routineBus, b);
    }

    // Attente de la fin de tous les threads
    for (int i = 0; i < NB_BUS_X + NB_BUS_Y; i++) {
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&mutex);
    sem_destroy(&tunnel);
    return 0;
}
