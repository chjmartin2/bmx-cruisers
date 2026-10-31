#include "Core/ScrollPresentation.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    for(int hz:{30,60,120}){
        BMX::ScrollPresentation view;
        double previous=0;
        for(int i=1;i<=hz*10;++i){
            double time=double(i)/hz;int row=int(std::floor((time+1e-8)*10));
            double position=view.Update(time,row*.1,row,1.0/hz);
            assert(std::abs(position-time*600)<.01);
            assert(std::abs((position-previous)*hz-600)<.01);
            previous=position;
        }
        assert(view.Update(10,10,100,0)==previous);
        view.Reset();assert(view.Position()==0);
        std::cout<<hz<<" Hz: continuous six-metre-per-second travel between ten-Hz source rows; pause/reset passed\n";
    }
}
