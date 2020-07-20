/*
  ==============================================================================

    BreakbeatAudioSource.cpp
    Created: 5 Jul 2020 9:54:10pm
    Author:  Matthew

  ==============================================================================
*/

#include "BreakbeatAudioSource.h"

BreakbeatAudioSource::BreakbeatAudioSource()
{
    
}

BreakbeatAudioSource::~BreakbeatAudioSource()
{
    
}

int64_t BreakbeatAudioSource::getNumSamples() const
{
    ReferenceCountedForwardAndReverseBuffer::Ptr retainedBuffer(mCurrentBuffer);
    if(retainedBuffer == nullptr)
    {
        return 0;
    }
    
    auto* currentAudioBuffer = retainedBuffer->getCurrentAudioSampleBuffer();
    if(currentAudioBuffer == nullptr)
    {
        return 0;
    }
    
    return currentAudioBuffer->getNumSamples();
}

int BreakbeatAudioSource::getNumSlices() const
{
    return mNumSlices.load();
}

int64_t BreakbeatAudioSource::getSliceSize() const
{
    return mSliceSampleSize.load();
}

int64_t BreakbeatAudioSource::getStartReadPosition() const
{
    return mStartReadPosition.load();
}

juce::AudioSampleBuffer* BreakbeatAudioSource::getCurrentBuffer()
{
    ReferenceCountedForwardAndReverseBuffer::Ptr retainedBuffer(mCurrentBuffer);
    if(retainedBuffer == nullptr)
    {
        return nullptr;
    }
    
    return retainedBuffer->getForwardAudioSampleBuffer();
}

void BreakbeatAudioSource::setSampleChangeThreshold(float threshold)
{
    mSampleChangeThreshold.exchange(threshold);
}

void BreakbeatAudioSource::setReverseSampleThreshold(float threshold)
{
    mReverseSampleThreshold.exchange(threshold);
}

void BreakbeatAudioSource::setBlockDivisionFactor(double factor)
{
    mBlockDivisionPower.exchange(factor);
}

void BreakbeatAudioSource::setSampleBpm(double bpm)
{
    mBpm.exchange(bpm);
    calculateAudioBlocks();
}

void BreakbeatAudioSource::toggleRandomDirection()
{
    auto const status = mRandomDirection.load();
    mRandomDirection.exchange(!status);
}

void BreakbeatAudioSource::toggleRandomPosition()
{
    auto const status = mRandomDirection.load();
    mRandomPosition.exchange(!status);
}

void BreakbeatAudioSource::prepareToPlay (int, double)
{
    
}

void BreakbeatAudioSource::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    
    ReferenceCountedForwardAndReverseBuffer::Ptr retainedBuffer(mCurrentBuffer);
    
    if(retainedBuffer == nullptr)
    {
        return;
    }
    
    auto const numChannels = bufferToFill.buffer->getNumChannels();
    auto const numSamples = bufferToFill.numSamples;
    auto const outputStart = bufferToFill.startSample;
    
    auto sliceStartPosition = mStartReadPosition.load();
    auto sliceEndPosition = mEndReadPosition.load();
    auto const sliceSampleSize = mSliceSampleSize.load();
    auto const numSlices = mNumSlices.load();
    auto const sliceChangeThreshold = mSampleChangeThreshold.load();
    auto const sliceReverseThreshold = mReverseSampleThreshold.load();
    
    jassert(sliceSampleSize > 0);

    auto samplesRemaining = numSamples;
    auto const currentPosition = mNextReadPosition.load();
    
    auto readBufferStart = currentPosition;
    while(samplesRemaining > 0)
    {
        auto const changePerc = Random::getSystemRandom().nextFloat();
        bool const atSliceEnd = readBufferStart == sliceEndPosition;
        bool const willChange = atSliceEnd && changePerc > sliceChangeThreshold;
        
        sliceStartPosition = willChange ? (numSlices > 1 ? Random::getSystemRandom().nextInt(numSlices) * sliceSampleSize : 0) : sliceStartPosition;
        sliceEndPosition = willChange ? sliceStartPosition + sliceSampleSize : sliceEndPosition;
        if(willChange)
        {
            retainedBuffer->updateCurrentSampleBuffer(sliceReverseThreshold);
        };
        
        jassert(sliceEndPosition >= sliceStartPosition);
        
        readBufferStart = atSliceEnd ? sliceStartPosition : readBufferStart;
        auto const readBufferEnd = std::min(sliceEndPosition, readBufferStart + static_cast<int64_t>(samplesRemaining));
        jassert(readBufferStart >= 0 && readBufferEnd >= readBufferStart);
        
        auto const numThisTime = std::min(static_cast<int>(readBufferEnd - readBufferStart), samplesRemaining);
        for(auto ch = 0; ch < numChannels; ++ch)
        {
            bufferToFill.buffer->copyFrom(ch, outputStart, *retainedBuffer->getCurrentAudioSampleBuffer(), ch, static_cast<int>(readBufferStart), static_cast<int>(numThisTime));
        }
        
        samplesRemaining -= numThisTime;
        readBufferStart += numThisTime;
    }
    
    mNextReadPosition.exchange(readBufferStart);
    mStartReadPosition.exchange(sliceStartPosition);
    mEndReadPosition.exchange(sliceEndPosition);
}

void BreakbeatAudioSource::releaseResources()
{
    mCurrentBuffer = nullptr;
}

void BreakbeatAudioSource::setNextReadPosition (int64 newPosition)
{
    mNextReadPosition = newPosition;
    mStartReadPosition = newPosition;
}

int64 BreakbeatAudioSource::getNextReadPosition() const
{
    return mNextReadPosition.load();
}

int64 BreakbeatAudioSource::getTotalLength() const
{
    return 0.0;
}

bool BreakbeatAudioSource::isLooping() const
{
    return true;
}

// PositionableRegionAudioSource
void BreakbeatAudioSource::setEndReadPosition (int64 newPosition)
{
    mEndReadPosition = newPosition;
}

int64 BreakbeatAudioSource::getEndReadPosition() const
{
    return mEndReadPosition.load();
}

void BreakbeatAudioSource::setReader(juce::AudioFormatReader* reader)
{
    ReferenceCountedForwardAndReverseBuffer::Ptr newBuffer = new ReferenceCountedForwardAndReverseBuffer("", reader);
    jassert(newBuffer != nullptr);
            
    mCurrentBuffer = newBuffer;
    mBuffers.add(mCurrentBuffer);

    mNextReadPosition = 0;
    mEndReadPosition = static_cast<int>(reader->lengthInSamples);
    
    mDuration = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
    
    calculateAudioBlocks();
}

void BreakbeatAudioSource::setDirectionFowards(bool status)
{
    
}

void BreakbeatAudioSource::clearFreeBuffers()
{
    for(auto i = mBuffers.size(); --i >= 0;)
    {
        ReferenceCountedForwardAndReverseBuffer::Ptr buffer(mBuffers.getUnchecked(i));
        
        if(buffer->getReferenceCount() == 2)
        {
            mBuffers.remove(i);
        }
    }
}

void BreakbeatAudioSource::clear()
{
    mCurrentBuffer = nullptr;
}

void BreakbeatAudioSource::calculateAudioBlocks()
{
    auto const bps = mBpm / 60.0;
    mNumSlices = std::max(bps * mDuration / static_cast<double>(mBlockDivisionPower), 1.0);
    mSliceSampleSize = roundToInt(getNumSamples() / mNumSlices);
    
    jassert(mNumSlices > 0);
    
    setNextReadPosition(0);
    setEndReadPosition(mSliceSampleSize);
}
