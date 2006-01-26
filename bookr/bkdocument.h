/*
 * Bookr: document reader for the Sony PSP 
 * Copyright (C) 2005 Carlos Carrasco Martinez (carloscm at gmail dot com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef BKDOCUMENT_H
#define BKDOCUMENT_H

#include <map>
#include <string>

using namespace std;

#include "bklayer.h"

// BKDocument - an unified interface for any page-able, scroll-able,
// bookmark-able file.

class BKDocument : public BKLayer {
	private:
	int lastSuspendSerial;

	int processEventsForView();
	int processEventsForToolbar();

	protected:
	BKDocument();
	~BKDocument();

	public:
	// BKLater::update is implemented outside the document viewers.
	// The viewers receive the "cooked" events via the is/get/set
	// APIs. Of course they can still override this method if neeeded
	// but then they must call BKDocument::update(buttons) before they
	// start processing.
	virtual int update(unsigned int buttons);

	// "Blind" update method for the viwers that need it.
	virtual int updateContent() = 0;

	// Notify the view of a power resume event
	virtual int resume() = 0;

	// BKDocument has its own UI shell for the toolbar and labels.
	virtual void render();

	// Render event for the viewers.
	virtual void renderContent() = 0;

	// Factory with file detection
	static BKDocument* create(string& filePath);

	// Document metadata
	virtual void getFilePath(string&) = 0;
	virtual void getTitle(string&) = 0;
	virtual void getType(string&) = 0;

	// Pagination - a page is clearly defined for a PDF file, but
	// in the case of a text file is just an arbitrary division
	// based on screen size
	virtual bool isPaginated() = 0;
	virtual int getTotalPages() = 0;
	virtual int getCurrentPage() = 0;
	virtual int setCurrentPage(int) = 0;

	// Zoom
	// The type field is a hint for the shell UI to select an
	// apropiate icon.
	#define BKDOCUMENT_ZOOMTYPE_ABSOLUTE				0
	#define BKDOCUMENT_ZOOMTYPE_WIDTH_TO_SCREEN			1
	#define BKDOCUMENT_ZOOMTYPE_HEIGHT_TO_SCREEN		2
	#define BKDOCUMENT_ZOOMTYPE_LARGER_TEXT				3
	#define BKDOCUMENT_ZOOMTYPE_SMALLER_TEXT			4
	struct ZoomLevel {
		int type;
		string label;
		ZoomLevel(int t, char* l) : type(t), label(l) { }
	};
	virtual bool isZoomable() = 0;
	virtual void getZoomLevels(vector<BKDocument::ZoomLevel>& v) = 0;
	
	virtual int getCurrentZoomLevel() = 0;
	virtual int setZoomLevel(int) = 0;

	// Analog pad paning - can be ignored
	virtual int pan(int, int) = 0;

	// Rotation (0 = 0deg, 1 = 90deg, 2 = 180deg, 3 = 240deg)
	virtual bool isRotable() = 0;
	virtual int getRotation() = 0;
	virtual int setRotation(int) = 0;

	// Bookmark support. The returned map is a black box
	// for the bookmarking system.
	virtual bool isBookmarkable() = 0;
	virtual void getBookmarkPosition(map<string, int>&) = 0;
	virtual int setBookmarkPosition(const map<string, int>&) = 0;
};

#endif

