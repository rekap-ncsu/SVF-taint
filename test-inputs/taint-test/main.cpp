#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void taint_var(char* filename)
{
    printf("%s", filename);
}

int main(int argc, char** argv) 
{
    char filename[8];
    taint_var(filename);
    system(filename);
}