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

int64_t BreakbeatAudioSource::getNumSamples()
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

int64_t BreakbeatAudioSource::getStartReadPosition()
{
    return mStartReadPosition.load();
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
    ReferenceCountedForwardAndReverseBuffer::Ptr retainedBuffer(mCurrentBuffer);
    
    if(retainedBuffer == nullptr)
    {
        return;
    }
    
    auto const numChannels = bufferToFill.buffer->getNumChannels();
    auto const numSamples = bufferToFill.buffer->getNumSamples();
    auto const outputStart = bufferToFill.startSample;
    
    auto sourceStart = mStartReadPosition.load();
    auto sourceEnd = mEndReadPosition.load();
    auto const readPosition = mNextReadPosition.load();
    
    auto blockIdx = mBlockIdx.load();
    auto blockSampleSize = mBlockSampleSize.load();
    auto numAudioBlocks = mNumAudioBlocks.load();
    
    auto const randomPosition = mRandomPosition.load();
    
    auto* currentAudioSampleBuffer = retainedBuffer->getCurrentAudioSampleBuffer();
    
    auto samplesRemaining = numSamples;
    auto innerStart = readPosition;
    auto innerBufferSample = outputStart;
    while(samplesRemaining > 0)
    {
        auto newPos = innerStart == sourceEnd;
        innerStart = innerStart == sourceEnd ? sourceStart : innerStart;
        auto const innerEnd = std::min(sourceEnd, innerStart + static_cast<int64_t>(samplesRemaining));
        auto const numThisTime = innerEnd - innerStart;
        
        for(auto ch = 0; ch < numChannels; ++ch)
        {
            bufferToFill.buffer->copyFrom(ch, innerBufferSample, *currentAudioSampleBuffer, ch, static_cast<int>(innerStart), static_cast<int>(numThisTime));
        }
        
        if(newPos)
        {
            if(randomPosition)
            {
                auto changePerc = Random::getSystemRandom().nextFloat();
                mBlockIdx = changePerc > mSampleChangeThreshold ? static_cast<int>(Random::getSystemRandom().nextInt(numAudioBlocks)) : blockIdx;
                auto nextPos = static_cast<int64_t>(std::min(blockSampleSize * blockIdx, static_cast<int>(getNumSamples()) - blockSampleSize));
                auto endPos = std::min(static_cast<int>(nextPos) + blockSampleSize, static_cast<int>(getNumSamples()));
                setNextReadPosition(nextPos);
                setEndReadPosition(endPos);
                
                sourceStart = nextPos;
                sourceEnd = endPos;
                innerStart = nextPos;
            }
            else
            {
                setNextReadPosition(0);
                setEndReadPosition(getNumSamples());
                
                sourceStart = getStartReadPosition();
                sourceEnd = getEndReadPosition();
                innerStart = getNextReadPosition();
            }
            
            newPos = false;
            
            continue;
        }
        
        innerStart += numThisTime;
        innerBufferSample += numThisTime;
        samplesRemaining -= numThisTime;
    }
    
    mNextReadPosition = innerStart;
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
            
    mCurrentBuffer = newBuffer;
    mBuffers.add(mCurrentBuffer);

    mNextReadPosition = 0;
    mEndReadPosition = static_cast<int>(reader->lengthInSamples);
    
    mDuration = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
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

void BreakbeatAudioSource::calculateAudioBlocks(int bpm)
{
    auto const bps = bpm / 60.0;
    mNumAudioBlocks = std::max(bps * mDuration / static_cast<double>(mBlockDivisionPower), 1.0);
    mBlockSampleSize = roundToInt(getNumSamples() / mNumAudioBlocks);
}
