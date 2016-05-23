int one(int i, int j)
{
	int n;
	n = i + j;
}

int main() {
	int a;
	int i;
	for(i = 1; i != 3; ++i) {
		a = i;
		one(a,2);
	}

	int j;
	for(j = 5; j != 1; --j) {
		j *= j;
	}
}