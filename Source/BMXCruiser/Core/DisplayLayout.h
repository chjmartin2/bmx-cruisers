#pragma once
#include <algorithm>
#include <cmath>
namespace BMX {
struct DisplaySize {int Width,Height;};
inline DisplaySize FitWindow(int workWidth,int workHeight,double dpi){
    const double width=std::max(320.,workWidth-32*std::max(1.,dpi));
    const double height=std::max(180.,workHeight-64*std::max(1.,dpi));
    const double fit=std::min({1.,width/1920.,height/1080.});
    return {int(std::floor(1920*fit/2))*2,int(std::floor(1080*fit/2))*2};
}
struct HUDLayout {
    double Scale,X,Y,DPI;
    HUDLayout(int width,int height,double dpi):DPI(std::max(1.,dpi)){
        const double w=width/DPI,h=height/DPI;
        Scale=std::min(w/1920.,h/1080.);X=(w-1920*Scale)*.5;Y=(h-1080*Scale)*.5;
    }
    double PixelX(double x)const{return (X+x*Scale)*DPI;}
    double PixelY(double y)const{return (Y+y*Scale)*DPI;}
    double DesignX(double x)const{return (x/DPI-X)/Scale;}
    double DesignY(double y)const{return (y/DPI-Y)/Scale;}
};
inline double RenderPercentage(int width,int height,bool uhd){
    double viewHeight=std::min(double(height),width*9./16.);
    return std::clamp(100*(uhd?2160.:1080.)/std::max(viewHeight,1.),25.,400.);
}
}
