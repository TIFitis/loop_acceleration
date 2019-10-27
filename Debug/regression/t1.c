#include <assert.h>
int main(){
    int useless = 10;
    useless += 100;
    int useless_2 = 100;
    if(useless)useless_2 ++;
    else useless = useless_2;
    if(useless_2)useless_2*=25;
    
    int i = 0;
    int buflen = 100;
    while(1){
        i+=2;
        assert(i <= buflen);
    }
    return 0;
}
