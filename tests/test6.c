int num(int a, int b)
{
	if(a != 0) {
		if(b != 0) {
			a += b;
		}
		else {
			b += a;
		}
	}
}

int main()
{
	int x;
	int y;

	x = 5;
	y = 7;

	num(x,y);
}