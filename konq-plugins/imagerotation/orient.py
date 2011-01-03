#! /usr/bin/env python
import os
import sys
import exif

def compose(delta, old):
  map=[0, 4, 2, 6, 5, 1, 7, 3]
  unmap=[1, 6, 3, 8, 2, 5, 4, 7]
  x = map[delta-1]
  y = map[old-1]
  z = ((x^y)&4) + ((y+(x&3)*(((y&4)>>1)+1))&3)
  return unmap[z]

def deg2o(d):
  map={90:6, 270:8, 180:3}
  if map.has_key(d):
    return map[d]
  else:
    return 0

if len(sys.argv) < 2:
  print 'Usage: %s [[+]orientnum] file\n' % sys.argv[0]
  sys.exit(1)
try:
  if len(sys.argv) == 2:
    filename=sys.argv[1]
    file=open(filename, "r");
  else:
    filename=sys.argv[2]
    mod=sys.argv[1]
    fd = os.open(filename, os.O_RDWR)
    file=os.fdopen(fd,'r')
    # check file exists and is readable
    file.read(1)
    file.seek(0,0)
except:
  print 'Cannot open', filename
  sys.exit(1)

tags=exif.process_file(file,0,1)
if not tags:
  print 'no EXIF information in', filename
  sys.exit(1)
if not tags.has_key('Exif Offset') \
    or not tags.has_key('Image Orientation'):
  print 'cannot get orientation info in', filename
  sys.exit(1)

exifp = tags['Exif Offset']
endian = tags['Exif Endian']
tagp = tags['Image Orientation'].field_offset

orientp = exifp + tagp

if endian == 'M': # MM byte order
  orientp += 1

file.seek(orientp)
o = ord(file.read(1))

if o < 1 or o > 8:
  print 'orientation out of range', o
  sys.exit(1)

if len(sys.argv) == 2:
  print 'orientation is', o
  sys.exit(0)

try:
  if mod[0] == '+':
    deltao = int(mod)
    if 1 <= deltao and deltao <= 8:
      newo = compose(deltao, o)
    elif deg2o(deltao) != 0:
      newo = compose(deg2o(deltao), o)
    else:
      print 'cannot understand orientation modification', mod
      sys.exit(1) # it will still hit the except ... how to fix?
  else:
    newo = int(mod)
except:
  print 'expected numeric orientation and got',mod
  sys.exit(1)

if newo < 1 or newo > 8:
  newo = deg2o(newo)
  if newo == 0:
    print 'cannot understand orientation', deltao
    sys.exit(1)

os.lseek(fd,orientp,0)
os.write(fd,chr(newo))

# Thumbnail orientation :
thumb_ifdp = 0
if tags.has_key('Thumbnail Orientation'):
  thumb_tagp = tags['Thumbnail Orientation'].field_offset
  thumb_orientp = exifp + thumb_tagp
  if endian == 'M': # MM byte order
    thumb_orientp += 1
  os.lseek(fd,thumb_orientp,0)
  os.write(fd,chr(newo))

print 'orientation changed from', o, 'to', newo
