#!/usr/bin/env python3
"""Offline asset rebake; requires Pillow, librsvg, cairo and gobject. Run at repo root."""
from pathlib import Path
import ctypes as c,json,argparse,tempfile

parser=argparse.ArgumentParser(description="Rebuild bundled flags and relief from their source assets")
parser.add_argument("--height-source",type=Path,required=True,help="Natural Earth III elev_bump_4k.jpg; see data/assets.json")
args=parser.parse_args()
workspace=tempfile.TemporaryDirectory()
work=Path(workspace.name)
from PIL import Image
rsvg=c.CDLL('librsvg-2.so.2');cairo=c.CDLL('libcairo.so.2');gobject=c.CDLL('libgobject-2.0.so.0')
rsvg.rsvg_handle_new_from_data.argtypes=[c.c_void_p,c.c_size_t,c.c_void_p];rsvg.rsvg_handle_new_from_data.restype=c.c_void_p
rsvg.rsvg_handle_render_cairo.argtypes=[c.c_void_p,c.c_void_p];rsvg.rsvg_handle_render_cairo.restype=c.c_int
class Dimensions(c.Structure):_fields_=[('width',c.c_int),('height',c.c_int),('em',c.c_double),('ex',c.c_double)]
rsvg.rsvg_handle_get_dimensions.argtypes=[c.c_void_p,c.POINTER(Dimensions)]
cairo.cairo_image_surface_create.argtypes=[c.c_int,c.c_int,c.c_int];cairo.cairo_image_surface_create.restype=c.c_void_p
cairo.cairo_create.argtypes=[c.c_void_p];cairo.cairo_create.restype=c.c_void_p
cairo.cairo_scale.argtypes=[c.c_void_p,c.c_double,c.c_double]
cairo.cairo_surface_write_to_png.argtypes=[c.c_void_p,c.c_char_p];cairo.cairo_destroy.argtypes=[c.c_void_p];cairo.cairo_surface_destroy.argtypes=[c.c_void_p];gobject.g_object_unref.argtypes=[c.c_void_p]
width,height=96,72;atlas=Image.new('RGBA',(16*width,12*height),(0,0,0,0));metadata=json.loads(Path('data/flags.json').read_text());mapping=[dict(code=code,**item) for code,item in metadata['nations'].items()];rendered={}
for item in mapping:
 iso=item['iso'];source=Path('data/flags')/(iso+'.svg')
 if iso not in rendered:
  data=source.read_bytes();handle=rsvg.rsvg_handle_new_from_data(data,len(data),None);assert handle
  dimensions=Dimensions();rsvg.rsvg_handle_get_dimensions(handle,c.byref(dimensions))
  surface=cairo.cairo_image_surface_create(0,width,height);ctx=cairo.cairo_create(surface);cairo.cairo_scale(ctx,width/dimensions.width,height/dimensions.height)
  assert rsvg.rsvg_handle_render_cairo(handle,ctx)
  path=work/('flag-'+iso+'.png');assert cairo.cairo_surface_write_to_png(surface,str(path).encode())==0
  cairo.cairo_destroy(ctx);cairo.cairo_surface_destroy(surface);gobject.g_object_unref(handle)
  rendered[iso]=Image.open(path).copy()
 index=item['index'];atlas.paste(rendered[iso],((index%16)*width,(index//16)*height))
atlas.save('data/flags-atlas.png',optimize=True)

heightmap=Image.open(args.height_source).convert('L').resize((2048,1024),Image.Resampling.LANCZOS);heightmap.save('data/terrain-height.png',optimize=True)
print('Flags:',len(mapping),'source SVGs:',len(rendered),'atlas bytes:',Path('data/flags-atlas.png').stat().st_size,'terrain bytes:',Path('data/terrain-height.png').stat().st_size)
