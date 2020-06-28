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
        addAndMakeVisible (openButton);
        openButton.setButtonText ("Open...");
        openButton.onClick = [this] { openButtonClicked(); };

        addAndMakeVisible (clearButton);
        clearButton.setButtonText ("Clear");
        clearButton.onClick = [this] { clearButtonClicked(); };
        
        addAndMakeVisible (randomSlicesToggle);
        randomSlicesToggle.setButtonText("Random Slices");
        randomSlicesToggle.onClick = [this] { randomPosition = !randomPosition; };
        
        addAndMakeVisible(sampleBPMLabel);
        sampleBPMLabel.setText("Sample BPM: ", dontSendNotification);
        sampleBPMLabel.attachToComponent(&sampleBPMField, true);
        sampleBPMLabel.setEditable(false);
        sampleBPMLabel.setJustificationType(Justification::right);
        sampleBPMLabel.setColour(Label::textColourId, Colours::white);
        
        addAndMakeVisible(sampleBPMField);
        sampleBPMField.setText("120", dontSendNotification);
        sampleBPMField.setColour(Label::textColourId, Colours::white);
        sampleBPMField.setEditable(true);
        sampleBPMField.onTextChange = [this]
        {
            sampleBPM = sampleBPMField.getText().getIntValue();
            calculateAudioBlocks();
        };
        sampleBPMField.onEditorShow = [this]
        {
            auto* ed = sampleBPMField.getCurrentTextEditor();
            ed->setInputRestrictions(3, "1234567890");
        };
        
        addAndMakeVisible(sliceSizeLabel);
        sliceSizeLabel.setText("Slice size: ", dontSendNotification);
        sliceSizeLabel.setColour(Label::textColourId, Colours::white);
        sliceSizeLabel.setEditable(false);
        sliceSizeLabel.attachToComponent(&sliceSizeDropDown, true);
        sliceSizeLabel.setJustificationType(Justification::right);
        
        addAndMakeVisible(sliceSizeDropDown);
        sliceSizeDropDown.addItemList({"1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/128"}, 1);
        sliceSizeDropDown.setSelectedId(3);
        sliceSizeDropDown.onChange = [this]
        {
            auto selectionId = sliceSizeDropDown.getSelectedId();
            
            switch(selectionId)
            {
                case 1:
                    blockDivisionPower = 4;
                    break;
                case 2:
                    blockDivisionPower = 2;
                    break;
                default:
                    blockDivisionPower = 1.0 / std::pow(2, selectionId - 3);
                    break;
            }
            
            calculateAudioBlocks();
        };
        
        addAndMakeVisible(changeSampleProbabilityLabel);
        changeSampleProbabilityLabel.setText("Change Sample Probability: ", dontSendNotification);
        changeSampleProbabilityLabel.setColour(Label::textColourId, Colours::white);
        changeSampleProbabilityLabel.setEditable(false);
        changeSampleProbabilityLabel.attachToComponent(&changeSampleProbabilitySlider, true);
        changeSampleProbabilityLabel.setJustificationType(Justification::right);
        
        addAndMakeVisible(changeSampleProbabilitySlider);
        changeSampleProbabilitySlider.setRange(0.0, 1.0, 0.1);
        changeSampleProbabilitySlider.setValue(0.3, dontSendNotification);
        changeSampleProbabilitySlider.onValueChange = [this]()
        {
            sampleChangeThreshold = 1.0 - changeSampleProbabilitySlider.getValue();
        };
        
        addAndMakeVisible(reverseSampleProbabilityLabel);
        reverseSampleProbabilityLabel.setText("Change Sample Probability: ", dontSendNotification);
        reverseSampleProbabilityLabel.setColour(Label::textColourId, Colours::white);
        reverseSampleProbabilityLabel.setEditable(false);
        reverseSampleProbabilityLabel.attachToComponent(&reverseSampleProbabilitySlider, true);
        reverseSampleProbabilityLabel.setJustificationType(Justification::right);
        
        addAndMakeVisible(reverseSampleProbabilitySlider);
        reverseSampleProbabilitySlider.setRange(0.0, 1.0, 0.1);
        reverseSampleProbabilitySlider.setValue(0.3, dontSendNotification);
        reverseSampleProbabilitySlider.onValueChange = [this]()
        {
            reverseSampleThreshold = 1.0 - reverseSampleProbabilitySlider.getValue();
        };
        
        setSize (500, 200);

        formatManager.registerBasicFormats();
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
        if(activeBuffer == nullptr)
        {
            auto reversePerc = Random::getSystemRandom().nextFloat();
            activeBuffer = std::unique_ptr<AudioSampleBuffer>(reversePerc > reverseSampleThreshold ? &reverseFileBuffer : &forwardFileBuffer);
        }
        
        bool changeDirection = false;
        
        auto const numInputChannels = activeBuffer->getNumChannels();
        auto const numOutputChannels = bufferToFill.buffer->getNumChannels();
        
        auto outputSamplesRemaining = bufferToFill.numSamples;
        auto outputSamplesOffset = bufferToFill.startSample;
        
        while(outputSamplesRemaining > 0)
        {
            auto bufferSamplesRemaining = sampleToEndOn - position;
            auto samplesThisTime = std::min(outputSamplesRemaining, bufferSamplesRemaining);
            
            for(auto ch = 0; ch < numOutputChannels; ++ch)
            {
                bufferToFill.buffer->copyFrom(ch, outputSamplesOffset, *activeBuffer, ch % numInputChannels, position, samplesThisTime);
            }
            
            outputSamplesRemaining -= samplesThisTime;
            outputSamplesOffset += samplesThisTime;
            position += samplesThisTime;
            
            if(position == sampleToEndOn)
            {
                if(randomPosition)
                {
                    auto changePerc = Random::getSystemRandom().nextFloat();
                    blockIdx = changePerc > sampleChangeThreshold ? Random::getSystemRandom().nextInt(numAudioBlocks) : blockIdx;
                        
                    // Move that many blocks along the fileBuffer
                    position = blockSampleSize * blockIdx;
                    sampleToEndOn = position + blockSampleSize;
                }
                else
                {
                    position = 0;
                    sampleToEndOn = activeBuffer->getNumSamples();
                }
                
                changeDirection = true;
            }
        }
        
        if(changeDirection)
        {
            changeDirection = false;
            activeBuffer.release();
        }
    }

    void releaseResources() override
    {
        forwardFileBuffer.setSize (0, 0);
        reverseFileBuffer.setSize (0, 0);
        
        if(activeBuffer)
        {
            activeBuffer.release();
        }
    }

    void resized() override
    {
        openButton.setBounds (10, 10, getWidth() - 20, 20);
        clearButton.setBounds (10, 40, getWidth() - 20, 20);
        randomSlicesToggle.setBounds(10, 70, getWidth() - 20, 20);
        sampleBPMField.setBounds(100, 100, getWidth() - 120, 20);
        sliceSizeDropDown.setBounds(100, 130, getWidth() - 120, 20);
        changeSampleProbabilitySlider.setBounds(100, 160, getWidth() - 120, 20);
        reverseSampleProbabilitySlider.setBounds(100, 190, getWidth() - 120, 20);
    }

