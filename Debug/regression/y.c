#include <assert.h>
int main(){
    int i = 0;
    int buflen = 100;
    while(i < buflen){
        char c;
        if(c){
            i+=1;
        }
        assert(i <= buflen);
        i+=1;
    }
    return 0;
}
