/*
  ==============================================================================

    This file was auto-generated and contains the startup code for a PIP.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "BreakbeatMaker.h"

class Application    : public JUCEApplication
{
public:
    //==============================================================================
    Application() {}

    const String getApplicationName() override       { return "BreakbeatMaker"; }
    const String getApplicationVersion() override    { return "1.0.0"; }

    void initialise (const String&) override
    {
        mPropertyFile =
            juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory)
            .getChildFile("Application Support").getChildFile("toous").getChildFile("BreakbeatMaker.xml");
        
        loadProperties();
        
        mMainWindow = std::make_unique<MainWindow>("BreakbeatMaker", &mContent, *this);
        
        juce::MessageManager::callAsync([this]()
        {
            if(mRecentFiles.getNumFiles() && mRecentFiles.getFile(0).existsAsFile())
            {
                auto path = mRecentFiles.getFile(0).getFullPathName();
                mContent.newFileOpened(path);
            }
        });
    }
    void shutdown() override
    {
        saveProperties();
    }

private:
    class MainWindow    : public juce::DocumentWindow
    {
    public:
        MainWindow (const String& name, Component* c, JUCEApplication& a)
            : DocumentWindow (name, Desktop::getInstance().getDefaultLookAndFeel()
                                                          .findColour (ResizableWindow::backgroundColourId),
                              DocumentWindow::allButtons),
              app (a)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (c, true);

           #if JUCE_ANDROID || JUCE_IOS
            setFullScreen (true);
           #else
            setResizable (true, false);
            setResizeLimits (300, 250, 10000, 10000);
            centreWithSize (getWidth(), getHeight());
           #endif

            setVisible (true);
        }

        void closeButtonPressed() override
        {
            app.systemRequestedQuit();
        }

    private:
        JUCEApplication& app;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };
    
    void saveProperties()
    {
        auto xml = std::make_unique<juce::XmlElement>("BreakbeatMaker");
        if(xml == nullptr)
        {
            return;
        }
        
        xml->setAttribute("recentFiles", mRecentFiles.toString());
        xml->writeTo(mPropertyFile);
    }
    
    void loadProperties()
    {
        if(!mPropertyFile.existsAsFile())
        {
            mPropertyFile.create();
        }
        
        auto xml = juce::parseXML(mPropertyFile);
        if(xml != nullptr && xml->hasTagName("BreakbeatMaker"))
        {
            mRecentFiles.restoreFromString(xml->getStringAttribute("recentFiles"));
        }
    }

    std::unique_ptr<MainWindow> mMainWindow;
    juce::RecentlyOpenedFilesList mRecentFiles;
    MainContentComponent mContent {mRecentFiles};
    
    juce::File mPropertyFile;
};

//==============================================================================
START_JUCE_APPLICATION (Application)
