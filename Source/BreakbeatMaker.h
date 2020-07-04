/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2017 - ROLI Ltd.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             LoopingAudioSampleBufferTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Explores audio sample buffer looping.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2017, linux_make

 type:             Component
 mainClass:        MainContentComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

#include <iostream>
#include "ReferenceCountedForwardAndReverseBuffer.h"
//==============================================================================

#define MAX_FILE_LENGTH 15.0 // seconds

class MainContentComponent
: public juce::AudioAppComponent
, private juce::Thread
, private juce::ChangeListener
, private juce::AsyncUpdater
{
public:
    MainContentComponent()
    : juce::Thread("Background Thread")
    {
        addAndMakeVisible (mOpenButton);
        mOpenButton.setButtonText ("Open...");
        mOpenButton.onClick = [this] { mOpenButtonClicked(); };

        addAndMakeVisible (mClearButton);
        mClearButton.setButtonText ("Clear");
        mClearButton.onClick = [this] { mClearButtonClicked(); };
        
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
        
        setSize (500, 440);

        mFormatManager.registerBasicFormats();
        setAudioChannels (0, 2);
        mThumbnail.addChangeListener(this);
        
        startThread();
    }

    ~MainContentComponent() override
    {
        stopThread (4000);
        shutdownAudio();
    }
    
    // juce::Thread
    void run() override
    {
        while(!threadShouldExit())
        {
            checkForPathToOpen();
            checkForBuffersToFree();
            wait(500);
        }
    }
    
    void checkForBuffersToFree()
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
    
    void checkForPathToOpen()
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

    void prepareToPlay (int, double) override
    {
        
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        ReferenceCountedForwardAndReverseBuffer::Ptr retainedBuffer(mCurrentBuffer);
        
        if(retainedBuffer == nullptr)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }
        
        auto* currentAudioSampleBuffer = retainedBuffer->getCurrentAudioSampleBuffer();
        auto position = retainedBuffer->getPosition();
        auto const numInputChannels = currentAudioSampleBuffer->getNumChannels();
        auto const numOutputChannels = bufferToFill.buffer->getNumChannels();
        
        auto outputSamplesRemaining = bufferToFill.numSamples;
        auto outputSamplesOffset = bufferToFill.startSample;
        
        bool changeDirection = false;
        
        while(outputSamplesRemaining > 0)
        {
            auto bufferSamplesRemaining = mSampleToEndOn - position;
            auto samplesThisTime = std::min(outputSamplesRemaining, bufferSamplesRemaining);
            
            for(auto ch = 0; ch < numOutputChannels; ++ch)
            {
                bufferToFill.buffer->copyFrom(ch, outputSamplesOffset, *currentAudioSampleBuffer, ch % numInputChannels, position, samplesThisTime);
            }
            
            outputSamplesRemaining -= samplesThisTime;
            outputSamplesOffset += samplesThisTime;
            position += samplesThisTime;
            
            if(position == mSampleToEndOn)
            {
                if(mRandomPosition)
                {
                    auto changePerc = Random::getSystemRandom().nextFloat();
                    mBlockIdx = changePerc > mSampleChangeThreshold ? Random::getSystemRandom().nextInt(mNumAudioBlocks) : mBlockIdx;
                        
                    // Move that many blocks along the fileBuffer
                    position = mBlockSampleSize * mBlockIdx;
                    mSampleToEndOn = position + mBlockSampleSize;
                }
                else
                {
                    position = 0;
                    mSampleToEndOn = currentAudioSampleBuffer->getNumSamples();
                }
                
                changeDirection = true;
            }
        }
        
        retainedBuffer->setPosition(position);
        
        if(changeDirection)
        {
            changeDirection = false;
            retainedBuffer->updateCurrentSampleBuffer(mReverseSampleThreshold);
        }
    }

    void releaseResources() override
    {
        mCurrentBuffer = nullptr;
    }

    void resized() override
    {
        mOpenButton.setBounds (10, 10, getWidth() - 20, 20);
        mClearButton.setBounds (10, 40, getWidth() - 20, 20);
        mRandomSlicesToggle.setBounds(10, 70, getWidth() - 20, 20);
        mmSampleBPMField.setBounds(100, 100, getWidth() - 120, 20);
        mSliceSizeDropDown.setBounds(100, 130, getWidth() - 120, 20);
        mChangeSampleProbabilitySlider.setBounds(100, 160, getWidth() - 120, 20);
        mReverseSampleProbabilitySlider.setBounds(100, 190, getWidth() - 120, 20);
    }
    
    void paint(juce::Graphics& g) override
    {
        juce::Rectangle<int> thumbnailBounds (10, 220, getWidth()-20, 220);
        
        if(mThumbnail.getNumChannels() == 0)
        {
            g.setColour(juce::Colours::darkgrey);
            g.fillRect(thumbnailBounds);
            g.setColour(juce::Colours::white);
            g.drawFittedText("No file loaded", thumbnailBounds, juce::Justification::centred, 1);
        }
        else
        {
            g.setColour(juce::Colours::white);
            g.fillRect(thumbnailBounds);
            g.setColour(juce::Colours::red);
            mThumbnail.drawChannels(g, thumbnailBounds, 0.0, mThumbnail.getTotalLength(), 1.0f);
        }
    }
    
    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if(source == &mThumbnail)
        {
            repaint();
        }
    }
    
    void handleAsyncUpdate() override
    {
        if(mFileSource != nullptr)
        {
            mThumbnail.setSource(mFileSource.get());
            mFileSource.release();
        }
    }

