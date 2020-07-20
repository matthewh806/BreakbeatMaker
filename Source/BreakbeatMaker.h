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
#include "BreakbeatAudioSource.h"
#include "FileRecorder.h"
#include "SliceExporter.h"
//==============================================================================

#define MAX_FILE_LENGTH 15.0 // seconds

class MainContentComponent
: public juce::AudioAppComponent
, private juce::Thread
, private juce::ChangeListener
, private juce::AsyncUpdater
{
public:
    enum ColourIds
    {
        backgroundColourId      = 0x3004000,
        playingButtonColourId   = 0x3004001,
        recordingButtonColourId = 0x3004003,
        defaultButtonColourId   = 0x3004002
    };
    
    MainContentComponent(juce::RecentlyOpenedFilesList& recentFiles);
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
    void setFileOutputPath();
    void exportAudioSlices();

private:
    enum class TransportState
    {
        Stopped,
        Starting,
        Playing,
        Stopping
    };
    
    class WaveformComponent
    : public juce::Component
    , public juce::FileDragAndDropTarget
    , private juce::AsyncUpdater
    {
    public:
        WaveformComponent(MainContentComponent& parent, juce::AudioFormatManager& formatManager);
        ~WaveformComponent() override;
        
        juce::AudioThumbnail& getThumbnail();
        
        void setSampleStartEnd(int64_t start, int64_t end);
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
        // juce::FileDragAndDropTarget
        bool isInterestedInFileDrag (const StringArray& files) override;
        void filesDropped (const StringArray& files, int x, int y) override;
        
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;
        
    private:
        MainContentComponent& mParentComponent;
        
        juce::AudioFormatManager& mAudioFormatManager;
        juce::AudioThumbnailCache mThumbnailCache;
        juce::AudioThumbnail mThumbnail;
        
        int64_t mStartSample = 0;
        int64_t mEndSample = 0;
        
        double mSampleRate = 44100.0;
    };
    
    void clearButtonClicked();
    
    void checkForBuffersToFree();
    void checkForPathToOpen();
    
    void changeState(TransportState state);
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
    
    juce::Label mFileNameLabel;
    juce::Label mFileSampleRateLabel;
    
    juce::TextButton mStopButton;
    juce::TextButton mPlayButton;
    juce::TextButton mRecordButton;
        
    juce::String mFileName;
    double mFileSampleRate;
    
    TransportState mState;
    
    juce::RecentlyOpenedFilesList& mRecentFiles;
    juce::AudioFormatManager mFormatManager;
    
    std::unique_ptr<juce::FileInputSource> mFileSource;
    BreakbeatAudioSource mAudioSource;
    juce::AudioTransportSource mTransportSource;
    
    std::vector<float*> mTemporaryChannels;
    
    FileRecorder mRecorder {mFormatManager};
    bool mRecording = false;
    
    SliceExporter mSliceExporter {mFormatManager};
    
    WaveformComponent mWaveformComponent { *this, mFormatManager };
    
    juce::String mChosenPath;
    
    bool mRandomPosition;
    int mSampleBPM = 120;
    
    float mSampleChangeThreshold = 0.7;
    float mReverseSampleThreshold = 0.7;
    
    double mBlockDivisionPower = 1.0; // This should be stored as powers of 2 (whole = 1, half = 2, quarter = 4 etc)
    
    juce::File mRecordedFile {juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory).getChildFile("toous").getChildFile("temp_recording.wav")};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
