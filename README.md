# bin2js
Fast .bin to .js payload converter for use with sleirsgoevy's PS4 jailbreaks.

```
Usage: bin2js [-cr] file1/directory1 [file2/directory2 ...]
   Or: bin2js [-c] file1 -o file2

  Converts PS4 binary payload files to JavaScript payload files, following
  the pattern "filename.bin" => "filename.js" unless an output file is
  explicitly specified (option -o). Existing output files will be overwritten.

  Options:
    -c  Compress JavaScript code further, to one single line
    -h  Display this help info
    -o  Write to the specified output file only
    -r  Recursively traverse subdirectories
    --  All arguments following this option are treated as non-options
```

Batch-convert an unlimited number of files and/or (sub)directories, for example:

    bin2js goldhen.bin dumper.bin payloads/ morepayloads/ ftp.bi
    
Which will automatically create "goldhen.js", "dumper.js", "ftp.js", and convert all .bin files in the directories "payloads/"
and "morepayloads/" from *.bin to *.js.

Or convert a single file and specify the output file name:

    bin2js goldhen.bin -o goldhen.js

To compile, just enter
    
    make bin2js

Windows binaries are available in the Release section: https://github.com/hippie68/bin2js/releases.
