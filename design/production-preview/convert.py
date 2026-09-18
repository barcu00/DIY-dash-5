"""Lossless conversion of production LVGL framebuffers, no UI drawing."""
import sys
from pathlib import Path
from PIL import Image
output = Path(sys.argv[1])
paths = list(output.glob('firmware-*.ppm'))
assert len(paths) == 25
for path in paths:
    with Image.open(path) as image:
        assert image.size == (800, 480)
        if path.stem == 'firmware-strip-style':
            pixels=image.convert('RGB')
            green={(x,y) for y in range(20,135) for x in range(8,790)
                   if pixels.getpixel((x,y))[1]>180 and
                   pixels.getpixel((x,y))[0]<80 and pixels.getpixel((x,y))[2]<120}
            blocks=[]; ticks=[]
            while green:
                pending=[green.pop()]; component=[]
                while pending:
                    x,y=pending.pop(); component.append((x,y))
                    for neighbor in ((x-1,y),(x+1,y),(x,y-1),(x,y+1)):
                        if neighbor in green:
                            green.remove(neighbor); pending.append(neighbor)
                if len(component)>100: blocks.append(component)
                elif len(component)>=5: ticks.append(component)
            assert len(blocks)>=10, ('missing Strip blocks',len(blocks))
            for block in blocks:
                width=max(x for x,y in block)-min(x for x,y in block)+1
                height=max(y for x,y in block)-min(y for x,y in block)+1
                assert (width,height,len(block))==(18,28,504), ('uneven/slanted Strip block',width,height,len(block))
            assert len(ticks)>=10, ('missing Strip tick marks',len(ticks))
            for tick in ticks:
                width=max(x for x,y in tick)-min(x for x,y in tick)+1
                assert width<=3, ('Strip tick is tilted instead of vertical',width,tick)
        if path.stem in {'firmware-analog-style','firmware-modern-motorsport'}:
            pixels=image.convert('RGB')
            for row in range(6):
                top=16+row*68
                ys=[y for y in range(top+1,top+54)
                    if any(min(pixels.getpixel((x,y)))>210 for x in range(600,780))]
                assert ys, ('missing digits',row)
                assert abs((min(ys)+max(ys))/2-(top+30.5))<=1.5, ('off-center digits',row,ys)
        image.save(path.with_suffix('.png'))
