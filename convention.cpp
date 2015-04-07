#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <set>
#include <vector>
#include <algorithm>
#include <cassert>

using namespace std;

#define MAX 100*10000 //0x0FFFFFFF
#define NMAX 200000
#define KMAX 18 //2^18 > NMAX

int N, len; // N: num of reqs; len of time(zipped)
struct Req {
	int b, e;
	Req(int b, int e):b(b),e(e) {}
	Req():b(0),e(0) {}
	bool operator<(const Req &r) const
	{ return (b < r.b || b == r.b && e < r.e); } // sort by begin point
	bool operator==(const Req &r) const
	{ return (b == r.b && e == r.e); }
};
Req req[NMAX], tmp[NMAX];

set<Req> s, t;
vector<Req> v;
vector<int> pts; //endpoints

// binary search and zip
// rank from 0
int rank_int(int p, vector<int>& arr)
{
	int l, r, m;
	l = 0; r = arr.size()-1; m = (l+r)>>1;
	for (; l < r; m = (l+r)>>1)
		if (arr[m] < p)	l = m + 1;
		else if (arr[m] == p) break;
		else 		r = m - 1; // >
	return m;
}

int rank_req(Req p, vector<Req>& arr)
{
	int l, r, m;
	l = 0; r = arr.size()-1; m = (l+r)>>1;
	for (; l < r; m = (l+r)>>1)
		if (arr[m] < p)	l = m + 1;
		else if (arr[m] == p) break;
		else 		r = m - 1; // >
	return m;
}

void
discrete()
{
	int i, j = 0;
	for (i = 0; i < N; i++) {
		pts.push_back(req[i].b);
		pts.push_back(req[i].e);
	}
	sort(pts.begin(), pts.end());
	vector<int>::iterator itv = unique(pts.begin(), pts.end());
	len = itv - pts.begin();
	pts.resize(len);

	cout << "len: " << len << endl;
//	for (i = 0; i < len; i++)
//		cout << pts[i] << ",";
//	cout << endl;


	for (i = 0; i < N; i++) {
		req[i].b = rank_int(req[i].b, pts);
		req[i].e = rank_int(req[i].e, pts);
#if 0
		cout << "req[" << i+1 << "]: " << req[i].b << ", " << req[i].e << endl;
#endif
	}

	// *** filter out the bigger req that cover other req.
	// v is not filtered
	v.insert(v.begin(), req, req+N);
	sort(v.begin(), v.end());
	// s is filtered.
	Req p, q;
#if 10
	s.insert(v[N-1]);
	for (i = N-2; i >= 0; i--) {
		q = v[i];
		if (q.b <= p.b && q.e >= p.e) {
			cout << "- v[" << i+1 << "]: " << q.b << ", " << q.e;
			cout << " * v[" << i+1 << "]: " << p.b << ", " << p.e << endl;
			continue;
		}
		p = q;
		s.insert(p);
	}
#else
	s.insert(v[N-1]);
	for (i = N-2; i >= 0; i--) {
		q = v[i];
		p = v[i-1];
		if (q.b <= p.b && q.e >= p.e) {
			cout << "- v[" << i+1 << "]: " << q.b << ", " << q.e;
			cout << " * v[" << i+1 << "]: " << p.b << ", " << p.e << endl;
			continue;
		}
		p = q;
		s.insert(p);
	}
#endif
	cout << "N1: " << s.size() << endl;
}

// rank in v.
int
right(int i)
{
	Req a(v[i].e+1, 0); // unintersect seq
	set<Req>::iterator its = s.lower_bound(a); // >= a; next seg.
	if (its == s.end())
		return MAX; // no right
	return rank_req(*its, v); // rank in array
}

// right^(2^k)(t)
int lut[KMAX][NMAX];
int
init_lut()
{
	int i, j, k;
	for (i = 0; i < N; i++) {
		lut[0][i] = right(i);
		//cout << "right(" << i << ") = " << lut[0][i] << endl;
	}
	for (k = 1; k < KMAX; k++)
		for (i = 0; i < N; i++) {
			j = lut[k-1][i];
			if (j == MAX)
				lut[k][i] = j;
			else
				lut[k][i] = lut[k-1][j];
		}
}

