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
    MainContentComponent();
    ~MainContentComponent() override;
    
    // juce::Component
    void resized() override;
    void paint(juce::Graphics& g) override;
    
    // juce::AudioAppComponent
    void prepareToPlay (int, double) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    
    // juce::Thread
    void run() override;
    
    // juce::ChangeListener
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    
    // juce::AsyncUpdater
    void handleAsyncUpdate() override;
    
    void newFileOpened(String& filePath);

private:
    class WaveformComponent
    : public juce::Component
    , public juce::FileDragAndDropTarget
    {
    public:
        WaveformComponent(MainContentComponent& parent, juce::AudioFormatManager& formatManager);
        ~WaveformComponent() override;
        
        juce::AudioThumbnail& getThumbnail();
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
        // juce::FileDragAndDropTarget
        bool isInterestedInFileDrag (const StringArray& files) override;
        void filesDropped (const StringArray& files, int x, int y) override;
        
    private:
        MainContentComponent& mParentComponent;
        
        juce::AudioFormatManager& mAudioFormatManager;
        juce::AudioThumbnailCache mThumbnailCache;
        juce::AudioThumbnail mThumbnail;
    };
    
    void clearButtonClicked();
    
    void checkForBuffersToFree();
    void checkForPathToOpen();
    
    void calculateAudioBlocks();
    //==========================================================================
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
    
    std::unique_ptr<juce::FileInputSource> mFileSource;
    
    juce::ReferenceCountedArray<ReferenceCountedForwardAndReverseBuffer> mBuffers;
    ReferenceCountedForwardAndReverseBuffer::Ptr mCurrentBuffer;
    
    WaveformComponent mWaveformComponent { *this, mFormatManager };
    
    juce::String mChosenPath;
    
    bool mRandomPosition;
    int mSampleBPM = 120;
    
    float mSampleChangeThreshold = 0.7;
    float mReverseSampleThreshold = 0.7;
    
    float mDuration = 44100.0;
    std::atomic<int> mNumAudioBlocks {1};
    std::atomic<int> mSampleToEndOn;
    std::atomic<int> mBlockSampleSize {1}; // in samples
    std::atomic<int> mBlockIdx {0};
    double mBlockDivisionPower = 1.0; // This should be stored as powers of 2 (whole = 1, half = 2, quarter = 4 etc)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
