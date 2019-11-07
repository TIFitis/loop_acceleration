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
    while(i <= 3){
        char c;
        if(c){
            i+=2;
            if(i){
                i+=3;
            }
            else {
                i+=4;
            }
        }
        if(c){
            i+=5;
        }
        assert(i <= buflen);
        i+=6;
    }
    return 0;
}
