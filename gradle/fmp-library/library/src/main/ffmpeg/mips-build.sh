#!/bin/bash

if [ "$NDK" = "" ]; then
	echo NDK variable not set, assuming ${HOME}/android-ndk
	export NDK=${HOME}/android-ndk
fi

# Detect OS
OS=`uname`
HOST_ARCH=`uname -m`
export CCACHE=; type ccache >/dev/null 2>&1 && export CCACHE=ccache
if [ $OS == 'Linux' ]; then
        if [ $HOST_ARCH == 'i686' ]; then
            HOST_ARCH=x86
        fi
	export HOST_SYSTEM=linux-$HOST_ARCH
elif [ $OS == 'Darwin' ]; then
	export HOST_SYSTEM=darwin-$HOST_ARCH
fi

SOURCE=`pwd`

TOOLCHAIN=/tmp/servestream
SYSROOT=$TOOLCHAIN/sysroot/

export CROSS_PREFIX=mipsel-linux-android-
$NDK/build/tools/make-standalone-toolchain.sh --toolchain=${CROSS_PREFIX}4.6 --platform=android-9 \
--system=$HOST_SYSTEM --install-dir=$TOOLCHAIN

export PATH=$TOOLCHAIN/bin:$PATH
export CC="$CCACHE ${CROSS_PREFIX}gcc"
export CXX=${CROSS_PREFIX}g++
export LD=${CROSS_PREFIX}ld
export AR=${CROSS_PREFIX}ar
export STRIP=${CROSS_PREFIX}strip

rm -rf build/ffmpeg
mkdir -p build/ffmpeg
cd ffmpeg

# Don't build any neon version for now
for version in mips; do

	DEST=$SOURCE/build/ffmpeg
    FLAGS="--target-os=linux"
    FLAGS="$FLAGS --enable-cross-compile"
    FLAGS="$FLAGS --cross-prefix=$CROSS_PREFIX"
    FLAGS="$FLAGS --arch=mips"
    FLAGS="$FLAGS --cpu=mips32r2"
    FLAGS="$FLAGS --enable-runtime-cpudetect"
    FLAGS="$FLAGS --enable-yasm"
    FLAGS="$FLAGS --disable-mipsfpu"
    FLAGS="$FLAGS --disable-mipsdspr1"
    FLAGS="$FLAGS --disable-mipsdspr2"
    FLAGS="$FLAGS --sysroot=$SYSROOT"
	FLAGS="$FLAGS --enable-shared --disable-symver"
	FLAGS="$FLAGS --enable-small --optimization-flags=-O2"
	FLAGS="$FLAGS --disable-doc"
	FLAGS="$FLAGS --disable-ffmpeg"
	FLAGS="$FLAGS --disable-ffplay"
	FLAGS="$FLAGS --disable-ffprobe"
	FLAGS="$FLAGS --disable-ffserver"
	FLAGS="$FLAGS --disable-avdevice"
    #FLAGS="$FLAGS --disable-swresample"
	FLAGS="$FLAGS --disable-postproc"
	FLAGS="$FLAGS --disable-avfilter"
	#FLAGS="$FLAGS --disable-everything"
	FLAGS="$FLAGS --disable-gpl"
	FLAGS="$FLAGS --disable-encoders"
	#FLAGS="$FLAGS --disable-decoders"
	FLAGS="$FLAGS --disable-hwaccels"
	FLAGS="$FLAGS --disable-muxers"
	#FLAGS="$FLAGS --disable-demuxers"
	#FLAGS="$FLAGS --disable-parsers"
	FLAGS="$FLAGS --disable-bsfs"
	FLAGS="$FLAGS --disable-protocols"
	FLAGS="$FLAGS --disable-indevs"
	FLAGS="$FLAGS --disable-outdevs"
	FLAGS="$FLAGS --disable-devices"
	FLAGS="$FLAGS --disable-filters"
	#FLAGS="$FLAGS --enable-demuxer=aac,flac,h263,h264,m4v,matroska,mp3,mpegvideo,ogg,pcm_alaw,pcm_f32be,pcm_f32le,pcm_f64be,pcm_f64le,pcm_mulaw,pcm_s16be,pcm_s16le,pcm_s24be"
	#FLAGS="$FLAGS --enable-demuxer=pcm_s24le,pcm_s32be,pcm_s32le,pcm_s8,pcm_u16be,pcm_u16le,pcm_u24be,pcm_u24le,pcm_u32be,pcm_u32le,pcm_u8,rtp,rtsp,sdp,wav"
	#FLAGS="$FLAGS --enable-parser=aac,aac_latm,flac,h263,h264,mpeg4video,mpegaudio,mpegvideo,vorbis,vp8"
	#FLAGS="$FLAGS --enable-decoder=aac,aac_latm,flac,h264,mjpeg,mpeg4,mp3,vorbis,wmalossless,wmapro,wmav1,wmav2,wmavoice"
	FLAGS="$FLAGS --enable-encoder=png"
	FLAGS="$FLAGS --enable-protocol=file,http,https,mmsh,mmst,pipe,rtmp"
	FLAGS="$FLAGS --disable-debug"

    if [ ! -z "$SSL" ]; then
        FLAGS="$FLAGS --enable-openssl"
    fi

	case "$version" in
		mips)
			EXTRA_CFLAGS=""
			EXTRA_LDFLAGS=""
			ABI="mips"
            CFLAGS="-std=c99 -O3 -Wall -pipe -fpic -fasm \
                -ftree-vectorize -ffunction-sections -funwind-tables -fomit-frame-pointer -funswitch-loops \
                -finline-limit=300 -finline-functions -fpredictive-commoning -fgcse-after-reload -fipa-cp-clone \
                -Wno-psabi -Wa,--noexecstack"
            if [ ! -z "$SSL" ]; then
                EXTRA_LDFLAGS="-L$SSL_LD/libs/mips"
                CFLAGS="-I$SSL/include -std=c99 -O3 -Wall -pipe -fpic -fasm \
                        -ftree-vectorize -ffunction-sections -funwind-tables -fomit-frame-pointer -funswitch-loops \
                        -finline-limit=300 -finline-functions -fpredictive-commoning -fgcse-after-reload -fipa-cp-clone \
                        -Wno-psabi -Wa,--noexecstack"
                #CFLAGS="-I$SSL/include"
                SSL_OBJS=`find $SSL_LD/obj/local/mips/objs/ssl $SSL_LD/obj/local/mips/objs/crypto -type f -name "*.o"`
            fi
			;;
	esac
	DEST="$DEST/$ABI"
	FLAGS="$FLAGS --prefix=$DEST"

	mkdir -p $DEST
	echo $FLAGS --extra-cflags="$CFLAGS $EXTRA_CFLAGS" --extra-ldflags="$EXTRA_LDFLAGS" > $DEST/info.txt
	./configure $FLAGS --extra-cflags="$CFLAGS $EXTRA_CFLAGS" --extra-ldflags="$EXTRA_LDFLAGS" --extra-cflags='' | tee $DEST/configuration.txt
	[ $PIPESTATUS == 0 ] || exit 1
	make clean
	make -j4 || exit 1
	make install || exit 1

done

