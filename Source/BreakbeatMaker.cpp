/*
  ==============================================================================

    BreakbeatMaker.cpp
    Created: 4 Jul 2020 5:39:51pm
    Author:  Matthew

  ==============================================================================
*/

#include "BreakbeatMaker.h"

MainContentComponent::WaveformComponent::WaveformComponent(MainContentComponent& parent, juce::AudioFormatManager& formatManager)
: mParentComponent(parent)
, mAudioFormatManager(formatManager)
, mThumbnailCache(5)
, mThumbnail(512, mAudioFormatManager, mThumbnailCache)
{
}

MainContentComponent::WaveformComponent::~WaveformComponent()
{
    
}


juce::AudioThumbnail& MainContentComponent::WaveformComponent::getThumbnail()
{
    return mThumbnail;
}

void MainContentComponent::WaveformComponent::setSampleStartEnd(int start, int end)
{
    mStartSample = std::max(start, 0);
    mEndSample = std::min(end, static_cast<int>(mThumbnail.getTotalLength() * mSampleRate));
    
    triggerAsyncUpdate();
}

void MainContentComponent::WaveformComponent::resized()
{
    
}

void MainContentComponent::WaveformComponent::paint(juce::Graphics& g)
{
    juce::Rectangle<int> thumbnailBounds (10, 10, getWidth()-20, getHeight()-20);
    
    if(mThumbnail.getNumChannels() == 0)
    {
        g.setColour(juce::Colours::darkgrey);
        g.fillRect(thumbnailBounds);
        g.setColour(juce::Colours::white);
        g.drawFittedText("Drag and drop and audio file", thumbnailBounds, juce::Justification::centred, 1);
    }
    else
    {
        g.setColour(juce::Colours::white);
        g.fillRect(thumbnailBounds);
        g.setColour(juce::Colours::red);
        mThumbnail.drawChannels(g, thumbnailBounds, 0.0, mThumbnail.getTotalLength(), 1.0f);
    }
    
    juce::Range<int> sampleRange { mStartSample, mEndSample };
    if(sampleRange.getLength() == 0)
    {
        return;
    }
    
    auto const sampleStartRatio = static_cast<double>(sampleRange.getStart() / mSampleRate) / mThumbnail.getTotalLength();
    auto const sampleSizeRatio = static_cast<double>(sampleRange.getLength() / mSampleRate) / mThumbnail.getTotalLength();
    
    juce::Rectangle<int> clipBounds {
        thumbnailBounds.getX() + static_cast<int>(thumbnailBounds.getWidth() * sampleStartRatio),
        thumbnailBounds.getY(),
        static_cast<int>(thumbnailBounds.getWidth() * sampleSizeRatio),
        thumbnailBounds.getHeight()
    };
    
    g.setColour(juce::Colours::blue.withAlpha(0.4f));
    g.fillRect(clipBounds);
}

bool MainContentComponent::WaveformComponent::isInterestedInFileDrag (const StringArray& files)
{
    for(auto fileName : files)
    {
        if(!fileName.endsWith(".wav") && !fileName.endsWith(".aif") && !fileName.endsWith(".aiff"))
            return false;
    }
    
    return true;
}

void MainContentComponent::WaveformComponent::filesDropped (const StringArray& files, int x, int y)
{
    // only deal with one file for now.
    juce::ignoreUnused(x, y);
    
    auto const filePath = files[0];
    juce::File f { filePath };
    
    if(f.existsAsFile())
    {
        auto path = f.getFullPathName();
        mParentComponent.newFileOpened(path);
    }
}

void MainContentComponent::WaveformComponent::handleAsyncUpdate()
{
    repaint();
}

