#include <assert.h>
int main(){
    int i = 0;
    int buflen = 100;
    int n1 = 0, n2 = 0;
    while(i<buflen){
        i = i + 10;
        n1++;
    }
    assert(n1 != buflen/10);
    return 0;
}