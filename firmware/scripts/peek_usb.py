
def main():
   base = 0x40006800
   regs = peek32( base, 23 )

   print 'Main registers:'

   for i in range(8):
      print '0x%08x - endpoint[%d]: 0x%08x' % (base+4*i, i,regs[i])

   names = [ 'ctrl', 'status', 'frame', 'addr', 'btable', 'lpm', 'battery' ]

   for i in range(len(names)):
      print '0x%08x - %-12.12s 0x%08x' % (base+64+4*i, names[i]+':',regs[16+i])

   print 'Buffer description table:'
   print '           -          txAddr   txCt  rxAddr   rxCt'
   base = 0x40006C00

   # Note - I read an extra two words because that causes my
   # peek to read data in 16-bit units rather then 32-bit
   tbl = []
   tbl = peek16( base, 513 )

   for i in range(8):
      print '0x%08x - Entry %d: 0x%04x  0x%04x  0x%04x  0x%04x' % (base+i*8, i, tbl[4*i], tbl[4*i+1], tbl[4*i+2], tbl[4*i+3] )

   addr = base
   for i in range(8):
      S = '0x%08x: ' % addr
      for j in range(16):
         S += '0x%04x ' % tbl[16*i+j]
      addr += 32
      print S

main()
