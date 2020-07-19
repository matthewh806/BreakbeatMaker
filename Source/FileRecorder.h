/*
  ==============================================================================

    FileRecorder.h
    Created: 19 Jul 2020 7:52:46pm
    Author:  Matthew

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class FileRecorder
{
public:
    FileRecorder(juce::AudioFormatManager& audioFormatManager);
    virtual ~FileRecorder() = default;
    
    bool startRecording(juce::File file, int numChannels, double sampleRate, int bitDepth);
    void stopRecording();
    bool isRecording() const;
    void processBlock(juce::AudioBuffer<float> const& buffer);
    
private:
    juce::AudioFormatManager& mFormatManager;
    std::shared_ptr<juce::AudioFormatWriter::ThreadedWriter> mThreadedWriter;
    juce::TimeSliceThread mBackgroundThread;
    std::atomic<juce::int64> mRecordingDuration;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileRecorder);
};
