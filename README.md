# *xarrow* - An arrow, watching your cursor
## Features:
- [x] X11 resources
- [x] Transparency
- [x] Relatively low CPU usage
- [ ] Works in Wayland

## Installation

### Manual

To install *xarrow*, `cd` to project diretory and run `make install` to install to `/usr/bin` or
`make local_install` to install to `/usr/local/bin`.

#### Dependencies
Generally, you need GCC, make, and X11.
##### Ubuntu
```shell
sudo apt install build-essential libx11-dev
```
##### Arch
```shell
sudo pacman -S base-devel libx11
```

### AUR
[xarrow](https://aur.archlinux.org/packages/xarrow) is available on Arch User Repository.


## Usage
```shell
xarrow -bg red -fg blue -transparency off
```
This command will launch xarrow with opaque blue background and red foreground.

You can specify default values in X resource file in your home directory.

*~/.Xresources:*
```
Xarrow.foreground: #FF00FF
Xarrow.background: #00FF00
```

## TODO
- CLI arguments should override x-resources

## Screenshots
![image](https://user-images.githubusercontent.com/66270324/223154703-51b937bd-f926-420c-b9f0-e79841a52b3d.png)
