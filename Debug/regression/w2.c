//
// Created by supreets51 on 7/11/19.
//

// Fails!

#include <assert.h>
int main(){
    int i = 7;
    int buflen = 100;
    int j = 1;
    while(i > -3){
        i-=2;
        //i+=5;
        j+=i;
        assert(i >= -1);
    }
    return 0;
}
