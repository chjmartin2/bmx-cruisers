#pragma once
#include <algorithm>
#include <cmath>

namespace BMX {
// Reconstruct continuous travel from the original ten-Hz row events. This is a
// bounded display prediction, never an input to the original simulation.
class ScrollPresentation {
public:
    void Reset(){Distance=0;Speed=600;LastRow=0;LastEventTime=0;}
    double Update(double now,double rowTime,int rows,double elapsed){
        if(rows<LastRow)Reset();
        if(rows!=LastRow&&rowTime>LastEventTime){
            const double measured=(rows-LastRow)*60.0/(rowTime-LastEventTime);
            Speed+=(std::clamp(measured,120.0,1200.0)-Speed)*.3;
            LastRow=rows;LastEventTime=rowTime;
        }
        if(elapsed<=0)return Distance;
        const double phase=std::clamp(now-LastEventTime,0.0,.14);
        const double target=rows*60.0+Speed*phase;
        Distance+=Speed*elapsed;
        Distance+=(target-Distance)*(1-std::exp(-8*elapsed));
        return Distance;
    }
    double Position()const{return Distance;}
private:
    double Distance=0,Speed=600,LastEventTime=0;
    int LastRow=0;
};
}
