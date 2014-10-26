/* opkg_active_list.c - the opkg package management system

   Tick Chen <tick@openmoko.com>

   Copyright (C) 2008 Openmoko

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/


#include <stdlib.h>
#include <libopkg/active_list.h>
#include <active_list.h>
#include <stdio.h>

struct active_test {
    char *str;
    struct active_list list;
};

struct active_test *active_test_new(char *str) {
    struct active_test *ans = (struct active_test *)calloc(1, sizeof(struct active_test));
    ans->str = str;
    active_list_init(&ans->list);
    return ans;
}
void active_test_add(struct active_list *head, struct active_test *node) {
    active_list_add(head, &node->list);
}

void active_test_add_depend(struct active_test *A, struct active_test *B) {
    active_list_add_depend(&A->list, &B->list);
}

/*
．--A---B----C----D-----E----F
    |             |__k---L
    |                    |_ N
    |__ G ---H ---I---J
             |_M      |_O

Then the sequence will be
+: G M H I O J A B K N L C D E F
-: F E D C L N K B A J O I H M G
*/
void make_list(struct active_list *head) {
    struct active_test *A = active_test_new("A");
    struct active_test *B = active_test_new("B");
    struct active_test *C = active_test_new("C");
    struct active_test *D = active_test_new("D");
    struct active_test *E = active_test_new("E");
    struct active_test *F = active_test_new("F");
    struct active_test *G = active_test_new("G");
    struct active_test *H = active_test_new("H");
    struct active_test *I = active_test_new("I");
    struct active_test *J = active_test_new("J");
    struct active_test *K = active_test_new("K");
    struct active_test *L = active_test_new("L");
    struct active_test *M = active_test_new("M");
    struct active_test *N = active_test_new("N");
    struct active_test *O = active_test_new("O");

    active_test_add(head, A);
    active_test_add(head, B);
    active_test_add(head, C);
    active_test_add(head, D);
    active_test_add(head, E);
    active_test_add(head, F);
    active_test_add(head, G);
    active_test_add(head, H);
    active_test_add(head, I);
    active_test_add(head, J);
    active_test_add(head, K);
    active_test_add(head, L);
    active_test_add(head, M);
    active_test_add(head, N);
    active_test_add(head, O);
    active_test_add_depend(H, M);
    active_test_add_depend(A, G);
    active_test_add_depend(A, H);
    active_test_add_depend(A, I);
    active_test_add_depend(A, J);
    active_test_add_depend(J, O);
    active_test_add_depend(C, K);
    active_test_add_depend(C, L);
    active_test_add_depend(L, N);
}

int active_test_compare(const void *a, const void *b) {
    struct active_list *first = (struct active_list *)a;
    struct active_list *second = (struct active_list *)b;
    return strcmp(list_entry(first, struct active_test, list),
            list_entry(second, struct active_test, list));
}

void show_list(struct active_list *head) {
    struct active_list *ptr;
    struct active_test *test;
    for(ptr = active_list_next(head, NULL); ptr ;ptr = active_list_next(head, ptr)) {
        test = list_entry(ptr, struct active_test, list);
        printf ("%s ",test->str);
    }
    printf("\n");
}

int main (void) {
    struct active_list head;
    struct active_list *ptr;
    struct active_test *test;
    active_list_init(&head);
    make_list(&head);

    printf("pos order: ");
    show_list(&head);
/*    for(ptr = active_list_next(&head, &head); ptr ;ptr = active_list_next(&head, ptr)) {
        test = list_entry(ptr, struct active_test, list);
        printf ("%s ",test->str);
    }*/
    printf("neg order: ");
    for(ptr = active_list_prev(&head, &head); ptr ;ptr = active_list_prev(&head, ptr)) {
        test = list_entry(ptr, struct active_test, list);
        printf ("%s ",test->str);
    }
    printf("\npos order after sort: ");
    active_list_sort(&head, &active_test_compare);
    show_list(&head);

    printf("after clear: ");
    active_list_clear(&head);
    for(ptr = active_list_next(&head, NULL); ptr ;ptr = active_list_next(&head, ptr)) {
        test = list_entry(ptr, struct active_test, list);
        printf ("%s ",test->str);
    }
    printf("\n");


}
