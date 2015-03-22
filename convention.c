#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NMAX 200000
#define KMAX 18 //2^18 > NMAX

int N;
int *In2End; // input idx to sort idx.
int *End2In; // sort idx to input idx.

// segment tree
typedef struct tree_node
{
	int rl, rr, kl, kr; // range and key
} Node;
Node *node;
#define LCHILD(i) (i<<1)
#define RCHILD(i) (LCHILD(i)+1)

typedef struct {
	int beg;
	int end;
} CompanyRequest;
CompanyRequest *cr;

//const int kmax = (int)(log(0.1*NMAX)/log(2.0))+1;
//Sparse-Table of RIGHT[NMAX][NMAX]
//i:1..kmax-1
//j:0..NMAX-1
//int **RIGHT = NULL; //RIGHT[kmax][NMAX];
int *RIGHT[KMAX];

// sort by end point
int
ptr_cmp(const void *pl, const void *pr)
{
	int *l = (int *)pl, *r = (int *)pr;
	return (*l - *r);
}

int
int_cmp(const void *pl, const void *pr)
{
	int l = (int)pl, r = (int)pr;
	return (l - r);
}

void
discretization()
{
	int i, j, pi, pv, **arr;
	arr = (int **) calloc(N*2, sizeof(int *));
	for (i = 0; i < N; i++) {
		arr[i*2]   = &cr[i].beg;
		arr[i*2+1] = &cr[i].end;
	}
	// sort by ptr of int
	qsort(arr, N*2, sizeof(*arr), ptr_cmp);
	pi = pv = -1;
	for (i = 0; i < N; i++)
		if (pv == *arr[i]) {
			*arr[i] = pi; // discrete
		} else {
			pv = *arr[i];
			*arr[i] = ++pi;
		}
	free(arr);
}

int
endpoint_cmp(const void *pl, const void *pr)
{
	int l, r;
	l = (int)pl;
	r = (int)pr;
	return (cr[l].end - cr[r].end);
}

int
idx_cmp(const void *pl, const void *pr)
{
	int l, r;
	l = (int)pl;
	r = (int)pr;
	return (End2In[l] - End2In[r]);
}

// input order <-> endpoint order
void
mapping()
{
	int i;
	// sort index to input order index.
	End2In = (int *) calloc(N, sizeof(int));
	for (i = 0; i < N; i++)
		End2In[i] = i;
	qsort(End2In, N, sizeof(int), endpoint_cmp);
	// input order index to sort index.
	In2End = (int *) calloc(N, sizeof(int));
	for (i = 0; i < N; i++)
		In2End[End2In[i]] = i;
}

void build_tree(int idx, int l, int r)
{
	int m = (l+r)>>1;
	node[idx].rl = l;
	node[idx].rr = r;
	node[idx].kl = -1;
	node[idx].kr = -1;
	if (l == r)
		return;
	build_tree(LCHILD(idx), l, m);
	build_tree(RCHILD(idx), m+1, r);
}

// insert: 1, l, r, l, r
// delete: 1, l, r, -1, -1
void
insert_tree(int idx, int l, int r, int kl, int kr)
{
	int rr, rl, rm;
	rl = node[idx].rl;
	rr = node[idx].rr;
	rm = (rl+rl)>>1;

	if (rl == l && rr == r) { // match,mark
		node[idx].kl = kl;
		node[idx].kr = kr;
	}
	if (rl == rr)
		return; // on leaf node
	if (r <= rm) // on left
		insert_tree(LCHILD(idx), l, rm, kl, kr);
	else if (l > rm) // on right
		insert_tree(RCHILD(idx), rm+1, r, kl, kr);
	else {	// on both
		insert_tree(LCHILD(idx), l, rm, kl, kr);
		insert_tree(RCHILD(idx), rm+1, r, kl, kr);
	}
}

void insert(int l, int r)
{
	insert_tree(1, l, r, l, r);
}

void delete(int l, int r)
{
	insert_tree(1, l, r, -1, -1);
}

// root node at node[1];
void init_tree()
{
	int i;
	node = (Node *) calloc(N*2, sizeof(Node));
	build_tree(1, 0, N-1);
	insert(0, N-1);
}

