#include "MultibandStudio.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr int kSpectrumTag = 100;
const IColor kBg(255, 13, 16, 23);
const IColor kPanel(255, 23, 28, 38);
const IColor kText(255, 225, 232, 242);
const IColor kMuted(255, 120, 133, 153);
const IColor kBand[4] = {
  IColor(255, 55, 190, 255), IColor(255, 67, 220, 180),
  IColor(255, 255, 183, 77), IColor(255, 255, 91, 125)
};
}

void MultibandStudio::Biquad::Set(double sr, double fc, bool lp)
{
  fc = std::max(10.0, std::min(fc, sr * 0.45));
  const double w = 2.0 * M_PI * fc / sr;
  const double c = std::cos(w), s = std::sin(w);
  const double alpha = s / std::sqrt(2.0); // Butterworth Q
  double nb0, nb1, nb2;
  if (lp) { nb0=(1-c)*0.5; nb1=1-c; nb2=(1-c)*0.5; }
  else { nb0=(1+c)*0.5; nb1=-(1+c); nb2=(1+c)*0.5; }
  const double a0 = 1.0 + alpha;
  b0=nb0/a0; b1=nb1/a0; b2=nb2/a0;
  a1=(-2.0*c)/a0; a2=(1.0-alpha)/a0;
}

double MultibandStudio::Biquad::Process(double x)
{
  const double y = b0*x + z1;
  z1 = b1*x - a1*y + z2;
  z2 = b2*x - a2*y;
  return y;
}

void MultibandStudio::Splitter::Set(double sr, double fc)
{
  for (int i=0;i<2;i++) { low[i].Set(sr,fc,true); high[i].Set(sr,fc,false); }
}
double MultibandStudio::Splitter::Low(double x) { return low[1].Process(low[0].Process(x)); }
double MultibandStudio::Splitter::High(double x) { return high[1].Process(high[0].Process(x)); }

MultibandStudio::MultibandStudio(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, 1))
{
  GetParam(kXover1)->InitDouble("Low / Low-Mid", 120.0, 40.0, 500.0, 1.0, "Hz");
  GetParam(kXover2)->InitDouble("Low-Mid / High-Mid", 1000.0, 250.0, 5000.0, 1.0, "Hz");
  GetParam(kXover3)->InitDouble("High-Mid / High", 6000.0, 1500.0, 16000.0, 1.0, "Hz");
  const int base[4] = {kLowThreshold,kLowMidThreshold,kHighMidThreshold,kHighThreshold};
  const char* names[4] = {"Low","Low-Mid","High-Mid","High"};
  for (int b=0;b<4;b++) {
    GetParam(base[b])->InitDouble((std::string(names[b])+" Threshold").c_str(), -24.0, -60.0, 0.0, 0.1, "dB");
    GetParam(base[b]+1)->InitDouble((std::string(names[b])+" Ratio").c_str(), 3.0, 1.0, 20.0, 0.1, ":1");
    GetParam(base[b]+2)->InitDouble((std::string(names[b])+" Attack").c_str(), 10.0, 0.1, 100.0, 0.1, "ms");
    GetParam(base[b]+3)->InitDouble((std::string(names[b])+" Release").c_str(), 120.0, 10.0, 1000.0, 1.0, "ms");
    GetParam(base[b]+4)->InitDouble((std::string(names[b])+" Makeup").c_str(), 0.0, -12.0, 18.0, 0.1, "dB");
  }
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() { return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT)); };
  mLayoutFunc = [&](IGraphics* g) {
    g->AttachPanelBackground(kBg);
    g->EnableMouseOver(true);
      IVStyle style = DEFAULT_STYLE
      .WithColor(kBG, kPanel).WithColor(kFG, kText)
      .WithColor(kPR, IColor(255, 76, 150, 255))
      .WithLabelText(DEFAULT_LABEL_TEXT.WithFGColor(kMuted).WithSize(12.f))
      .WithValueText(DEFAULT_VALUE_TEXT.WithFGColor(kText).WithSize(13.f));
    const IRECT b = g->GetBounds();
    g->AttachControl(new ITextControl(IRECT(28,12,600,54), "MULTIBAND  /  STUDIO", IText(26, kText)));
    g->AttachControl(new ITextControl(IRECT(28,48,500,72), "FOUR-BAND DYNAMICS PROCESSOR", IText(11, kMuted)));
    g->AttachControl(new IVSpectrumAnalyzerControl<2>(IRECT(28,82,1072,340), "LIVE SPECTRUM", style), kSpectrumTag);
    const char* xo[3] = {"XOVER 1","XOVER 2","XOVER 3"};
    const int xp[3] = {kXover1,kXover2,kXover3};
    for (int i=0;i<3;i++) {
      float x = 80.f + i*150.f;
      g->AttachControl(new IVKnobControl(IRECT(x,355,x+92,455), xp[i], xo[i], style));
    }
    const int params[4][5] = {
      {kLowThreshold,kLowRatio,kLowAttack,kLowRelease,kLowMakeup},
      {kLowMidThreshold,kLowMidRatio,kLowMidAttack,kLowMidRelease,kLowMidMakeup},
      {kHighMidThreshold,kHighMidRatio,kHighMidAttack,kHighMidRelease,kHighMidMakeup},
      {kHighThreshold,kHighRatio,kHighAttack,kHighRelease,kHighMakeup}
    };
    const char* labels[5] = {"THRESH","RATIO","ATTACK","RELEASE","MAKEUP"};
    for (int band=0;band<4;band++) {
      float left = 28.f + band*264.f;
      IRECT panel(left,475,left+248.f,735);
      g->FillRoundRect(kPanel,panel,10.f);
      g->AttachControl(new ITextControl(IRECT(left+14,482,left+230,510), (std::string("BAND 0")+char('1'+band)).c_str(), IText(15,kBand[band])));
      for (int j=0;j<5;j++) {
        int col=j%3, row=j/3;
        float x=left+8.f+col*78.f, y=514.f+row*106.f;
        g->AttachControl(new IVKnobControl(IRECT(x,y,x+72.f,y+94.f),params[band][j],labels[j],style));
      }
    }
  };
