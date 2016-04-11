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

import wseemann.media.fmpdemo.provider.Media.MediaColumns;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.database.sqlite.SQLiteStatement;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

import java.util.HashMap;

/**
 * Provides access to a database of media files. Each media has a title, the note
 * itself, a creation date and a modified data.
 */
public class MediaProvider extends ContentProvider {
	
    private static final String TAG = MediaProvider.class.getName();

    private static final String DATABASE_NAME = "media.db";
    private static final int DATABASE_VERSION = 2;
    private static final String MEDIA_TABLE_NAME = "media_files";

    private static HashMap<String, String> sMediaProjectionMap;

    private static final int MEDIA = 1;
    private static final int MEDIA_ID = 2;

    private static final UriMatcher sUriMatcher;

    /**
     * This class helps open, create, and upgrade the database file.
     */
    private static class DatabaseHelper extends SQLiteOpenHelper {

        DatabaseHelper(Context context) {
            super(context, DATABASE_NAME, null, DATABASE_VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL("CREATE TABLE " + MEDIA_TABLE_NAME + " ("
                    + MediaColumns._ID + " INTEGER PRIMARY KEY,"
                    + MediaColumns.URI + " TEXT,"
                    + MediaColumns.TITLE + " TEXT,"
                    + MediaColumns.ALBUM + " TEXT,"
                    + MediaColumns.ARTIST + " TEXT,"
                    + MediaColumns.DURATION + " INTEGER,"
                    + MediaColumns.TRACK + " TEXT,"
                    + MediaColumns.YEAR + " INTEGER,"
                    + MediaColumns.ARTWORK + " BLOB"
                    + ");");
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            Log.w(TAG, "Upgrading database from version " + oldVersion + " to "
                    + newVersion + ", which will destroy all old data");
            db.execSQL("DROP TABLE IF EXISTS " + MEDIA_TABLE_NAME);
            onCreate(db);
        }
    }

    private DatabaseHelper mOpenHelper;

    @Override
    public boolean onCreate() {
        mOpenHelper = new DatabaseHelper(getContext());
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        SQLiteQueryBuilder qb = new SQLiteQueryBuilder();
        qb.setTables(MEDIA_TABLE_NAME);

        switch (sUriMatcher.match(uri)) {
        case MEDIA:
            qb.setProjectionMap(sMediaProjectionMap);
            break;

        case MEDIA_ID:
            qb.setProjectionMap(sMediaProjectionMap);
            qb.appendWhere(MediaColumns._ID + "=" + uri.getPathSegments().get(1));
            break;

        default:
            throw new IllegalArgumentException("Unknown URI " + uri);
        }

        // If no sort order is specified use the default
        String orderBy;
        if (TextUtils.isEmpty(sortOrder)) {
            orderBy = MediaColumns.DEFAULT_SORT_ORDER;
        } else {
            orderBy = sortOrder;
        }

        // Get the database and run the query
        SQLiteDatabase db = mOpenHelper.getReadableDatabase();
        Cursor c = qb.query(db, projection, selection, selectionArgs, null, null, orderBy);

        // Tell the cursor what uri to watch, so it knows when its source data changes
        c.setNotificationUri(getContext().getContentResolver(), uri);
        return c;
    }

    @Override
    public String getType(Uri uri) {
        switch (sUriMatcher.match(uri)) {
        case MEDIA:
            return MediaColumns.CONTENT_TYPE;

        case MEDIA_ID:
            return MediaColumns.CONTENT_ITEM_TYPE;

        default:
            throw new IllegalArgumentException("Unknown URI " + uri);
        }
    }

    @Override
    public Uri insert(Uri uri, ContentValues initialValues) {
        // Validate the requested uri
        if (sUriMatcher.match(uri) != MEDIA) {
            throw new IllegalArgumentException("Unknown URI " + uri);
        }

        ContentValues values;
        if (initialValues != null) {
            values = new ContentValues(initialValues);
        } else {
            values = new ContentValues();
        }

        // Make sure that the fields are all set
        if (values.containsKey(MediaColumns.URI) == false) {
            values.put(MediaColumns.URI, Media.UNKNOWN_STRING);
        }
        
        if (values.containsKey(MediaColumns.TITLE) == false) {
            values.put(MediaColumns.TITLE, Media.UNKNOWN_STRING);
        }
        
        if (values.containsKey(MediaColumns.ALBUM) == false) {
            values.put(MediaColumns.ALBUM, Media.UNKNOWN_STRING);
        }
        
        if (values.containsKey(MediaColumns.ARTIST) == false) {
            values.put(MediaColumns.ARTIST, Media.UNKNOWN_STRING);
        }
        
        if (values.containsKey(MediaColumns.DURATION) == false) {
            values.put(MediaColumns.DURATION, Media.UNKNOWN_INTEGER);
        }
        
        if (values.containsKey(MediaColumns.TRACK) == false) {
            values.put(MediaColumns.TRACK, Media.UNKNOWN_STRING);
        }
        
        if (values.containsKey(MediaColumns.YEAR) == false) {
            values.put(MediaColumns.YEAR, Media.UNKNOWN_INTEGER);
        }
        
        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        long rowId = db.insert(MEDIA_TABLE_NAME, MediaColumns.URI, values);
        if (rowId > 0) {
            Uri audioUri = ContentUris.withAppendedId(MediaColumns.CONTENT_URI, rowId);
            getContext().getContentResolver().notifyChange(audioUri, null);
            return audioUri;
        }

        throw new SQLException("Failed to insert row into " + uri);
    }

