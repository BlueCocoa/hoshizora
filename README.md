# hoshizora
Mix two images using alpha channel to create a beautiful effect

Tested on macOS

### Installation
```$ sudo make install```

### Usage

```hoshizora -f front.jpg -b back.jpg -o rem.png -d 64```

```
-f front layer, any image format that supported by OpenCV
-b back layer, any image format that supported by OpenCV
-o output image, png image format gray + alpha
-i increase brightness of front layer
     negetive value is accepted in case you need to decrease the brightness for front layer
-d decrease brightness of back layer
     negetive value is accepted in case you need to increase the brightness for back layer
```

### Screenshots

![Screenshots](https://raw.githubusercontent.com/BlueCocoa/hoshizora/master/screenshot.png)
