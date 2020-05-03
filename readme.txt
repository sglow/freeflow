This project contains hardware design (schematics & layout) and firmware design for a
small PCB used to sense air flow and air pressure.  The intent is to use a 3D printed 
Venturi tube with this board to help with the current Covid-19 medical emergency.
The project has two potential uses:

- Part of a larger ventilator project.  The airflow sensor is a key component of a ventilator
  design.

- Stand alone use for sharing a commercial ventilator among multiple patients.
  One problem with sharing a ventilator between patients is equalizing the flow.
  This sensor can be used inline at each patient to allow the emergency workers 
  to regulate flow using flow valves.

The schematics and layout files here were made using the open source gEDA tools; gschem
for schematic capture and pcb for layout.  These tools can be downloaded here:
   http://www.geda-project.org/

As of 4/6/20 this project is in the early stages.  There have been two revisions of the PCB
(check the git history to see earlier versions).  The main difference is the type of sensors
used to measure the pressure drop across the venturi tube.

Firmware is working but very preliminary.  Things like calibration values are hard coded at
the moment.  It's under active development and changing rapidly.

I'll try to keep this file updated as things progress.

- Steve Glow
