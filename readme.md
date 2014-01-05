# Biowatch
Biowatch can be used to track multiple targets in a scene using particle filtering technique. Biowatch can either track objects by their color, or by their **darkness**(default) in the scene. You start the program with a movie file, click on the targets and adjust size if needed, and unpause -- and voila! the selected objects are tracked automatically. 

The tracking is not perfect and it is possible that the tracker will lose the target. In this case you can pause the tracking, roll back to to the time when you the target was lost, and manually fix the tracked location. The tracker will then continue with the corrected location when unpaused. 

## Installation

### From source (not recommended)

### From binary on a Mac
Download and unzip the BioWatch.zip file, which will give you `BioWatch.app` in your current folder. Note that this is not a standard Mac app bundle, so you can't just click on it from Finder and expect it to run. We must run it from command line. For example if you unzipped the Biowatch.zip file in the current folder, then you can run the following command to get command line options:

    $ ./BioWatch.app/Contents/MacOs/track_pf --help 


`track_pf` here is the main executable.

## Usage

### Command Line
The simplest usage is:

    $ ./BioWatch.app/Contents/MacOs/track_pf --video=/path/to/myvideo.mov
    
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

### Runtime Control
Keyboard control:

Key     | Action
--------|-------
 [space]| Pause/unpause
[delete]| Delete selected object
 [<],[>]| Rotate selected object
 [w],[s]| Change selected tracked target position up/down
 [a],[d]| Change selected tracked target position left/right
 [r],[f]| Change selected tracked target size
     [z]| Show measurement
     [p]| Show particles
     [o]| Write output
[escape]| Write output and quit


Mouse control:

- Double click or right click to add a new object
- While paused, drag and object to manually update position



## Output Format
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
