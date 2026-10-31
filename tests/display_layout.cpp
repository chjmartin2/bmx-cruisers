#include "Core/DisplayLayout.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    for(auto size:{BMX::DisplaySize{1920,1040},BMX::DisplaySize{3840,2080},BMX::DisplaySize{2560,1400}})for(double dpi:{1.,1.5,2.}){
        auto window=BMX::FitWindow(size.Width,size.Height,dpi);
        assert(window.Width+32*dpi<=size.Width&&window.Height+64*dpi<=size.Height);
        assert(std::abs(double(window.Width)/window.Height-16./9.)<.005);
    }
    for(auto size:{BMX::DisplaySize{1920,1080},BMX::DisplaySize{3840,2160},BMX::DisplaySize{2560,1080},BMX::DisplaySize{1280,1024}})for(double dpi:{1.,1.5,2.}){
        BMX::HUDLayout ui(size.Width,size.Height,dpi);
        assert(ui.PixelX(0)>=0&&ui.PixelY(0)>=0);
        assert(ui.PixelX(1920)<=size.Width+.01&&ui.PixelY(1080)<=size.Height+.01);
        for(double x:{80.,344.,730.,1920.})assert(std::abs(ui.DesignX(ui.PixelX(x))-x)<1e-6);
        for(double y:{0.,560.,802.,1080.})assert(std::abs(ui.DesignY(ui.PixelY(y))-y)<1e-6);
    }
    assert(BMX::RenderPercentage(1920,1080,true)==200);
    assert(BMX::RenderPercentage(3840,2160,true)==100);
    assert(BMX::RenderPercentage(3840,2160,false)==50);
    assert(BMX::RenderPercentage(1600,900,true)==240);
    std::cout<<"Single-monitor window bounds, 100/150/200% DPI, 1080p/4K/ultrawide/tall HUD bounds and click mapping, independent render scale passed\n";
}
