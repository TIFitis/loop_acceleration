#include <assert.h>
int main(){
    int i = 0;
    int buflen = 100;
    int j = 1;
    while(i<j){
        i+=4;
        j+=i;
        assert(j <= buflen);
    }
    return 0;
}

