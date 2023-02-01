#include "syscall.h"

int d[15];
int main(void)
{
	int n;
	int flag = 0;
	for (n=10; n>0; n--) {
		d[n] = n;
		PrintInt(n);
		if(n != d[n]){
			flag = 1;
		}
	}
	if(flag){
		MSG("Failed on closing file");
	}else{
    	
	}

}
