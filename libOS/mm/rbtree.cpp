#include "rbtree.h"
#include <libOS/panic.h>
#include <libOS/util.h>

namespace mm {

/* This is for debugging */
void malloc_rbtree_base::verify_tree(rbtree_node *node) {

    if (!not_null(node)) return;

    if (not_null(node->left)) {
        LIBOS_ASSERT(node->left->parent == node);
        LIBOS_ASSERT(node->left != node->right);
        verify_tree(node->left);
    }

    if (not_null(node->right)) {
        LIBOS_ASSERT(node->right->parent == node);
        LIBOS_ASSERT(node->left != node->right);
        verify_tree(node->right);
    }
}


/*
 * returns true if successful
 */
bool malloc_rbtree_base::insert_bst(rbtree_node *node) {

    rbtree_node *cur = root;
    rbtree_node *p = &sentinel;
    while (not_null(cur)) {
        p = cur;
        if (less_than(node, cur)) {
            cur = cur->left;
        } else if (less_than(cur, node)) {
            cur = cur->right;
        } else {
            libos_print("insertion failed!");
            return false;
        }
    }

    node->parent = p;
    if (p == &sentinel)
        root = node;
    else if (less_than(node, p))
        p->left = node;
    else
        p->right = node;

    node->left = &sentinel;
    node->right = &sentinel;
    node->color = RED;
    return true;
}

void malloc_rbtree_base::rotation_left(rbtree_node *p) {
    rbtree_node *b, *q;
    LIBOS_ASSERT(not_null(p));
    q = p->right;
    LIBOS_ASSERT(not_null(q));
    b = q->left;

    //p->set_right(b);
    p->right = b;
    if (not_null(b))
        b->parent = p;

    q->parent = p->parent;
    if (not_null(p->parent)) {
        if (p->parent->left == p) {
            p->parent->left = q; 
        }
        else {
            LIBOS_ASSERT(p->parent->right == p);
            p->parent->right = q;
        }
    } else {
        root = q;
    }

    //q->set_left(p);
    q->left = p;
    p->parent = q;


    //verify_tree(root);
}

void malloc_rbtree_base::rotation_right(rbtree_node *q) {
    rbtree_node *b, *p;
    LIBOS_ASSERT(not_null(q));
    p = q->left;
    LIBOS_ASSERT(not_null(p));
    b = p->right;

    //q->set_left(b);
    q->left = b;
    if (not_null(b))
        b->parent = q;

    p->parent = q->parent;
    if (not_null(q->parent)) {
        if (q->parent->left == q)
            q->parent->left = p;
        else {
            LIBOS_ASSERT(q->parent->right == q);
            q->parent->right = p;
        }
    } else {
        root = p;
    }

    //p->set_right(q);
    p->right = q;
    q->parent = p;

    //verify_tree(root);
}

/*
 * returns true if successful
 */
bool malloc_rbtree_base::__insert(rbtree_node *node) {
    node->parent = &sentinel;
    node->left = &sentinel;
    node->right = &sentinel;

    if (!insert_bst(node))
        return false;

    rbtree_node *cur = node;

    while (cur->parent->color == RED) {

        /* Case : A
         * Parent is the left child of Grandparent */
        if (cur->parent == cur->parent->parent->left) {
            rbtree_node *uncle = cur->parent->parent->right;

            /* Case : 1
             * Uncle is also red */
            if (uncle->color == RED) {
                cur->parent->parent->color = RED;
                cur->parent->color = BLACK;
                uncle->color = BLACK;
                cur = cur->parent->parent;
            }
            else {
                /* Case : 2
                 * Cur is right child of its parent */

                if (cur == cur->parent->right) {
                    cur = cur->parent;
                    rotation_left(cur);
                }

                /* Case : 3
                 * Cur is left child of its parent */
                cur->parent->color = BLACK;
                cur->parent->parent->color = RED;
                rotation_right(cur->parent->parent);
            }
        }

        /* Case : B
         * Parent is the right child of Grandparent */
        else {
            LIBOS_ASSERT(cur->parent == cur->parent->parent->right);
            rbtree_node *uncle = cur->parent->parent->left;

            /* Case: 1
             * Uncle is also red */
            if (uncle->color == RED) {
                cur->parent->parent->color = RED;
                cur->parent->color = BLACK;
                uncle->color = BLACK;
                cur = cur->parent->parent;
            }

            else {
                /* Case : 2
                 * Cur is the left child of its parent */

                if (cur == cur->parent->parent->left) {
                    cur = cur->parent;
                    rotation_right(cur);
                }

                /* Case : 3
                 * Cur is the left child of its parent */
                cur->parent->color = BLACK;
                cur->parent->parent->color = RED;
                rotation_left(cur->parent->parent);
            }

        }
    }

    this->root->color = BLACK;

    //verify_tree(root);
    return true;
}

rbtree_node *malloc_rbtree_base::__lookup(malloc_rbtree_key_base *key) {
    rbtree_node *cur = this->root;

    while (not_null(cur)) {
        if (less_than(key, cur)) {
            if (cur->left)
                cur = cur->left;
            else
                return nullptr;

            continue;
        }

        if (!less_than(cur, key)) {
            return cur;
        }

        if (cur->right)
            cur = cur->right;
        else
            return cur;
    }

    return nullptr;
}

rbtree_node *malloc_rbtree_base::__begin(rbtree_node *start) {
    rbtree_node *cur = start;

    if (!not_null(cur)) return nullptr;

    while (not_null(cur->left)) {
        cur = cur->left;
    }

    return cur;
}


rbtree_node *malloc_rbtree_base::__next(rbtree_node *node) {
    LIBOS_ASSERT(node);
    if (not_null(node->right)) return __begin(node->right); 

    while (not_null(node->parent)) {
        if (node == node->parent->left) {
            return node->parent;
        } else 
            node = node->parent;
    }
    return nullptr;
}

void malloc_rbtree_base::transplant(rbtree_node *u, rbtree_node *v) {
    if (!not_null(u->parent))
        root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;

    v->parent = u->parent;
}

void malloc_rbtree_base::delete_fixup(rbtree_node *x) {
    while (x != root && x->color == BLACK) {
        if (x == x->parent->left) {
            rbtree_node *w = x->parent->right;

            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rotation_left(x->parent);
                w = x->parent->right;
            }

            if (w->left->color == BLACK &&
                w->right->color == BLACK) {
                
                w->color = RED;
                x = x->parent;
            } else {
                if (w->right->color == BLACK) {
                    w->left->color = BLACK;
                    w->color = RED;
                    rotation_right(w);
                    w = x->parent->right;
                }

                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                rotation_left(x->parent);
                x = root;
            }
        } else {
            rbtree_node *w = x->parent->left;

            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rotation_right(x->parent);
                w = x->parent->left;
            }

            if (w->right->color == BLACK &&
                w->left->color == BLACK) {
                
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left->color == BLACK) {
                    w->right->color = BLACK;
                    w->color = RED;
                    rotation_left(w);
                    w = x->parent->left;
                }

                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                rotation_right(x->parent);
                x = root;
            }
        }
    }

    x->color = BLACK;
}

rbtree_node *malloc_rbtree_base::__erase(rbtree_node *z) {
    rbtree_node *ret = __next(z);

    rbtree_node *y = z;
    Color y_original_color = y->color;

    rbtree_node *x;
    if (!not_null(z->left)) {
        x = z->right;
        transplant(z, z->right);
    } else if (!not_null(z->right)) {
        x = z->left;
        transplant(z, z->left);
    } else {
        y = __begin(z->right);
        x = y->right;
        if (y->parent == z)
            x->parent = y;
        else {
            transplant(y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }

        transplant(z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }

    if (y_original_color == BLACK)
        delete_fixup(x);

    return ret;
}

}
