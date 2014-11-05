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

package wseemann.media.fmpdemo.view;

import wseemann.media.fmpdemo.R;
import wseemann.media.fmpdemo.R.drawable;
import wseemann.media.fmpdemo.service.MusicUtils;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.Looper;
import android.support.v4.app.Fragment;
import android.util.AttributeSet;
import android.view.View;

public final class CoverView extends View {
	
	private CoverViewListener mListener;
	
	private Bitmap mBitmap = null;

	public CoverView(Context context, AttributeSet attributes) {
		super(context, attributes);
	}

	public void setup(Looper looper, Fragment fragment) {
		// Verify that the host activity implements the callback interface
	    try {
	    	// Instantiate the CoverViewListener so we can send events to the host
	        mListener = (CoverViewListener) fragment;
	    } catch (ClassCastException e) {
	        // The activity doesn't implement the interface, throw exception
	        throw new ClassCastException(fragment.toString()
	        	+ " must implement CoverViewListener");
	    }
	}

	@Override
	protected void onDraw(Canvas canvas) {
		int width = getWidth();
		int height = getHeight();
		
		//canvas.drawColor(android.R.color.transparent);
		
		if (mBitmap != null) {
			int xOffset = (width - mBitmap.getWidth()) / 2;
			int yOffset = (height - mBitmap.getHeight()) / 2;
			canvas.drawBitmap(mBitmap, xOffset, yOffset, null);
		}
	}

	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		if (w != 0 && h != 0) {
			if (mListener != null) {
				mListener.onCoverViewInitialized();
			}
		}
	}
	
	public boolean generateBitmap(long id) {
		Bitmap b = null;
		
		int width = getWidth();
		int height = getHeight();
		
		if (width == 0 || height == 0) {
			return false;
		}

		int scale = Math.min(width, height);
		
		Context context = getContext();

		b = MusicUtils.getDefaultArtwork(context, R.drawable.albumart_mp_unknown, scale, scale);
		
		mBitmap = b;
		postInvalidate();
		
		return true;
	}

	@Override
	protected void onMeasure(int widthSpec, int heightSpec) {
		int width = View.MeasureSpec.getSize(widthSpec);
		int height = View.MeasureSpec.getSize(heightSpec);

		if (View.MeasureSpec.getMode(widthSpec) == View.MeasureSpec.EXACTLY
			&& View.MeasureSpec.getMode(heightSpec) == View.MeasureSpec.EXACTLY) {
			setMeasuredDimension(width, height);
		} else {
			int size = Math.min(width, height);
			setMeasuredDimension(size, size);
		}
	}
	
    /* The activity that creates an instance of this class must
     * implement this interface in order to receive event callbacks. */
    public interface CoverViewListener {
        public void onCoverViewInitialized();
    }
}
