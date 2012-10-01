/*
 * Binary search tree
 * Written by Peter Somogyi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, visit the http://fsf.org website.
 */

#include "bst.h"
#include <stdlib.h>
#include <string.h>


static struct bst_node *bst_alloc_node(unsigned long key, unsigned long data)
{
	struct bst_node *node = (struct bst_node *)malloc(sizeof(struct bst_node));

	if (!node)
		return NULL;

	memset(node, 0, sizeof(struct bst_node));
	node->key = key;
	node->data = data;

	return node;
}

struct bst_tree *bst_new_tree(void)
{
	struct bst_tree *tree = (struct bst_tree *)malloc(sizeof(struct bst_tree));

	if (!tree)
		return NULL;

	memset(tree, 0, sizeof(struct bst_tree));

	return tree;
}

int bst_insert(struct bst_tree *tree, unsigned long key, int data, int *ex_data)
{
	struct bst_node	*node = (struct bst_node *)tree->root;
	struct bst_node	*next = node;
	int	depth = 0;

	if (!tree)
		return -2;

	/* avoid recursion, we can have hundred thousands of nodes */
	while(next) {
		/* this needs to be FAST */
		depth++;
		node = next;
		if (node->key < key) {
			next = node->right;
		} else if (node->key > key) {
			next = node->left;
		} else { /* equal: */
			*ex_data = node->data;
			tree->hit++;
			return 1;
		}
	}

	next = bst_alloc_node(key, data);
	if (!next)
		return -1;

	/* insert it */
	if (node) {
		if (key < node->key)
			node->left = next;
		else
			node->right = next; 
	} else {
		tree->root = next;
	}

	/* update statistics */
	if (tree->depth < depth)
		tree->depth = depth;

        tree->nodes++;

	return 0;
}

int bst_free(struct bst_tree *tree)
{
	struct bst_node	**nlist;
	struct bst_node	*node;
	int	depth = 0;
	int	rc = 0;
	int	count = 0;

	if (!tree)
		return -2;

	nlist = (struct bst_node **)malloc(sizeof(struct bst_node *) * (tree->depth + 1));
	if (!nlist)
		return -1;

	nlist[depth] = tree->root;

	while(depth>=0 && nlist[depth]) {
		node = nlist[depth];

		if (node->right) {
			if (depth >= tree->depth)
				return -4;
			nlist[++depth] = node->right;
			node->right = NULL;
		} else if (node->left) {
			if (depth >= tree->depth)
				return -5;
			nlist[++depth] = node->left;
			node->left = NULL;
		} else {
			free(node);
			count++;
			depth--;
		}
	}

	if (count != tree->nodes)
		rc = -3;

	free(nlist);
	free(tree);

	return rc;
}
