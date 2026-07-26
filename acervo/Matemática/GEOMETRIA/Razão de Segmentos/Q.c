#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool dist_alt( int *alt );

#include "Q.h"
#include "/home/usuario/Documentos/LATEX/SEDUC/include/thirdparty/rdtsc.h"


int main(){
    
    srand(rdtsc());
    
    Q1();
    Q2();
    Q3();
    
    return 0;
}


bool dist_alt( int *alt ){
    int i, j;
    for( i=0; i<5; i++ ){
        for( j=i+1; j<5; j++ ){
            if( alt[i] == alt[j] ){
                return true;
            }
        }
    }
    return false;
}