#endif
}

void MultibandStudio::UpdateFilters()
{
  const double sr = GetSampleRate() > 0.0 ? GetSampleRate() : 44100.0;
  const double f[3] = {GetParam(kXover1)->Value(),GetParam(kXover2)->Value(),GetParam(kXover3)->Value()};
  for (int c=0;c<2;c++) for (int i=0;i<3;i++) mSplit[i][c].Set(sr,f[i]);
}

void MultibandStudio::OnReset()
{
  UpdateFilters();
  for (auto& band : mEnvelope) for (double& e : band) e=0.0;
  mSender.SetFFTSize(2048);
}

double MultibandStudio::Compress(double x, int band, int channel)
{
  const int p[4] = {kLowThreshold,kLowMidThreshold,kHighMidThreshold,kHighThreshold};
  const int base=p[band];
  const double level=std::fabs(x);
  const double sr=GetSampleRate()>0.0?GetSampleRate():44100.0;
  const double attack=std::max(0.0001,GetParam(base+2)->Value()*0.001);
  const double release=std::max(0.001,GetParam(base+3)->Value()*0.001);
  const double coeff=std::exp(-1.0/(sr*(level>mEnvelope[band][channel]?attack:release)));
  mEnvelope[band][channel]=coeff*mEnvelope[band][channel]+(1.0-coeff)*level;
  const double db=20.0*std::log10(std::max(1.0e-9,mEnvelope[band][channel]));
  const double threshold=GetParam(base)->Value();
  const double ratio=std::max(1.0,GetParam(base+1)->Value());
  const double reduction=db>threshold ? (threshold + (db-threshold)/ratio - db) : 0.0;
  const double makeup=GetParam(base+4)->Value();
  return x*std::pow(10.0,(reduction+makeup)/20.0);
}

void MultibandStudio::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const int nIn=NInChansConnected(), nOut=NOutChansConnected();
  if (nIn<1 || nOut<1) return;
  mSender.ProcessBlock(inputs,nFrames,kSpectrumTag,nIn);
  UpdateFilters();
  for (int s=0;s<nFrames;s++) {
    for (int c=0;c<nOut;c++) {
      const int ch=std::min(c,1);
      const double x=inputs[std::min(c,nIn-1)][s];
      const double lo=mSplit[0][ch].Low(x);
      const double r1=mSplit[0][ch].High(x);
      const double lm=mSplit[1][ch].Low(r1);
      const double r2=mSplit[1][ch].High(r1);
      const double hm=mSplit[2][ch].Low(r2);
      const double hi=mSplit[2][ch].High(r2);
      const double y=Compress(lo,0,ch)+Compress(lm,1,ch)+Compress(hm,2,ch)+Compress(hi,3,ch);
      outputs[c][s]=static_cast<sample>(y);
    }
  }
}

bool MultibandStudio::OnMessage(int msgTag,int,int dataSize,const void* pData)
{
  if (msgTag==IVSpectrumAnalyzerControl<>::kMsgTagFFTSize) {
    mSender.SetFFTSize(*reinterpret_cast<const int*>(pData)); return true;
  }
  if (msgTag==IVSpectrumAnalyzerControl<>::kMsgTagOverlap) {
    mSender.SetFFTSizeAndOverlap(mSender.GetFFTSize(),*reinterpret_cast<const int*>(pData)); return true;
  }
  if (msgTag==IVSpectrumAnalyzerControl<>::kMsgTagWindowType) {
    mSender.SetWindowType(static_cast<ISpectrumSender<2>::EWindowType>(*reinterpret_cast<const int*>(pData))); return true;
  }
  return false;
}

void MultibandStudio::OnIdle() { mSender.TransmitData(*this); }
void MultibandStudio::OnUIOpen()
{
  const double sr=GetSampleRate();
  const int fft=mSender.GetFFTSize(), overlap=mSender.GetOverlap();
  const int window=static_cast<int>(mSender.GetWindowType());
  SendControlMsgFromDelegate(kSpectrumTag,IVSpectrumAnalyzerControl<>::kMsgTagSampleRate,sizeof(double),&sr);
  SendControlMsgFromDelegate(kSpectrumTag,IVSpectrumAnalyzerControl<>::kMsgTagFFTSize,sizeof(int),&fft);
  SendControlMsgFromDelegate(kSpectrumTag,IVSpectrumAnalyzerControl<>::kMsgTagOverlap,sizeof(int),&overlap);
  SendControlMsgFromDelegate(kSpectrumTag,IVSpectrumAnalyzerControl<>::kMsgTagWindowType,sizeof(int),&window);
}
