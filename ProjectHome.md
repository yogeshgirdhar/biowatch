# Biowatch #

<a href='http://www.youtube.com/watch?feature=player_embedded&v=ZrZpALx6qaI' target='_blank'><img src='http://img.youtube.com/vi/ZrZpALx6qaI/0.jpg' width='425' height=344 /></a>


Biowatch can be used to track multiple targets in a scene using particle filtering technique. Biowatch can either track objects by their color, or by their darkness(default) in the scene. You start the program with a movie file, click on the targets and adjust size if needed, and unpause -- and voila! the selected objects are tracked automatically.

The tracking is not perfect and it is possible that the tracker will lose the target. In this case you can pause the tracking, roll back to to the time when you the target was lost, and manually fix the tracked location. The tracker will then continue with the corrected location when unpaused.

## Installation ##

### Building from source ###
You need OpenCV, Boost, GCC4.7 or greater, and CMake to build this project. Once these dependencies are installed then do the following to build the project:

```
$ git clone https://code.google.com/p/biowatch/
$ cd biowatch
$ mkdir build
$ cd build
$ cmake ..
$ make
```

Not that if the default compiler is not GCC4.7 or higher, but you have it installed then you should specify the compiler manually during the `cmake` step
```
CXX=g++-4.7 cmake ..
```


`track_pf` binary will now be in the build folder.


### From binary on a Mac ###
Download and unzip the [BioWatch.zip](https://www.dropbox.com/sh/069gx2wsk8n03mz/y8Y7-XCq97) file, which will give you `BioWatch.app` in your current folder. Note that this is not a standard Mac app bundle, so you can't just click on it from Finder and expect it to run. We must run it from command line. For example if you unzipped the Biowatch.zip file in the current folder, then you can run the following command to get command line options:
```
    $ ./BioWatch.app/Contents/MacOs/track_pf --help 
```

`track_pf` here is the main executable.

## Usage ##

### Command Line ###
The simplest usage is:
```
    $ ./BioWatch.app/Contents/MacOs/track_pf --video=/path/to/myvideo.mov
```
other options are:

```
  --help                       help
  --video arg                  Video file
  --scale arg (=1)             scaling
  --subsample arg (=1)         Temporal subsampling rate for input video.
  --owidth arg (=20)           Maximum width of the object (post scaling)
  --oheight arg (=4)           Maximum height of the object (post scaling)
  --senstivity arg (=4)        2.0 = low, 4 = mid (default), 10=high
  --skip arg (=0)              skip the initial frames
  --mask arg                   mask file
  --begin arg (=0)             begin time in seconds
  --end arg (=-1)              end time in seconds
  -o [ --out ] arg (=out.txt)  Output coordinates
  --particles arg (=100)       Number of particles used. Higher number will 
                               result in smoother tracking.
  --sigma_pos arg (=1)         more jumpy the object is, higher this number 
                               should be. It is used after multiplying it with 
                               max_len
  --sigma_theta arg (=30)      more irratic the object turning is, higher this 
                               number should be. 
  
```

### Runtime Control ###

Keyboard control:
```
 [space]: toggle pausing
[delete]: delete selected object
 [<],[>]: rotate selected object
 [w],[s]: change selected tracked target position UP/DOWN
 [a],[d]: change selected tracked target position LEFT/RIGHT
 [r],[f]: change selected tracked target size
     [z]: show measurement
     [p]: show particles
     [o]: write output
[escape]: write output and quit
```

Mouse control:
> - double click or right click to add a new object
> - While paused, drag and object to manually update position


## Output Format ##
The coordinates of each target is dumped in a file when you press the `[o]` key, or when you exist the program by hitting `[esc]`. The output is a simple text file with the following format

```
<oid> <time> <X> <Y> <widht> <height> <angle> <valid?>
```

Here:

`<oid>` = object id. This is a number between 0-(n-1), where n is the number of objects you are tracking.

`<time>` = Video frame number.

`<X>,<Y>` = position of the object in pixels coordinates.

`<width>,<heigh>` = size of the object in pixels.

`<angle>` = orientation of the tracked object -- in degrees between -180,180

`<valid?>` = If 1 then the data on this line is valid, otherwise the object was not tracked at this time step.


## How to Cite ##
_Yogesh Girdhar and Sofia Ibarraran, Biowatch: A multiple target tracker for analyzing animal behavior, 2014. Available at https://code.google.com/p/biowatch_