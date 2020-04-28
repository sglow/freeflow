// This file describes the geometry of the freeflow venturi tube.
// I have tried to document the various parameters, most of which can be
// changed to adjust tube geometry.
//
// These are just openscad special variables related to rendering.
// They define how many fascets in an arc.  I have them set for pretty high 
// resolution (slow) rendering.  Making these bigger produces faster but 
// rougher renderings
$fa = 1;      // Minimum angle fragment
$fs = .1;     // Minimum size of a fragment

// M for margin.  I use this to make things overlap slightly for better rendering
M = .01;

// These dimensions describe the PCB that sits on top of the venturi
pcb_length = in2mm(3);
pcb_width  = in2mm(1);

// These are the X axis offsets of the two pressure sensors from the end of the pcb.
// The sensors are both centered in the Y axis, so I don't include that here.
// These values are correct for the rev-2 freeflow board.
pcb_tap_outer  = in2mm(0.3);
pcb_tap_center = in2mm(1.6);

// Tubing that connects to the venturi outer & inner diameter
tube_od = 22;
tube_id = 15;

// Amount that my venturi will overlap the tube on the enterence.
// I'm using a female enterence and male exit
tube_overlap = 10;

// Thickness of the wall of the overlapping section
overlap_thickness = 1.25;

// The outer diameter of the venturi tube enterence will be the 
// same as the tube_id.  The throat diameter is the inner diameter
// of the constricted throat of the venturi.  Smaller gives larger
// readings, especially at low flow rates, but also causes more
// pressure drop through the tube.
throat_d = tube_id * 0.4;
echo( "throat_d ", throat_d );

// These angles define how quickly the venturi will narrow on 
// both entry and exit.  Angles are relative to the center line
// running through the tube.  These angles are pretty typical for
// venturi tubes according to my research.
angle_entry = 12;
angle_exit = 7;

// The exit of the venturi will have the same inner/outer diameter 
// as the tube that attaches to the entry.  I'm assuming that a fitting
// sized for that tube will be connected to the exit end
exit_id = tube_id;
exit_od = tube_od;

// Find the entry and exit distance based on the change of diameter
// and the angles.  
Lnarrow = (tube_id - throat_d)/2 / tan(angle_entry);
Lexpand  = (exit_id - throat_d)/2 / tan(angle_exit);

// This is the length of the exit section.  The fitting attached to the 
// end of the venturi will overlap by up to this much
exit_overlap = 10;

// The length of the throat section is customarily the same as the
// throat diameter
Lthroat = throat_d;

// The length of the enterence to the venturi (before it starts to narrow)
// The outer tap is located in this area, so this is somewhat dictated by the PCB dimensions

pcb_dtap = pcb_tap_center - pcb_tap_outer;
if( pcb_dtap < Lnarrow + Lthroat/2 )
   echo( "<b>ERROR - PCB spacing is too small for this tube!</b>" );
else if( pcb_dtap < Lnarrow + Lthroat/2 + tube_id/2 )
   echo( "<b>Warning - outer tap closer to narrowing then we would like!</b>" );

Lentry = pcb_dtap - Lnarrow - Lthroat/2 + tube_id/2;

// There will be two holes in the side of the tube where we will sense
// pressure.  This is the diameter of those holes
tap_d = 2.5;

VenturiTube();

// This module is the venturi tube itself without the board mount
module VenturiTube()
{
   // This is the Z offset of the bottom of the PCB relative to the bottom of the tube
   pcb_zoff = tube_overlap+Lentry+Lnarrow+Lthroat/2-pcb_tap_center;

