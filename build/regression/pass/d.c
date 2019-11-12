#include <assert.h>
int main(){
    int i = 0, j=0;
    int buflen = 100;
    while(i<=buflen){
        i = i+2;
        j = j+i;
        assert(i != 8);
    }
    return 0;
}
