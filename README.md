Hylozoic Series 3 Interactive Control System
======================

 Combining techniques in architecture, the arts, electronics, and software, the Living Architecture System Group (LASG) develops interactive art sculptures that engage occupants in an immersive environment. The overarching goal of this research is to develop architectural systems that possess life-like qualities. In the Hylozoic Series kinetic sculptures built by [Philip Beesley Architect Inc.](http://philipbeesleyarchitect.com/), the designers use a network of microcontrollers to control and sample a sizable number of actuators and sensors. Each node in the network can perform a simple set of interactive behaviours. Though complexity may emerge in previous systems through superposition of a set of simple and prescripted behaviours, the responses of the systems to occupants remain rather robotic and ultimately dictated by the will of the designers. 

To enable the sculpture to automatically generate interactive behaviours and
adapt to changes, [Curiosity-Based Learning Algorithm (CBLA)](https://ece.uwaterloo.ca/~dkulic/pubs/ChanIROS2015.pdf), a reinforcement learning algorithm which selects actions that lead to maximum potential knowledge gains, is introduced.

To realize the CBLA system on a physical interactive art sculpture, an overhaul of the previous series’ interactive control hardware was necessary. CBLA requires the system to be able to sense the consequences of its own actions and its surrounding at a much higher resolution and frequency than previously implemented behaviour algorithms. This translates to the need to interface and collect samples from a substantially larger number of sensors. 

The **Hylozoic Series 3 Interactive Control System** consists of a new set of hardware and control system software was developed. It enables the control and sampling of hundreds of devices on a centralized computer through USB connections. Moving the computation from an embedded platform simpliﬁes the implementation of the CBLA system, which is a computationally intensive and complex
program. In addition, the large amount of data generated by the system can now be recorded without sacriﬁcing response time nor resolution.

## Organizations

### Hardware

The electronics that interface with the sensors and actuators were mostly customly designed. The [schematic](Hardware/Design Files), [PCB design](Hardware/Design Files), BOM, as well as [fabrication files](Hardware/fabrication) (like Gerber and quotes) for previous production runs can be found in the [Hardware](Hardware) directory. 

### Software

The [firmware](Software/teensy_firmware) that runs on the Teensy microcontroller, the [Python-based software framework](Software/pyHylozoic3) (pyHylozoic3), and [installation-specific behaviours](Software/complex_behaviours) can be found in the [Software](Software) directory. In addition, some of the necessary [libraries](Software/_libraries), and some [basic demo codes](Software/basic_behaviours) can also be in the directory.

In addition to the [communication framework](Software/pyHylozoic3/interactive_system/interactive_system), the pyHylozoic3 module includes an implemenation of the [CBLA Engine](Software/pyHylozoic3/cbla/cbla/cbla_engine), and [Abstract Node system](Software/pyHylozoic3/abstract_node/abstract_node) that supports the deployments of a network of multiple [CBLA Nodes](Software/pyHylozoic3/cbla/cbla).


## Contributors
* Matthew Tsz Kiu Chan (University of Waterloo)
* Mohammedreza Memarian (University of Waterloo)
* Matt Borland (Philip Beesley Architect Inc.)
* David Kadish (Philip Beesley Architect Inc.)
