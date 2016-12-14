FFmpegMediaPlayer
============================

View the project page <a href=http://wseemann.github.io/FFmpegMediaPlayer/>here</a>.

Donations
------------

Donations can be made via PayPal:

**This project needs you!** If you would like to support this project's further development, the creator of this project or the continuous maintenance of this project, **feel free to donate**. Your donation is highly appreciated (and I love food and coffee). Thank you!

**PayPal**

- [**Donate 5 $**] (https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=2BDTFVEW9LFZY): Thank's for creating this project, here's a coffee for you!
- [**Donate 10 $**] (https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=2BDTFVEW9LFZY): Wow, I am stunned. Let me take you to the movies!
- [**Donate 15 $**] (https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=2BDTFVEW9LFZY): I really appreciate your work, let's grab some lunch! 
- [**Donate 25 $**] (https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=2BDTFVEW9LFZY): That's some awesome stuff you did right there, dinner is on me!
- [**Donate 50 $**] (https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=2BDTFVEW9LFZY): I really really want to support this project, great job!
- [**Donate 100 $**] (https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=2BDTFVEW9LFZY): You are the man! This project saved me hours (if not days) of struggle and hard work, simply awesome!
- Of course, you can also [**choose what you want to donate**](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=2BDTFVEW9LFZY), all donations are awesome!

Overview
--------

FFmpegMediaPlayer is a reimplementation of Android's MediaPlayer class. The FFmpegMediaPlayer class provides a unified interface for playing audio files and streams.

Key Features:
* ARM, ARMv7, x86, x86_64, MIPS and ARM_64 support

* Support for additional formats and protocols not provided by Android's MediaPlayer class

Supported protocols:
* file, http, https, mms, mmsh and rtmp

Supported formats (audio and video):
* aac, acc+, avi, flac, mp2, mp3, mp4, ogg, 3gp and more!

Additional support for:
* ICY Metadata (SHOUTcast metadata)

Using FMP in your application (Android Studio)
------------

Add the following maven dependency to your project's `build.gradle` file:

    dependencies {
        compile 'com.github.wseemann:FFmpegMediaPlayer:1.0.4'
    }

or, if your application supports individual architectures extract the appropriate AAR file into you projects "libs" folder:

[Prebuilt AARs] (https://github.com/wseemann/FFmpegMediaPlayer/releases/download/v1.0.4/prebuilt-aars.zip)

(with HTTPS support)

[Prebuilt AARs with HTTPS] (https://github.com/wseemann/FFmpegMediaPlayer/releases/download/v1.0.3/prebuilt-aars-with-https.zip)

Demo Application
------------

A sample application that makes use of FFmpegMediaPlayer can be downloaded [here] (https://github.com/wseemann/FFmpegMediaPlayer/blob/master/FMPDemo.apk?raw=true). Note: The sample application is compiled with support for ALL available formats. This results in a larger library and APK. FFmpeg can be recompiled with a subset of codecs enabled for those wanting a smaller size.

Installation
------------

FFmpegMediaPlayer relies on FFmpeg and native code. The build process
is complex and may be confusing for those unfamiliar the Android NDK. For this
reason I've precompiled the modules created by the build process and checked them
in at: https://github.com/wseemann/FFmpegMediaPlayer/releases/download/v1.0.3/prebuilt-aars.zip.
The modules are also included with the library. If you don't want to build the modules
you can simple unzip the prebuilt ones and copy them to your projects "libs" folder. (Note:
copy them to YOUR projects "libs" folder, NOT the "libs" folder located in
FFmpegMediaPlayer/fmp-library. Once this step is complete you can use the
library (See: Installation in Eclipse (Kepler)). If you want to compile the modules yourself
follow the Ant instructions listed below before attempting to use the library.

Download and install the [Android SDK](http://developer.android.com/sdk/index.html).
Download the [Android NDK](http://developer.android.com/tools/sdk/ndk/index.html).
Clone/Download/Fork the repo through GitHub or via (read-only)

    git clone https://github.com/wseemann/FFmpegMediaPlayer.git

### Ant

Note: The build instructions and scripts assume you are running Unix or Linux. Building
on other operating systems is currently not supported.

Execute the following in the FFmpegMediaPlayer/fmp-library/
directory (assuming /path/to/android_sdk/tools is in your PATH):

    android update project --path .

Open the newly created local.properties file and add the following lines:

    ndk.dir=<path to NDK>
    libs.dir=<path to target libs folder>

where <path to NDK> is the path to your Android NDK, for example:

    ndk.dir=/home/wseemann/Android/android-ndk-r8e

and <path to target libs folder> is the path to the "libs" folder in the project that will use the
library, for example:

    libs.dir=/home/wseemann/Android/MyAndroidDemo/libs

**Note:** If you wish to enable https support (for use with API 8+ only) navigate to FFmpegMediaPlayer/fmp-library/ and execute

    ant build-ffmpeg-with-ssl

To compile the library, navigate to FFmpegMediaPlayer/fmp-library/ and
execute

    ant clean debug

### Installation in Eclipse (Kepler)

The first step is to choose File > Import or right-click in the Project Explorer
and choose Import. If you don't use E-Git to integrate Eclipse with Git, skip
the rest of this paragraph. Choose "Projects from Git" as the import source.
From the Git page, click Clone, and enter the URI of this repository. That's the
only text box to fill in on that page. On the following pages, choose which
branches to clone (probably all of them) and where to keep the local checkout,
and then click Finish. Once the clone has finished, pick your new repository
from the list, and on the following page select 'Use the New Projects wizard'.

From here the process is the same even if you don't use E-Git. Choose 'Android
Project from Existing Code' and then browse to where you checked out 
FFmpegMediaPlayer. Select the fmp-library folder and click Finish.

Finally, to add the library to your application project, right-click your
project in the Package Explorer and select Properties. Pick the "Android" page,
and click "Add..." from the bottom half. You should see a list including the
FFmpegMediaPlayer project as well as any others in your workspace.

Usage
------------

Sample code:

    FFmpegMediaPlayer mp = new FFmpegMediaPlayer();
    mp.setOnPreparedListener(new FFmpegMediaPlayer.OnPreparedListener() {
			
        @Override
        public void onPrepared(FFmpegMediaPlayer mp) {
            mp.start();			
        }
    });
    mp.setOnErrorListener(new FFmpegMediaPlayer.OnErrorListener() {
			
        @Override
        public boolean onError(FFmpegMediaPlayer mp, int what, int extra) {
            mp.release();
            return false;
        }
    });
		
    try {
        mp.setDataSource("<some path or URL>");
        mp.prepareAsync();
    } catch (IllegalArgumentException e) {
        e.printStackTrace();
    } catch (SecurityException e) {
        e.printStackTrace();
    } catch (IllegalStateException e) {
        e.printStackTrace();
    } catch (IOException e) {
        e.printStackTrace();
    }

FFmpeg
-----------
This software uses code from <a href=http://ffmpeg.org>FFmpeg</a> licensed under the <a href=http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html>LGPLv2.1</a> and its source can be downloaded <a href=https://github.com/wseemann/FFmpegMediaPlayer/blob/master/fmp-library/ffmpeg-2.1-android-2013-11-13.tar.gz>here</a>. It also uses code from <a href=https://www.libsdl.org>SDL</a> licensed under the <a href=http://www.gzip.org/zlib/zlib_license.html>zlib license</a>.

License
------------

```
FFmpegMediaPlayer: A unified interface for playing audio files and streams.

Copyright 2016 William Seemann

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
