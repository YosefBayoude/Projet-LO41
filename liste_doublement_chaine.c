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
Element *listeCreer() {
    Element *element = (Element*) malloc(sizeof(Element));
    element->suivant = NULL;
    element->precedent = NULL;
    return element;
}

Element* listeNouvelElement(Processus* p){
    Element *element = (Element*) malloc(sizeof(Element));
    element->suivant = NULL;
    element->precedent = NULL;
    element->data = p;
    return element;
}

/* Get the head of the list. */
Element *listeValeurTete(Element *liste) {
    if (liste == NULL) return NULL;

    Element *tete = liste;
    Element *prev = tete->precedent;
    while (prev) {
        tete = prev;
        prev = tete->precedent;
    }
    return tete;
}

/* Get the tail of the list. */
Element *listeValeurQueue(Element *liste) {
    if (liste == NULL) return NULL;
    
    Element *queue = liste;
    Element *suivant = queue->suivant;
    while (suivant) {
        queue = suivant;
        suivant = queue->suivant;
    }
    return queue;
}

/* Free the memory of a node, including its data contents if applicable. */
void listeLibererElement(Element *node) {
    /*if (node->data)
        free(node->data);*/
    free(node);
}

/* Free the memory of an entire list. */
void listeLibererListe(Element *node) {
    Element *curr = listeValeurTete(node);
    Element *next = NULL;
    while (curr) {
        next = curr->suivant;
        listeLibererElement(curr);
        curr = next;
    }
}

/* Insert a node on to the head of the list. Returns a pointer to the added
 * node.
 */
Element *listeAjouterTete(Element *liste, Element *nouvel_element) {
    if (!liste){
        liste = nouvel_element;
        return liste;
    } else {
        Element *tete = listeValeurTete(liste);
        assert (tete);
        tete->precedent = nouvel_element;
        nouvel_element->suivant = tete;
        return nouvel_element;
    }
}

/* Insert a node on to the tail of the list. Returns a pointer to the added
 * node.
 */
Element *listeAjouterQueue(Element *liste, Element *nouvel_element) {
    if (!liste){
        liste = nouvel_element;
        return liste;
    } else {
        Element *queue = listeValeurQueue(liste);
        assert (queue);
        queue->suivant = nouvel_element;
        nouvel_element->precedent = queue;
        return listeValeurTete(nouvel_element);
    }
}


Element* listeSupprimerTete(Element *liste){
    if (liste == NULL){
        return NULL;
    }
    liste = listeValeurTete(liste);
    if (liste->suivant){
        Element* nouvelle_liste = liste->suivant;
        nouvelle_liste->precedent = NULL;
        listeLibererElement(liste);
        return nouvelle_liste;
    } else {
        return NULL;
    }
} 

void printListeProcessus(Element *node){
    Element *curr = listeValeurTete(node);
    int count = 0;
    while (curr) {
        printf("Priorite: %d : ", node->data->priorite);
        if (curr->data) {
            Processus *p = (Processus *)curr->data;
            printf("P: %d, tps exec: %d; ", p->mon_pid, p->temps_exec);
        }
        curr = curr->suivant;
        count++;
    }
    printf("\n");
    //printf(" =>  %d elements\n", count);

}

#endif