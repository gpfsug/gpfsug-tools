
struct bst_node {
	struct bst_node *left;
	struct bst_node *right;
	unsigned long key;
 	int data; /* any custom value */
};

struct bst_tree {
	struct bst_node *root;

	/* statistics */
	/* if depth is comparable to #nodes,
	 * don't use this data structure ! */
	int	depth;
	int	nodes;
	int	hit;
};

struct bst_tree *bst_new_tree(void);

/* Instert 'data' with 'key'
 * Worst case is O(n), but with almost random keys
 * the average will be log n
 */
int bst_insert(struct bst_tree *tree, unsigned long key, int data, int *ex_data);

/* return 0 on success */
int bst_free(struct bst_tree *tree);

