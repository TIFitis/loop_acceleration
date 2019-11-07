#include <assert.h>
int main(){
    int i = 0;
    int buflen = 100;
    while(i<=buflen){
        i = i + buflen;
        assert(i <= buflen);
    }
    return 0;
}
