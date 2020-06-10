#include "utils.h"
#include <cstdlib>

#include <iostream>   
#include <unistd.h>
using namespace std; 

void print_string2hex(uint8_t* data, size_t n) {
    for(int i = 0; i < n; i++) {
        printf("%X", data[i]);
        if((i + 1) % 2 == 0 && i < n - 1)
            printf(":");
    }
    printf("\n");
}

void print_bar(int s) {
	 cout << "\nEncrypting data...... \n";

		for( int i=1; i <= 100; i++ )      // 打印百分比 
		{
            cout << ('#');
			cout.width(3);//i的输出为3位宽
			cout << i << "%";
            fflush(stdout);

			sleep(rand() % s);
			cout << "\b\b\b\b";//回删三个字符，使数字在原地变化
		}
	    cout << "\n\n";
 
	return ;

}