private:
    void openButtonClicked()
    {
        shutdownAudio();
        
        juce::FileChooser chooser {"Select an audio file shorter than 2 seconds to play...", {}, "*.wav, *.aif, *.aiff"};
        
        if(chooser.browseForFileToOpen())
        {
            auto file = chooser.getResult();
            std::unique_ptr<AudioFormatReader> reader { formatManager.createReaderFor(file) };
            
            if(reader != nullptr)
            {
                // get length
                duration = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
                
                if(duration < 15.0)
                {
                    // read into sample buffer
                    forwardFileBuffer.setSize(reader->numChannels, static_cast<int>(reader->lengthInSamples));
                    reverseFileBuffer.setSize(reader->numChannels, static_cast<int>(reader->lengthInSamples));
                    
                    reader->read(&forwardFileBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
                    
                    for(auto ch = 0; ch < reverseFileBuffer.getNumChannels(); ++ch)
                    {
                        reverseFileBuffer.copyFrom(ch, 0, forwardFileBuffer, ch, 0, forwardFileBuffer.getNumSamples());
                        reverseFileBuffer.reverse(ch, 0, reverseFileBuffer.getNumSamples());
                    }
                    
                    position = 0;
                    sampleToEndOn = static_cast<int>(reader->lengthInSamples);
                    setAudioChannels(0, reader->numChannels);
                    
                    calculateAudioBlocks();
                }
                else
                {
                     juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
                                                  juce::translate("Audio file too long!"),
                                                       juce::translate("The file is ") + juce::String(duration) + juce::translate("seconds long. 15 Second limit!"));
                }
                
            }
        }
    }

    void clearButtonClicked()
    {
        shutdownAudio();
    }
    
    void calculateAudioBlocks()
    {
        auto const numSrcSamples = forwardFileBuffer.getNumSamples();
        if(numSrcSamples == 0)
        {
            numAudioBlocks = 1;
            blockSampleSize = forwardFileBuffer.getNumSamples();
            return;
        }
        
        // length, bpm
        auto bps = sampleBPM / 60.0;
        numAudioBlocks = std::max(bps * duration / static_cast<double>(blockDivisionPower), 1.0);
        blockSampleSize = roundToInt(numSrcSamples / numAudioBlocks);
    }

    //==========================================================================
    TextButton openButton;
    TextButton clearButton;
    ToggleButton randomSlicesToggle;
    Label sampleBPMLabel;
    Label sampleBPMField;
    Label sliceSizeLabel;
    ComboBox sliceSizeDropDown;
    Label changeSampleProbabilityLabel;
    Slider changeSampleProbabilitySlider;
    Label reverseSampleProbabilityLabel;
    Slider reverseSampleProbabilitySlider;

    AudioFormatManager formatManager;
    AudioSampleBuffer forwardFileBuffer;
    AudioSampleBuffer reverseFileBuffer;
    std::unique_ptr<AudioSampleBuffer> activeBuffer;
    
    int position;
    int sampleToEndOn;
    bool randomPosition;
    int sampleBPM = 120;
    
    float sampleChangeThreshold = 0.7;
    float reverseSampleThreshold = 0.7;
    
    float duration = 44100.0;
    int numAudioBlocks = 1;
    int blockSampleSize = 1; // in samples
    int blockIdx = 0;
    double blockDivisionPower = 1.0; // This should be stored as powers of 2 (whole = 1, half = 2, quarter = 4 etc)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
