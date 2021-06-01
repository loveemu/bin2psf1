bin2psf1
========
[![Travis Build Status](https://travis-ci.com/loveemu/bin2psf1.svg?branch=master)](https://travis-ci.com/loveemu/bin2psf1) [![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/u277yvva4lli3wtr/branch/master?svg=true)](https://ci.appveyor.com/project/loveemu/bin2psf1/branch/master)

Convert a binary file into PSF1 format.
<https://github.com/loveemu/bin2psf1>

Downloads
---------

- [Latest release](https://github.com/loveemu/bin2psf1/releases/latest)

Usage
-----

Syntax: `bin2psf1 <Input file> <Start address (hex)> <Initial PC (hex)> <Initial SP (hex)>`

Note: Padding bytes will be added unless input file size is a multiple of 2048 bytes.

### Options ###

--help
  : Show help

-o, --out-filename filename
  : Specify output filename

-r, --region name
  : Region (North America, Japan or Europe)
