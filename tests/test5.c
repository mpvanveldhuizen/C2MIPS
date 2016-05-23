int double(int n) {
	int result;
	result = n + n;
	return result;

}

int main() {
	int i = 5;

	while(i != 0) {
		double(i);
		--i;
	}
}