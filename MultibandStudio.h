#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"

using namespace iplug;
using namespace igraphics;

enum EParams {
  kXover1 = 0, kXover2, kXover3,
  kLowThreshold, kLowRatio, kLowAttack, kLowRelease, kLowMakeup,
  kLowMidThreshold, kLowMidRatio, kLowMidAttack, kLowMidRelease, kLowMidMakeup,
  kHighMidThreshold, kHighMidRatio, kHighMidAttack, kHighMidRelease, kHighMidMakeup,
  kHighThreshold, kHighRatio, kHighAttack, kHighRelease, kHighMakeup,
  kNumParams
};

class MultibandStudio final : public Plugin {
public:
  MultibandStudio(const InstanceInfo& info);
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void OnIdle() override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;
  void OnUIOpen() override;

private:
  struct Biquad {
    double b0=1.0, b1=0.0, b2=0.0, a1=0.0, a2=0.0, z1=0.0, z2=0.0;
    void Set(double sampleRate, double cutoff, bool lowpass);
    double Process(double x);
    void Reset() { z1 = z2 = 0.0; }
  };
  struct Splitter {
    Biquad low[2], high[2];
    void Set(double sr, double fc);
    double Low(double x);
    double High(double x);
  };
  Splitter mSplit[3][2];
  double mEnvelope[4][2] = {};
  ISpectrumSender<2> mSender;
  void UpdateFilters();
  double Compress(double x, int band, int channel);
};
