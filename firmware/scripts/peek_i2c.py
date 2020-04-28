
def main():
   reg = peek32( 0x40005400, 8 )

   print '0x%08x: ctrl[0]:    0x%08x' % (0x40005400, reg[0])
   print '0x%08x: ctrl[1]:    0x%08x' % (0x40005404, reg[1])
   print '0x%08x: status:     0x%08x' % (0x40005418, reg[6])

main()
