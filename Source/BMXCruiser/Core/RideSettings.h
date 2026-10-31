#pragma once
#include <algorithm>
namespace BMX {
struct RideSettings {
    static constexpr int Count=5;
    static constexpr int Normal=2;
    static double Speed(int index){constexpr double speeds[]={.65,.8,1.,1.2,1.4};return speeds[std::clamp(index,0,Count-1)];}
    static double SteeringScale(int index){return 1./Speed(index);}
};
}
