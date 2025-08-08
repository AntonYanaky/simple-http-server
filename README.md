# Simple C HTML Framework

A lightweight, dependency-free C library for programmatically building and rendering HTML documents. This framework provides a simple tree-based node structure, allowing you to create complex HTML layouts dynamically on the server before sending them to a client.

It's designed for performance and control, making it ideal for building fast static site generators, custom web server backends, or any application where HTML needs to be generated from C code.
Features

- Node-Based Structure: Represent the entire HTML document as a tree of node objects.

- Dynamic Content: Create elements with any tag and text content.

- Attribute Support: Easily add any number of HTML attributes (like class, id, or href) to your nodes.

- Hierarchical Nesting: Build complex layouts by adding child nodes to parent nodes.

- Memory Safe: Includes a simple function to recursively free the entire node tree, preventing memory leaks.

- String Rendering: Convert the entire node tree into a single, valid HTML string ready to be sent over a network.

## Getting Started

To use the framework, simply include the framework.h header file in your project.

```c
#include "framework.h"
```
You will need to compile the framework's C source file along with your main application code.

## API Reference

The framework exposes a simple API for creating, managing, and rendering nodes.
```c
struct node *createNode(const char *tag, const char* content, attribute *attributes, int attributeCount);
```
Creates a new HTML node. This is the fundamental building block of the framework.

- tag: The HTML tag for the element (e.g., "div", "p", "a").

- content: The text content inside the element. Can be NULL for empty elements.

- attributes: An array of attribute structs. Use a compound literal for easy inline creation. Can be NULL if there are no attributes.

- attributeCount: The number of attributes in the array.

Example:

```c
// Create a paragraph node with no attributes
node *p = createNode("p", "This is some text.", NULL, 0);

// Create a link node with two attributes
node *link = createNode(
    "a",
    "Click Here",
    (attribute[]){
        {.key = "href", .value = "/about"},
        {.key = "class", .value = "text-blue-500"}
    },
    2
);
```
```c
int createChild(struct node *parent, struct node *child);
```

Appends a child node to a parent node. This is how you build the HTML tree structure.

- parent: The node to which the child will be added.

- child: The node to be added.

Example:

```c
node *container = createNode("div", "", NULL, 0);
node *heading = createNode("h1", "Welcome", NULL, 0);

// Add the heading as a child of the container div
createChild(container, heading);
```
```c
char *renderNodes(struct node *parent);
```
Takes a parent node and recursively renders it and all of its children into a single, dynamically allocated HTML string.

- parent: The root node of the tree (or sub-tree) you want to render.

- Returns: A char* containing the full HTML string. This string must be freed by the caller after use.

Example:

```c
// Assume 'container' is the root node of your page
char *html_output = renderNodes(container);
if (html_output) {
    printf("%s\n", html_output);
    // ... send the string over a network, etc. ...
    free(html_output); // IMPORTANT: Free the rendered string
}
```
```c
void freeNodes(struct node *parent);
```
Recursively frees a parent node and all of its children from memory. This is crucial for preventing memory leaks.

- parent: The root node of the tree you want to deallocate.

Example:

```c
// Build your page structure
node *page = createNode("div", "", NULL, 0);
// ... add children to page ...

// Render it
char *html = renderNodes(page);
// ... use the html ...
free(html);

// Now, free the entire node tree
freeNodes(page);
```
## Full Example

Here is how to build a simple HTML card component.

```c
#include "framework.h"
#include <stdio.h>

int main() {
    node *card = createNode("div", "", (attribute[]){
        {.key = "class", .value = "card bg-white p-4 shadow-md"}
    }, 1);

    node *title = createNode("h2", "Framework Card", (attribute[]){
        {.key = "class", .value = "text-2xl font-bold"}
    }, 1);

    node *content = createNode("p", "This card was generated using the C HTML framework.", (attribute[]){
        {.key = "class", .value = "mt-2 text-gray-700"}
    }, 1);

    createChild(card, title);
    createChild(card, content);

    char* final_html = renderNodes(card);
    printf("--- Generated HTML ---\n%s\n", final_html);

    free(final_html);
    freeNodes(card);

    return 0;
}
```