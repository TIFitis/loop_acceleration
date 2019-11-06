#include <assert.h>
int main(){
    int i = 0;
    int buflen = 100;
    while(1){
        i+=2;
        i+=5;
        assert(i <= buflen);
    }
    return 0;
}