MainContentComponent::MainContentComponent()
: juce::Thread("Background Thread")
{
    addAndMakeVisible (mClearButton);
    mClearButton.setButtonText ("Clear");
    mClearButton.onClick = [this] { clearButtonClicked(); };
    
    addAndMakeVisible (mRandomSlicesToggle);
    mRandomSlicesToggle.setButtonText("Random Slices");
    mRandomSlicesToggle.onClick = [this] { mRandomPosition = !mRandomPosition; };
    
    addAndMakeVisible(mmSampleBPMLabel);
    mmSampleBPMLabel.setText("Sample BPM: ", dontSendNotification);
    mmSampleBPMLabel.attachToComponent(&mmSampleBPMField, true);
    mmSampleBPMLabel.setEditable(false);
    mmSampleBPMLabel.setJustificationType(Justification::right);
    mmSampleBPMLabel.setColour(Label::textColourId, Colours::white);
    
    addAndMakeVisible(mmSampleBPMField);
    mmSampleBPMField.setText("120", dontSendNotification);
    mmSampleBPMField.setColour(Label::textColourId, Colours::white);
    mmSampleBPMField.setEditable(true);
    mmSampleBPMField.onTextChange = [this]
    {
        mSampleBPM = mmSampleBPMField.getText().getIntValue();
        calculateAudioBlocks();
    };
    mmSampleBPMField.onEditorShow = [this]
    {
        auto* ed = mmSampleBPMField.getCurrentTextEditor();
        ed->setInputRestrictions(3, "1234567890");
    };
    
    addAndMakeVisible(mSliceSizeLabel);
    mSliceSizeLabel.setText("Slice size: ", dontSendNotification);
    mSliceSizeLabel.setColour(Label::textColourId, Colours::white);
    mSliceSizeLabel.setEditable(false);
    mSliceSizeLabel.attachToComponent(&mSliceSizeDropDown, true);
    mSliceSizeLabel.setJustificationType(Justification::right);
    
    addAndMakeVisible(mSliceSizeDropDown);
    mSliceSizeDropDown.addItemList({"1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/128"}, 1);
    mSliceSizeDropDown.setSelectedId(3);
    mSliceSizeDropDown.onChange = [this]
    {
        auto selectionId = mSliceSizeDropDown.getSelectedId();
        
        switch(selectionId)
        {
            case 1:
                mBlockDivisionPower = 4;
                break;
            case 2:
                mBlockDivisionPower = 2;
                break;
            default:
                mBlockDivisionPower = 1.0 / std::pow(2, selectionId - 3);
                break;
        }
        
        calculateAudioBlocks();
    };
    
    addAndMakeVisible(mChangeSampleProbabilityLabel);
    mChangeSampleProbabilityLabel.setText("Swap %: ", dontSendNotification);
    mChangeSampleProbabilityLabel.setColour(Label::textColourId, Colours::white);
    mChangeSampleProbabilityLabel.setEditable(false);
    mChangeSampleProbabilityLabel.attachToComponent(&mChangeSampleProbabilitySlider, true);
    mChangeSampleProbabilityLabel.setJustificationType(Justification::right);
    
    addAndMakeVisible(mChangeSampleProbabilitySlider);
    mChangeSampleProbabilitySlider.setRange(0.0, 1.0, 0.1);
    mChangeSampleProbabilitySlider.setValue(0.3, dontSendNotification);
    mChangeSampleProbabilitySlider.onValueChange = [this]()
    {
        mSampleChangeThreshold = 1.0 - mChangeSampleProbabilitySlider.getValue();
    };
    
    addAndMakeVisible(mReverseSampleProbabilityLabel);
    mReverseSampleProbabilityLabel.setText("Reverse %: ", dontSendNotification);
    mReverseSampleProbabilityLabel.setColour(Label::textColourId, Colours::white);
    mReverseSampleProbabilityLabel.setEditable(false);
    mReverseSampleProbabilityLabel.attachToComponent(&mReverseSampleProbabilitySlider, true);
    mReverseSampleProbabilityLabel.setJustificationType(Justification::right);
    
    addAndMakeVisible(mReverseSampleProbabilitySlider);
    mReverseSampleProbabilitySlider.setRange(0.0, 1.0, 0.1);
    mReverseSampleProbabilitySlider.setValue(0.3, dontSendNotification);
    mReverseSampleProbabilitySlider.onValueChange = [this]()
    {
        mReverseSampleThreshold = 1.0 - mReverseSampleProbabilitySlider.getValue();
    };
    
    addAndMakeVisible(mWaveformComponent);
    
    setSize (500, 440);

    mFormatManager.registerBasicFormats();
    setAudioChannels (0, 2);
    
    mWaveformComponent.getThumbnail().addChangeListener(this);
    
    startThread();
}

