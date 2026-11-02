#include "CruiserWorld.h"
#include "ProceduralMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Materials/MaterialInterface.h"

namespace {
struct Geometry {TArray<FVector> V,N;TArray<int32>I;TArray<FVector2D>UV;TArray<FLinearColor>C;};
int Vertex(Geometry& G,FVector P,FVector N,FVector2D UV,FLinearColor C){int Index=G.V.Num();G.V.Add(P);G.N.Add(N);G.UV.Add(UV);G.C.Add(C);return Index;}
}

// Interpolate the actual meadow triangles, including their diagonal. The
// road/bridge deck is at zero; shoreline fences must never follow the waterbed.
float ACruiserWorld::FenceGroundHeight(float X,float Y) const{
    constexpr int Columns=136;
    if(GroundHeights.Num()<Columns*2)return 0;
    int Rows=GroundHeights.Num()/Columns;
    float U=FMath::Clamp((Y+2025)/30,0.f,float(Columns-1.001));
    float V=FMath::Clamp((X+750)/30,0.f,float(Rows-1.001));
    int C=int(U),R=int(V);U-=C;V-=R;
    auto Z=[&](int Col,int Row){return GroundHeights[Row*Columns+Col];};
    float A=Z(C,R),B=Z(C+1,R),D=Z(C+1,R+1),E=Z(C,R+1);
    return FMath::Max(0.f,U>=V?A+(B-A)*U+(D-B)*V:A+(E-A)*V+(D-E)*U);
}

