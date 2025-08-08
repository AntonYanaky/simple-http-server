#ifndef FRAMEWORK_H_
#define FRAMEWORK_H_

#include <stdlib.h>
#include <string.h>

#define CLASSES_CHARS 4
#define TAG_CHARS 5
#define REDUNDENCY_CHARS 2

typedef struct attribute {
    char *key;
    char *value;
} attribute;

typedef struct node {
    char* tag;
    char* content;
    attribute *attributes;
    int attributeCount;
    //<tag class="classes flex flex 1" href="whatever">content</tag>

    int childCount;
    struct node** children;
} node;

struct node *createNode(const char *tag, const char* content, attribute *attributes, int attributeCount);

int createChild(struct node *parent, struct node *child);

char *renderNodes(struct node *parent);

void freeNodes(struct node *parent);

int calculateSize(struct node *parent);

void renderNodesHelper(struct node *parent, char **cursor);

struct node *createNode(const char *tag, const char* content, attribute *attributes, int attributeCount) {
    struct node *newNode; 
    if (!(newNode = calloc(1, sizeof(node)))) return NULL;

    if (tag == NULL || *tag == '\0') {
        goto failure;
    }
    newNode->tag = strdup(tag);
    if (newNode->tag == NULL) {
        goto failure;
    }

    if (content!=NULL) {
        newNode->content = strdup(content);
        if (newNode->content == NULL) {
            goto failure;
        }
    }

    newNode->attributeCount = 0;

    if (attributeCount > 0) {
        newNode->attributes = calloc(attributeCount, sizeof(attribute));
        if (newNode->attributes == NULL) goto failure;
        newNode->attributeCount = attributeCount;
        for (int i = 0; i < attributeCount; i++) {
            newNode->attributes[i].key = strdup(attributes[i].key);
            if (newNode->attributes[i].key == NULL ) goto failure;
            newNode->attributes[i].value = strdup(attributes[i].value);
            if (newNode->attributes[i].value == NULL ) goto failure;
        }
    }

    return newNode;

    failure:
    free(newNode->tag);
    free(newNode->content);
    for (int i = 0; i < newNode->attributeCount; i++) {
        free(newNode->attributes[i].key);
        free(newNode->attributes[i].value);
    }
    free(newNode->attributes);
    free(newNode);
    return NULL;
}

int createChild(struct node *parent, struct node *child) {
    if (parent == NULL || child == NULL) return -1;
    
    struct node **temp;
    if (!(temp = (struct node **) realloc(parent->children, sizeof(*parent->children) * (parent->childCount+1)))) return -1;
    parent->children = temp;

    parent->childCount++;
    parent->children[parent->childCount-1] = child;
    return 1;
}

void freeNodes(struct node *parent){
    if (parent == NULL) return;
    for (; 0 < parent->childCount; parent->childCount--){
        freeNodes(parent->children[parent->childCount - 1]);
    }

    for (int i = 0; i < parent->attributeCount; i++) {
        free(parent->attributes[i].key);
        free(parent->attributes[i].value);
    }
    free(parent->attributes);
    free(parent->tag);
    free(parent->content);
    free(parent->children);
    free(parent);
}

int calculateSize(struct node *parent) {
    if (parent==NULL) return 0;

    // <></>
    int size = TAG_CHARS;

    // <tag></tag>
    if (parent->tag != NULL && *parent->tag != '\0') {
        size += strlen(parent->tag) * 2;
    }

    // <tag key="value" key="value"></>
    for (int i = 0; i < parent->attributeCount; i++) {
        size += strlen(parent->attributes[i].key) + strlen(parent->attributes[i].value) + CLASSES_CHARS;
    }

    if (parent->content != NULL && *parent->content != '\0') {
        size += strlen(parent->content);
    }

    for (int i = 0; i < parent->childCount; i++) {
        size += calculateSize(parent->children[i]);
    }

    return size + REDUNDENCY_CHARS;
}

void renderNodesHelper(struct node *parent, char **cursor) {
    if (parent == NULL || cursor == NULL) return;

    *cursor += sprintf(*cursor, "<%s", parent->tag);

    for (int i = 0; i < parent->attributeCount; i++) { 
        *cursor += sprintf(*cursor, " %s=\"%s\"",parent->attributes[i].key, parent->attributes[i].value);
    }
    *cursor += sprintf(*cursor, ">");

    if (parent->content != NULL && *parent->content != '\0') {
        *cursor += sprintf(*cursor, "%s", parent->content);
    }

    for (int i = 0; i < parent->childCount; i++){
        renderNodesHelper(parent->children[i], cursor);
    }

    *cursor += sprintf(*cursor, "</%s>", parent->tag);
}

char *renderNodes(struct node *parent) {
    // <tag classes="classes">content{children}</tag>
    if (parent == NULL) return NULL;
    
    char *buffer;
    if (!(buffer = malloc(calculateSize(parent) + 1))) return NULL;

    char *cursor = buffer;

    renderNodesHelper(parent, &cursor);

    *(cursor) = '\0';
    return buffer;
}

#endif