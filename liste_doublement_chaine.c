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



/***************************************************************************/

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

int listeEstVide(Element* liste){
    return liste;
}

/* Get the head of the list. */
Element *listeValeurTete(Element *liste) {
    if (liste){
        Element *tete = liste;
        Element *prev = tete->precedent;
        while (prev) {
            tete = prev;
            prev = tete->precedent;
        }
        return tete;
    }
    return liste;
}

/* Get the tail of the list. */
Element *listeValeurQueue(Element *node) {
    Element *tail = node;
    Element *next = tail->suivant;
    while (next) {
        tail = next;
        next = tail->suivant;
    }
    return tail;
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
Element *listeAjouterQueue(Element *node, Element *node_add) {
    

        Element *tail = listeValeurQueue(node);
        assert (tail);
        tail->suivant = node_add;
        node_add->precedent = tail;
    return listeValeurTete(node_add);
}

/* Insert a node before the specified node. Returns a pointer to the added
 * node.
 */
Element *listeAjouterAvant(Element *node, Element *node_add) {
    assert (node && node_add);
    Element *prev = node->precedent;
    if (prev)
        prev->suivant = node_add;
    node_add->precedent = prev;
    node_add->suivant = node;
    node->precedent = node_add;
    return node_add;
}

/* Insert a node after the specified node. Returns a pointer to the added
 * node.
 */
Element *listeAjouterApres(Element *node, Element *node_add) {
    assert (node && node_add);
    Element *next = node->suivant;
    if (next)
        next->precedent = node_add;
    node_add->precedent = node;
    node_add->suivant = next;
    node->suivant = node_add;
    return node_add;
}

/* Find a node in the list with the specified data. Returns a pointer to the
 * node if found, otherwise NULL. */
Element *listeTrouver(Element *node, Processus *data) {
    Element *curr = listeValeurTete(node);
    while (curr) {
        if (curr->data == data)
            return curr;
        curr = curr->suivant;
    }
    return NULL;
}

/* Delete a node from the list containing the data. Returns 1 on success,
 * or 0 if not found.
 */
Element* listeSupprimer(Element *node, Processus *data) {
    Element * element = listeTrouver(node, data);
    if (element){
        Element * precedent = element->precedent;
        Element * suivant = element->suivant;
        Element* tete;
        if(element->precedent){
            printf("ici\n");
            element->precedent->suivant = element->suivant;
        }
        if(element->suivant){
            element->suivant->precedent = element->precedent;
        }
        if (suivant){
            tete = listeValeurTete(suivant);
        } else if (precedent){
            tete = listeValeurTete(precedent);
        }
        listeLibererElement(element);
        return tete;
    }
    return 1;

    /*Element *curr = listeValeurTete(node);
    while (curr) {
        if (curr->data) {
            if (curr->data == data) {
                Element *next = curr->suivant;
                Element *prev = curr->precedent;
                prev->suivant = next;
                next->precedent = prev;
                listeLibererElement(curr);
                return 1;
            }
        }
    }
    return 0;*/
}

Element* listeSupprimerTete(Element *liste){
    liste = listeValeurTete(liste);
    if (liste->suivant){
        Element* nouvelle_liste = liste->suivant;
        liste->suivant->precedent = NULL;
        listeLibererElement(liste);
        return nouvelle_liste;
    }
} 

void printListeProcessus(Element *node){
    Element *curr = listeValeurTete(node);
    int count = 0;
    printf("Liste de processus : \n");
    while (curr) {
        if (curr->data) {
            Processus *p = (Processus *)curr->data;
            printf("Processus: %ld; prio: %d\n", p->type, p->priorite);
        }
        curr = curr->suivant;
        count++;
    }
    printf("Il y a %d elements\n", count);

}

/***************************************************************************/

/***************************************************************************
 * Helper Functions 
 ***************************************************************************/

/* Iterate the list from the head treating each node as a person, printing its
 * contents.
 */
/*
void print_person_list(Element *node) {
    Element *curr = listeValeurTete(node);
    while (curr) {
        if (curr->data) {
            person_t *person = (person_t *)curr->data;
            printf("name: %s; age: %d\n", person->name, person->age);
        }
        curr = curr->suivant;
    }
}

// Create a person with a name and age, returning a pointer to the person. 
person_t *create_person(char *name, int age) {
    person_t *person = malloc(sizeof(person_t));
    person->name = name;
    person->age = age;
    return person;
}

// Find a person by name in a list of persons, returning a pointer to the person. 
person_t *find_person(Element *node, char *name) {
    Element *curr = listeValeurTete(node);
    while (curr) {
        person_t *person = (person_t *)curr->data;
        if (strcmp(person->name, name) == 0)
            return person;
        curr = curr->suivant;
    }
    return NULL;
}
*/
/***************************************************************************/

/***************************************************************************
 * Test Functions (INCOMPLETE)
 ***************************************************************************/

/*
Element *test_list() {
    Element *head = listeCreer();
    head->data = create_person("John Meikle", 23);

    Element *node1 = listeCreer();
    node1->data = create_person("Bob Dole", 32);
    listeAjouterQueue(head, node1);

    Element *node2 = listeCreer();
    node2->data = create_person("John Smith", 56);
    listeAjouterQueue(head, node2);

    return head;
}

void test_list_node_init() {
    Element *node = listeCreer();
    assert (node);
    listeLibererElement(node);
}

void test_list_get_head() {
    Element *head = test_list();
    assert (listeValeurTete(head) == head);
    listeLibererListe(head);
}

void test_list_get_tail() {
    Element *head = test_list();
    // ...
    listeLibererListe(head);
}

// ...

void test_all() {
    test_list_node_init();
    test_list_get_head();
    test_list_get_tail();
}*/

/***************************************************************************/

/*
int main(void) {
    Element *head = listeCreer();
    head->data = create_person("John Meikle", 23);

    Element *node1 = listeCreer();
    node1->data = create_person("Bob Dole", 32);
    listeAjouterQueue(head, node1);

    Element *new_head = listeCreer();
    new_head->data = create_person("John Smith", 56);
    list_add_head(head, new_head);

    print_person_list(head);
    assert(find_person(head, "John Meikle"));
    listeLibererListe(head);
    return 0;
}*/

#endif