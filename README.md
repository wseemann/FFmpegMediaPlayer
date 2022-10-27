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

This project uses FFmpeg version 4.2.2.

Key Features:
* ARMv7, x86, x86_64 and ARM_64 support (Note: ARM and MIPS aren't supported as of version 1.0.4)
* Support for API 16+

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
        implementation 'com.github.wseemann:FFmpegMediaPlayer-core:1.0.5'
        implementation 'com.github.wseemann:FFmpegMediaPlayer-native:1.0.5'
    }

Optionally, to support individual ABIs:

    dependencies {
        implementation 'com.github.wseemann:FFmpegMediaPlayer-core:1.0.5'
        implementation 'com.github.wseemann:FFmpegMediaPlayer-native-armeabi-v7a:1.0.5'
        implementation 'com.github.wseemann:FFmpegMediaPlayer-native-x86:1.0.5'
        implementation 'com.github.wseemann:FFmpegMediaPlayer-native-x86_64:1.0.5'
        implementation 'com.github.wseemann:FFmpegMediaPlayer-native-arm64-v8a:1.0.5'
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
library. If you want to compile the modules yourself follow the build instructions
listed below before attempting to use the library.

Download and install the [Android SDK](http://developer.android.com/sdk/index.html).
Download the [Android NDK](http://developer.android.com/tools/sdk/ndk/index.html).
Clone/Download/Fork the repo through GitHub or via (read-only)

    git clone https://github.com/wseemann/FFmpegMediaPlayer.git

### Android Studio (Gradle)

Note: The build instructions and scripts assume you are running Unix or Linux. Building
on other operating systems is currently not supported.

Execute the following in the root project directory (assuming /path/to/android_sdk/tools is in your PATH):

    android update project --path .

Open the newly created local.properties file and add the following lines:

    sdk.dir=<path to SDK>
    ndk.dir=<path to NDK>

where <path to SDK> is the path to your Android SDK, for example:

    sdk.dir=/Users/wseemann/Library/Android/sdk

where <path to NDK> is the path to your Android NDK, for example:

    ndk.dir=/home/wseemann/Android/android-ndk-r20

To compile the library in Android Studio, highlight `core` in the project explorer and run Build->Make Module 'core'. This will also build the native FFmpeg binaries.

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

Copyright 2022 William Seemann

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