MainContentComponent::~MainContentComponent()
{
    stopThread (4000);
    shutdownAudio();
}

void MainContentComponent::resized()
{
    mClearButton.setBounds (10, 10, getWidth() - 20, 20);
    mRandomSlicesToggle.setBounds(10, 40, getWidth() - 20, 20);
    mmSampleBPMField.setBounds(100, 70, getWidth() - 120, 20);
    mSliceSizeDropDown.setBounds(100, 100, getWidth() - 120, 20);
    mChangeSampleProbabilitySlider.setBounds(100, 130, getWidth() - 120, 20);
    mReverseSampleProbabilitySlider.setBounds(100, 160, getWidth() - 120, 20);
    
    mWaveformComponent.setBounds(10, 220, getWidth() - 20, 200);
}

void MainContentComponent::paint(juce::Graphics& g)
{
    
}

void MainContentComponent::prepareToPlay (int, double)
{
    
}

void MainContentComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    ReferenceCountedForwardAndReverseBuffer::Ptr retainedBuffer(mCurrentBuffer);
    
    if(retainedBuffer == nullptr)
    {
        return;
    }
    
    auto sampleToEndOn = mSampleToEndOn.load();
    auto blockIdx = mBlockIdx.load();
    auto const blockSampleSize = mBlockSampleSize.load();
    auto const numAudioBlocks = mNumAudioBlocks.load();
    auto position = retainedBuffer->getPosition(); // loads atomically
    
    auto* currentAudioSampleBuffer = retainedBuffer->getCurrentAudioSampleBuffer();
    
    auto const numInputChannels = currentAudioSampleBuffer->getNumChannels();
    auto const numOutputChannels = bufferToFill.buffer->getNumChannels();
    
    auto outputSamplesRemaining = bufferToFill.numSamples;
    auto outputSamplesOffset = bufferToFill.startSample;
    
    bool changeDirection = false;
    
    while(outputSamplesRemaining > 0)
    {
        auto bufferSamplesRemaining = std::max(sampleToEndOn - position, 0);
        auto samplesThisTime = std::min(outputSamplesRemaining, bufferSamplesRemaining);
        
        for(auto ch = 0; ch < numOutputChannels; ++ch)
        {
            bufferToFill.buffer->copyFrom(ch, outputSamplesOffset, *currentAudioSampleBuffer, ch % numInputChannels, position, samplesThisTime);
        }
        
        outputSamplesRemaining -= samplesThisTime;
        outputSamplesOffset += samplesThisTime;
        position += samplesThisTime;
        
        if(position >= sampleToEndOn)
        {
            if(mRandomPosition)
            {
                auto changePerc = Random::getSystemRandom().nextFloat();
                blockIdx = changePerc > mSampleChangeThreshold ? Random::getSystemRandom().nextInt(numAudioBlocks) : blockIdx;
                    
                // Move that many blocks along the fileBuffer
                position = std::min(blockSampleSize * blockIdx, currentAudioSampleBuffer->getNumSamples());
                // TODO check why it was out of range in the first place. Thread issues?
                sampleToEndOn = std::min(position + blockSampleSize, currentAudioSampleBuffer->getNumSamples());
                
                mWaveformComponent.setSampleStartEnd(position, sampleToEndOn);
            }
            else
            {
                position = 0;
                sampleToEndOn = currentAudioSampleBuffer->getNumSamples();
            }
            
            changeDirection = true;
        }
    }
    
    mSampleToEndOn.exchange(sampleToEndOn);
    mBlockIdx.exchange(blockIdx);
    retainedBuffer->setPosition(position);
    
    if(changeDirection)
    {
        changeDirection = false;
        retainedBuffer->updateCurrentSampleBuffer(mReverseSampleThreshold);
    }
}

