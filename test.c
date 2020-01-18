#include <stdio.h>
#include <memory.h>




int main() 
{
    float a[] = {1.234,2.355,-5.555};
    float f = 19.25;
    char *c = (char*)malloc(12);
    void *vp = calloc(3, sizeof(float));
    c = a;
    vp =a;
    for(int i=0;i<4;i++)
        printf("%d\n",c[i]);
 
}