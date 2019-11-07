#include <assert.h>
int main(){
    int i = 0;
    int buflen = 100;
    int j = 1;
    while(i<buflen/3){
        i+=2;
        j+=i;
        assert(j <= buflen);
    }
    return 0;
}

