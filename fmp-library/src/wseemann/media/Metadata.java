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

package wseemann.media;

import java.util.Collections;
import java.util.HashMap;
import java.util.Set;

/**
   Class to hold the media's metadata.  Metadata are used
   for human consumption and can be embedded in the media (e.g
   shoutcast) or available from an external source. The source can be
   local (e.g thumbnail stored in the DB) or remote.

   Metadata is like a Bundle. It is sparse and each key can occur at
   most once. The key is an integer and the value is the actual metadata.

   The caller is expected to know the type of the metadata and call
   the right get* method to fetch its value.
   
   @hide
 */
public class Metadata
{
    // The metadata are keyed using integers rather than more heavy
    // weight strings. We considered using Bundle to ship the metadata
    // between the native layer and the java layer but dropped that
    // option since keeping in sync a native implementation of Bundle
    // and the java one would be too burdensome. Besides Bundle uses
    // String for its keys.
    // The key range [0 8192) is reserved for the system.
    //
    // We manually serialize the data in Parcels. For large memory
    // blob (bitmaps, raw pictures) we use MemoryFile which allow the
    // client to make the data purge-able once it is done with it.
    //

    /**
     * {@hide}
     */
    public static final int ANY = 0;  // Never used for metadata returned, only for filtering.
                                      // Keep in sync with kAny in MediaPlayerService.cpp

    // Playback capabilities.
    /**
     * Indicate whether the media can be paused
     */
    public static final int PAUSE_AVAILABLE         = 1; // Boolean
    /**
     * Indicate whether the media can be backward seeked
     */
    public static final int SEEK_BACKWARD_AVAILABLE = 2; // Boolean
    /**
     * Indicate whether the media can be forward seeked
     */
    public static final int SEEK_FORWARD_AVAILABLE  = 3; // Boolean
    /**
     * Indicate whether the media can be seeked
     */
    public static final int SEEK_AVAILABLE          = 4; // Boolean

    /**
     * The metadata key to retrieve the name of the set this work belongs to.
     */
    public static final String METADATA_KEY_ALBUM = "album";
    /**
     * The metadata key to retrieve the main creator of the set/album, if different 
     * from artist. e.g. "Various Artists" for compilation albums.
     */
    public static final String METADATA_KEY_ALBUM_ARTIST = "album_artist";
    /**
     * The metadata key to retrieve the main creator of the work.
     */
    public static final String METADATA_KEY_ARTIST = "artist";
    /**
     * The metadata key to retrieve the any additional description of the file.
     */
    public static final String METADATA_KEY_COMMENT = "comment";
    /**
     * The metadata key to retrieve the who composed the work, if different from artist.
     */
    public static final String METADATA_KEY_COMPOSER = "composer";
    /**
     * The metadata key to retrieve the name of copyright holder.
     */
    public static final String METADATA_KEY_COPYRIGHT = "copyright";
    /**
     * The metadata key to retrieve the date when the file was created, preferably in ISO 8601.
     */
    public static final String METADATA_KEY_CREATION_TIME = "creation_time";
    /**
     * The metadata key to retrieve the date when the work was created, preferably in ISO 8601.
     */
    public static final String METADATA_KEY_DATE = "date";
    /**
     * The metadata key to retrieve the number of a subset, e.g. disc in a multi-disc collection.
     */
    public static final String METADATA_KEY_DISC = "disc";
    /**
     * The metadata key to retrieve the name/settings of the software/hardware that produced the file.
     */
    public static final String METADATA_KEY_ENCODER = "encoder";
    /**
     * The metadata key to retrieve the person/group who created the file.
     */
    public static final String METADATA_KEY_ENCODED_BY = "encoded_by";
    /**
     * The metadata key to retrieve the original name of the file.
     */
    public static final String METADATA_KEY_FILENAME = "filename";
    /**
     * The metadata key to retrieve the genre of the work.
     */
    public static final String METADATA_KEY_GENRE = "genre";
    /**
     * The metadata key to retrieve the main language in which the work is performed, preferably
     * in ISO 639-2 format. Multiple languages can be specified by separating them with commas.
     */
    public static final String METADATA_KEY_LANGUAGE = "language";
    /**
     * The metadata key to retrieve the artist who performed the work, if different from artist.
     * E.g for "Also sprach Zarathustra", artist would be "Richard Strauss" and performer "London 
     * Philharmonic Orchestra".
     */
    public static final String METADATA_KEY_PERFORMER = "performer";
    /**
     * The metadata key to retrieve the name of the label/publisher.
     */
    public static final String METADATA_KEY_PUBLISHER = "publisher";
    /**
     * The metadata key to retrieve the name of the service in broadcasting (channel name).
     */
    public static final String METADATA_KEY_SERVICE_NAME = "service_name";
    /**
     * The metadata key to retrieve the name of the service provider in broadcasting.
     */
    public static final String METADATA_KEY_SERVICE_PROVIDER = "service_provider";
    /**
     * The metadata key to retrieve the name of the work.
     */
    public static final String METADATA_KEY_TITLE = "title";
    /**
     * The metadata key to retrieve the number of this work in the set, can be in form current/total.
     */
    public static final String METADATA_KEY_TRACK = "track";
    /**
     * The metadata key to retrieve the total bitrate of the bitrate variant that the current stream 
     * is part of.
     */
    public static final String METADATA_KEY_VARIANT_BITRATE = "bitrate";
    /**
     * The metadata key to retrieve the duration of the work in milliseconds.
     */
    public static final String METADATA_KEY_DURATION = "duration";
    /**
     * The metadata key to retrieve the audio codec of the work.
     */
    public static final String METADATA_KEY_AUDIO_CODEC = "audio_codec";
    /**
     * The metadata key to retrieve the video codec of the work.
     */
    public static final String METADATA_KEY_VIDEO_CODEC = "video_codec";
    /**
     * This key retrieves the video rotation angle in degrees, if available.
     * The video rotation angle may be 0, 90, 180, or 270 degrees.
     */
    public static final String METADATA_KEY_VIDEO_ROTATION = "rotation";
    
