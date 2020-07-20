/*
  ==============================================================================

    ReferenceCountedForwardAndReverseBuffer.h
    Created: 4 Jul 2020 5:31:02pm
    Author:  Matthew

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ReferenceCountedForwardAndReverseBuffer
: public juce::ReferenceCountedObject
{
public:
    typedef juce::ReferenceCountedObjectPtr<ReferenceCountedForwardAndReverseBuffer> Ptr;
    
    ReferenceCountedForwardAndReverseBuffer(const juce::String& nameToUse, juce::AudioFormatReader* formatReader);
    ~ReferenceCountedForwardAndReverseBuffer();
    
    int getPosition() const;
    void setPosition(int pos);
    
    void updateCurrentSampleBuffer(float reverseThreshold);
    
    juce::AudioSampleBuffer* getCurrentAudioSampleBuffer();
    juce::AudioSampleBuffer* getForwardAudioSampleBuffer();
    
private:
    juce::String mName;
    juce::AudioSampleBuffer mForwardBuffer;
    juce::AudioSampleBuffer mReverseBuffer;
    
    juce::AudioSampleBuffer* mActiveBuffer;
    
    std::atomic<int> mPosition {0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReferenceCountedForwardAndReverseBuffer)
};