void ACruiserWorld::DressCourse(const TSharedPtr<FJsonObject>& Data,const TSharedPtr<FJsonObject>& Visual){
    FRandomStream Random(65021985);
    TMap<int32,Geometry> Timber,PoolRails,Foliage,Stones;
    auto Wood=[&](FVector A,FVector B,float Radius,bool Pool=false){
        auto& G=(Pool?PoolRails:Timber).FindOrAdd(FMath::FloorToInt(A.X/1200));int Base=G.V.Num();
        FVector Axis=(B-A).GetSafeNormal(),U=FVector::CrossProduct(Axis,FVector::UpVector).GetSafeNormal();
        if(U.IsNearlyZero())U=FVector::RightVector;FVector V=FVector::CrossProduct(Axis,U).GetSafeNormal();
        float Phase=Random.FRandRange(0,6.28),Shade=Random.FRandRange(.67,1.05);const int Rings=5,Sides=8;
        for(int r=0;r<=Rings;++r)for(int s=0;s<=Sides;++s){
            float T=float(r)/Rings,Angle=s*2*PI/Sides+PI/8;
            FVector N=U*FMath::Cos(Angle)+V*FMath::Sin(Angle);
            float Warp=Pool?1:1+.10*FMath::Sin(s*2.7+r*.8+Phase)+.045*FMath::Cos(r*4.1+Phase);
            FVector P=FMath::Lerp(A,B,T)+N*Radius*Warp+V*FMath::Sin(T*PI)*Radius*.12;
            Vertex(G,P,N,FVector2D((B-A).Size()*T/110.f,s/8.f),FLinearColor(Shade,Shade,Shade,1));
            if(r<Rings&&s<Sides){int Q=Base+r*(Sides+1)+s;G.I.Append({Q,Q+Sides+1,Q+1,Q+1,Q+Sides+1,Q+Sides+2});}
        }
        for(int End=0;End<2;++End){int Center=Vertex(G,End?B:A,End?Axis:-Axis,FVector2D(.5,.5),FLinearColor(Shade*.85,Shade*.85,Shade*.85,1));int Ring=Base+End*Rings*(Sides+1);for(int s=0;s<Sides;++s)if(End)G.I.Append({Center,Ring+s,Ring+s+1});else G.I.Append({Center,Ring+s+1,Ring+s});}
    };
    for(auto Value:Data->GetArrayField(TEXT("fences"))){
        auto P=Value->AsObject();float X=P->GetNumberField(TEXT("y"))*60,Y=(P->GetNumberField(TEXT("x"))-20)*60;
        int Code=P->GetIntegerField(TEXT("code"));bool Bridge=P->GetBoolField(TEXT("bridge"));
        bool Pool=false;
        for(auto Item:Visual->GetObjectField(TEXT("site_features"))->GetArrayField(TEXT("pools"))){auto B=Item->AsObject()->GetArrayField(TEXT("bounds"));FBox2D Region(FVector2D(B[1]->AsNumber()*7.5,(B[0]->AsNumber()-160)*7.5),FVector2D(B[3]->AsNumber()*7.5,(B[2]->AsNumber()-160)*7.5));Pool|=Region.ExpandBy(130).IsInsideOrOn(FVector2D(X,Y));}
        bool H=Code==74||Code==77||Code==78||Code==79||Code==82;
        bool V=Code==72||Code==73||Code==75||Code==76||Code==80||Code==81||Code==83||Bridge;
        // Bridge timber is a solid hazard too. Use the visible endpoints and
        // thickness, leaving the riding gap between the two rails open.
        auto Contact=[&](FVector2D A,FVector2D B,float Radius){FVisibleObstacle O;O.A=A;O.B=B;O.Radius=FVector2D(Radius);VisibleObstacles.Add(O);};
        if(H)Contact(FVector2D(X,Y-30),FVector2D(X,Y+30),3);
        if(V)Contact(FVector2D(X-30,Y),FVector2D(X+30,Y),Bridge?7:3);
        if(Code!=77)Contact(FVector2D(X,Y),FVector2D(X,Y),Bridge?6:5);
        // Original endpoints and openings are retained. Only the timber surface varies.
        float Low=36+Random.FRandRange(-1.5,1.5),High=68+Random.FRandRange(-2,2);
        if(H)for(float Z:{Low,High})Wood(FVector(X,Y-30,FenceGroundHeight(X,Y-30)+Z),FVector(X,Y+30,FenceGroundHeight(X,Y+30)+Z+Random.FRandRange(-1,1)),Pool?2:4,Pool);
        if(V)for(float Z:{Low,High})Wood(FVector(X-30,Y,FenceGroundHeight(X-30,Y)+Z),FVector(X+30,Y,FenceGroundHeight(X+30,Y)+Z+Random.FRandRange(-1,1)),Bridge?7:Pool?2:4,Pool);
        if(Code!=77)Wood(FVector(X,Y,FenceGroundHeight(X,Y)-1),FVector(X+Random.FRandRange(-2,2),Y+Random.FRandRange(-2,2),FenceGroundHeight(X,Y)+Random.FRandRange(83,99)),Pool?2.5:6,Pool);
        if(Pool)for(float Offset:{-22.5f,-7.5f,7.5f,22.5f}){
            if(H)Wood(FVector(X,Y+Offset,FenceGroundHeight(X,Y+Offset)+7),FVector(X,Y+Offset,FenceGroundHeight(X,Y+Offset)+78),1,true);
            if(V)Wood(FVector(X+Offset,Y,FenceGroundHeight(X+Offset,Y)+7),FVector(X+Offset,Y,FenceGroundHeight(X+Offset,Y)+78),1,true);
        }
    }
    for(auto Value:Data->GetArrayField(TEXT("shrubs"))){
        auto P=Value->AsObject();if(P->GetIntegerField(TEXT("color"))==1)continue;
        float X=P->GetNumberField(TEXT("y"))*60,Y=(P->GetNumberField(TEXT("x"))-20)*60;
        float W=P->GetNumberField(TEXT("width"))*60,D=P->GetNumberField(TEXT("depth"))*60;
        auto& G=Foliage.FindOrAdd(FMath::FloorToInt(X/1200));
        float Height=FMath::Clamp(FMath::Min(W,D)*Random.FRandRange(.65,.90),42.f,95.f),Hue=Random.FRandRange(.80,1.12);
        int Count=GraphicsPreset==0?96:GraphicsPreset==1?144:FMath::Clamp(int(W*D/8),280,1000);
        if(GraphicsPreset<2){
            // A small opaque crown keeps the entire hazard readable even when
            // the optional scanned bushes and most individual leaves are off.
            const int Base=G.V.Num(),Rings=6,Sides=12;
            for(int R=0;R<=Rings;++R)for(int S=0;S<=Sides;++S){
                float A=S*2*PI/Sides,Polar=R*PI/Rings;
                FVector N(FMath::Sin(Polar)*FMath::Cos(A),FMath::Sin(Polar)*FMath::Sin(A),FMath::Cos(Polar));
                float Lobe=1+.08f*FMath::Sin(A*3+X);
                FVector Position=FVector(X,Y,Height*.43f)+N*FVector(D*.36f*Lobe,W*.36f*Lobe,Height*.45f);
                Vertex(G,Position,N,FVector2D(S/float(Sides),R/float(Rings)),FLinearColor(.05f,.12f,.018f,1));
                if(R<Rings&&S<Sides){int Q=Base+R*(Sides+1)+S;G.I.Append({Q,Q+1,Q+Sides+1,Q+1,Q+Sides+2,Q+Sides+1});}
            }
        }
        for(int i=0;i<Count;++i){
            float Angle=Random.FRandRange(0,2*PI),Radius=FMath::Sqrt(Random.FRandRange(.02,1));
            float Lobe=1+.14*FMath::Sin(Angle*5+X)+.08*FMath::Cos(Angle*7+Y);
            FVector Center(X+FMath::Cos(Angle)*D*.44*Radius*Lobe,Y+FMath::Sin(Angle)*W*.44*Radius*Lobe,Height*(.23+.70*FMath::Sqrt(1-Radius*Radius))+Random.FRandRange(-6,6));
            FRotator Turn(Random.FRandRange(-42,42),Random.FRandRange(0,360),Random.FRandRange(-30,30));
            FVector U=Turn.RotateVector(FVector::ForwardVector),V=Turn.RotateVector(FVector::RightVector),N=FVector::CrossProduct(U,V);
            float L=Random.FRandRange(3,6.5)*(GraphicsPreset==0?2.f:GraphicsPreset==1?1.5f:1.f),B=L*Random.FRandRange(.32,.48),Tint=Random.FRandRange(.70,1.35);
            FLinearColor Color(.065*Tint*Hue,.155*Tint,.026*Tint/Hue,1);
            int Base=G.V.Num();
            Vertex(G,Center-U*L,N,FVector2D(.5,0),Color);Vertex(G,Center-V*B,N,FVector2D(0,.46),Color);
            Vertex(G,Center+U*L,N,FVector2D(.5,1),Color);Vertex(G,Center+V*B,N,FVector2D(1,.46),Color);
            Vertex(G,Center+N*L*.18,N,FVector2D(.5,.46),Color);
            G.I.Append({Base,Base+4,Base+1,Base+1,Base+4,Base+2,Base+2,Base+4,Base+3,Base+3,Base+4,Base});
        }
    }
    TArray<uint8> Cells;FFileHelper::LoadFileToArray(Cells,*(CourseFile(TEXT("-terrain.bin"))));
    int Height=Data->GetIntegerField(TEXT("height"))*8;
    // Low gravel and leaf litter add wear, never new collision or visual barriers.
    for(int i=0;i<(GraphicsPreset==0?1400:GraphicsPreset==1?6000:16000);++i){
        int PX=Random.RandRange(0,319),PY=Random.RandRange(0,Height-1),Index=PY*320+PX;
        if(!Cells.IsValidIndex(Index)||Cells[Index]!=1||IsOriginalSite(PY*7.5,(PX-160)*7.5))continue;
        float X=PY*7.5,Y=(PX-160)*7.5;
        auto& G=Stones.FindOrAdd(FMath::FloorToInt(X/1200));float Size=Random.FRandRange(.4,1.7),Shade=Random.FRandRange(.13,.35);
        int B=G.V.Num();FLinearColor C(Shade,Shade*.94,Shade*.82,1);
        for(int j=0;j<5;++j){float A=j*2*PI/5;Vertex(G,FVector(X+FMath::Cos(A)*Size,Y+FMath::Sin(A)*Size,.16),FVector::UpVector,FVector2D(0,0),C);}
        Vertex(G,FVector(X,Y,Random.FRandRange(.35,1.1)),FVector::UpVector,FVector2D(.5,.5),C);
        for(int j=0;j<5;++j)G.I.Append({B+j,B+5,B+(j+1)%5});
    }
    auto Publish=[&](TMap<int32,Geometry>& Chunks,UMaterialInterface* Material,bool Shadow){for(auto& Entry:Chunks){
        auto& G=Entry.Value;if(!G.V.Num())continue;
        auto* Mesh=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(Mesh);Mesh->SetupAttachment(Scene);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCastShadow(Shadow);Mesh->RegisterComponent();
        Mesh->CreateMeshSection_LinearColor(0,G.V,G.I,G.N,G.UV,G.C,TArray<FProcMeshTangent>(),false);Mesh->SetMaterial(0,Material);
        Mesh->SetCullDistance(7500);
    }};
    Publish(Timber,Materials[3],true);
    Publish(PoolRails,Materials[6],true);
    Publish(Foliage,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Leaf.M_Leaf")),true);
    Publish(Stones,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Gravel.M_Gravel")),false);
}
