#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int *randperm( int n ){
    int *N = (int*) calloc( n, sizeof(int) );
    int i, j, aux;
    
    for( i=0; i<n; i++ ){
        N[i] = i;
    }
    
    for( i=0; i<n; i++ ){
        j = rand()%n;
        aux = N[j];
        N[j] = N[i];
        N[i] = aux;
    }
    
    return N;
}

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
#include "/home/usuario/Documentos/LATEX/SEDUC/include/thirdparty/rdtsc.h"


int main(){
    
    srand(rdtsc());
    
    Q1();
    
    return 0;
}


