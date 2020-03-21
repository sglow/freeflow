#!/usr/bin/python

from Footprint import *;

c=SM_Cap( "0603" );
c.Print( 'packages/sm_0603' )

c=SM_Cap( "0805" );
c.Print( 'packages/sm_0805' )
#c.Info( pnum='RC0603FR-0730KL', mfg='Yageo', dk='311-30.0KHRCT-ND', value='30k', ref='R1' ); c.Print( 'packages/sm_0603' )

# 32-pin version of STM32
p = Part()
p.InitFourSided( N=32, W=mm(7.2), L=mm(7.2), pitch=mm(0.8), pinW=mm(.5), pinL=mm(1.2), rowD=mm(9.7-1.2), exact=True )
p.Info( mfg="ST", pnum='STM32L412KBT6', dk='497-18229-ND' );
p.Print( "packages/STM32" );

# Diodes for power supply inputs
p=SM_Cap()
p.InitExact( length=mm(1.7), width=mm(1.25), name='diode', padL=mm(1), padW=mm(.9), pitch=mm(2.3) )
p.Info( mfg='Toshiba', pnum='CUS10S30,H3F', dk='CUS10S30H3FCT-ND' );
p.Print( 'packages/diode' );

# Hirose DS13 header, through hole
def Hirose(n, ra=False ):
   if( ra ):
      pnum = 'DF13-%dP-1.25DS(50)' % n
      dk = 'H33%d-ND' % (n+18)
      if( n>12 ): print 'Check Hirose part numbers'
   else:
      pnum = 'DF13-%dP-1.25DSA' % n
      if( n < 9 ): 
         dk = 'H21%d-ND' % (n+89)
      else:
         dk = ['H122316', 'H2198', 'H126373', 'H2320', None, 'H2321', 'H2322'][n-8] + '-ND'
   
   pi = PinInfo( pinW=mm(0.6), exact=True )
   p = Part('J1');
   if( ra ): w = mm(5.4); x=mm(-4.5)+w/2
   else:     w = mm(3.4); x=mm(1.21)-w/2
   p.AddOutline( H=mm(1.25*(n-1)+2.9), W=w );
   o = 0.5 * (n-1)

   for i in range(n):
      p.pins.append( Pin( n=i+1, x=x, y=mm(1.25*(i-o)), info=pi ) );
   p.Info( mfg='Hirose', pnum=pnum, dk=dk  );
   p.Print( 'packages/header1x%d' % n );

Hirose( 2, ra=True );
Hirose( 4, ra=False );

# 4-pin header for display
p = Header()
p.Info( mfg='Sullins', pnum='PPTC041LFBN-RC', dk='S7002-ND' )
p.Create( N=4, pinW=mil(40), rows=1, pitch=mil(100), L=mil(420), W=mil(100), exact=True )
p.Print( 'packages/CON4' )

# Linear regulator
p = Part();
pi = PadInfo( pinW=mm(.95), pinL=mm(2.15), exact=True )
for i in range(3):
   p.pins.append( Pad( n=i+1, hor=True, x=mm(-5.8/2), y=mm(2.3*(i-1)), info=pi ) )
pi = PadInfo( pinW=mm(3.25), pinL=mm(2.15), exact=True )
p.pins.append( Pad( n=4, hor=True, x=mm(5.8/2), y=0, info=pi ) )
p.AddOutline( H=mm(6.7), W=mm(3.7), type="BOX" )
p.Info( pnum='TPS7B6933QDCYRQ1', mfg='TI', dk='296-43891-2-ND' )
p.Print( "packages/TPS7B" );

# USB connector
p = Part();
pi = PadInfo( pinW=mm(.4), pinL=mm(1.35), exact=True )
x = mm(4.8-2.5)
for i in range(5):
   p.pins.append( Pad( i+1, hor=True, x=x, y=mm((i-2)*.65), info=pi ))
pi = PinInfo( pinW=mm(1), plated=0, exact=True )
for i in range(2):
   p.pins.append( Pin( n=-1, x=x, y=mm(2.5-5*i), info=pi ))
x = mm(1.25-2.5)
pi = PinInfo( pinW=mm(1.2), plated=0, exact=True )
for i in range(2):
   p.pins.append( Pin( n=-1, x=x, y=mm(3.5-7*i), info=pi ))
pi = PadInfo( pinW=mm(1.1), pinL=mm(2.1), exact=True )
for i in range(2):
   p.pins.append( Pad( n=-1, hor=True, x=x, y=mm(3.3/2-i*3.3), info=pi ))

p.AddOutline( H=mm(8.04), W=mm(5), type='BOX' )
p.Info( pnum='DX4R005JJ2R1800', mfg='JAE', dk='670-2675-1-ND' );
p.Print( 'packages/USB' );


