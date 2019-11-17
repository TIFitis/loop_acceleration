#include <assert.h>
int main(){
    int i = 0;
    int buflen = 100;
    int j=0, n = 1, a = 2, x;
    while(i < buflen && j<buflen){
        i += 2;
        i -= 2;
    }
    assert(0);
    return 0;
}