void MainContentComponent::releaseResources()
{
    mCurrentBuffer = nullptr;
}

void MainContentComponent::run()
{
    while(!threadShouldExit())
    {
        checkForPathToOpen();
        checkForBuffersToFree();
        wait(500);
    }
}

void MainContentComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if(source == &mWaveformComponent.getThumbnail())
    {
        repaint();
    }
}

void MainContentComponent::handleAsyncUpdate()
{
    if(mFileSource != nullptr)
    {
        mWaveformComponent.getThumbnail().setSource(mFileSource.get());
        mFileSource.release();
    }
}

void MainContentComponent::newFileOpened(String& filePath)
{
    mChosenPath.swapWith(filePath);
    notify();
}

void MainContentComponent::clearButtonClicked()
{
    mCurrentBuffer = nullptr;
    mWaveformComponent.getThumbnail().clear();
}

void MainContentComponent::checkForPathToOpen()
{
    juce::String pathToOpen;
    pathToOpen.swapWith(mChosenPath);
    
    if(pathToOpen.isEmpty())
    {
        return;
    }
    
    juce::File file(pathToOpen);
    std::unique_ptr<AudioFormatReader> reader { mFormatManager.createReaderFor(file) };
    
    if(reader != nullptr)
    {
        // get length
        mDuration = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
        
        if(mDuration < MAX_FILE_LENGTH)
        {
            ReferenceCountedForwardAndReverseBuffer::Ptr newBuffer = new ReferenceCountedForwardAndReverseBuffer(file.getFileName(), reader.get());
            
            mCurrentBuffer = newBuffer;
            mBuffers.add(mCurrentBuffer);

            mSampleToEndOn = static_cast<int>(reader->lengthInSamples);
            calculateAudioBlocks();
            
            mFileSource = std::make_unique<juce::FileInputSource>(file);
            triggerAsyncUpdate();
        }
        else
        {
             juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
                                          juce::translate("Audio file too long!"),
                                               juce::translate("The file is ") + juce::String(mDuration) + juce::translate("seconds long. ") + juce::String(MAX_FILE_LENGTH) + "Second limit!");
        }
        
    }
}

void MainContentComponent::calculateAudioBlocks()
{
    ReferenceCountedForwardAndReverseBuffer::Ptr retainedBuffer(mCurrentBuffer);
    if(retainedBuffer == nullptr)
    {
        mNumAudioBlocks = 1;
        
        // TODO: What was my intention here...? Its odd mBlockSampleSize = 0?
        mBlockSampleSize = 0;
        return;
    }
    
    auto* currentAudioBuffer = retainedBuffer->getCurrentAudioSampleBuffer();
    if(currentAudioBuffer == nullptr)
    {
        mNumAudioBlocks = 1;
                   
        // TODO: What was my intention here...? Its odd mBlockSampleSize = 0?
        mBlockSampleSize = 0;
        return;
    }

    auto const numSrcSamples = currentAudioBuffer->getNumSamples();
    if(numSrcSamples == 0)
    {
        mNumAudioBlocks = 1;
        
        // TODO: What was my intention here...? Its odd mBlockSampleSize = 0?
        mBlockSampleSize = retainedBuffer->getCurrentAudioSampleBuffer()->getNumSamples();
        return;
    }
    
    // length, bpm
    auto bps = mSampleBPM / 60.0;
    mNumAudioBlocks = std::max(bps * mDuration / static_cast<double>(mBlockDivisionPower), 1.0);
    mBlockSampleSize = roundToInt(numSrcSamples / mNumAudioBlocks);
}

void MainContentComponent::checkForBuffersToFree()
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
