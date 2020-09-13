# ntrview
A Wii U client for NTR CFW's RemotePlay.

No fancy features.

## Configuration
ntrview uses a .ini file at `sd:/wiiu/apps/ntrview/ntrview.ini`. Here's a sample file:
```
[3ds]
ip=10.0.0.45
priority=1
priorityFactor=5
jpegQuality=80
QoS=18

[display]
background_r=0x00
background_g=0x00
background_b=0x00
```
`ip` is the IP address of your N3DS, while `priority`, `priorityFactor`, `jpegQuality` and `QoS` are standard NTR CFW options. If you're looking for values to try, you could check out the presets included with [Snickerstream](https://github.com/RattletraPM/Snickerstream/blob/master/Snickerstream.au3#L1039) - the sample file here uses Preset 1. `background_*` are the RGB background colour behind the 3DS image, which peeks through because of the 3DS's 15:9 aspect ratio.

ntrview has a set of sensible defaults, that you can read in main.cpp. Therefore, a minimal config file only needs to set the 3DS IP:
```
[3ds]
ip=192.168.192.15
```

Here's some other snippets you might want to include in your configs:
```
[profile:0]
layout_1080p_tv_top_x=20
layout_1080p_tv_top_y=180
layout_1080p_tv_top_w=1200
layout_1080p_tv_top_h=720

layout_1080p_tv_btm_x=1260
layout_1080p_tv_btm_y=420
layout_1080p_tv_btm_w=640
layout_1080p_tv_btm_h=480

layout_drc_top_x=227
layout_drc_top_y=0
layout_drc_top_w=400
layout_drc_top_h=240

layout_drc_btm_x=267
layout_drc_btm_y=240
layout_drc_btm_w=320
layout_drc_btm_h=240
```
The `layout_` options control where the top and bottom screens are drawn. In the example given, the bottom screen will be drawn on the TV at coordinates (1260,420) of size 640x480. The TV might use `layout_1080p_tv`, `layout_720p_tv` or `layout_480p_tv` depending on what resolution the console is set to. The Gamepad always uses 480p.

I'm aware that this part of the README sucks, if anyone wants to write better documentation or a Wiki or make tutorials or whatever I'd very much appreciate that.
