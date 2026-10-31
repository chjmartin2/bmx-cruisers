#include "CruiserWorld.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void ACruiserWorld::StartMusic(){
    auto Loop=[&](const TCHAR* File,UAudioComponent* Component,TObjectPtr<USoundWaveProcedural>& Stream,TArray<uint8>& PCM,ESoundGroup Group){
        if(!FFileHelper::LoadFileToArray(PCM,*(FPaths::ProjectContentDir()/TEXT("Audio")/File)))return;
        Stream=NewObject<USoundWaveProcedural>(this);Stream->SetSampleRate(48000);Stream->NumChannels=2;
        Stream->Duration=INDEFINITELY_LOOPING_DURATION;Stream->SoundGroup=Group;
        Stream->QueueAudio(PCM.GetData(),PCM.Num());Component->SetSound(Stream);Component->SetVolumeMultiplier(0);Component->Play();
    };
    Loop(TEXT("course1-studio.pcm"),Music,MusicStream,MusicPCM,SOUNDGROUP_Music);
    Loop(TEXT("rolling.pcm"),Rolling,RollingStream,RollingPCM,SOUNDGROUP_Effects);
    Loop(TEXT("park.pcm"),Ambience,AmbienceStream,AmbiencePCM,SOUNDGROUP_Effects);
    EffectsStream=NewObject<USoundWaveProcedural>(this);EffectsStream->SetSampleRate(48000);EffectsStream->NumChannels=2;
    EffectsStream->Duration=INDEFINITELY_LOOPING_DURATION;EffectsStream->SoundGroup=SOUNDGROUP_Effects;
    Effects->SetSound(EffectsStream);
    CelebrationStream=NewObject<USoundWaveProcedural>(this);CelebrationStream->SetSampleRate(48000);CelebrationStream->NumChannels=2;CelebrationStream->Duration=INDEFINITELY_LOOPING_DURATION;
    FFileHelper::LoadFileToArray(PopPCM,*(FPaths::ProjectContentDir()/TEXT("Audio/firework.pcm")));CelebrationAudio->SetSound(CelebrationStream);
    UpdateAudio();
}

void ACruiserWorld::PlayEffect(const FString& Name){
    if(EffectsMuted||!EffectsStream)return;
    TArray<uint8> PCM;
    if(!FFileHelper::LoadFileToArray(PCM,*(FPaths::ProjectContentDir()/TEXT("Audio")/(Name+TEXT(".pcm")))))return;
    Effects->Stop();EffectsStream->ResetAudio();EffectsStream->QueueAudio(PCM.GetData(),PCM.Num());Effects->SetVolumeMultiplier(.7);Effects->Play();
}

void ACruiserWorld::UpdateAudio(){
    auto Refill=[](USoundWaveProcedural* Stream,const TArray<uint8>& PCM){if(Stream&&PCM.Num()&&Stream->GetAvailableAudioByteCount()<96000)Stream->QueueAudio(PCM.GetData(),PCM.Num());};
    Refill(MusicStream,MusicPCM);Refill(RollingStream,RollingPCM);Refill(AmbienceStream,AmbiencePCM);
    bool Riding=Simulation->GetState()==BMX::RunState::Riding&&!Paused;
    Music->SetVolumeMultiplier(Muted?0:Paused?.12:IsMenu()?.32:.25);
    Rolling->SetVolumeMultiplier(EffectsMuted||!Riding?0:.40);
    Rolling->SetPitchMultiplier(FMath::Clamp(float(ActiveSpeed),.65f,1.4f));
    Ambience->SetVolumeMultiplier(EffectsMuted?0:Paused?.08:.17);
    Effects->SetVolumeMultiplier(EffectsMuted?0:.7);
    CelebrationAudio->SetVolumeMultiplier(EffectsMuted?0:.38);
    if(CelebrationTime>=0&&CelebrationTime>NextFireworkPop&&PopPCM.Num()){
        NextFireworkPop=CelebrationTime+.8f;CelebrationStream->QueueAudio(PopPCM.GetData(),PopPCM.Num());if(!CelebrationAudio->IsPlaying())CelebrationAudio->Play();
    }
    if(CelebrationTime<0&&CelebrationAudio->IsPlaying()){CelebrationAudio->Stop();CelebrationStream->ResetAudio();}
}
