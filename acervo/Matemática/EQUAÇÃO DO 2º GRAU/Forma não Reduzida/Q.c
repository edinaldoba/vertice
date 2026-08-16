#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


int mdc( int a, int b ){
    int i, k=1, p[]={2,3,5,7};
    for( i=0; i<4; i++ ){
        while( a%p[i]==0 && b%p[i]==0 ){
            a/=p[i];
            b/=p[i];
            k*=p[i];
        }
    }
    return k;
}


int prod( int *A, int n ){
    int i, P=1;
    for( i=0; i<n; i++ ){
        P *= A[i];
    }
    return P;
}


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
#include "/home/usuario/Documentos/LATEX/SEDUC/acervo/rdtsc.h"


int main(){
    
    srand(rdtsc());
    
    Q1();
    
    return 0;
}