    @Override
    public int bulkInsert(Uri uri, ContentValues[] values) {
    	// Validate the requested uri
    	if (sUriMatcher.match(uri) != MEDIA) {
    		throw new IllegalArgumentException("Unknown URI " + uri);
    	}
    	
    	int numInserted = 0;
    	
        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        db.beginTransaction();
        	        
        try {
        	//standard SQL insert statement, that can be reused
            SQLiteStatement insert = 
            		db.compileStatement("insert into " + MEDIA_TABLE_NAME 
            				+ " (" + MediaColumns.URI + ","
        	                + MediaColumns.TITLE + ","
        	                + MediaColumns.ALBUM + ","
        	                + MediaColumns.ARTIST + ","
        	                + MediaColumns.DURATION + ","
        	                + MediaColumns.TRACK + ","
        	                + MediaColumns.YEAR + ")"
        	                + " values " + "(?,?,?,?,?,?,?)");
        	
        	for (ContentValues value : values) {
        		// Make sure that the fields are all set
                if (value.containsKey(MediaColumns.URI) == false) {
                	value.put(MediaColumns.URI, Media.UNKNOWN_STRING);
                }
                
                if (value.containsKey(MediaColumns.TITLE) == false) {
                	value.put(MediaColumns.TITLE, Media.UNKNOWN_STRING);
                }
                
                if (value.containsKey(MediaColumns.ALBUM) == false) {
                	value.put(MediaColumns.ALBUM, Media.UNKNOWN_STRING);
                }
                
                if (value.containsKey(MediaColumns.ARTIST) == false) {
                	value.put(MediaColumns.ARTIST, Media.UNKNOWN_STRING);
                }
                
                if (value.containsKey(MediaColumns.DURATION) == false) {
                	value.put(MediaColumns.DURATION, Media.UNKNOWN_INTEGER);
                }
                
                if (value.containsKey(MediaColumns.TRACK) == false) {
                	value.put(MediaColumns.TRACK, Media.UNKNOWN_STRING);
                }
                
                if (value.containsKey(MediaColumns.YEAR) == false) {
                	value.put(MediaColumns.YEAR, Media.UNKNOWN_INTEGER);
                }
        		
                insert.bindString(1, value.getAsString(MediaColumns.URI));
        	    insert.bindString(2, value.getAsString(MediaColumns.TITLE));
        	    insert.bindString(3, value.getAsString(MediaColumns.ALBUM));
        	    insert.bindString(4, value.getAsString(MediaColumns.ARTIST));
        	    insert.bindLong(5, value.getAsInteger(MediaColumns.DURATION));
        	    insert.bindString(6, value.getAsString(MediaColumns.TRACK));
        	    insert.bindLong(7, value.getAsInteger(MediaColumns.YEAR));
        	    insert.execute();
        	    numInserted++;
        	}
            
        	db.setTransactionSuccessful();
        } finally {
        	db.endTransaction();
        }
        
        return numInserted;
    }
    
    @Override
    public int delete(Uri uri, String where, String[] whereArgs) {
        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        int count;
        switch (sUriMatcher.match(uri)) {
        case MEDIA:
            count = db.delete(MEDIA_TABLE_NAME, where, whereArgs);
            break;

        case MEDIA_ID:
            String noteId = uri.getPathSegments().get(1);
            count = db.delete(MEDIA_TABLE_NAME, MediaColumns._ID + "=" + noteId
                    + (!TextUtils.isEmpty(where) ? " AND (" + where + ')' : ""), whereArgs);
            break;

        default:
            throw new IllegalArgumentException("Unknown URI " + uri);
        }

        getContext().getContentResolver().notifyChange(uri, null);
        return count;
    }

    @Override
    public int update(Uri uri, ContentValues values, String where, String[] whereArgs) {
        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        int count;
        switch (sUriMatcher.match(uri)) {
        case MEDIA:
            count = db.update(MEDIA_TABLE_NAME, values, where, whereArgs);
            break;

        case MEDIA_ID:
            String noteId = uri.getPathSegments().get(1);
            count = db.update(MEDIA_TABLE_NAME, values, MediaColumns._ID + "=" + noteId
                    + (!TextUtils.isEmpty(where) ? " AND (" + where + ')' : ""), whereArgs);
            break;

        default:
            throw new IllegalArgumentException("Unknown URI " + uri);
        }

        getContext().getContentResolver().notifyChange(uri, null);
        return count;
    }

    static {
        sUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
        sUriMatcher.addURI(Media.AUTHORITY, "uris", MEDIA);
        sUriMatcher.addURI(Media.AUTHORITY, "uris/#", MEDIA_ID);

        sMediaProjectionMap = new HashMap<String, String>();
        sMediaProjectionMap.put(MediaColumns._ID, MediaColumns._ID);
        sMediaProjectionMap.put(MediaColumns.URI, MediaColumns.URI);
        sMediaProjectionMap.put(MediaColumns.TITLE, MediaColumns.TITLE);
        sMediaProjectionMap.put(MediaColumns.ALBUM, MediaColumns.ALBUM);
        sMediaProjectionMap.put(MediaColumns.ARTIST, MediaColumns.ARTIST);
        sMediaProjectionMap.put(MediaColumns.DURATION, MediaColumns.DURATION);
        sMediaProjectionMap.put(MediaColumns.TRACK, MediaColumns.TRACK);
        sMediaProjectionMap.put(MediaColumns.YEAR, MediaColumns.YEAR);
        sMediaProjectionMap.put(MediaColumns.ARTWORK, MediaColumns.ARTWORK);
    }
}
