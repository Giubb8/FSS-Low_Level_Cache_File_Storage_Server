#include<stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]){
    char string_1[12]="ciao Fabio";
    char * string_2=string_1;
    char * string_3=&(string_1);

    printf("String1: %s\nString2: %s\nString3: %s\n",string_1,string_2,string_3);


    return 0;
}
