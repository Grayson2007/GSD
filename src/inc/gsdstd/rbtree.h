#ifndef rbtree_h
#define rbtree_h
#include "stdtype.h"

#define RED false
#define BLACK true
typedef struct s_rbtnode {
        ulong key;
        handle ref;
        struct s_rbtnode* parent;
        union {
                struct {
                        struct s_rbtnode* left;
                        struct s_rbtnode* right;
                };
                struct s_rbtnode* children[2];
        };
        bool color;
} rbtnode;

typedef struct {
        rbtnode* root;
} rbt;

status rbt_find(rbt* root,ulong key,rbtnode* outref);
status rbt_insert(rbt* root,ulong key,handle ref);
status rbt_delete(rbt* root,ulong key,handle* outref);


#endif