    // Shorthands to set the MediaPlayer's metadata filter.
    /**
     * {@hide}
     */
    public static final Set<Integer> MATCH_NONE = Collections.EMPTY_SET;
    /**
     * {@hide}
     */
    public static final Set<Integer> MATCH_ALL = Collections.singleton(ANY);

    /**
     * {@hide}
     */
    public static final int STRING_VAL     = 1;

    private static final String TAG = "media.Metadata";

    private HashMap<String, String> mMetadata =
            new HashMap<String, String>();

    /**
     * {@hide}
     */
    public Metadata() { }

    /**
     * Check a parcel containing metadata is well formed. The header
     * is checked as well as the individual records format. However, the
     * data inside the record is not checked because we do lazy access
     * (we check/unmarshall only data the user asks for.)
     *
     * Format of a metadata parcel:
     <pre>
                         1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     metadata total size                       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |     'M'       |     'E'       |     'T'       |     'A'       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                .... metadata records ....                     |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     </pre>
     *
     * @param parcel With the serialized data. Metadata keeps a
     *               reference on it to access it later on. The caller
     *               should not modify the parcel after this call (and
     *               not call recycle on it.)
     * @return false if an error occurred.
     * {@hide}
     */
    public boolean parse(HashMap<String, String> metadata) {
    	mMetadata = metadata;
    	
        return true;
    }

    /**
     * @return The set of metadata ID found.
     */
    public Set<String> keySet() {
        return mMetadata.keySet();
    }

    /**
     * @return true if a value is present for the given key.
     */
    public boolean has(final String metadataId) {
        /*if (!checkMetadataId(metadataId)) {
            throw new IllegalArgumentException("Invalid key: " + metadataId);
        }*/
        return mMetadata.containsKey(metadataId);
    }

    // Accessors.
    // Caller must make sure the key is present using the {@code has}
    // method otherwise a RuntimeException will occur.

    /**
     * {@hide}
     */
    public String getString(final String key) {
        //checkType(key, STRING_VAL);
        return mMetadata.get(key);
    }

    /**
     * Check val is either a system id or a custom one.
     * @param val Metadata key to test.
     * @return true if it is in a valid range.
     **/
    /*private boolean checkMetadataId(final String val) {
        if (val <= ANY || (LAST_SYSTEM < val && val < FIRST_CUSTOM)) {
            Log.e(TAG, "Invalid metadata ID " + val);
            return false;
        }
        return true;
    }*/

    /**
     * Check the type of the data match what is expected.
     */
    /*private void checkType(final String key, final String expectedType) {
        final String pos = mKeyToPosMap.get(key);

        mParcel.setDataPosition(pos);

        final int type = mParcel.readInt();
        if (type != expectedType) {
            throw new IllegalStateException("Wrong type " + expectedType + " but got " + type);
        }
    }*/
}
