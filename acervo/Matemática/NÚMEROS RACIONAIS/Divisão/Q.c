#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool dist_alt( char **alt ){
    int i, j;
    for( i=0; i<5; i++ ){
        for( j=i+1; j<5; j++ ){
            if( strcmp( alt[i], alt[j] ) == 0 ){
                return true;
            }
        }
    }
    return false;
}


#include "Q.h"
#include "/home/usuario/Documentos/LATEX/SEDUC/acervo/rdtsc.h"


int main(){
    
    srand(rdtsc());
    
    Q1();
    
    return 0;
}


