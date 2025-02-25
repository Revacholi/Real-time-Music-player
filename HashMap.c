

#include <stdio.h>
#include <stdlib.h>
#include "HashMap.h"


//typedef struct {
//	char* key;
//    int value;
//} Item;
//
//item* linear_search(item* items, size_t size, const char* key) {
//    for (size_t i=0; i<size; i++) {
//        if (strcmp(items[i].key, key) == 0) {
//            return &items[i];
//        }
//    }
//    return NULL;
//}

//struct Node newNode = {};
//Node *createNode(int key, int value) {
////    Node *newNode = (Node *)malloc(sizeof(Node));
////	newNode = {key, value, NULL};
//	newNode.value = value;
//	newNode.key = key;
//	newNode.next = NULL;
////    newNode->key = key;
////    newNode->value = value;
////    newNode->next = NULL;
//    return &newNode;
//}
//
//unsigned int hash(int key) {
//    return key % TABLE_SIZE;
//}
//
//void insertTable(Node *table[], int key, int value) {
//    unsigned int index = hash(key);
//    Node *newNode = createNode(key, value);
//    if (table[index] == NULL) {
//        table[index] = newNode;
//    } else {
//        Node *current = table[index];
//        while (current->next != NULL) {
//            current = current->next;
//        }
//        current->next = newNode;
//    }
//}
//
//int searchTable(Node *table[], int key) {
//    unsigned int index = hash(key);
//    Node *current = table[index];
//    while (current != NULL) {
//        if (current->key == key) {
//            return current->value;
//        }
//        current = current->next;
//    }
//    return -1; // not found
//}

