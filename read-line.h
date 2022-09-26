#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

typedef struct TrieNode {
    struct TrieNode * children[256];
    bool isEnd;
} TrieNode;

struct TrieNode * getNode() {
    TrieNode * pNode = (TrieNode *) malloc(sizeof(TrieNode));
    pNode->isEnd = false;
    for (int i = 0; i < 256; i++) {
        pNode->children[i] = NULL;
    }
    return pNode;
}

void insert(TrieNode * root, char * key) {
    TrieNode * cur = root;
    for (int i = 0; i < strlen(key); i++) {
        int index = key[i] - 0;
        if (!cur->children[index]) {
            cur->children[index] = getNode();
        }
        cur = cur->children[index];
    }
    cur->isEnd = true;
}

bool isLastNode(TrieNode * root) {
    for (int i = 0; i < 256; i++) {
        if (root->children[i]) return false;
    }
    return true;
}

bool hasOneChild(TrieNode * root) {
    int cnt = 0;
    for (int i = 0; i < 256; i++) {
        if (root->children[i]) cnt++;
    }
    return cnt == 1;
}

void tab(TrieNode * root, char * pref, char * arg, int cur, int i) {
    if (i < strlen(arg) && root->children[arg[i]]) {
        tab(root->children[arg[i]], pref, arg, cur, i + 1);          
    } else {
        if (!hasOneChild(root)) {
            return;    
        }
        for (int i = 0; i < 256; i++) {
            if (root->children[i]) {
                char c = 0 + i;
                pref[cur] = c;
                tab(root->children[i], pref, arg, cur + 1, i);
            }
        }
    }
}
