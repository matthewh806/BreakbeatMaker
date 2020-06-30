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

//==============================================================================
class MainContentComponent   : public AudioAppComponent
{
public:
    MainContentComponent()
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
        mChangeSampleProbabilityLabel.setText("Change Sample Probability: ", dontSendNotification);
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
        mReverseSampleProbabilityLabel.setText("Change Sample Probability: ", dontSendNotification);
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
        
        setSize (500, 200);

        mFormatManager.registerBasicFormats();
    }

    ~MainContentComponent() override
    {
        shutdownAudio();
    }

    void prepareToPlay (int, double) override
    {
        
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        if(mActiveBuffer == nullptr)
        {
            auto reversePerc = Random::getSystemRandom().nextFloat();
            mActiveBuffer = std::unique_ptr<AudioSampleBuffer>(reversePerc > mReverseSampleThreshold ? &mReverseFileBuffer : &mForwardFileBuffer);
        }
        
        bool changeDirection = false;
        
        auto const numInputChannels = mActiveBuffer->getNumChannels();
        auto const numOutputChannels = bufferToFill.buffer->getNumChannels();
        
        auto outputSamplesRemaining = bufferToFill.numSamples;
        auto outputSamplesOffset = bufferToFill.startSample;
        
        while(outputSamplesRemaining > 0)
        {
            auto bufferSamplesRemaining = mSampleToEndOn - mPosition;
            auto samplesThisTime = std::min(outputSamplesRemaining, bufferSamplesRemaining);
            
            for(auto ch = 0; ch < numOutputChannels; ++ch)
            {
                bufferToFill.buffer->copyFrom(ch, outputSamplesOffset, *mActiveBuffer, ch % numInputChannels, mPosition, samplesThisTime);
            }
            
            outputSamplesRemaining -= samplesThisTime;
            outputSamplesOffset += samplesThisTime;
            mPosition += samplesThisTime;
            
            if(mPosition == mSampleToEndOn)
            {
                if(mRandomPosition)
                {
                    auto changePerc = Random::getSystemRandom().nextFloat();
                    mBlockIdx = changePerc > mSampleChangeThreshold ? Random::getSystemRandom().nextInt(mNumAudioBlocks) : mBlockIdx;
                        
                    // Move that many blocks along the fileBuffer
                    mPosition = mBlockSampleSize * mBlockIdx;
                    mSampleToEndOn = mPosition + mBlockSampleSize;
                }
                else
                {
                    mPosition = 0;
                    mSampleToEndOn = mActiveBuffer->getNumSamples();
                }
                
                changeDirection = true;
            }
        }
        
        if(changeDirection)
        {
            changeDirection = false;
            mActiveBuffer.release();
        }
    }

    void releaseResources() override
    {
        mForwardFileBuffer.setSize (0, 0);
        mReverseFileBuffer.setSize (0, 0);
        
        if(mActiveBuffer)
        {
            mActiveBuffer.release();
        }
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

private:
    void mOpenButtonClicked()
    {
        shutdownAudio();
        
        juce::FileChooser chooser {"Select an audio file shorter than 2 seconds to play...", {}, "*.wav, *.aif, *.aiff"};
        
        if(chooser.browseForFileToOpen())
        {
            auto file = chooser.getResult();
            std::unique_ptr<AudioFormatReader> reader { mFormatManager.createReaderFor(file) };
            
            if(reader != nullptr)
            {
                // get length
                mDuration = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
                
                if(mDuration < 15.0)
                {
                    // read into sample buffer
                    mForwardFileBuffer.setSize(reader->numChannels, static_cast<int>(reader->lengthInSamples));
                    mReverseFileBuffer.setSize(reader->numChannels, static_cast<int>(reader->lengthInSamples));
                    
                    reader->read(&mForwardFileBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
                    
                    for(auto ch = 0; ch < mReverseFileBuffer.getNumChannels(); ++ch)
                    {
                        mReverseFileBuffer.copyFrom(ch, 0, mForwardFileBuffer, ch, 0, mForwardFileBuffer.getNumSamples());
                        mReverseFileBuffer.reverse(ch, 0, mReverseFileBuffer.getNumSamples());
                    }
                    
                    mPosition = 0;
                    mSampleToEndOn = static_cast<int>(reader->lengthInSamples);
                    setAudioChannels(0, reader->numChannels);
                    
                    calculateAudioBlocks();
                }
                else
                {
                     juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
                                                  juce::translate("Audio file too long!"),
                                                       juce::translate("The file is ") + juce::String(mDuration) + juce::translate("seconds long. 15 Second limit!"));
                }
                
            }
        }
    }

    void mClearButtonClicked()
    {
        shutdownAudio();
    }
    
    void calculateAudioBlocks()
    {
        auto const numSrcSamples = mForwardFileBuffer.getNumSamples();
        if(numSrcSamples == 0)
        {
            mNumAudioBlocks = 1;
            mBlockSampleSize = mForwardFileBuffer.getNumSamples();
            return;
        }
        
        // length, bpm
        auto bps = mSampleBPM / 60.0;
        mNumAudioBlocks = std::max(bps * mDuration / static_cast<double>(mBlockDivisionPower), 1.0);
        mBlockSampleSize = roundToInt(numSrcSamples / mNumAudioBlocks);
    }

    //==========================================================================
    TextButton mOpenButton;
    TextButton mClearButton;
    ToggleButton mRandomSlicesToggle;
    Label mmSampleBPMLabel;
    Label mmSampleBPMField;
    Label mSliceSizeLabel;
    ComboBox mSliceSizeDropDown;
    Label mChangeSampleProbabilityLabel;
    Slider mChangeSampleProbabilitySlider;
    Label mReverseSampleProbabilityLabel;
    Slider mReverseSampleProbabilitySlider;

    AudioFormatManager mFormatManager;
    AudioSampleBuffer mForwardFileBuffer;
    AudioSampleBuffer mReverseFileBuffer;
    std::unique_ptr<AudioSampleBuffer> mActiveBuffer;
    
    int mPosition;
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
