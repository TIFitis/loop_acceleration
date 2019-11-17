#include <assert.h>
int main(){
    int i = 0;
    int buflen = 100;
    int n1 = 0, n2 = 0;
    while(i<buflen){
        i = i + 2;
        n1++;
    }
    while(i > 0){
        i--;
        n2++;
    }
    assert(n1 == n2);
    return 0;
}