# buzzer
p = Part();
pi = PadInfo( pinW=mm(1.3), pinL=mm(1.5), exact=True )
p.pins.append( Pad( 1, hor=True, x=mm(-2.25), y=mm(-1.25-1.3/2), info=pi ))
p.pins.append( Pad( 2, hor=True, x=mm(-2.25), y=mm(  .95+1.3/2), info=pi ))
p.pins.append( Pad( 3, hor=True, x=mm( 2.25), y=mm(  .95+1.3/2), info=pi ))
p.AddOutline( H=mm(5), W=mm(5), type="BOX" )
p.Print( 'packages/buzzer' )
# p.Info( pnum='CMT-0525-75-SMT-TR', mfg='CUI', dk='102-CMT-0525-75-SMT-CT-ND' );

# Dual FET
p = RectPart( 'Q1' )
p.Create( N=6, W=mm(1.3), L=mm(2.5), pitch=mm(.65), pinW=mm(.42), pinL=mm(.6), padExt=mm(2.5), exact=True )
p.Info( pnum='DMN63D8LDW-7', mfg='Diodes Inc', dk='DMN63D8LDW-7CT-ND' )
p.Print( 'packages/dual_fet' )

# Gage pressure sensor.  Note that pins are numbered backward from normal
p = Part();
p.InitFourSided( N=12, W=mm(5), L=mm(5), pitch=mm(1.27), pinW=mm(0.7), pinL=mm(0.65), rowD=mm(4.2), exact=True )
for i in p.pins: 
   i.X *= -1;
pi = PinInfo( pinW=mm(1), exact=True )
p.pins.append( Pin( n=-1, x=mm(2.05-4.2/2), y=mm(4.2/2-1.24), info=pi ) )
p.Info( pnum='MPRLS0001PG0000SA', mfg='Honeywell', dk='480-7100-1-ND' );
p.Print( "packages/MPRLS0001PG" );

# Differential pressure sensor
p = RectPart()
p.Create( N=8, W=mil(485), L=mil(485), pitch=mil(100), pinW=mil(60), pinL=mil(100), rowD=mil(660), exact=True )
p.Info( mfg='NXP', pnum='MP3V5004DP', dk='MP3V5004DP-ND' );
p.Print( "packages/MP3V5004DP" );
      
# Quad encoder w/ switch
p = RectPart()
p.Create( N=6, W=mm(13.4), L=mm(12.4), pitch=mm(2.5), pinW=mm(1), pinL=0, rowD=mm(14.5), smt=False, exact=True, missingPins=[5], outline='BOX' )
pi = PinInfo( pinW=mm(2.6), exact=True )
p.pins.append( Pin( n=-1, x=mm(.25), y=mm( 11.2/2), info=pi ));
p.pins.append( Pin( n=-1, x=mm(.25), y=mm(-11.2/2), info=pi ));
p.Info( mfg='Bourns', pnum='PEC12R-4222F-S0024', dk='PEC12R-4222F-S0024-ND' );
p.Print( 'packages/PEC12R' );
      
# on/off switch
p=Part();
pi = PinInfo( pinW=mm(.8), exact=True );
p.pins.append( Pin( n=1, x=mm(1), y=mm(-2.54), info=pi ));
p.pins.append( Pin( n=2, x=mm(1), y=mm( 2.54), info=pi ));
p.AddOutline( H=mm(9.8), W=mm(5.85) );
p.Info( mfg='NKK', pnum='A11JP', dk='360-2973-ND' )
p.Print( 'packages/A11JP' );
      
# Push button momentary switch
p=Part()
p.Info( mfg='E-switch', pnum='800AWSP9M2QE', dk='EG2605-ND' );
p.AddOutline( H=mm(8.15), W=mm(5.08) );
pi = PinInfo( pinW=mm(.8), exact=True );
p.pins.append( Pin( n=1, x=0, y=mm(-2.54), info=pi ));
p.pins.append( Pin( n=2, x=0, y=mm( 2.54), info=pi ));
p.Print( 'packages/800AWS' );

# Oscillator
p = RectPart();
p.Create( 4, W=mm(2.5), L=mm(3.2), pitch=mm(2.4), pinW=mm(1.4), pinL=mm(1.2), rowD=mm(1.9), name="osc", smt=True, exact=True )
p.Info( mfg='Epson', pnum='SG-310SCF 25.0000MB3', dk='SER3652CT-ND' );
p.Print( "packages/osc" );

# Temperature / humidity sensor
p = Header()
p.Info( mfg='Honeywell', pnum='HIH8120-021-001', dk='480-5706-1-ND' )
p.Create( N=4, pinW=mm(1.05), rows=1, pitch=mil(50), L=mm(4.9), W=m(2), exact=True )
p.Print( 'packages/HIH8120' )
