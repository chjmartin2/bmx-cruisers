#pragma once
namespace BMX {
struct RunRules {
    static bool CanContinue(bool game,int course,int cleared,bool complete) {
        return complete&&course>=1&&course<5&&(!game||cleared==course-1);
    }
    static bool Qualifies(bool game,int course,int cleared,bool complete,int pace,int startingPace) {
        return game&&complete&&course==5&&cleared==4&&pace==startingPace;
    }
};
}
