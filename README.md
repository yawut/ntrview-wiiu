# ntrview
A Wii U client for NTR CFW's RemotePlay.

Top screen only for now, no fancy features.

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
