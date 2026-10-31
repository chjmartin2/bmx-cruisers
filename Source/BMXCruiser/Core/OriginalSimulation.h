#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include "../ThirdParty/m6502.h"

namespace BMX {
enum class RunState { Ready, Riding, Crashed, Complete, Invalid };

// Original instructions drive the 2D state. Unreal only presents this state in 3D.
// This adapter models the subset of VIC/CIA/Kernal services used by this game;
// it is not a general Commodore 64 emulator.
class OriginalSimulation {
public:
    static constexpr double ClockHz = 985248.0;
    static constexpr double CentimetersPerPixel = 7.5;
    bool Load(const uint8_t* ram, size_t size, const uint8_t* io, size_t ioSize);
    void Reset();
    void Start() { if (State == RunState::Ready) State = RunState::Riding; }
    void SetInput(bool left, bool right, bool up, bool down);
    // 1 leaves the original C64 execution untouched. Other paces compensate
    // lateral steps inside the simulation, so collision and presentation agree.
    void SetSteeringScale(double scale);
    void Advance(double seconds);
    void TickCycles(uint64_t cycles);
    int Score() const;
    int SpriteX() const { return SteeringStepActive ? SteeringStartX : IO[0] + ((IO[0x10] & 1) ? 256 : 0); }
    int SpriteY() const { return IO[1]; }
    int SpriteFrame() const { return RAM[0x7f8]; }
    double ForwardPixels() const { return ScrollRows * 8.0 + 192.0 - (SpriteY() - 51 + 10.5); }
    double RightPixels() const { return SpriteX() - 24.0 + 12.0 - 160.0; }
    int Rows() const { return ScrollRows; }
    RunState GetState() const { return State; }
    const std::array<uint8_t, 65536>& Memory() const { return RAM; }
    uint64_t Cycles() const { return Ticks; }
    uint64_t LastScrollCycle() const { return LastScrollTick; }
    uint16_t PC() const { return Cpu.PC; }
private:
    std::array<uint8_t, 65536> Source{}, RAM{};
    std::array<uint8_t, 4096> SourceIO{}, IO{};
    m6502_t Cpu{};
    uint64_t Pins = 0, Ticks = 0, NextIRQ = 16421;
    uint64_t LastScrollTick=0;
    double FractionalCycles = 0;
    double SteeringScale=1,SteeringFraction=0;
    int SteeringStartX=0,SteeringDirection=0;
    bool SteeringStepActive=false;
    uint8_t Joystick = 0xff, Collision = 0;
    int ScrollRows = 0;
    bool Loaded = false, IRQActive = false;
    int ServiceDelay = 0;
    RunState State = RunState::Invalid;
    uint8_t Read(uint16_t address);
    void Write(uint16_t address, uint8_t value);
    void Prefetch(uint16_t address);
    void CheckRasterCollision(int line);
    bool BackgroundSolid(int x, int y) const;
    void FinishSteeringStep();
};
}
