
def main():
   stat = peek32( 0x40020000, 2 )
   ch6 = peek32( 0x4002006c, 4 )
   sel = peek32( 0x400200a8, 1 )

   print '0x%08x: int status: 0x%08x' % (0x40020000, stat[0])
   print '0x%08x: int clear:  0x%08x' % (0x40020004, stat[1])
   print '0x%08x: ch6 config: 0x%08x' % (0x4002006C, ch6[0])
   print '0x%08x: ch6 count:  0x%08x' % (0x40020070, ch6[1])
   print '0x%08x: ch6 paddr:  0x%08x' % (0x40020074, ch6[2])
   print '0x%08x: ch6 maddr:  0x%08x' % (0x40020078, ch6[3])
   print '0x%08x: chan sel:   0x%08x' % (0x400200A8, sel[0])

main()
