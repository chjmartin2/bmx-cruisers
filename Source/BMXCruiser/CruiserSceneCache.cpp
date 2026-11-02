#include "Core/SceneCache.h"
#include "Misc/FileHelper.h"
#include "Misc/Compression.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"

bool LoadCruiserSceneCache(const FString& Path,FCruiserSceneCache& Out){
 TArray<uint8> File;if(!FFileHelper::LoadFileToArray(File,*Path)||File.Num()<16||FMemory::Memcmp(File.GetData(),"BMXSCN02",8))return false;
 uint32 Size,CRC;FMemory::Memcpy(&Size,File.GetData()+8,4);FMemory::Memcpy(&CRC,File.GetData()+12,4);
 if(Size>128*1024*1024||Size<4)return false;
 TArray<uint8> Data;Data.SetNumUninitialized(Size);
 if(!FCompression::UncompressMemory(NAME_Zlib,Data.GetData(),Size,File.GetData()+16,File.Num()-16)||FCrc::MemCrc32(Data.GetData(),Size)!=CRC)return false;
 FMemoryReader R(Data,true);uint32 Fields=0;R<<Fields;if(Fields>32)return false;
 for(uint32 F=0;F<Fields;++F){
  uint32 Length=0;R<<Length;if(Length>64||R.Tell()+Length>R.TotalSize())return false;
  TArray<ANSICHAR> Label;Label.SetNumZeroed(Length+1);R.Serialize(Label.GetData(),Length);FString Name(UTF8_TO_TCHAR(Label.GetData()));
  uint32 Count=0;R<<Count;if(Count>512)return false;
  auto& Chunks=Out.Add(Name);Chunks.SetNum(Count);
  for(auto& C:Chunks){
   uint32 V=0,I=0;R<<V;R<<I;if(V>2000000||I>12000000||I%3||R.Tell()+int64(V)*60+int64(I)*4>R.TotalSize())return false;
   C.Vertices.SetNumUninitialized(V);C.Normals.SetNumUninitialized(V);C.UV.SetNumUninitialized(V);C.Colors.SetNumUninitialized(V);C.Indices.SetNumUninitialized(I);
   for(uint32 J=0;J<V;++J){double P[3];float A[9];R.Serialize(P,sizeof(P));R.Serialize(A,sizeof(A));for(double X:P)if(!FMath::IsFinite(X))return false;for(float X:A)if(!FMath::IsFinite(X))return false;C.Vertices[J]=FVector(P[0],P[1],P[2]);C.Normals[J]=FVector(A[0],A[1],A[2]);C.UV[J]=FVector2D(A[3],A[4]);C.Colors[J]=FLinearColor(A[5],A[6],A[7],A[8]);}
   R.Serialize(C.Indices.GetData(),I*4);for(int32 Index:C.Indices)if(Index<0||uint32(Index)>=V)return false;
  }
 }
 return !R.IsError()&&R.Tell()==R.TotalSize();
}
