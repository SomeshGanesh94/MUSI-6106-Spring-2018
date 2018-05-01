# MUSI-6106-Spring-2018
Repository for class project MUSI 6106 Audio software engineering - SynthIO
A plugin host meant to estimate the parameters in a VST instrument to recreate an audio sample (of 1 second). This project has been coded around the JUCE Plugin host to host and modify parameters in different VST instruments. Hence, most of the files have new code written along with the code from the JUCE team.

Somesh Ganesh
Agneya A. Kerure

## Requirements
* JUCE - https://github.com/WeAreROLI/JUCE
* VST SDK - https://www.steinberg.net/en/company/developers.html
* Essentia - https://github.com/MTG/essentia
* Dlib - http://dlib.net

Install the required packages from the above websites. You do not need to link the dlib library anywhere in the Projucer.

## Instructions
* Clone Repository
* Run Projucer
* Link JUCE Module folder for all modules in Projucer
* Run!

Once you build the project, add a VST instrument to the host (preferably one with less number of parameters). You can then click the different buttons to
* Generate the audio dataset for this VST instrument (~10 mins)
* Extract features for all these audio files (~10 mins)
* Train a regression model for the different parameters in the VST instrument (~3 mins)

Please use a synth with number of parameters < 5 as the project is still under development and will take a long time to generate the dataset. The times mentioned above are based on testing on AUMidiSynth by Apple where number of parameters = 4.
