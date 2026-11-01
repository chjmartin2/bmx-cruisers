#include "CruiserWorld.h"

// A common tyre/vehicle footprint for every cosmetic ride style. Contacts are
// evaluated after presentation smoothing, so the visible bike is authoritative.
bool ACruiserWorld::VisibleContact(FVector Position)const{
 constexpr float Radius=18,HalfWheelbase=55;
 auto Cross=[](FVector2D A,FVector2D B){return A.X*B.Y-A.Y*B.X;};
 for(float Offset:{-HalfWheelbase,0.f,HalfWheelbase}){
  FVector2D P(Position.X+Offset,Position.Y);
  const auto* Waters=WaterContactBuckets.Find(FMath::FloorToInt(P.X/300));
  if(Waters)for(int Index:*Waters){const auto& T=WaterContacts[Index];
   if(!T.Bounds.IsInsideOrOn(P))continue;
   double A=Cross(T.B-T.A,P-T.A),B=Cross(T.C-T.B,P-T.B),C=Cross(T.A-T.C,P-T.C);
   if((A>=0&&B>=0&&C>=0)||(A<=0&&B<=0&&C<=0))return true;
  }
  const auto* Obstacles=ObstacleContactBuckets.Find(FMath::FloorToInt(P.X/300));
  if(Obstacles)for(int Index:*Obstacles){const auto& O=VisibleObstacles[Index];
   if(O.Ellipse){FVector2D D=P-O.A;double X=D.X/(O.Radius.X+Radius),Y=D.Y/(O.Radius.Y+Radius);if(X*X+Y*Y<=1)return true;}
   else {FVector2D V=O.B-O.A;double U=FMath::Clamp(FVector2D::DotProduct(P-O.A,V)/FMath::Max(1.,V.SizeSquared()),0.,1.);if((P-(O.A+V*U)).SizeSquared()<=FMath::Square(O.Radius.X+Radius))return true;}
  }
 }
 return false;
}
bool ACruiserWorld::SweepVisibleContact(FVector From,FVector To)const{
 // Subdivide travel, not time: slow render frames cannot tunnel through a fence.
 int Steps=FMath::Max(1,FMath::CeilToInt(FVector::Dist2D(From,To)/10));
 for(int I=0;I<=Steps;++I)if(VisibleContact(FMath::Lerp(From,To,double(I)/Steps)))return true;
 return false;
}
