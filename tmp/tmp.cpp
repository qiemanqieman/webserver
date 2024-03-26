using namespace std;
#include "pch.h"

bool isPowerOfTwo(int n) {
    return n > 0 && (n & (n - 1)) == 0;
}

typedef struct AA
{
int b1:5;
int b2:2;
}AA;

int main()
{
AA aa;
char cc[100];
strcpy(cc,"0123456789abcdefghijklmnopqrstuvwxyz");
memcpy(&aa,cc,sizeof(AA));
cout << aa.b1 <<endl;
cout << aa.b2 <<endl;
char c = '0';
cout << (int)c << endl;
}