// l,r: [0,len-1]
// i,j: [0,N-1]
int
size_MIS(int l, int r)
{
#if 0
	cout << "    l: " << l << "; r: " << r;
#endif
	if (l >= r) return 0;
	int res = 0, i, j, k, n;
	set<Req>::iterator its = s.lower_bound(Req(l, 0)); // 1st req in time range:[l,r]
	// if range is too small to hold a req
	if (its == s.end() || its->e > r || its->b >= r)
		return 0;
#if 0
	cout << "; L1: " << its->b << "; R1: " << its->e;
#endif
	i = rank_req(*its, v);
	for (k = KMAX - 1; k >= 0; k--) {
		n = lut[k][i];
		if (n == MAX) continue;
		if (v[n].e <= r) {
			res += 1<<k;
			i = n;
		}
	}
	res++;
#if 0
	cout << "; sz: " << res << endl;
#endif
	return res;
}

void
emis(int *no, int *arr)
{
	set<Req>::iterator it;
	int l, r, ts, ls, rs;
	t.insert(Req(0, len-1));
	*no = 0;
	for (int i = 0; i < N; i++) {
		Req q = req[i];
#if 0
		cout << "#" << i+1 << " Req: [" << q.b << "," << q.e << "] ";
#endif
		//it = s.find(q);
		//if (it == s.end()) continue;
		// find the seg inc q in
		if (t.empty()) continue; // not possible?
		it = t.lower_bound(Req(q.b, MAX));
		it--;
		l = it->b; r = it->e;
#if 0
		cout << "; L:" << l << "; R:" << r << endl;
#endif
		if (l > q.b || r < q.e)
			continue;
		ts = size_MIS(l, r);
		ls = size_MIS(l, q.b-1);
		rs = size_MIS(q.e+1, r);
#if 0
		cout << "    ts: " << ts << "; ls: " << ls << "; rs: " << rs << endl;
#endif
		if (ts == ls + 1 + rs) {
			t.erase(Req(l, r));
			if (ls > 0) t.insert(Req(l, q.b-1));
			if (rs > 0) t.insert(Req(q.e+1, r));
			arr[(*no)++] = i + 1;
#if 0
			cout << " got: " << i + 1 << endl;
#endif
		}
	}
}

void
work(int *no, int *arr)
{
	discrete();
	init_lut();
	emis(no, arr);
	N = 0; s.clear(); t.clear(); v.clear(); pts.clear();
}

void
test(const char *file_in, const char *file_out)
{
	int i, *idx, no = 0;

	FILE *fp = fopen(file_in, "r");

	fscanf(fp, "%d\n", &N);
	cout << "N: " << N << endl;
	for (i = 0; i < N; i++)
		fscanf(fp, "%d %d\n", &req[i].b, &req[i].e);

	idx = (int *) calloc(N, sizeof(int));
	work(&no, idx);

	cout << "no: " << no << endl;
	cout << "output: {" << endl;
	for (i = 0; i < no; i++)
		cout << idx[i] << " ";
	cout << endl;
	cout << "}" << endl;

	FILE *fp2 = fopen(file_out, "r");
	int no2, *idx2 = (int *) calloc(N, sizeof(int));
	fscanf(fp2, "%d\n", &no2);
	for (i = 0; i < no2; i++)
		fscanf(fp2, "%d", idx2+i);
	if (no2 != no) {
		cout << "not match!! 1" << endl;
		return;
	}
	for (i = 0; i < no2; i++)
		if (idx2[i] != idx[i]) {
			cout << "not match!! 2" << endl;
			cout << "idx[" << i << "]: " << idx[i]
				<< "; idx2[" << i << "]: " << idx2[i] << endl;
			return;
		}
}

int
main()
{
	test("./convention.in", "./convention.out");
	return 0;
}
