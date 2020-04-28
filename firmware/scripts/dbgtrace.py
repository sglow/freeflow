
def main():
   cl = cmdline.split()
   if( len(cl) > 1 and cl[1] == '-r' ):
      SetVar( 'trace_ctrl', 2 )
      return

   ct = GetVar( 'trace_samples' )
   if( ct < 1 ):
      print 'Nothing in the trace buffer'
      return

   if( len(cl) > 1 ):
      maxLines = int(cl[1],0)
      if( ct > 4*maxLines ):
         ct = 4*maxLines;

   dat = peek16( 0x20006000, ct )
   for i in range(len(dat)/4):
      print '%5d 0x%04x 0x%04x 0x%04x' % (dat[4*i], dat[4*i+1], dat[4*i+2], dat[4*i+3])

main()
