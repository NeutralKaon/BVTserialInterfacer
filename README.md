# BVT Serial Interfacer 

This program is a command line interface to the Bruker BVT3000-series of variable temperature devices. 

They exist to alter the temperature of samples in spectrometers, from ~77K or below to as hot as you are willing to go. You probably know if you have one. 

While you can control what they do with the EDTE command in TopSpin, it requires a license of TopSpin and a Bruker spectrometer. You may wish to use them independently, or alternatively, to hook up the hardware to something more exotic. 

I found myself needing to do exactly this, and with few other available tools that are "lightweight" (and can run easily on a low-cost, low-spec computer such as a Raspberry Pi) decided to create one. 


