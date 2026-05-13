#ifndef __NF_UTIL_PARSE_S1_H__
#define __NF_UTIL_PARSE_S1_H__

typedef enum {
	Variable,
	Value
} NodeKind;

typedef struct treeNode
{ 
	struct treeNode * child;
	struct treeNode * sibling;
	NodeKind kind;
	char * name; 
	int lineno;
} TreeNode;

TreeNode* S1_GetRootTreeByModel(char *model);
TreeNode* S1_GetSubTree(TreeNode* base_tree, char *cate, int idx);
int S1_GetSubTreeNum(TreeNode* base_tree, char *cate);
char* S1_GetItem(TreeNode* base_tree, char *item, int idx);
int S1_GetItemNum(TreeNode* base_tree, char *item);

#endif
