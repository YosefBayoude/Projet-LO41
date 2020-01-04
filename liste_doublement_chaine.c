#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "projet.h"
#ifndef PROJET
#define PROJET


/* Represents a node in a doubly-linked list. */
typedef struct element {
    struct element *precedent;
    struct element *suivant;
    Processus *data;
} Element;


/***************************************************************************
 * List Functions 
 ***************************************************************************/

/* Initialises a node in a doubly-linked list, returning a pointer to the node. */

Element* listeNouvelElement(Processus* p){
    Element *element = (Element*) malloc(sizeof(Element));
    element->suivant = NULL;
    element->precedent = NULL;
    element->data = p;
    return element;
}

/* Get the head of the list. */
Element *listeValeurTete(Element *e) {
    if (e == NULL) return NULL;

    Element *tete = e;
    Element *precedent = tete->precedent;
    while (precedent) {
        tete = precedent;
        precedent = tete->precedent;
    }
    return tete;
}

/* Get the tail of the list. */
Element *listeValeurQueue(Element *e) {
    if (e == NULL) return NULL;
    
    Element *queue = e;
    Element *suivant = queue->suivant;
    while (suivant) {
        queue = suivant;
        suivant = queue->suivant;
    }
    return queue;
}

/* Free the memory of a node, including its data contents. */
void listeLibererElement(Element *e) {
    //free(e->data);
    free(e);
}


void listeCopierElement(Element** dest, Element** source){
    *dest = *source;
    (*dest)->data = (*source)->data;
    (*dest)->precedent = (*source)->precedent;
    (*dest)->suivant = (*source)->suivant;
}

/* Insert a node on to the head of the list. Returns a pointer to the added
 * node.
 */
Element* listeAjouterTete(Element *liste, Processus* p) {
    if (!liste){
        return listeNouvelElement(p);
    } else {
        Element *tete = listeValeurTete(liste);
        Element* nouvel_element = listeNouvelElement(p);
        tete->precedent = nouvel_element;
        nouvel_element->suivant = tete;
        return nouvel_element;
    }
}

/* Insert a node on to the tail of the list. Returns a pointer to the added
 * node.
 */
Element *listeAjouterQueue(Element *liste, Processus* p) {
    if (!liste){
        return listeNouvelElement(p);
    } else {
        Element *queue = listeValeurQueue(liste);
        Element* nouvel_element = listeNouvelElement(p);
        queue->suivant = nouvel_element;
        nouvel_element->precedent = queue;
        return listeValeurTete(nouvel_element);
    }
}


Element* listeSupprimerTete(Element *liste){
    if (!liste){
        return NULL;
    } else {
        Element* tete = listeValeurTete(liste);
        if (tete->suivant){
            Element* nouvelle_tete = tete->suivant;
            nouvelle_tete->precedent = NULL;
            listeLibererElement(tete);
            return nouvelle_tete;
        } else {
            listeLibererElement(liste);
            return NULL;
        }

    }

} 

Element* listeSupprimerQueue(Element *liste){
    if (!liste){
        return NULL;
    } else {
        Element* queue = listeValeurQueue(liste);
        if (queue->precedent){
            Element* nouvelle_queue = queue->precedent;
            nouvelle_queue->suivant = NULL;
            listeLibererElement(queue);
            return listeValeurTete(liste);
        } else {
            listeLibererElement(liste);
            return NULL;
        }

    }
}

void printListeProcessus(Element *node){
    Element *curr = listeValeurTete(node);
    int count = 0;
    while (curr) {
        if (curr->data) {
            Processus *p = (Processus *)curr->data;
            printf("P: %d, tps exec: %d; ", p->mon_pid, p->temps_exec);
        }
        curr = curr->suivant;
        count++;
    }
    printf("\n");

}

#endif