private:
    void mOpenButtonClicked()
    {
        juce::FileChooser chooser {"Select an audio file shorter than " + juce::String(MAX_FILE_LENGTH) + " seconds to play...", {}, "*.wav, *.aif, *.aiff"};
        if(chooser.browseForFileToOpen())
        {
            auto file = chooser.getResult();
            auto path = file.getFullPathName();
            mChosenPath.swapWith(path);
            notify();
        }
    }

    void mClearButtonClicked()
    {
        mCurrentBuffer = nullptr;
        mThumbnail.clear();
    }
    
    void calculateAudioBlocks()
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

    //==========================================================================
    juce::TextButton mOpenButton;
    juce::TextButton mClearButton;
    juce::ToggleButton mRandomSlicesToggle;
    juce::Label mmSampleBPMLabel;
    juce::Label mmSampleBPMField;
    juce::Label mSliceSizeLabel;
    juce::ComboBox mSliceSizeDropDown;
    juce::Label mChangeSampleProbabilityLabel;
    juce::Slider mChangeSampleProbabilitySlider;
    juce::Label mReverseSampleProbabilityLabel;
    juce::Slider mReverseSampleProbabilitySlider;

    juce::AudioFormatManager mFormatManager;
    juce::AudioThumbnailCache mThumbnailCache { 5 };
    juce::AudioThumbnail mThumbnail { 512, mFormatManager, mThumbnailCache };
    
    std::unique_ptr<juce::FileInputSource> mFileSource;
    
    juce::ReferenceCountedArray<ReferenceCountedForwardAndReverseBuffer> mBuffers;
    ReferenceCountedForwardAndReverseBuffer::Ptr mCurrentBuffer;
    
    juce::String mChosenPath;
    
    int mSampleToEndOn;
    bool mRandomPosition;
    int mSampleBPM = 120;
    
    float mSampleChangeThreshold = 0.7;
    float mReverseSampleThreshold = 0.7;
    
    float mDuration = 44100.0;
    int mNumAudioBlocks = 1;
    int mBlockSampleSize = 1; // in samples
    int mBlockIdx = 0;
    double mBlockDivisionPower = 1.0; // This should be stored as powers of 2 (whole = 1, half = 2, quarter = 4 etc)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
