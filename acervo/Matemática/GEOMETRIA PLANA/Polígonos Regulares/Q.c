#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "../../../bancoQ.h"
#include "Q.h"
#include "/home/usuario/Documentos/LATEX/SEDUC/include/thirdparty/rdtsc.h"


int main()
{    
    srand(rdtsc());
    
    int *rnd[3];
    rnd[0] = randperm(3);
    rnd[1] = randperm(3);
    rnd[2] = randperm(5);
    
    Q1( rnd );
    
    Q2( rnd );
    
    return 0;
}