// query which range that i belongs to
void query_tree(int idx, int i, int *pl, int *pr)
{
	int rr, rl, rm;
	rl = node[idx].rl;
	rr = node[idx].rr;
	rm = (rl+rl)>>1;

	if (rl == i && rr == i) { // match,mark
		*pl = node[idx].kl;
		*pr = node[idx].kr;
		return;
	}
	if (i <= rm) // on left
		query_tree(LCHILD(idx), i, pl, pr);
	else //if (i > rm) // on right
		query_tree(RCHILD(idx), i, pl, pr);
}

// query point i on its range
void query(int i, int *pl, int *pr)
{
	query_tree(1, i, pl, pr);
}

// 1: intersect
// 0: no intersect
// index is in sort order.
int
intersect(int i, int j)
{
	i = End2In[i];
	j = End2In[j];
	if (cr[i].end < cr[j].beg || cr[i].beg > cr[j].end)
		return 0;
	return 1;
}

// -1 for do not have
int
right(int i)
{
	int j;
	// j:[1..N-1]
	for (j = i + 1; j < N; j++)
		if (!intersect(i, j))
			return j;
	return i; // no right
}

// right^(2^k)(t)
int
rightk(int k, int t)
{
	if (k == 0)
		return RIGHT[0][t];
	return rightk(k-1, rightk(k-1, t));
}

// right^(2^k)(t)
int
init_RIGHT()
{
	int i, j;
	// i:[0..N-2],j:[1..N-1]
	for (j = 0; j < N; j++)
		RIGHT[0][j] = right(j);
	for (i = 0; i <= KMAX; i++)
		for (j = 1; j < N-1; j++)
			RIGHT[i][j] = rightk(i, j);
}

int
size_MIS(int i, int j)
{
	int k, cur, res = 0;
	if (i > j)
		return 0;
	if (i == j)
		return 1;
	cur = i;
	for (k = KMAX; k >= 0; k--)
		if (RIGHT[k][cur] <= j) {
			res += (1 << k);	
			cur = RIGHT[k][cur];
		}
	return res + 1;
}

// use bin search??
void
neighbour(int i0, int *pi, int *pj)
{
	int i, j;
	// linear search?
	for (i = i0; i >= 0; i--)
		if (!intersect(i0, i))
			break;
	*pi = ++i;
	for (j = i0; j < N; j++)
		if (!intersect(i0, j))
			break;
	*pj = --j;
}

void
early_MIS(int *pOut, int *pIdx)
{
	int *invalid, i0, i, j, l, r, k, nValid;
	invalid = (int *) calloc(N, sizeof(int));
	nValid = 0;
	// range tree init
	init_tree();
	// enum i0 by input order
	for (i0 = 0; i0 < N; i0++) {
		i0 = In2End[i0]; // to endpoint order
		if (invalid[i0])
			continue;
		query(i0, &l, &r); // i0 in which range
		neighbour(i0, &i, &j); // i0's neighbours
		if (size_MIS(l,i-1)+1+size_MIS(j+1,r)==size_MIS(l,r)) {
			//[l..r] => [l..i-1],[i..j],[j+1,r]
			if (l < i)
				insert(l, i-1); // new range
			delete(i, j); // delete range
			for (k = i; k <=j; k++)
				invalid[k] = 1; // invalid idx
			invalid[i0] = 0;
			pIdx[nValid++] = End2In[i0]+1;
			if (j < r)
				insert(j+1, r); // new range
		}
	}
	qsort(pIdx, nValid, sizeof(int), int_cmp);
	*pOut = nValid;
	free(invalid);
}

void
ConventionArrangement(int nCr, CompanyRequest *pCr, int *pOut, int *pIdx)
{
	int i, l, r;
	if (nCr <= 1)
		return;
	N = nCr;
	cr = pCr;
	discretization();
	mapping(); // input order <-> endpoint order
	// right lookup table
	init_RIGHT();
	early_MIS(pOut, pIdx);
}

void
test()
{
	int i, *idx, no;

	scanf("%d\n", &N);
	cr = (CompanyRequest *) calloc(N, sizeof(*cr));
	for (i = 0; i < N; i++)
		scanf("%d %d\n", &cr[i].beg, &cr[i].end);
	idx = (int *) calloc(N, sizeof(int));

	ConventionArrangement(N, cr, &no, idx);

	printf("output:\n");
	printf("%d\n", no);
	for (i = 0; i < no; i++)
		printf("%d ", idx[i]);
	printf("\n");
}

int
main()
{
	test();
	return 0;
}
