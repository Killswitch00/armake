armake
======

[![](https://img.shields.io/travis/KoffeinFlummi/armake/master.svg?style=flat-square)](https://travis-ci.org/KoffeinFlummi/armake)
[![](https://img.shields.io/badge/license-GPLv2-red.svg?style=flat-square)](https://github.com/KoffeinFlummi/armake/blob/master/LICENSE)
[![](https://img.shields.io/github/tag/KoffeinFlummi/armake.svg?style=flat-square)](https://github.com/KoffeinFlummi/armake/releases)
[![](https://img.shields.io/badge/AUR-armake-blue.svg?style=flat-square)](https://aur.archlinux.org/packages/armake)
[![](https://img.shields.io/badge/PPA-koffeinflummi%2Farmake-orange.svg?style=flat-square)](https://launchpad.net/~koffeinflummi/+archive/ubuntu/armake)


A C implementation of Arma modding tools (PAA conversion, binarization/rapification, PBO packing). (WIP)


### Setup

#### From Source

```
$ make
$ sudo make install
```

**Dependencies:**
- An FTS library (glibc has one)
- OpenSSL

#### Arch Linux

[PKGBUILD](https://aur.archlinux.org/packages/armake/) or [PKGBUILD (development)](https://aur.archlinux.org/packages/armake-git/)
```
$ pacaur -S armake      # or use yaourt or whatever AUR helper you use
$ pacaur -S armake-git
```

#### Ubuntu & Other Debian Derivatives

[PPA](https://launchpad.net/~koffeinflummi/+archive/ubuntu/armake)

```
$ sudo add-apt-repository ppa:koffeinflummi/armake
$ sudo apt-get update
$ sudo apt-get install armake
```


### Usage

See `$ armake --help` or [src/usage](https://github.com/KoffeinFlummi/armake/blob/master/src/usage).


### Credits & Thanks

- [Mikero](https://dev.withsix.com/projects/mikero-pbodll) for his great documentation of the various file formats used.
- [T_D](https://github.com/Braini01) for great documentation, lots of pointers and even some code contributions.
- [jonpas](https://github.com/jonpas) for all kinds of help with development and testing.
- [kju](https://forums.bistudio.com/user/768005-kju/) for some pointers and "PR work".
- [Glowbal](https://github.com/Glowbal) for the name.
- [4d4a5852](https://github.com/4d4a5852) for his work with [a3lib](https://github.com/4d4a5852/a3lib.py).


### Used Libraries

- [docopt](https://github.com/docopt/docopt.c)
- [MiniLZO](http://www.oberhumer.com/opensource/lzo/)
- [STB's image libraries](https://github.com/nothings/stb)
- [Paul E. Jones's SHA-1 implementation](https://www.packetizer.com/security/sha1/)


### Disclaimer

This isn't official BI software. As such, it may not compile certain addons correctly or lag behind BI when new file format versions are introduced. As stated in the GPL, this software is provided without any warranty, so use at your own risk.


---

<p align="center">
    <a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=WQ55N7RKXUCF8">
        <img src="https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif" style="max-width:100%;">
    </a>
    <br>
    <b>BTC</b> 1K3mKJDJYgJcQFavLL9zHBCGfKj6gmZC7J
</p>
