/*
  ==============================================================================

    ReferenceCountedForwardAndReverseBuffer.cpp
    Created: 4 Jul 2020 5:31:02pm
    Author:  Matthew

  ==============================================================================
*/

#include "ReferenceCountedForwardAndReverseBuffer.h"

ReferenceCountedForwardAndReverseBuffer::ReferenceCountedForwardAndReverseBuffer(const juce::String& nameToUse, juce::AudioFormatReader* formatReader)
: mName(nameToUse)
, mForwardBuffer(formatReader->numChannels, static_cast<int>(formatReader->lengthInSamples))
, mReverseBuffer(formatReader->numChannels, static_cast<int>(formatReader->lengthInSamples))
{
    std::cout << "Buffer named: '" << mName << "' constructed. numChannels: " << formatReader->numChannels << ", numSamples" << formatReader->lengthInSamples << "\n";
    formatReader->read(&mForwardBuffer, 0, static_cast<int>(formatReader->lengthInSamples), 0, true, true);
    
    for(auto ch = 0; ch < mReverseBuffer.getNumChannels(); ++ch)
    {
        mReverseBuffer.copyFrom(ch, 0, mForwardBuffer, ch, 0, mReverseBuffer.getNumSamples());
        mReverseBuffer.reverse(ch, 0, mReverseBuffer.getNumSamples());
    }
    
    mActiveBuffer = &mForwardBuffer;
}

ReferenceCountedForwardAndReverseBuffer::~ReferenceCountedForwardAndReverseBuffer()
{
    std::cout << "Buffer named: '" << mName << "' destroyed." << "\n";
}

int ReferenceCountedForwardAndReverseBuffer::getPosition() const
{
    return mPosition.load();
}

void ReferenceCountedForwardAndReverseBuffer::setPosition(int pos)
{
    mPosition.exchange(pos);
}

void ReferenceCountedForwardAndReverseBuffer::updateCurrentSampleBuffer(float reverseThreshold)
{
    auto reversePerc = Random::getSystemRandom().nextFloat();
    mActiveBuffer = reversePerc > reverseThreshold ? &mReverseBuffer : &mForwardBuffer;
}

juce::AudioSampleBuffer* ReferenceCountedForwardAndReverseBuffer::getCurrentAudioSampleBuffer()
{
    return mActiveBuffer;
}

juce::AudioSampleBuffer* ReferenceCountedForwardAndReverseBuffer::getForwardAudioSampleBuffer()
{
    return &mForwardBuffer;
}
