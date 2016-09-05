# hoshizora
Mix two images using alpha channel to create a beautiful effect

Tested on macOS

### Installation
```$ sudo make install```

### Usage

```hoshizora -f front.jpg -b back.jpg -o rem.png -1 224 -2 210```

```
-f front layer, any image format that supported by OpenCV
-b back layer, any image format that supported by OpenCV
-o output image, any image format within alpha channel and is supported by OpenCV
-1 front layer threshold, optional argument
-2 back layer threshold, optional argument
```

### Screenshots

![Screenshots](https://raw.githubusercontent.com/BlueCocoa/hoshizora/master/screenshot.png)
