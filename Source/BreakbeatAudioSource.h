/*
  ==============================================================================

    BreakbeatAudioSource.h
    Created: 5 Jul 2020 9:54:10pm
    Author:  Matthew

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ReferenceCountedForwardAndReverseBuffer.h"

class PositionableRegionAudioSource
: public juce::PositionableAudioSource
{
    virtual void setEndReadPosition (int64 newPosition) = 0;
    virtual int64 getEndReadPosition() const = 0;
};

class BreakbeatAudioSource
: public PositionableRegionAudioSource
{
public:
    BreakbeatAudioSource();
    ~BreakbeatAudioSource() override;
    
    int64_t getNumSamples() const;
    int getNumSlices() const;
    int64_t getSliceSize() const;
    int64_t getStartReadPosition() const;
    
    juce::AudioSampleBuffer* getCurrentBuffer();
    
    void setSampleChangeThreshold(float threshold);
    void setReverseSampleThreshold(float threshold);
    
    void setBlockDivisionFactor(double factor);
    
    void setSampleBpm(double bpm);
    
    void toggleRandomPosition();
    void toggleRandomDirection();
    
    // juce::AudioAppComponent
    void prepareToPlay (int, double) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    
    // juce::PositionableAudioSource
    void setNextReadPosition (int64 newPosition) override;
    int64 getNextReadPosition() const override;
    int64 getTotalLength() const override;
    bool isLooping() const override;
    
    // PositionableRegionAudioSource
    void setEndReadPosition (int64 newPosition) override;
    int64 getEndReadPosition() const override;
    
    void setReader(juce::AudioFormatReader* reader);
    void setDirectionFowards(bool status);
    
    void clearFreeBuffers();
    void clear();
    
    void calculateAudioBlocks();
    
private:
    
    std::atomic<int64_t> mNextReadPosition {0};
    std::atomic<int64_t> mStartReadPosition {0};
    std::atomic<int64_t> mEndReadPosition {0};
    
    std::atomic<float> mSampleChangeThreshold {0.7};
    std::atomic<float> mReverseSampleThreshold {0.7};
    
    std::atomic<bool> mRandomPosition {false};
    std::atomic<bool> mRandomDirection {false};
    
    std::atomic<int> mNumSlices {1};
    std::atomic<int> mSampleToEndOn;
    std::atomic<int> mSliceSampleSize {1}; // in samples
    std::atomic<int> mBlockIdx {0};
    std::atomic<int> mBpm {120};
    std::atomic<double> mBlockDivisionPower {1.0}; // This should be stored as powers of 2 (whole = 1, half = 2, quarter = 4 etc)
    
    double mDuration = 44100;
    
    juce::ReferenceCountedArray<ReferenceCountedForwardAndReverseBuffer> mBuffers;
    ReferenceCountedForwardAndReverseBuffer::Ptr mCurrentBuffer;
};
