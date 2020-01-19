#include <stdio.h>
#include <memory.h>




int main() 
{
    FILE *fp = fopen("backup/test_weights", "w");
    int arr[] = {1,2,3,0};
    float weight1[] = {1,2,3,4,5,6,7,8};
    float weight2[] = {1.1,2.2,3.3,4.4};
    float bias[] = {0.5, 0.6};
    fwrite(arr, sizeof(int), 4, fp);
    fwrite(bias, sizeof(float), 2, fp);
    fwrite(weight1, sizeof(float), 8, fp);
    fwrite(bias, sizeof(float), 1, fp);
    fwrite(weight2, sizeof(float), 4, fp);
    fclose(fp);
}