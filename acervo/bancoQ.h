int numeros_primos[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547 };

struct tempo{ int h, m, s; };

struct tempo horas( float h ){
    struct tempo temp;
    temp.s = 3600*h;
    temp.m = temp.s/60;
    temp.h = temp.m/60;
    temp.s %= 60;
    temp.m %= 60;
    return temp;
}

struct tempo minutos( float m ){
    struct tempo temp;
    temp.s = 60*m;
    temp.m = temp.s/60;
    temp.h = temp.m/60;
    temp.s %= 60;
    temp.m %= 60;
    return temp;
}

struct tempo segundos( float s ){
    struct tempo temp;
    temp.s = s;
    temp.m = temp.s/60;
    temp.h = temp.m/60;
    temp.s %= 60;
    temp.m %= 60;
    return temp;
}

// void hms( struct tempo temp, char *str ){
//     str[0] = '\0';
//
//     char str0[10];
//     if( temp.h!=0 ){
//         sprintf( str0, "%dh", temp.h );
//         strcat( str, str0 );
//     }
//     if( temp.m!=0 || temp.s!=0 ){
//         sprintf( str0, "%dmin", temp.m );
//         strcat( str, str0 );
//     }
//     if( temp.s!=0 ){
//         sprintf( str0, "%ds", temp.s );
//         strcat( str, str0 );
//     }
// }
//
//
// void hms(struct tempo temp, char *str) {
//     str[0] = '\0'; // Limpa a string
//
//     if (temp.h > 0 && temp.m > 0 && temp.s > 0)
//         sprintf(str, "%dh %dmin %ds", temp.h, temp.m, temp.s);
//     else if (temp.h > 0 && temp.m > 0)
//         sprintf(str, "%dh %dmin", temp.h, temp.m);
//     else if (temp.h > 0 && temp.s > 0)
//         sprintf(str, "%dh %ds", temp.h, temp.s);
//     else if (temp.h > 0)
//         sprintf(str, "%dh", temp.h);
//     else if (temp.m > 0 && temp.s > 0)
//         sprintf(str, "%dmin %ds", temp.m, temp.s);
//     else if (temp.m > 0)
//         sprintf(str, "%dmin", temp.m);
//     else
//         sprintf(str, "%ds", temp.s);
// }


void hms( struct tempo temp, char *str ){
    str[0] = '\0';
    if( temp.h!=0 )              sprintf(str + strlen(str), "%dh",   temp.h );
    if( temp.m!=0 || temp.s!=0 ) sprintf(str + strlen(str), "\\,%dmin", temp.m );
    if( temp.s!=0 )              sprintf(str + strlen(str), "\\,%ds",   temp.s );
}


// int mdc( int a, int b ){
//     int i, k=1, p[]={2,3,5,7,11,13,17,19,23};
//     for( i=0; i<9; i++ ){
//         while( a%p[i]==0 && b%p[i]==0 ){
//             a/=p[i];
//             b/=p[i];
//             k*=p[i];
//         }
//     }
//     return k;
// }


// int mdc(int a, int b) { // Usando algoritmo de Euclides MDC(a, b) = MDC(b, a mod b)
//     while (b != 0) {
//         int r = a % b;
//         a = b;
//         b = r;
//     }
//     return a;
// }

int mdc(int a, int b) { // Usando recursividade
    // Caso base: quando o resto (b) chega a zero, o MDC é o próprio 'a'
    if (b == 0) return a;

    // Chamada recursiva: o antigo 'b' vira o novo 'a',
    // e o resto (a % b) vira o novo 'b'
    return mdc(b, a % b);
}



int mmc( int a, int b ){
    return a * b / mdc( a, b );
}


void simplificar_fracao(int num, int den, char *str) {
    if (den == 0) {
        sprintf(str, "undefined"); // Segurança matemática
        return;
    }

    int comum = mdc(abs(num), abs(den));
    int n = num / comum;
    int d = den / comum;

    if (d == 1) {
        sprintf(str, "%d", n);
    } else {
        // Se o denominador for negativo, passa o sinal para o numerador
        if (d < 0) { n = -n; d = -d; }
        sprintf(str, "\\frac{%d}{%d}", n, d);
    }
}



int prod( int *A, int n ){
    int i, P=1;
    for( i=0; i<n; i++ ){
        P *= A[i];
    }
    return P;
}

bool fatraiz( unsigned a, int ind, char *str ){
    
    if( a == 0 || a == 1 || roundf(powf(a,1./ind)) == powf(a,1./ind) ){
        sprintf( str, "%d", (int)powf(a,1./ind) );
        return true;
    }
    
    int i, k=1, r=1, n;
    
    for( i=0; i<100; i++ ){
        n = 0;
        while( a % numeros_primos[i] == 0 ){
            a /= numeros_primos[i];
            n ++;
        }
        k *= powf( numeros_primos[i], n/ind );
        r *= powf( numeros_primos[i], n%ind );
    }
    
    if( k == 1 ){
        if( ind == 2 ) sprintf( str, "\\sqrt{%d}", r );
        else           sprintf( str, "\\sqrt[%d]{%d}", ind, r );
    }
    else{
        if( ind == 2 ) sprintf( str, "%d\\sqrt{%d}", k, r );
        else           sprintf( str, "%d\\sqrt[%d]{%d}", k, ind, r );
    }
    
    return (r<=30 && r!=17 && r!=19 && r!=23 && r!=29);
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


void intperm( int *P, int n ){
    int i, j, aux;
    for( i=0; i<n; i++ ){
        j = rand()%n;
        aux = P[j];
        P[j] = P[i];
        P[i] = aux;
    }
}


int fatorial( int n ){ // Sugestão da GG, usar unsigned long long :-)
    if( n==0 || n==1 )
        return 1;
    else
        return n * fatorial(n-1);
}

int arranjo( int n, int p ){
    if( p==0 )
        return 1;
    else
        return ( n-p+1 ) * arranjo( n, p-1 );
}

int combinacao( int n, int p ){
    if( p==0 || p==n )
        return 1;
    else
        return combinacao( n-1, p ) + combinacao( n-1, p-1 );
}

char **alocar_alternativas(int n_alt, int len_alt){
    int i;
    char **alt = (char**) calloc( n_alt, sizeof(char*) );
    for( i=0; i<n_alt; i++ ){
        alt[i] = (char*) calloc( len_alt, sizeof(char) );
    }
    return alt;
}

void liberar_alternativas(char **alt, int n_alt){
    int i;
    for( i=0; i<n_alt; i++ ){
        free(alt[i]);
    }
    free(alt);
}


bool dist_alt( char **alt, int len ){
    int i, j;
    for( i=0; i<5; i++ ){
        for( j=i+1; j<5; j++ ){
            if( strncmp( alt[i], alt[j], len ) == 0 ){
                return true;
            }
        }
    }
    return false;
}

bool tem_alternativa_repetida(char **alt) {
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 5; j++) {
            // strcmp garante que "10" != "100" e "2" != "20"
            if (strcmp(alt[i], alt[j]) == 0) {
                return true;
            }
        }
    }
    return false;
}