   // OK, build the venturi tube.
   difference()
   {
      // This section is the outer shell of the venturi tube.
      union()
      {
         // This is the PCB mount
         hull()
         {
            translate( [tube_od/2+overlap_thickness, -pcb_width/2, pcb_zoff ] )
               cube( [2, pcb_width, pcb_length] );
            translate( [0, -pcb_width/4, pcb_zoff ] )
               cube( [1, pcb_width/2, pcb_length] );
         }

         // This first section is the area in which the venturi tube overlaps
         // the tubing that connects to it.
         cylinder( d=tube_od+2*overlap_thickness, h=tube_overlap );

         // The rest of of the outer shell slowly tapers to the exit outer
         // diameter (which is probably the same as the tube od).  The length
         // is just the sum of all the sections below
         L = Lentry+Lnarrow+Lthroat+Lexpand;
         translate( [0,0,tube_overlap-M] )
            cylinder( d1=tube_od+2*overlap_thickness, d2=exit_od, h=L+M );

         // This last section is the outer part of the section which will fit
         // into the fitting 
         translate( [0,0,tube_overlap+L-M] )
            cylinder( d=exit_od, h=exit_overlap+M );
      }

      // Everything after here is removed from that outer shell.

      // This is the area in which the external tube will enter
      translate( [0,0,-M] ) 
         cylinder( d=tube_od, h=tube_overlap+M );

      // This is the outer section of the venturi.
      // The outer tap will end up in the center of this section.
      translate( [0,0,tube_overlap-M] ) 
         cylinder( d=tube_id, h=Lentry+M );

      // This is the narrowing entry part
      translate( [0,0,tube_overlap+Lentry-M] ) 
         cylinder( d1=tube_id, d2=throat_d, h=Lnarrow+M );

      // This is the throat.  The center tap is in the middle of this
      translate( [0,0,tube_overlap+Lentry+Lnarrow-M] ) 
         cylinder( d=throat_d, h=Lthroat+M );

      // Expanding section
      translate( [0,0,tube_overlap+Lentry+Lnarrow+Lthroat-M] ) 
         cylinder( d1=throat_d, d2=exit_id, h=Lexpand+M );

      // Finally, the section of the venturi which will be overlaped by the exit fitting
      translate( [0,0,tube_overlap+Lentry+Lnarrow+Lthroat+Lexpand-M] ) 
         cylinder( d=exit_id, h=exit_overlap+2*M );

      // Add a hole for the outer pressure sensor tap
      translate( [0,0,pcb_zoff+pcb_tap_outer] )
         rotate( [0,90,0] ) cylinder( d=tap_d, h=tube_od );

      // And center tap
      translate( [0,0,pcb_zoff+pcb_tap_center] )
         rotate( [0,90,0] ) cylinder( d=tap_d, h=tube_od );

      // Hollow out an area below the PCB to allow for through hole parts that might
      // be sticking through.  I should parameterize this better
      translate( [tube_od/2+overlap_thickness+1+M, -pcb_width/2+1, pcb_zoff+1 ] )
         cube( [1, pcb_width-7, pcb_length-7] );
      translate( [tube_od/2+overlap_thickness+1+M, -pcb_width/2+6, pcb_zoff+6 ] )
         cube( [1, pcb_width-7, pcb_length-7] );

      // Add holes for the mounting screws.
      // I'm using brass heat-set inserts in these holes.
      // McMaster-Carr part number 94459A120
      translate( [tube_od/2+overlap_thickness+2,in2mm(0.375),pcb_zoff+in2mm(0.125)] )
         rotate( [0,90,0] ) cylinder( d=3.25, h=8, center=true);
      translate( [tube_od/2+overlap_thickness+2,-in2mm(0.375),pcb_zoff+pcb_length-in2mm(0.125)] )
         rotate( [0,90,0] ) cylinder( d=3.25, h=8, center=true);

      // Cutouts for o-rings (McMaster-Carr 2418T111)
      translate( [tube_od/2+overlap_thickness+2, 0, pcb_zoff+pcb_tap_outer ] )
         rotate( [0,90,0] ) cylinder( d=6.5, h=7, center=true );
      translate( [tube_od/2+overlap_thickness+2, 0, pcb_zoff+pcb_tap_center ] )
         rotate( [0,90,0] ) cylinder( d=6.5, h=7, center=true );
   }
}

function in2mm(x) = x*25.4;
