#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

// local function declaration
struct list_head *merge_two_list(struct list_head *left,
                                 struct list_head *right);

struct list_head *merge_recur(struct list_head *head);

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */


/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *head = malloc(sizeof(struct list_head));
    if (!head) {
        free(head);
        return NULL;
    }
    INIT_LIST_HEAD(head);

    return head;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;
    if (list_empty(head)) {
        free(head);
        return;
    }
    element_t *c, *n;
    list_for_each_entry_safe (c, n, head, list) {
        list_del(&c->list);
        q_release_element(c);
    }
    free(head);  // q_new function has malloced it
    return;
}
/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head || !s)  // if the argument is broken
        return false;
    element_t *new_element = malloc(sizeof(element_t));
    if (!new_element) {
        return false;
    }
    new_element->value = strdup(s);
    if (!new_element->value) {  // If allocate failed
        free(new_element);
        return false;
    }
    list_add(&new_element->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    return q_insert_head(head->prev, s);
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!sp || !head || list_empty(head))
        return NULL;
    element_t *rm_element = list_entry(head->next, element_t, list);

    memcpy(sp, rm_element->value, bufsize);
    sp[bufsize - 1] = '\0';

    list_del(head->next);
    return rm_element;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *rm_element = list_last_entry(head, element_t, list);
    if (sp) {
        strncpy(sp, rm_element->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    list_del(head->prev);
    return rm_element;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int len = 0;
    struct list_head *li;

    list_for_each (li, head)
        len++;
    return len;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || list_empty(head))
        return false;
    struct list_head *n = head->next;
    for (struct list_head *p = head->prev; n->next != p && n != p;) {
        n = n->next;
        p = p->prev;
    }
    list_del(n);
    q_release_element(list_entry(n, element_t, list));
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head || list_empty(head))
        return false;
    element_t *c, *n;  // current and next element
    bool is_dup = false;
    list_for_each_entry_safe (c, n, head,
                              list) {  // current node (iterator) is allowed to
                                       // be removed from the list.
        int len;
        if (&c->list != head && &n->list != head) {
            int c_len = strlen(c->value), n_len = strlen(n->value);
            if (c_len < n_len) {
                len = c_len;
            } else {
                len = n_len;
            }
        } else {
            len = 0;
        }
        if (c->list.next != head &&
            memcmp(c->value, n->value, len) == 0)  // duplicate string detection
        {
            list_del(&c->list);  // delete node
            q_release_element(c);
            is_dup = true;
        } else if (is_dup) {
            list_del(&c->list);
            q_release_element(c);
            is_dup = false;
        }
    }

    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head || list_empty(head))
        return;
    struct list_head *n = head->next;
    while (n != head && n->next != head) {
        struct list_head *t = n;
        list_move(n, t->next);
        n = n->next;
    }
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head)) {
        return;
    }
    struct list_head *n, *s, *t;
    list_for_each_safe (n, s, head) {
        t = n->next;
        n->next = n->prev;
        n->prev = t;
    }
    t = head->next;
    head->next = head->prev;
    head->prev = t;
    return;
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    // https://leetcode.com/problems/reverse-nodes-in-k-group/
    if (!head || list_empty(head) || k <= 0)
        return;
    struct list_head *n, *s, iter, *tmp_head = head;
    int i = 0;
    INIT_LIST_HEAD(&iter);
    list_for_each_safe (n, s, head) {
        i++;
        if (i == k) {
            list_cut_position(&iter, tmp_head, n);
            q_reverse(&iter);
            list_splice_init(&iter, tmp_head);
            i = 0;
            tmp_head = s->prev;
        }
    }
    return;
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    if (!head || list_empty(head))
        return;
    // disconnect the circular structure
    head->prev->next = NULL;
    head->next = merge_recur(head->next);
    // reconnect the list (prev and circular)
    struct list_head *c = head, *n = head->next;
    while (n) {
        n->prev = c;
        c = n;
        n = n->next;
    }
    c->next = head;
    head->prev = c;
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    if (!head)
        return 0;
    struct list_head *c = head->next;
    element_t *c_ele = list_entry(c, element_t, list);
    while (c_ele->list.next != head) {
        element_t *n_ele = list_entry(c_ele->list.next, element_t, list);
        if (strcmp(n_ele->value, c_ele->value) < 0) {
            list_del(&n_ele->list);
            q_release_element(n_ele);
        } else {
            c_ele = n_ele;
        }
    }
    return q_size(head);
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    if (!head)
        return 0;
    struct list_head *c = head->prev;
    element_t *c_ele = list_entry(c, element_t, list);
    while (c_ele->list.prev != head) {
        element_t *n_ele = list_entry(c_ele->list.prev, element_t, list);
        if (strcmp(n_ele->value, c_ele->value) < 0) {
            list_del(&n_ele->list);
            q_release_element(n_ele);
        } else {
            c_ele = n_ele;
        }
    }
    return q_size(head);
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    // https://leetcode.com/problems/merge-k-sorted-lists/
    if (!head)
        return 0;
    queue_contex_t *c_cont;
    queue_contex_t *n_cont;
    struct list_head *sorted = NULL;

    list_for_each_entry_safe (c_cont, n_cont, head, chain) {  // iterate context
        c_cont->q->prev->next = NULL;
        c_cont->q->prev = NULL;
        sorted = merge_two_list(sorted, c_cont->q->next);
        INIT_LIST_HEAD(c_cont->q);  // reconnect the lists which are moved and
                                    // merged to "sorted" list;
    }
    LIST_HEAD(tmp);
    struct list_head *t = &tmp;
    t->next = sorted;
    struct list_head *c = t;
    while (sorted) {
        sorted->prev = c;
        c = sorted;
        sorted = sorted->next;
    }
    c->next = t;
    t->prev = c;
    int size = q_size(t);  // store size before splice to main queue
    list_splice(t, list_first_entry(head, queue_contex_t, chain)->q);
    return size;
}

// local function implementation
struct list_head *merge_two_list(struct list_head *left,
                                 struct list_head *right)
{
    struct list_head head;
    struct list_head *h = &head;
    if (!left && !right) {
        return NULL;
    }
    while (left && right) {
        if (strcmp(list_entry(left, element_t, list)->value,
                   list_entry(right, element_t, list)->value) < 0) {
            h->next = left;
            left = left->next;
            h = h->next;
        } else {
            h->next = right;
            right = right->next;
            h = h->next;
        }
    }
    // after merge, there are still one node still not connect yet

    if (left) {
        h->next = left;
    } else if (right) {
        h->next = right;
    }
    return head.next;
}

struct list_head *merge_recur(struct list_head *head)
{
    if (!head->next)
        return head;

    struct list_head *slow = head;
    // split list
    for (struct list_head *fast = head->next; fast && fast->next;
         fast = fast->next->next) {
        slow = slow->next;
    }

    struct list_head *mid = slow->next;  // the start node of right part
    slow->next = NULL;

    struct list_head *left = merge_recur(head);
    struct list_head *right = merge_recur(mid);

    return merge_two_list(left, right);
}
