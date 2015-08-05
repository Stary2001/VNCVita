#!/usr/bin/env ruby

print "#include <vita2d.h>\n#include \"font.h\"\n unsigned char map_chr[] = \""

def pack_short x
  "\\x" + (x & 0xff).to_s(16).rjust(2, "0") + "\\x" + ((x & 0xff00) >> 8).to_s(16).rjust(2, "0")
end

arr = {}

i = 0

(0..15).each do |y|
  (0..15).each do |x|
    arr[i] = [x * 8, y * 8]
    i = i + 1
  end
end 

(0..255).each do |a| if arr[a].nil? then print(pack_short(0) * 2) else print(pack_short(arr[a][0]) + pack_short(arr[a][1])) end end

print "\";\npos_t *map = (pos_t*)map_chr; uint32_t map_len = 256;\n"
