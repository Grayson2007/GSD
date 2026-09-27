#include <gsdstd/rbtree.h>
#include <gsdstd/stdmem.h>
#define LEFT 0
#define RIGHT 1
static void rotate(rbt* root,rbtnode* node,int dir) {
        rbtnode* newparent = node->children[1-dir];
        rbtnode* newchild = newparent->children[dir];
        node->children[1-dir]  = newchild;
        if(newchild) { newchild->parent = node;}
        newparent->children[dir] = node;
        newparent->parent = node->parent;
        node->parent = newparent;
        newchild->parent = node;
}


status rbt_find(rbt* root,ulong key,rbtnode** outref) {

        rbtnode* curr = root->root;
        while(curr->key != key) {
                if(curr->key > key) {
                        if(!curr->left) { return gsd_not_present;}
                        curr = curr->left;
                        continue;
                }

                if(!curr->right) { return gsd_not_present;}
                curr = curr->right;
        }
        *outref = curr;
        return gsd_ok;
}
status rbt_insert(rbt* root,ulong key,handle ref)
{
        rbtnode* curr = root->root;
        while(curr->key != key) { // find the corrisponding parent for this new node 
                if(curr->key > key) {
                        if(!curr->left) { break;}
                        curr = curr->left;
                        continue;
                }

                if(!curr->right) { break;}
                curr = curr->right;
        }
        if(curr->key == key) { curr->ref = ref; return gsd_ok;} // if the key exists update the reference and return 
        rbtnode* new = gsdcalloc(sizeof(rbtnode),1); // Otherwise we will allocate a new node using gsdcalloc
        new->color = RED;
        new->parent = curr;
        if(curr->key > key) {
                curr->left = new;
        }
        else {
                curr->right = new;
        }
        // fix the insert 
        new->ref = ref;
        new->key = key;

        curr = new;
        while(curr->parent->color == RED) {
                
                int uncledir;
                rbtnode* uncle;
                int currdir;
                if(curr->parent->left == curr) {
                        uncledir = RIGHT;
                        uncle = curr->parent->right;
                        currdir = LEFT;
                }
                else 
                {
                        uncledir = LEFT;
                        uncle = curr->parent->left;
                        currdir = RIGHT;
                }

                if(uncle)
                {
                        if(uncle->color == BLACK) { goto black_uncle;}
                        curr->parent->color = BLACK;
                        uncle->color = BLACK;
                        curr = curr->parent->parent;
                        continue;
                }
                black_uncle:
                int parentdir = curr->parent == curr->parent->parent->left ? LEFT : RIGHT;
                if(currdir != parentdir) {
                        rotate(root,curr->parent,1-currdir);
                }
                // line case
                curr->parent->color = BLACK;
                curr->parent->parent->color = RED;
                rotate(root,curr->parent->parent,1-parentdir);
                curr = curr->parent->parent;
                continue;
        }
        if(!curr->parent) {
                root->root = curr;
                curr->color = BLACK;
        }
        return gsd_ok;
}
status rbt_delete(rbt* root,ulong key,handle* outref)
{
        rbtnode* target;
        status s = rbt_find(root,key,&target);
        if(s != gsd_ok) { return s;}
        

        
}