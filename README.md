# Orbiter Plugin

A spatial audio plugin for DAWs.  This plugin accepts user-supplied HRIRs in the form of SOFA (Spatially Oriented Format for Acoustics) files.  Only HRIRs, which are then converted into HRTFs, are accepted by the plugin.

![Orbiter GUI](/readme_resources/Orbiter_GUI.png)

## Supported DAWs
* Ableton Live
* Logic Pro X
* REAPER  

**Currently using automation in Logic Pro X will not work.  A fix is in the works**  

## Building
To read SOFA files, Orbiter uses [libBasicSOFA](https://github.com/superkittens/libBasicSOFA) which in turn uses the HDF5 library.  Orbiter also uses the JUCE framework.  
The recommended way to build is to open the project via the Projucer.  You may need to add the HDF5 library and libBasicSOFA manually in your project settings.

## Instructions for Use
Currently, no default SOFA file is provided by the plugin itself.  You will need to have one ready before using the plugin.  The following example set of [SOFA files](https://zenodo.org/record/206860#.XzygXy0ZNQI) have been known to work with Orbiter.  

To load a SOFA file, open the plugin GUI and click *Open SOFA* and select your desired file.  

Oribiter only accepts SOFA files with measurements in spherical coordinates.  Theta is the source angle on the horizontal head plane while Phi is the elevation angle.  While Theta can range from -179 to 180 degrees and Phi ranges from -90 to 90 degrees, the sliders map the values 0 - 1 to the available angles defined in the SOFA file.  Radius controls the distance of the source from the listener.  

The left side of the GUI represents the location of the sound source.  Moving the orange circle around will change the source's theta and radius parameters.  The elevation vertical slider changes the elevation (phi).  The rotary sliders to the right control the input/output gain and reverb settings.  

