/*
 * FFmpegMediaPlayer: A unified interface for playing audio files and streams.
 *
 * Copyright 2014 William Seemann
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package wseemann.media.fmpdemo.provider;

import android.net.Uri;
import android.provider.BaseColumns;

/**
 * Convenience definitions for MediaProvider
 */
public final class Media {
    public static final String AUTHORITY = "wseemann.media.fmpdemo.provider.Media";

    /**
     * The string that is used when a media attribute is not known.
     */
    public static final String UNKNOWN_STRING = "";
    
    /**
     * The integer that is used when a media attribute is not known.
     */
    public static final int UNKNOWN_INTEGER = -1;
    
    // This class cannot be instantiated
    private Media() {}
    
    /**
     * Media table
     */
    public static final class MediaColumns implements BaseColumns {
        // This class cannot be instantiated
        private MediaColumns() {}

        /**
         * The content:// style URL for this table
         */
        public static final Uri CONTENT_URI = Uri.parse("content://" + AUTHORITY + "/uris");

        /**
         * The MIME type of {@link #CONTENT_URI} providing a directory of uris.
         */
        public static final String CONTENT_TYPE = "vnd.android.cursor.dir/com.sourceforge.servestream.uri";

        /**
         * The MIME type of a {@link #CONTENT_URI} sub-directory of a single uri.
         */
        public static final String CONTENT_ITEM_TYPE = "vnd.android.cursor.item/com.sourceforge.servestream.uri";

        /**
         * The default sort order for this table
         */
        public static final String DEFAULT_SORT_ORDER = "_id DESC";
        
        /**
         * The URI of a media file
         * <P>Type: TEXT</P>
         */
        public static final String URI = "uri";
        
        /**
         * The title of the content 
         * <P>Type: TEXT</P>
         */
        public static final String TITLE = "title";
        
        /**
         * The album an audio file is from, if any 
         * <P>Type: TEXT</P>
         */
        public static final String ALBUM = "album";

        /**
         * The artist who created an audio file, if any 
         * <P>Type: TEXT</P>
         */
        public static final String ARTIST = "artist";

        /**
         * The duration of the media file, in ms 
         * <P>Type: INTEGER (long)</P>
         */
        public static final String DURATION = "duration";

        /**
         * The track number of a song on the album, if any. This number encodes both the track number and the disc number. For multi-disc sets, this number will be 1xxx for tracks on the first disc, 2xxx for tracks on the second disc, etc.
         * <P>Type: TEXT</P>
         */
        public static final String TRACK = "track";

        /**
         * The year a media file was created, if any 
         * <P>Type: INTEGER</P>
         */
        public static final String YEAR = "year";
        
        /**
         * The artwork associated with the media file, if any 
         * <P>Type: BLOB</P>
         */
        public static final String ARTWORK = "artwork";
    }
}
